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
#include "AI/XBSoldierPerceptionSubsystem.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/XBDummyCharacter.h"
#include "Character/XBCharacterBase.h"
#include "NavigationSystem.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Components/SplineComponent.h"
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

	// 🔧 修改 - 同步数据表移动模式到黑板，保证行为树与配置一致
	// 为什么要每帧比对：配置可能在运行时由外部覆盖（例如关卡/玩法热更新）
	if (Blackboard->GetValueAsInt(MoveModeKey) != static_cast<int32>(AIConfig.MoveMode))
	{
		Blackboard->SetValueAsInt(MoveModeKey, static_cast<int32>(AIConfig.MoveMode));
		UE_LOG(LogXBAI, Log, TEXT("假人AI同步MoveMode=%d 到黑板，Dummy=%s"),
			static_cast<int32>(AIConfig.MoveMode), *Dummy->GetName());
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

		// 🔧 修改 - 目标进入草丛或全灭时立刻清理目标
		// 为什么要清理：目标不可见或已全灭时继续追击会造成无效移动/卡死
		if (CurrentTarget->IsHiddenInBush() || IsLeaderArmyEliminated(CurrentTarget))
		{
			// 🔧 修改 - 丢失目标时需要触发正前方行走逻辑
			const bool bShouldForwardMove = true;
			Blackboard->SetValueAsObject(TargetLeaderKey, nullptr);
			Blackboard->SetValueAsBool(InCombatKey, false);
			// 🔧 修改 - 退出战斗时同步主将与士兵状态
			// 为什么要退出战斗：让士兵回归跟随/编队而非继续战斗逻辑
			Dummy->ExitCombat();
			HandleTargetLost(Dummy, Blackboard, bShouldForwardMove);
			bHadCombatTarget = false;
			UE_LOG(LogXBAI, Log, TEXT("假人AI目标丢失，已清理目标并进入回归流程: %s"), *Dummy->GetName());
		}
		else
		{
			Blackboard->SetValueAsBool(InCombatKey, true);
			// 🔧 修改 - 确保目标存在时进入战斗，带动士兵参战
			// 为什么要强制进入战斗：避免黑板与角色状态不一致
			Dummy->EnterCombat();
			bHadCombatTarget = true;

			// 🔧 修改 - 战斗时将行为目的地锁定为目标位置，确保主动靠近
			// 为什么要写入：MoveTo/行为树需要明确目的地，避免仍使用漫游目标
			Blackboard->SetValueAsVector(BehaviorDestinationKey, CurrentTarget->GetActorLocation());
			UE_LOG(LogXBAI, Verbose, TEXT("假人AI战斗靠近目标，更新目的地: %s -> %s"),
				*Dummy->GetName(), *CurrentTarget->GetName());
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

			AXBCharacterBase* FoundLeader = nullptr;
			if (FindEnemyLeader(Dummy, FoundLeader))
			{
				Blackboard->SetValueAsObject(TargetLeaderKey, FoundLeader);
				Blackboard->SetValueAsBool(InCombatKey, true);
				// 🔧 修改 - 进入战斗，确保士兵同步攻击逻辑
				Dummy->EnterCombat();
				bHadCombatTarget = true;

				UE_LOG(LogXBAI, Log, TEXT("假人主将 %s 发现敌方主将并进入战斗: %s"),
					*Dummy->GetName(), *FoundLeader->GetName());
			}
			else
			{
				// 🔧 修改 - 无视野敌人时尝试根据受击来源反击
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
					Blackboard->SetValueAsObject(TargetLeaderKey, DamageLeader);
					Blackboard->SetValueAsBool(InCombatKey, true);
					Dummy->EnterCombat();
					Dummy->ClearLastDamageLeader();
					bHadCombatTarget = true;

					UE_LOG(LogXBAI, Log, TEXT("假人主将 %s 受到伤害后反击主将: %s"),
						*Dummy->GetName(), *DamageLeader->GetName());
					return;
				}

				Blackboard->SetValueAsBool(InCombatKey, false);
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

	// 🔧 修改 - 写入初始位置和行为中心，保证站立/随机移动有基准
	// 为什么要写入：避免黑板初始值为零导致行为目标无效
	Blackboard->SetValueAsVector(HomeLocationKey, HomeLocation);
	Blackboard->SetValueAsVector(BehaviorCenterKey, HomeLocation);
	// 🔧 修改 - 行为树黑板使用整数保存枚举，避免蓝图无法读取C++枚举
	Blackboard->SetValueAsInt(MoveModeKey, static_cast<int32>(AIConfig.MoveMode));
	Blackboard->SetValueAsInt(RouteIndexKey, 0);
	Blackboard->SetValueAsBool(InCombatKey, false);
	Blackboard->SetValueAsVector(BehaviorDestinationKey, HomeLocation);

	UE_LOG(LogXBAI, Log, TEXT("假人AI黑板初始化完成: Dummy=%s, MoveMode=%d"),
		*Dummy->GetName(), static_cast<int32>(AIConfig.MoveMode));
}

/**
 * @brief  搜索视野内敌方主将
 * @param  Dummy 假人主将
 * @param  OutLeader 输出主将
 * @return 是否找到
 * @note   详细流程分析: 感知查询 -> 过滤无效目标 -> 选择最近主将
 *         性能/架构注意事项: 只遍历感知结果
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

	UXBSoldierPerceptionSubsystem* Perception = World->GetSubsystem<UXBSoldierPerceptionSubsystem>();
	if (!Perception)
	{
		return false;
	}

	const FXBLeaderAIConfig& AIConfig = Dummy->GetLeaderAIConfig();
	FXBPerceptionResult Result;
	// 🔧 修改 - 感知查询失败直接返回，避免空结果被误判
	if (!Perception->QueryEnemiesInRadius(Dummy, Dummy->GetActorLocation(), AIConfig.VisionRange, Dummy->GetFaction(), Result))
	{
		return false;
	}

	float BestDistance = MAX_FLT;
	for (AActor* Actor : Result.DetectedEnemies)
	{
		// 🔧 修改 - 只认主将，过滤非主将对象
		AXBCharacterBase* CandidateLeader = Cast<AXBCharacterBase>(Actor);
		if (!CandidateLeader || CandidateLeader == Dummy)
		{
			continue;
		}

		// 🔧 修改 - 草丛隐身或死亡目标不可锁定
		if (CandidateLeader->IsDead() || CandidateLeader->IsHiddenInBush())
		{
			continue;
		}

		const float Distance = FVector::Dist(Dummy->GetActorLocation(), CandidateLeader->GetActorLocation());
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

	// 🔧 修改 - 回归时以当前位置作为行为中心，保证随机移动自然过渡
	// 为什么要设置中心：避免随机移动以旧中心为基准导致越走越偏
	Blackboard->SetValueAsVector(BehaviorCenterKey, Dummy->GetActorLocation());

	// 🔧 修改 - 只有在确实“丢失已有目标”时才进入正前方行走阶段
	if (bForwardMoveAfterLostParam)
	{
		// 🔧 修改 - 设定短时正前方行走，模拟“继续追击的惯性”
		// 为什么要前进：让行为更加自然，避免目标一丢失就原地掉头
		const float ForwardDistance = FMath::Max(AIConfig.WanderRadius, 300.0f);
		const FVector ForwardDestination = Dummy->GetActorLocation() + Dummy->GetActorForwardVector() * ForwardDistance;
		Blackboard->SetValueAsVector(BehaviorDestinationKey, ForwardDestination);

		// 🔧 修改 - 标记阶段并设置结束时间，走完后回到原行为模式
		bForwardMoveAfterLost = true;
		if (UWorld* World = Dummy->GetWorld())
		{
			ForwardMoveEndTime = World->GetTimeSeconds() + FMath::Max(AIConfig.WanderInterval, 0.5f);
		}

		UE_LOG(LogXBAI, Log, TEXT("假人AI目标丢失，进入正前方行走阶段: %s"), *Dummy->GetName());

		// 🔧 修改 - 进入前进行为时先清零随机移动计时，确保阶段结束后可立刻刷新目的地
		NextWanderTime = 0.0f;
	}

	// 🔧 修改 - 非丢失目标场景下清理前进阶段标记，避免影响随机移动
	if (!bForwardMoveAfterLostParam)
	{
		// 为什么要清理：避免历史状态阻塞新的随机移动刷新
		bForwardMoveAfterLost = false;
		ForwardMoveEndTime = 0.0f;
	}

	if (AIConfig.MoveMode == EXBLeaderAIMoveMode::Route)
	{
		if (USplineComponent* SplineComp = Dummy->GetPatrolSplineComponent())
		{
			ResetRouteIndexToNearest(Dummy, Blackboard, SplineComp);
		}
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

	switch (AIConfig.MoveMode)
	{
	case EXBLeaderAIMoveMode::Stand:
	{
		// 🔧 修改 - 原地站立回到初始位置
		// 为什么要回到初始点：保证站立模式的行为可预测
		Blackboard->SetValueAsVector(BehaviorDestinationKey, Blackboard->GetValueAsVector(HomeLocationKey));
		UE_LOG(LogXBAI, Verbose, TEXT("假人AI站立模式更新目的地: %s"), *Dummy->GetName());
		break;
	}
	case EXBLeaderAIMoveMode::Wander:
	{
		// 🔧 修改 - 若当前目的地无效，忽略间隔直接重新计算
		// 为什么要检测无效：避免黑板残留零向量导致无法移动
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
			// 🔧 修改 - 无导航系统时保持当前目的地（回退为行为中心），避免无效向量
			// 为什么要回退中心：中心点通常位于可行走区域，风险更低
			Blackboard->SetValueAsVector(BehaviorDestinationKey, Blackboard->GetValueAsVector(BehaviorCenterKey));
			UE_LOG(LogXBAI, Warning, TEXT("假人AI随机移动失败：无导航系统，回退到行为中心: %s"), *Dummy->GetName());
			NextWanderTime = CurrentTime + AIConfig.WanderInterval;
			return;
		}

		// 🔧 修改 - 使用已修正的行为中心，避免 FLT_MAX 参与随机采样
		const FVector BehaviorCenter = Blackboard->GetValueAsVector(BehaviorCenterKey);
		FNavLocation RandomLocation;
		if (NavSystem->GetRandomPointInNavigableRadius(BehaviorCenter, AIConfig.WanderRadius, RandomLocation))
		{
			Blackboard->SetValueAsVector(BehaviorDestinationKey, RandomLocation.Location);
			NextWanderTime = CurrentTime + AIConfig.WanderInterval;
			UE_LOG(LogXBAI, Verbose, TEXT("假人AI随机移动更新目的地: %s"), *Dummy->GetName());
		}
		else
		{
			// 🔧 修改 - 随机点失败时回退为行为中心，保证目的地有效
			// 为什么要回退：随机点失败时继续使用无效点会导致停滞
			Blackboard->SetValueAsVector(BehaviorDestinationKey, BehaviorCenter);
			UE_LOG(LogXBAI, Warning, TEXT("假人AI随机移动失败：无法找到可行走点，回退到行为中心: %s"), *Dummy->GetName());
			NextWanderTime = CurrentTime + AIConfig.WanderInterval;
		}
		break;
	}
	case EXBLeaderAIMoveMode::Route:
	{
		USplineComponent* SplineComp = Dummy->GetPatrolSplineComponent();
		if (!SplineComp || SplineComp->GetNumberOfSplinePoints() <= 0)
		{
			if (!bLoggedMissingSpline)
			{
				UE_LOG(LogXBAI, Warning, TEXT("假人AI未找到巡逻路线样条，回退为原地站立: %s"), *Dummy->GetName());
				bLoggedMissingSpline = true;
			}

			Blackboard->SetValueAsVector(BehaviorDestinationKey, Blackboard->GetValueAsVector(HomeLocationKey));
			return;
		}

		const int32 PointCount = SplineComp->GetNumberOfSplinePoints();
		int32 CurrentIndex = Blackboard->GetValueAsInt(RouteIndexKey);
		CurrentIndex = FMath::Clamp(CurrentIndex, 0, PointCount - 1);

		const FVector TargetLocation = SplineComp->GetLocationAtSplinePoint(CurrentIndex, ESplineCoordinateSpace::World);
		const float Distance = FVector::Dist(Dummy->GetActorLocation(), TargetLocation);
		if (Distance <= AIConfig.RouteAcceptanceRadius)
		{
			// 🔧 修改 - 到达后切换到下一点
			// 为什么要切换：保证巡逻路线持续前进而非停在原点
			CurrentIndex = (CurrentIndex + 1) % PointCount;
			Blackboard->SetValueAsInt(RouteIndexKey, CurrentIndex);
		}

		const FVector NextLocation = SplineComp->GetLocationAtSplinePoint(CurrentIndex, ESplineCoordinateSpace::World);
		Blackboard->SetValueAsVector(BehaviorDestinationKey, NextLocation);
		UE_LOG(LogXBAI, Verbose, TEXT("假人AI路线模式更新目的地: %s"), *Dummy->GetName());
		break;
	}
	default:
		break;
	}
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
