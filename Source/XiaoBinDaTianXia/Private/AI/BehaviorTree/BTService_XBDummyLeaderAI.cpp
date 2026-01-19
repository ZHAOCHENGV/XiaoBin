/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTService_XBDummyLeaderAI.cpp

/**
 * @file BTService_XBDummyLeaderAI.cpp
 * @brief  行为树服务 - 假人主将AI状态更新
 * @return 无
 * @note   详细流程分析: 目标搜索 -> 战斗状态切换 -> 行为中心/目的地更新 -> 丢失目标前进阶段收敛
 *         性能/架构注意事项: 该服务以短间隔Tick运行，日志建议在调试阶段使用，避免发布版刷屏
 */

#include "AI/BehaviorTree/BTService_XBDummyLeaderAI.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/Components/XBCombatComponent.h"
#include "Character/XBDummyCharacter.h"
#include "Character/XBCharacterBase.h"
#include "NavigationSystem.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Components/SplineComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Utils/XBBlueprintFunctionLibrary.h"
#include "Utils/XBLogCategories.h"

// ✨ 新增 - 黑板键默认名称集中管理，避免数据表配置
namespace XBDummyLeaderBlackboardKeys
{
	static const FName TargetLeader(TEXT("TargetLeader"));
	static const FName InCombat(TEXT("IsInCombat"));
	static const FName HomeLocation(TEXT("HomeLocation"));
	static const FName BehaviorCenter(TEXT("BehaviorCenter"));
	static const FName BehaviorDestination(TEXT("BehaviorDestination"));
	static const FName MoveMode(TEXT("MoveMode"));
	static const FName RouteIndex(TEXT("RoutePointIndex"));
	static const FName SelectedAbilityType(TEXT("SelectedAbilityType"));
}

UBTService_XBDummyLeaderAI::UBTService_XBDummyLeaderAI()
{
	// 🔧 修改 - 启用实例化，避免服务状态相互干扰
	bCreateNodeInstance = true;
	NodeName = TEXT("假人主将AI状态更新");
	Interval = 0.1f;
	RandomDeviation = 0.0f;
}

/**
 * @brief 服务进入时初始化
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @return 无
 * @note   详细流程分析: 初始化时间与黑板默认值
 *         性能/架构注意事项: 仅在进入时执行一次
 */
void UBTService_XBDummyLeaderAI::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	NextSearchTime = 0.0f;
	NextWanderTime = 0.0f;
	bHadCombatTarget = false;
	bLoggedMissingSpline = false;
	bRouteForward = true;

	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!AIController || !Blackboard)
	{
		return;
	}

	if (AXBDummyCharacter* Dummy = Cast<AXBDummyCharacter>(AIController->GetPawn()))
	{
		// 🔧 修改 - 初始化黑板基础值，保证行为树启动时有可用数据
		// 为什么要在这里初始化：避免黑板空值导致后续分支判断全部短路
		InitializeBlackboard(Dummy, Blackboard);
	}
}

/**
 * @brief 服务Tick更新
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @param DeltaSeconds 帧间隔
 * @return 无
 * @note   详细流程分析: 目标搜索 -> 战斗状态 -> 行为目的地
 *         性能/架构注意事项: 通过间隔控制感知与随机移动频率
 */
