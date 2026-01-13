/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTService_XBDummyLeaderAI.cpp

/**
 * @file BTService_XBDummyLeaderAI.cpp
 * @brief 行为树服务 - 假人主将AI状态更新
 *
 * @note ✨ 新增 - 负责目标搜索/回归/行为目的地更新
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

	// 🔧 修改 - 同步数据表移动模式到黑板，保证行为树与配置一致
	if (Blackboard->GetValueAsInt(MoveModeKey) != static_cast<int32>(AIConfig.MoveMode))
	{
		Blackboard->SetValueAsInt(MoveModeKey, static_cast<int32>(AIConfig.MoveMode));
		UE_LOG(LogXBAI, Log, TEXT("假人AI同步MoveMode=%d 到黑板，Dummy=%s"),
			static_cast<int32>(AIConfig.MoveMode), *Dummy->GetName());
	}

	// 🔧 修改 - 先读取当前目标，判断是否需要回归
	AXBCharacterBase* CurrentTarget = Cast<AXBCharacterBase>(Blackboard->GetValueAsObject(TargetLeaderKey));
	if (CurrentTarget)
	{
		// 🔧 修改 - 目标进入草丛或全灭时立刻清理目标
		if (CurrentTarget->IsHiddenInBush() || IsLeaderArmyEliminated(CurrentTarget))
		{
			// 🔧 修改 - 丢失目标时需要触发正前方行走逻辑
			const bool bShouldForwardMove = true;
			Blackboard->SetValueAsObject(TargetLeaderKey, nullptr);
			Blackboard->SetValueAsBool(InCombatKey, false);
			// 🔧 修改 - 退出战斗时同步主将与士兵状态
			Dummy->ExitCombat();
			HandleTargetLost(Dummy, Blackboard, bShouldForwardMove);
			bHadCombatTarget = false;
		}
		else
		{
			Blackboard->SetValueAsBool(InCombatKey, true);
			// 🔧 修改 - 确保目标存在时进入战斗，带动士兵参战
			Dummy->EnterCombat();
			bHadCombatTarget = true;
		}
	}

	// 🔧 修改 - 重新读取目标，避免目标清理后仍使用旧指针
	CurrentTarget = Cast<AXBCharacterBase>(Blackboard->GetValueAsObject(TargetLeaderKey));

	// 🔧 修改 - 无目标时按间隔搜索敌方主将
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
					Dummy->ExitCombat();
					HandleTargetLost(Dummy, Blackboard, false);
					bHadCombatTarget = false;
				}
			}
		}
	}

	// ✨ 新增 - 处于“丢失目标后正前方移动”阶段时，先走完再恢复常规移动模式
	if (bForwardMoveAfterLost)
	{
		const float CurrentTime = Dummy->GetWorld()->GetTimeSeconds();
		if (CurrentTime < ForwardMoveEndTime)
		{
			return;
		}

		// 🔧 修改 - 结束正前方移动后恢复常规行为更新
		bForwardMoveAfterLost = false;
	}

	// 🔧 修改 - 非战斗状态下更新行为目的地
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
	Blackboard->SetValueAsVector(HomeLocationKey, HomeLocation);
	Blackboard->SetValueAsVector(BehaviorCenterKey, HomeLocation);
	// 🔧 修改 - 行为树黑板使用整数保存枚举，避免蓝图无法读取C++枚举
	Blackboard->SetValueAsInt(MoveModeKey, static_cast<int32>(AIConfig.MoveMode));
	Blackboard->SetValueAsInt(RouteIndexKey, 0);
	Blackboard->SetValueAsBool(InCombatKey, false);
	Blackboard->SetValueAsVector(BehaviorDestinationKey, HomeLocation);
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
	if (!Perception->QueryEnemiesInRadius(Dummy, Dummy->GetActorLocation(), AIConfig.VisionRange, Dummy->GetFaction(), Result))
	{
		return false;
	}

	float BestDistance = MAX_FLT;
	for (AActor* Actor : Result.DetectedEnemies)
	{
		AXBCharacterBase* CandidateLeader = Cast<AXBCharacterBase>(Actor);
		if (!CandidateLeader || CandidateLeader == Dummy)
		{
			continue;
		}

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
	Blackboard->SetValueAsVector(BehaviorCenterKey, Dummy->GetActorLocation());

	// 🔧 修改 - 只有在确实“丢失已有目标”时才进入正前方行走阶段
	if (bForwardMoveAfterLostParam)
	{
		// 🔧 修改 - 设定短时正前方行走，模拟“继续追击的惯性”
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

	switch (AIConfig.MoveMode)
	{
	case EXBLeaderAIMoveMode::Stand:
	{
		// 🔧 修改 - 原地站立回到初始位置
		Blackboard->SetValueAsVector(BehaviorDestinationKey, Blackboard->GetValueAsVector(HomeLocationKey));
		break;
	}
	case EXBLeaderAIMoveMode::Wander:
	{
		const float CurrentTime = Dummy->GetWorld()->GetTimeSeconds();
		if (CurrentTime < NextWanderTime)
		{
			return;
		}

		UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Dummy->GetWorld());
		if (!NavSystem)
		{
			// 🔧 修改 - 无导航系统时保持当前目的地（回退为行为中心），避免无效向量
			Blackboard->SetValueAsVector(BehaviorDestinationKey, Blackboard->GetValueAsVector(BehaviorCenterKey));
			UE_LOG(LogXBAI, Warning, TEXT("假人AI随机移动失败：无导航系统，回退到行为中心: %s"), *Dummy->GetName());
			NextWanderTime = CurrentTime + AIConfig.WanderInterval;
			return;
		}

		const FVector BehaviorCenter = Blackboard->GetValueAsVector(BehaviorCenterKey);
		FNavLocation RandomLocation;
		if (NavSystem->GetRandomPointInNavigableRadius(BehaviorCenter, AIConfig.WanderRadius, RandomLocation))
		{
			Blackboard->SetValueAsVector(BehaviorDestinationKey, RandomLocation.Location);
			NextWanderTime = CurrentTime + AIConfig.WanderInterval;
		}
		else
		{
			// 🔧 修改 - 随机点失败时回退为行为中心，保证目的地有效
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
			CurrentIndex = (CurrentIndex + 1) % PointCount;
			Blackboard->SetValueAsInt(RouteIndexKey, CurrentIndex);
		}

		const FVector NextLocation = SplineComp->GetLocationAtSplinePoint(CurrentIndex, ESplineCoordinateSpace::World);
		Blackboard->SetValueAsVector(BehaviorDestinationKey, NextLocation);
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