void UBTService_XBDummyLeaderAI::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!AIController || !Blackboard)
	{
		return;
	}

	AXBDummyCharacter* Dummy = Cast<AXBDummyCharacter>(AIController->GetPawn());
	if (!Dummy || Dummy->IsDead())
	{
		return;
	}

	const FXBLeaderAIConfig& AIConfig = Dummy->GetLeaderAIConfig();
	if (!AIConfig.bEnableAI)
	{
		return;
	}

	const FName TargetLeaderKey = XBDummyLeaderBlackboardKeys::TargetLeader;
	const FName InCombatKey = XBDummyLeaderBlackboardKeys::InCombat;
	const FName MoveModeKey = XBDummyLeaderBlackboardKeys::MoveMode;
	const FName BehaviorDestinationKey = XBDummyLeaderBlackboardKeys::BehaviorDestination;
	const FName SelectedAbilityTypeKey = XBDummyLeaderBlackboardKeys::SelectedAbilityType;

	// ✨ 修改 - 移动模式现在由 Actor 配置决定，不再从数据表同步
	// 使用 Dummy->GetDummyMoveMode() 替代废弃的 AIConfig.MoveMode
	const EXBLeaderAIMoveMode CurrentMoveMode = Dummy->GetDummyMoveMode();
	if (Blackboard->GetValueAsInt(MoveModeKey) != static_cast<int32>(CurrentMoveMode))
	{
		Blackboard->SetValueAsInt(MoveModeKey, static_cast<int32>(CurrentMoveMode));
		UE_LOG(LogXBAI, Log, TEXT("假人AI同步MoveMode=%d 到黑板，Dummy=%s"),
			static_cast<int32>(CurrentMoveMode), *Dummy->GetName());
	}

	// 🔧 修改 - 先读取当前目标，判断是否需要回归
	// 为什么要先读目标：战斗/移动逻辑依赖目标是否存在，必须先裁决
	AXBCharacterBase* CurrentTarget = Cast<AXBCharacterBase>(Blackboard->GetValueAsObject(TargetLeaderKey));
	if (CurrentTarget)
	{
		// 🔧 修改 - 重新锁定目标时取消丢失目标的前进行为，避免阻塞战斗更新
		// 为什么要清理：丢失目标前进阶段会提前return，必须在有目标时终止
		bForwardMoveAfterLost = false;
		ForwardMoveEndTime = 0.0f;

		// 🔧 修改 - 目标进入草丛、全灭或超出视野范围时立刻清理目标
		// 为什么要清理：目标不可见或已全灭时继续追击会造成无效移动/卡死
		const float DistToTarget = FVector::Dist(Dummy->GetActorLocation(), CurrentTarget->GetActorLocation());
		// 给 20% 的追击缓冲距离，避免在边缘反复切换
		const float LoseTargetRange = AIConfig.VisionRange * 1.2f;

		if (CurrentTarget->IsHiddenInBush() || 
			IsLeaderArmyEliminated(CurrentTarget) ||
			DistToTarget > LoseTargetRange)
		{
			// 🔧 修改 - 丢失目标时需要触发正前方行走逻辑
			const bool bShouldForwardMove = true;
			Blackboard->SetValueAsObject(TargetLeaderKey, nullptr);
			Blackboard->SetValueAsBool(InCombatKey, false);
			// 🔧 修改 - 目标丢失时清理能力选择，避免残留导致错误攻击范围
			Blackboard->SetValueAsInt(SelectedAbilityTypeKey, static_cast<int32>(EXBDummyLeaderAbilityType::None));
			// 🔧 修改 - 退出战斗时同步主将与士兵状态
			// 为什么要退出战斗：让士兵回归跟随/编队而非继续战斗逻辑
			Dummy->ExitCombat();
			HandleTargetLost(Dummy, Blackboard, bShouldForwardMove);
			bHadCombatTarget = false;
			
			if (DistToTarget > LoseTargetRange)
			{
				UE_LOG(LogXBAI, Log, TEXT("假人AI目标超出视野范围(%.1f > %.1f)，已丢失目标: %s"), 
					DistToTarget, LoseTargetRange, *Dummy->GetName());
			}
			else
			{
				UE_LOG(LogXBAI, Log, TEXT("假人AI目标丢失(草丛/全灭)，已清理目标并进入回归流程: %s"), *Dummy->GetName());
			}
		}
		else
		{
			Blackboard->SetValueAsBool(InCombatKey, true);
			// 🔧 修改 - 仅标记行为树进入战斗靠近阶段，士兵参战延迟到主将真实攻击触发
			// 为什么要延迟：避免仅因视野锁定就提前驱动士兵攻击，符合“主将先出手”的战斗节奏
			bHadCombatTarget = true;

			// 🔧 修改 - 战斗时将行为目的地锁定为目标位置，确保主动靠近
			// 为什么要写入：MoveTo/行为树需要明确目的地，避免仍使用漫游目标
			Blackboard->SetValueAsVector(BehaviorDestinationKey, CurrentTarget->GetActorLocation());
			// ✨ 新增 - 在战斗阶段选择一个可用能力并写入黑板
			// 为什么要在服务中选择：确保移动任务与攻击任务统一使用同一能力范围
			SelectCombatAbility(Dummy, Blackboard, CurrentTarget);
			// 降低日志频率
			// UE_LOG(LogXBAI, Verbose, TEXT("假人AI战斗靠近目标，更新目的地: %s -> %s"), *Dummy->GetName(), *CurrentTarget->GetName());
		}
	}

	// 🔧 修改 - 重新读取目标，避免目标清理后仍使用旧指针
	// 为什么要重新读：上方可能已经清理目标
	CurrentTarget = Cast<AXBCharacterBase>(Blackboard->GetValueAsObject(TargetLeaderKey));

	// 🔧 修改 - 无目标时按间隔搜索敌方主将
	// 为什么要限流：感知查询成本较高，避免每帧查询
	if (!CurrentTarget)
	{
		const float CurrentTime = Dummy->GetWorld()->GetTimeSeconds();
		if (CurrentTime >= NextSearchTime)
		{
			NextSearchTime = CurrentTime + AIConfig.TargetSearchInterval;

			// 🔧 新增 - 中立阵营不主动搜索敌人，只保留受击反击逻辑
			const bool bIsNeutral = (Dummy->GetFaction() == EXBFaction::Neutral);
			
			if (!bIsNeutral)
			{
				// 非中立阵营执行主动搜索
				AXBCharacterBase* FoundLeader = nullptr;
				if (FindEnemyLeader(Dummy, FoundLeader))
				{
					Blackboard->SetValueAsObject(TargetLeaderKey, FoundLeader);
					Blackboard->SetValueAsBool(InCombatKey, true);
					// 🔧 修改 - 仅进入追击态，士兵战斗状态等待主将攻击触发
					// 为什么要拆分：主将靠近时先行判断攻击条件，避免士兵提前冲锋
					bHadCombatTarget = true;
					// ✨ 修复 - 发现目标后立即选择能力，避免移动/攻击任务因能力未选择而失败
					SelectCombatAbility(Dummy, Blackboard, FoundLeader);

					UE_LOG(LogXBAI, Log, TEXT("假人主将 %s 发现敌方主将并进入战斗: %s"),
						*Dummy->GetName(), *FoundLeader->GetName());
					return; // 找到目标后直接返回
				}
			}

			// 🔧 修改 - 受击反击逻辑（所有阵营通用，包括中立）
			// 为什么要反击：让假人对最近的攻击来源做出响应
			AXBCharacterBase* DamageLeader = Dummy->GetLastDamageLeader();
			const bool bShouldCounterAttack =
				DamageLeader &&
				!DamageLeader->IsDead() &&
				!DamageLeader->IsHiddenInBush() &&
				(Dummy->GetFaction() == EXBFaction::Neutral ||
					UXBBlueprintFunctionLibrary::AreActorsHostile(Dummy, DamageLeader));

			if (bShouldCounterAttack)
			{
				// 中立阵营的反击范围检查（避免追击过远）
				if (bIsNeutral)
				{
					const float DistToAttacker = FVector::Dist(Dummy->GetActorLocation(), DamageLeader->GetActorLocation());
					if (DistToAttacker > AIConfig.VisionRange * 1.5f)
					{
						// 超出反击范围，清除受击记录
						Dummy->ClearLastDamageLeader();
						UE_LOG(LogXBAI, Verbose, TEXT("中立假人 %s 受击者太远(%.1f)，不反击"), 
							*Dummy->GetName(), DistToAttacker);
						return;
					}
				}

				Blackboard->SetValueAsObject(TargetLeaderKey, DamageLeader);
				Blackboard->SetValueAsBool(InCombatKey, true);
				// 🔧 修改 - 反击时保持追击态，士兵参战仍由主将攻击事件触发
				// 为什么要控制节奏：受到伤害后先由主将决定是否出手，再带动士兵
				Dummy->ClearLastDamageLeader();
				bHadCombatTarget = true;
				// ✨ 修复 - 反击时立即选择能力，避免移动/攻击任务因能力未选择而失败
				SelectCombatAbility(Dummy, Blackboard, DamageLeader);

				UE_LOG(LogXBAI, Log, TEXT("假人主将 %s 受到伤害后反击主将: %s"),
					*Dummy->GetName(), *DamageLeader->GetName());
				return;
			}

			Blackboard->SetValueAsBool(InCombatKey, false);
			// 🔧 修改 - 清理能力选择，保证下次进入战斗重新选择
			Blackboard->SetValueAsInt(SelectedAbilityTypeKey, static_cast<int32>(EXBDummyLeaderAbilityType::None));
			if (bHadCombatTarget)
			{
				// 🔧 修改 - 从战斗回归后重置行为中心
				// 为什么要重置：避免回到旧目标位置导致跑偏
				Dummy->ExitCombat();
				HandleTargetLost(Dummy, Blackboard, false);
				bHadCombatTarget = false;
			}
		}
	}

	// ✨ 新增 - 处于“丢失目标后正前方移动”阶段时，先走完再恢复常规移动模式
	// 为什么要短暂停顿：模拟“惯性”追击，避免立刻原地旋转导致观感突兀
	if (bForwardMoveAfterLost)
	{
		const float CurrentTime = Dummy->GetWorld()->GetTimeSeconds();
		if (CurrentTime < ForwardMoveEndTime && !CurrentTarget)
		{
			// 🔧 修改 - 前进阶段保持目的地不变，但允许继续感知目标
			return;
		}

		// 🔧 修改 - 结束正前方移动后恢复常规行为更新
		bForwardMoveAfterLost = false;
		// 🔧 修改 - 前进阶段结束后强制刷新随机移动时间，避免目的地长期不更新
		NextWanderTime = 0.0f;
		UE_LOG(LogXBAI, Log, TEXT("假人AI前进阶段结束，恢复常规移动: %s"), *Dummy->GetName());
	}

	// 🔧 修改 - 非战斗状态下更新行为目的地
	// 为什么不在战斗中更新：战斗移动由目标追击驱动
	if (!Blackboard->GetValueAsBool(InCombatKey))
	{
		UpdateBehaviorDestination(Dummy, Blackboard);
	}
}

/**
 * @brief  初始化黑板基础数据
 * @param  Dummy 假人主将
 * @param  Blackboard 黑板组件
 * @return 无
 * @note   详细流程分析: 写入初始位置/行为中心/行为模式
 *         性能/架构注意事项: 仅在服务激活时执行
 */
void UBTService_XBDummyLeaderAI::InitializeBlackboard(AXBDummyCharacter* Dummy, UBlackboardComponent* Blackboard)
{
	const FXBLeaderAIConfig& AIConfig = Dummy->GetLeaderAIConfig();
	const FVector HomeLocation = Dummy->GetActorLocation();

	const FName HomeLocationKey = XBDummyLeaderBlackboardKeys::HomeLocation;
	const FName BehaviorCenterKey = XBDummyLeaderBlackboardKeys::BehaviorCenter;
	const FName BehaviorDestinationKey = XBDummyLeaderBlackboardKeys::BehaviorDestination;
	const FName MoveModeKey = XBDummyLeaderBlackboardKeys::MoveMode;
	const FName RouteIndexKey = XBDummyLeaderBlackboardKeys::RouteIndex;
	const FName InCombatKey = XBDummyLeaderBlackboardKeys::InCombat;
	const FName SelectedAbilityTypeKey = XBDummyLeaderBlackboardKeys::SelectedAbilityType;

	// 🔧 修改 - 写入初始位置和行为中心，保证站立/随机移动有基准
	// 为什么要写入：避免黑板初始值为零导致行为目标无效
	Blackboard->SetValueAsVector(HomeLocationKey, HomeLocation);

	// ✨ 优化 - 行为中心优先使用 SpawnLocation，确保 Stand 模式归位准确
	if (Dummy->GetDummyMoveMode() == EXBLeaderAIMoveMode::Stand)
	{
		Blackboard->SetValueAsVector(BehaviorCenterKey, Dummy->GetSpawnLocation());
	}
	else
	{
		Blackboard->SetValueAsVector(BehaviorCenterKey, HomeLocation);
	}
	// ✨ 修改 - 使用 Dummy->GetDummyMoveMode() 替代废弃的 AIConfig.MoveMode
	Blackboard->SetValueAsInt(MoveModeKey, static_cast<int32>(Dummy->GetDummyMoveMode()));
	Blackboard->SetValueAsInt(RouteIndexKey, 0);
	Blackboard->SetValueAsBool(InCombatKey, false);
	Blackboard->SetValueAsVector(BehaviorDestinationKey, HomeLocation);
	Blackboard->SetValueAsInt(SelectedAbilityTypeKey, static_cast<int32>(EXBDummyLeaderAbilityType::None));

}

/**
 * @brief  选择当前战斗阶段的能力
 * @param  Dummy 假人主将
 * @param  Blackboard 黑板组件
 * @param  Target 当前目标
 * @return 选择的能力类型
 * @note   详细流程分析: 校验当前选择 -> 冷却判断 -> 选择可用能力写入黑板
 *         性能/架构注意事项: 仅在有目标时调用，避免频繁无效写入
 */
EXBDummyLeaderAbilityType UBTService_XBDummyLeaderAI::SelectCombatAbility(
	AXBDummyCharacter* Dummy,
	UBlackboardComponent* Blackboard,
	AXBCharacterBase* Target)
{
	if (!Dummy || !Blackboard || !Target)
	{
		return EXBDummyLeaderAbilityType::None;
	}

	UXBCombatComponent* CombatComp = Dummy->GetCombatComponent();
	if (!CombatComp)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人AI选择能力失败：战斗组件无效，Dummy=%s"), *Dummy->GetName());
		return EXBDummyLeaderAbilityType::None;
	}

	const FName SelectedAbilityTypeKey = XBDummyLeaderBlackboardKeys::SelectedAbilityType;
	const EXBDummyLeaderAbilityType CurrentType =
		static_cast<EXBDummyLeaderAbilityType>(Blackboard->GetValueAsInt(SelectedAbilityTypeKey));

	// 🔧 修复 - 每次都重新评估能力选择，确保攻击完成后立即选择新能力
	// 按优先级选择可用能力：技能优先，其次普攻
	EXBDummyLeaderAbilityType NewType = EXBDummyLeaderAbilityType::None;
	
	const bool bSkillOnCooldown = CombatComp->IsSkillOnCooldown();
	const bool bBasicOnCooldown = CombatComp->IsBasicAttackOnCooldown();
	
	// ✨ 新增 - 检查是否正在攻击（蒙太奇播放中），避免选中无法释放的技能
	const bool bIsAttacking = CombatComp->IsAttacking();
	
	// 🔧 修改 - 如果正在攻击，不改变当前选择，等待攻击完成
	if (bIsAttacking)
	{
		UE_LOG(LogXBAI, Verbose, TEXT("假人AI正在攻击中，保持当前能力选择: Dummy=%s"), *Dummy->GetName());
		return CurrentType;
	}
	
	// 选择可用的技能
	if (!bSkillOnCooldown)
	{
		NewType = EXBDummyLeaderAbilityType::SpecialSkill;
	}
	else if (!bBasicOnCooldown)
	{
		NewType = EXBDummyLeaderAbilityType::BasicAttack;
	}

	// 🔧 优化 - 仅在能力类型改变时写入黑板和打印日志，减少性能开销
	if (NewType != CurrentType)
	{
		Blackboard->SetValueAsInt(SelectedAbilityTypeKey, static_cast<int32>(NewType));
		
		if (NewType != EXBDummyLeaderAbilityType::None)
		{
			UE_LOG(LogXBAI, Log, TEXT("假人AI切换能力: Dummy=%s, %d -> %d"), 
				*Dummy->GetName(), static_cast<int32>(CurrentType), static_cast<int32>(NewType));
		}
		else
		{
			UE_LOG(LogXBAI, Verbose, TEXT("假人AI无可用能力，等待冷却: Dummy=%s"), *Dummy->GetName());
		}
	}

	return NewType;
}

/**
 * @brief  搜索视野内敌方主将
 * @param  Dummy 假人主将
 * @param  OutLeader 输出主将
 * @return 是否找到
 * @note   详细流程分析: 球形检测 -> 阵营过滤 -> 草丛过滤 -> 选择最近
 *         性能/架构注意事项: 直接使用球形检测确保实时位置准确
 */
bool UBTService_XBDummyLeaderAI::FindEnemyLeader(AXBDummyCharacter* Dummy, AXBCharacterBase*& OutLeader) const
{
	OutLeader = nullptr;
	if (!Dummy)
	{
		return false;
	}

	UWorld* World = Dummy->GetWorld();
	if (!World)
	{
		return false;
	}

	const FXBLeaderAIConfig& AIConfig = Dummy->GetLeaderAIConfig();
	const FVector Origin = Dummy->GetActorLocation();
	const float VisionRange = AIConfig.VisionRange;
	const EXBFaction MyFaction = Dummy->GetFaction();

	// ✨ 使用球形重叠检测
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.AddIgnoredActor(Dummy);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	// ✨ 添加自定义碰撞通道：ECC_GameTraceChannel4 = Leader, ECC_GameTraceChannel3 = Soldier
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel4); 
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel3);

	if (!World->OverlapMultiByObjectType(
		OverlapResults,
		Origin,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(VisionRange),
		QueryParams))
	{
		return false;
	}

	float BestDistance = MAX_FLT;
	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* Actor = Result.GetActor();
		if (!Actor || !IsValid(Actor))
		{
			continue;
		}

		// 🔧 只认主将，过滤非主将对象
		AXBCharacterBase* CandidateLeader = Cast<AXBCharacterBase>(Actor);
		if (!CandidateLeader || CandidateLeader == Dummy)
		{
			continue;
		}

		// 🔧 阵营判断 - 只攻击敌对阵营
		if (!UXBBlueprintFunctionLibrary::AreFactionsHostile(MyFaction, CandidateLeader->GetFaction()))
		{
			continue;
		}

		// 🔧 过滤死亡目标
		if (CandidateLeader->IsDead())
		{
			continue;
		}

		// 🔧 草丛隐身目标不可锁定
		if (CandidateLeader->IsHiddenInBush())
		{
			continue;
		}

		const float Distance = FVector::Dist(Origin, CandidateLeader->GetActorLocation());
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			OutLeader = CandidateLeader;
		}
	}

	return OutLeader != nullptr;
}

/**
 * @brief  判断敌方主将与士兵是否已全部阵亡
 * @param  Leader 目标主将
 * @return 是否全灭
 * @note   详细流程分析: 主将死亡 -> 遍历士兵状态
 *         性能/架构注意事项: 士兵数量通常有限，可接受遍历
 */
bool UBTService_XBDummyLeaderAI::IsLeaderArmyEliminated(AXBCharacterBase* Leader) const
{
	if (!Leader || !Leader->IsDead())
	{
		return false;
	}

	const TArray<AXBSoldierCharacter*>& Soldiers = Leader->GetSoldiers();
	for (AXBSoldierCharacter* Soldier : Soldiers)
	{
		if (Soldier && Soldier->GetSoldierState() != EXBSoldierState::Dead)
		{
			return false;
		}
	}

	return true;
}

/**
 * @brief  处理目标丢失后的回归逻辑
 * @param  Dummy 假人主将
 * @param  Blackboard 黑板组件
 * @param  bForwardMoveAfterLost 是否进入“丢失目标后正前方行走”流程
 * @return 无
 * @note   详细流程分析: 重置行为中心 -> 可选正前方行走 -> 校正路线索引
 *         性能/架构注意事项: 仅在目标切换时执行
 */
void UBTService_XBDummyLeaderAI::HandleTargetLost(AXBDummyCharacter* Dummy, UBlackboardComponent* Blackboard, bool bForwardMoveAfterLostParam)
{
	const FXBLeaderAIConfig& AIConfig = Dummy->GetLeaderAIConfig();
	const FName BehaviorCenterKey = XBDummyLeaderBlackboardKeys::BehaviorCenter;
	const FName BehaviorDestinationKey = XBDummyLeaderBlackboardKeys::BehaviorDestination;
	const FName HomeLocationKey = XBDummyLeaderBlackboardKeys::HomeLocation;

	// ✨ 核心修改 - 根据 DummyMoveMode 执行不同的回归逻辑
	const EXBLeaderAIMoveMode MoveMode = Dummy->GetDummyMoveMode();

	// ❌ 删除 - 移除"正前方行走"逻辑（导致主将莫名乱跑）
	// 原逻辑：if (bForwardMoveAfterLostParam) { 设置前进目的地 }

	// 清理前进阶段标记
	bForwardMoveAfterLost = false;
	ForwardMoveEndTime = 0.0f;
	NextWanderTime = 0.0f;

	switch (MoveMode)
	{
	case EXBLeaderAIMoveMode::Stand:
		{
			// ✨ 原地站立模式：回归出生点
			// 1. 获取最原始的出生点
			FVector SpawnLoc = Dummy->GetSpawnLocation();

			// 2. 投影到导航网格（确保可达）
			UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Dummy->GetWorld());
			if (NavSys)
			{
				FNavLocation ProjectedLoc;
				// 搜索范围略大一点(200,200,500)以容错
				if (NavSys->ProjectPointToNavigation(SpawnLoc, ProjectedLoc, FVector(200, 200, 500)))
				{
					SpawnLoc = ProjectedLoc.Location;
				}
			}

			// ✨ 修改 - 使用出生点XY + 当前Z高度，避免NavMesh高度差异导致的问题
			const FVector SpawnXY_CurrentZ = FVector(SpawnLoc.X, SpawnLoc.Y, Dummy->GetActorLocation().Z);
			Blackboard->SetValueAsVector(BehaviorCenterKey, SpawnXY_CurrentZ);
			Blackboard->SetValueAsVector(BehaviorDestinationKey, SpawnXY_CurrentZ);
			
			UE_LOG(LogXBAI, Log, TEXT("假人AI目标丢失(Stand模式)，重置中心点: %s"), *Dummy->GetName());
		}
		break;

	case EXBLeaderAIMoveMode::Wander:
		{
			// ✨ 随机移动模式：以当前战斗结束位置为中心开始范围随机移动
			const FVector CurrentLocation = Dummy->GetActorLocation();
			Blackboard->SetValueAsVector(BehaviorCenterKey, CurrentLocation);
			// 不立即设置目的地，让 UpdateBehaviorDestination 在下一帧刷新
			UE_LOG(LogXBAI, Log, TEXT("假人AI目标丢失(Wander模式)，以当前位置为中心开始随机移动: %s"), *Dummy->GetName());
		}
		break;

	case EXBLeaderAIMoveMode::Route:
		{
			// ✨ 固定路线模式：导航回巡逻路线上继续移动
			if (USplineComponent* SplineComp = Dummy->GetPatrolSplineComponent())
			{
				ResetRouteIndexToNearest(Dummy, Blackboard, SplineComp);
				UE_LOG(LogXBAI, Log, TEXT("假人AI目标丢失(Route模式)，回归巡逻路线: %s"), *Dummy->GetName());
			}
			else
			{
				// 无样条组件时回退为出生点
				const FVector HomeLocation = Blackboard->GetValueAsVector(HomeLocationKey);
				Blackboard->SetValueAsVector(BehaviorCenterKey, HomeLocation);
				Blackboard->SetValueAsVector(BehaviorDestinationKey, HomeLocation);
				UE_LOG(LogXBAI, Warning, TEXT("假人AI目标丢失(Route模式)但无巡逻路线，回退到出生点: %s"), *Dummy->GetName());
			}
		}
		break;

	case EXBLeaderAIMoveMode::Forward:
		{
			// ✨ 新增 - Forward模式战斗结束后按当前朝向继续直走
			// 不重置行为中心，让UpdateBehaviorDestination继续按当前朝向移动
			UE_LOG(LogXBAI, Log, TEXT("假人AI目标丢失(Forward模式)，继续向前: %s"), *Dummy->GetName());
		}
		break;

	default:
		{
			// 未知模式回退为当前位置
			Blackboard->SetValueAsVector(BehaviorCenterKey, Dummy->GetActorLocation());
			UE_LOG(LogXBAI, Warning, TEXT("假人AI目标丢失(未知模式)，保持当前位置: %s"), *Dummy->GetName());
		}
		break;
	}
}

/**
 * @brief  更新非战斗状态下的行为目的地
 * @param  Dummy 假人主将
 * @param  Blackboard 黑板组件
 * @return 无
 * @note   详细流程分析: 根据移动方式设置目的地
 *         性能/架构注意事项: 随机移动采用时间间隔控制
 */
void UBTService_XBDummyLeaderAI::UpdateBehaviorDestination(AXBDummyCharacter* Dummy, UBlackboardComponent* Blackboard)
{
	const FXBLeaderAIConfig& AIConfig = Dummy->GetLeaderAIConfig();
	const FName BehaviorDestinationKey = XBDummyLeaderBlackboardKeys::BehaviorDestination;
	const FName BehaviorCenterKey = XBDummyLeaderBlackboardKeys::BehaviorCenter;
	const FName HomeLocationKey = XBDummyLeaderBlackboardKeys::HomeLocation;
	const FName RouteIndexKey = XBDummyLeaderBlackboardKeys::RouteIndex;

	// 🔧 修改 - 读取行为中心前先做合法性校验，避免未初始化导致出现 FLT_MAX
	// 为什么要校验：黑板初始值可能为未写入状态，直接使用会导致随机点计算失败
	const FVector RawBehaviorCenter = Blackboard->GetValueAsVector(BehaviorCenterKey);
	const bool bBehaviorCenterInvalid = RawBehaviorCenter.ContainsNaN() ||
		!FMath::IsFinite(RawBehaviorCenter.X) ||
		!FMath::IsFinite(RawBehaviorCenter.Y) ||
		!FMath::IsFinite(RawBehaviorCenter.Z) ||
		RawBehaviorCenter.GetAbsMax() > HALF_WORLD_MAX;

	if (bBehaviorCenterInvalid)
	{
		// 🔧 修改 - 行为中心无效时回退为当前主将位置，并写回黑板以修复后续逻辑
		// 为什么要写回：避免每帧都走异常分支，保证随机移动可恢复
		const FVector SafeCenter = Dummy->GetActorLocation();
		Blackboard->SetValueAsVector(BehaviorCenterKey, SafeCenter);
		UE_LOG(LogXBAI, Warning, TEXT("假人AI行为中心无效，已回退为主将当前位置: %s"), *Dummy->GetName());
	}

	switch (Dummy->GetDummyMoveMode())
	{
	case EXBLeaderAIMoveMode::Stand:
	{
		// 🔧 修改 - 原地站立模式：回归出生点
		const FVector Center = Blackboard->GetValueAsVector(BehaviorCenterKey);
		const FVector AdjustedCenter = FVector(Center.X, Center.Y, Dummy->GetActorLocation().Z);
		Blackboard->SetValueAsVector(BehaviorDestinationKey, AdjustedCenter);
		
		// ✨ 修复 - 增大停止阈值到100cm，避免卡顿
		const float StopThresholdSq = FMath::Square(100.0f);  // 从10cm改为100cm
		const float DistSq = FVector::DistSquaredXY(Dummy->GetActorLocation(), Center);

		if (DistSq > StopThresholdSq)
		{
			// ✨ 修复 - 直接调用AIController移动，绕过RequestContinuousMove的距离检查
			if (AAIController* AIController = Cast<AAIController>(Dummy->GetController()))
			{
				AIController->MoveToLocation(AdjustedCenter);
			}
		}
		else if (AAIController* AIController = Cast<AAIController>(Dummy->GetController()))
		{
			AIController->StopMovement();
			AIController->ClearFocus(EAIFocusPriority::Gameplay);
		}
		break;
	}
	case EXBLeaderAIMoveMode::Wander:
	{
	
		const FVector CurrentDestination = Blackboard->GetValueAsVector(BehaviorDestinationKey);
		const bool bDestinationInvalid = CurrentDestination.ContainsNaN() ||
			(CurrentDestination.IsNearlyZero() && !Dummy->GetActorLocation().IsNearlyZero());

		const float CurrentTime = Dummy->GetWorld()->GetTimeSeconds();
		if (CurrentTime < NextWanderTime && !bDestinationInvalid)
		{
			return;
		}

		UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Dummy->GetWorld());
		if (!NavSystem)
		{
			Blackboard->SetValueAsVector(BehaviorDestinationKey, Blackboard->GetValueAsVector(BehaviorCenterKey));
			NextWanderTime = CurrentTime + AIConfig.WanderInterval;
			return;
		}

		const FVector BehaviorCenter = Blackboard->GetValueAsVector(BehaviorCenterKey);
		const FVector CurrentLocation = Dummy->GetActorLocation();
		const float MinDistance = AIConfig.MinMoveDistance;
		
		FNavLocation RandomLocation;
		bool bFoundValidPoint = false;
		
		for (int32 Attempt = 0; Attempt < 5 && !bFoundValidPoint; ++Attempt)
		{
			if (NavSystem->GetRandomPointInNavigableRadius(BehaviorCenter, AIConfig.WanderRadius, RandomLocation))
			{
				const float DistToNewPoint = FVector::Dist(CurrentLocation, RandomLocation.Location);
				if (DistToNewPoint >= MinDistance)
				{
					bFoundValidPoint = true;
				}
			}
		}
		
		if (bFoundValidPoint)
		{	
			Blackboard->SetValueAsVector(BehaviorDestinationKey, RandomLocation.Location);
			NextWanderTime = CurrentTime + AIConfig.WanderInterval;
		}
		else
		{
			Blackboard->SetValueAsVector(BehaviorDestinationKey, BehaviorCenter);
			NextWanderTime = CurrentTime + AIConfig.WanderInterval;
		}
		break;
	}
	case EXBLeaderAIMoveMode::Route:
	{
		USplineComponent* SplineComp = Dummy->GetPatrolSplineComponent();
		if (!SplineComp || SplineComp->GetNumberOfSplinePoints() < 2)
		{
			if (!bLoggedMissingSpline)
			{
				UE_LOG(LogXBAI, Warning, TEXT("假人AI未找到有效巡逻路线样条，回退为行动中心: %s"), *Dummy->GetName());
				bLoggedMissingSpline = true;
			}
			Blackboard->SetValueAsVector(BehaviorDestinationKey, Blackboard->GetValueAsVector(BehaviorCenterKey));
			return;
		}

		const FVector MyLoc = Dummy->GetActorLocation();
		const float DistToSpline = FVector::Dist(MyLoc, SplineComp->FindLocationClosestToWorldLocation(MyLoc, ESplineCoordinateSpace::World));
		
		// ✨ 阶段1：回归样条线（Approaching Phase）
		// 如果离路线太远（> 300），则先走到最近点，不使用 LookAhead
		// 目的：避免因远处投影点不稳定导致的“顿挫震荡”，即使开启 Observe 也能平滑靠近
		if (DistToSpline > 300.0f)
		{
			const FVector ClosestPoint = SplineComp->FindLocationClosestToWorldLocation(MyLoc, ESplineCoordinateSpace::World);
			
			// 仅在目标点变化显著时更新（稳定器）
			const FVector OldDest = Blackboard->GetValueAsVector(BehaviorDestinationKey);
			if (FVector::DistSquared(OldDest, ClosestPoint) > FMath::Square(200.0f)) // 更宽松的阈值
			{
				Blackboard->SetValueAsVector(BehaviorDestinationKey, ClosestPoint);
				UE_LOG(LogXBAI, Verbose, TEXT("假人AI回归样条线: Dist=%.1f, To=%s"), DistToSpline, *ClosestPoint.ToString());
			}
			return;
		}

		// ✨ 阶段2：沿样条线前进（Following Phase）
		// 已在路线上，启用 LookAhead 动态目标
		const float CurrentDistanceParams = SplineComp->GetDistanceAlongSplineAtLocation(MyLoc, ESplineCoordinateSpace::World);
		const float TotalLength = SplineComp->GetSplineLength();

		const float MoveSpeed = Dummy->GetCharacterMovement()->GetMaxSpeed(); // 修正：MaxWalkSpeed
		const float LookAheadDistance = FMath::Max(800.0f, MoveSpeed * 1.5f);
		const float EndTolerance = 100.0f;

			// 替换整个if (bIsClosedLoop)代码块
			const bool bIsClosedLoop = SplineComp->IsClosedLoop();
			if (bIsClosedLoop)
			{
				float TargetDistance = CurrentDistanceParams + LookAheadDistance;
				// ✨ 修复 - 确保TargetDistance在有效范围内，避免负数
				if (TargetDistance < 0.0f)
				{
					TargetDistance += TotalLength;
				}
				else if (TargetDistance >= TotalLength)
				{
					TargetDistance = FMath::Fmod(TargetDistance, TotalLength);
				}
    
				const FVector TargetLocation = SplineComp->GetLocationAtDistanceAlongSpline(TargetDistance, ESplineCoordinateSpace::World);
    
				// 阈值检测，避免微小抖动触发重寻路
				const FVector OldDestination = Blackboard->GetValueAsVector(BehaviorDestinationKey);
				if (FVector::DistSquared(OldDestination, TargetLocation) > FMath::Square(150.0f))
				{
					Blackboard->SetValueAsVector(BehaviorDestinationKey, TargetLocation);
				}
				RequestContinuousMove(Dummy, Blackboard->GetValueAsVector(BehaviorDestinationKey));
				break;
			}


			// ✨ 往复行走逻辑：0→1→0循环
			// 检测是否到达终点或起点，自动切换方向
			bool bDirectionChanged = false;
		if (CurrentDistanceParams >= TotalLength - EndTolerance)
		{
			if (bRouteForward)  // 只在首次到达时切换
			{
				bRouteForward = false;
				bDirectionChanged = true;
				UE_LOG(LogXBAI, Log, TEXT("假人AI到达样条线终点，切换为反向行走: %s"), *Dummy->GetName());
			}
		}
		else if (CurrentDistanceParams <= EndTolerance)
		{
			if (!bRouteForward)  // 只在首次到达时切换
			{
				bRouteForward = true;
				bDirectionChanged = true;
				UE_LOG(LogXBAI, Log, TEXT("假人AI到达样条线起点，切换为正向行走: %s"), *Dummy->GetName());
			}
		}			
			// 根据方向计算目标距离
			const float Direction = bRouteForward ? 1.0f : -1.0f;
			float TargetDistance = CurrentDistanceParams + LookAheadDistance * Direction;
			TargetDistance = FMath::Clamp(TargetDistance, 0.0f, TotalLength);

			const FVector TargetLocation = SplineComp->GetLocationAtDistanceAlongSpline(TargetDistance, ESplineCoordinateSpace::World);
			
			// ✨ 修复 - 方向切换时强制更新目标，绕过距离检查
			if (bDirectionChanged)
			{
				Blackboard->SetValueAsVector(BehaviorDestinationKey, TargetLocation);
				// 直接调用AI移动，绕过RequestContinuousMove的距离检查
				if (AAIController* AIController = Cast<AAIController>(Dummy->GetController()))
				{
					AIController->MoveToLocation(TargetLocation);
				}
			}
			else
			{
				// 正常流程使用阈值检测
				const FVector OldDestination = Blackboard->GetValueAsVector(BehaviorDestinationKey);
				if (FVector::DistSquared(OldDestination, TargetLocation) > FMath::Square(150.0f))
				{
					Blackboard->SetValueAsVector(BehaviorDestinationKey, TargetLocation);
				}
				RequestContinuousMove(Dummy, Blackboard->GetValueAsVector(BehaviorDestinationKey));
			}
			break;
	}
	
	case EXBLeaderAIMoveMode::Forward:
	{
		// ✨ 新增 - 向前行走模式
		const float ForwardCheckDistance = 500.0f;
		const FVector CurrentLoc = Dummy->GetActorLocation();
		FVector ForwardDir = Dummy->GetActorForwardVector();
		
		// ✨ 新增 - 定时随机转向逻辑（10-20秒）
		const float CurrentTime = Dummy->GetWorld()->GetTimeSeconds();
		if (CurrentTime >= NextForwardTurnTime)
		{
			// 随机左转或右转45-90度
			const float TurnAngle = FMath::FRandRange(-90.0f, 90.0f);
			const FRotator CurrentRot = Dummy->GetActorRotation();
			const FRotator NewRot = FRotator(0.0f, CurrentRot.Yaw + TurnAngle, 0.0f);
			ForwardDir = NewRot.Vector();
			
			// 设置下次转向时间（使用配置的随机间隔）
			NextForwardTurnTime = CurrentTime + FMath::FRandRange(AIConfig.ForwardTurnIntervalMin, AIConfig.ForwardTurnIntervalMax);
			
			UE_LOG(LogXBAI, Log, TEXT("假人AI定时随机转向: %s, 角度: %.1f"), *Dummy->GetName(), TurnAngle);
		}
		
		const FVector ForwardTarget = CurrentLoc + ForwardDir * ForwardCheckDistance;
		
		// 检查前方是否有可行走路径
		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Dummy->GetWorld());
		if (NavSys)
		{
			FNavLocation ProjectedLoc;
			bool bHasPath = NavSys->ProjectPointToNavigation(ForwardTarget, ProjectedLoc, FVector(200, 200, 500));
			
			if (bHasPath)
			{
				// 前方有路，继续前进
				Blackboard->SetValueAsVector(BehaviorDestinationKey, ProjectedLoc.Location);
				RequestContinuousMove(Dummy, ProjectedLoc.Location);
			}
			else
			{
				// 前方无路，向后旋转180度
				const FRotator CurrentRot = Dummy->GetActorRotation();
				const FRotator NewRot = FRotator(0.0f, CurrentRot.Yaw + 180.0f, 0.0f);
				const FVector NewForwardDir = NewRot.Vector();
				const FVector NewTarget = CurrentLoc + NewForwardDir * ForwardCheckDistance;
				
				if (NavSys->ProjectPointToNavigation(NewTarget, ProjectedLoc, FVector(200, 200, 500)))
				{
					Blackboard->SetValueAsVector(BehaviorDestinationKey, ProjectedLoc.Location);
					RequestContinuousMove(Dummy, ProjectedLoc.Location);
					UE_LOG(LogXBAI, Log, TEXT("假人AI前方无路，向后旋转180度: %s"), *Dummy->GetName());
				}
			}
		}
		break;
	}
	
	default:
		break;
	}
}

void UBTService_XBDummyLeaderAI::RequestContinuousMove(AXBDummyCharacter* Dummy, const FVector& Destination) const
{
	if (!Dummy)
	{
		return;
	}

	AAIController* AIController = Cast<AAIController>(Dummy->GetController());
	if (!AIController)
	{
		return;
	}

	const float DistSq = FVector::DistSquared(Dummy->GetActorLocation(), Destination);
	if (DistSq <= FMath::Square(50.0f))
	{
		return;
	}

	constexpr float AcceptanceRadius = 50.0f;
	AIController->MoveToLocation(
		Destination,
		AcceptanceRadius,
		true,
		true,
		true,
		false,
		nullptr,
		true
	);
}

/**
 * @brief  将路线索引重置到最近样条点
 * @param  Dummy 假人主将
 * @param  Blackboard 黑板组件
 * @param  SplineComp 巡逻样条
 * @return 无
 * @note   详细流程分析: 通过最近点重建路线索引
 *         性能/架构注意事项: 仅在回归时执行
 */
void UBTService_XBDummyLeaderAI::ResetRouteIndexToNearest(AXBDummyCharacter* Dummy, UBlackboardComponent* Blackboard, USplineComponent* SplineComp)
{
	if (!SplineComp)
	{
		return;
	}

	const FName RouteIndexKey = XBDummyLeaderBlackboardKeys::RouteIndex;

	const float InputKey = SplineComp->FindInputKeyClosestToWorldLocation(Dummy->GetActorLocation());
	const int32 ClampedIndex = FMath::Clamp(FMath::RoundToInt(InputKey), 0, SplineComp->GetNumberOfSplinePoints() - 1);
	Blackboard->SetValueAsInt(RouteIndexKey, ClampedIndex);
}
