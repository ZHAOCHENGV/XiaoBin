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
#include "Utils/XBLogCategories.h"

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

	const FName TargetLeaderKey = AIConfig.TargetLeaderKey.IsNone() ? TEXT("TargetLeader") : AIConfig.TargetLeaderKey;
	const FName InCombatKey = AIConfig.InCombatKey.IsNone() ? TEXT("IsInCombat") : AIConfig.InCombatKey;

	// 🔧 修改 - 先读取当前目标，判断是否需要回归
	AXBCharacterBase* CurrentTarget = Cast<AXBCharacterBase>(Blackboard->GetValueAsObject(TargetLeaderKey));
	if (CurrentTarget)
	{
		// 🔧 修改 - 目标进入草丛或全灭时立刻清理目标
		if (CurrentTarget->IsHiddenInBush() || IsLeaderArmyEliminated(CurrentTarget))
		{
			Blackboard->SetValueAsObject(TargetLeaderKey, nullptr);
			Blackboard->SetValueAsBool(InCombatKey, false);
			HandleTargetLost(Dummy, Blackboard);
			bHadCombatTarget = false;
		}
		else
		{
			Blackboard->SetValueAsBool(InCombatKey, true);
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
				bHadCombatTarget = true;

				UE_LOG(LogXBAI, Log, TEXT("假人主将 %s 发现敌方主将并进入战斗: %s"),
					*Dummy->GetName(), *FoundLeader->GetName());
			}
			else
			{
				Blackboard->SetValueAsBool(InCombatKey, false);
				if (bHadCombatTarget)
				{
					// 🔧 修改 - 从战斗回归后重置行为中心
					HandleTargetLost(Dummy, Blackboard);
					bHadCombatTarget = false;
				}
			}
		}
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

	const FName HomeLocationKey = AIConfig.HomeLocationKey.IsNone() ? TEXT("HomeLocation") : AIConfig.HomeLocationKey;
	const FName BehaviorCenterKey = AIConfig.BehaviorCenterKey.IsNone() ? TEXT("BehaviorCenter") : AIConfig.BehaviorCenterKey;
	const FName BehaviorDestinationKey = AIConfig.BehaviorDestinationKey.IsNone() ? TEXT("BehaviorDestination") : AIConfig.BehaviorDestinationKey;
	const FName MoveModeKey = AIConfig.MoveModeKey.IsNone() ? TEXT("MoveMode") : AIConfig.MoveModeKey;
	const FName RouteIndexKey = AIConfig.RouteIndexKey.IsNone() ? TEXT("RoutePointIndex") : AIConfig.RouteIndexKey;
	const FName InCombatKey = AIConfig.InCombatKey.IsNone() ? TEXT("IsInCombat") : AIConfig.InCombatKey;

	// 🔧 修改 - 写入初始位置和行为中心，保证站立/随机移动有基准
	Blackboard->SetValueAsVector(HomeLocationKey, HomeLocation);
	Blackboard->SetValueAsVector(BehaviorCenterKey, HomeLocation);
	Blackboard->SetValueAsEnum(MoveModeKey, static_cast<uint8>(AIConfig.MoveMode));
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
 * @return 无
 * @note   详细流程分析: 重置行为中心 -> 校正路线索引
 *         性能/架构注意事项: 仅在目标切换时执行
 */
void UBTService_XBDummyLeaderAI::HandleTargetLost(AXBDummyCharacter* Dummy, UBlackboardComponent* Blackboard)
{
	const FXBLeaderAIConfig& AIConfig = Dummy->GetLeaderAIConfig();
	const FName BehaviorCenterKey = AIConfig.BehaviorCenterKey.IsNone() ? TEXT("BehaviorCenter") : AIConfig.BehaviorCenterKey;

	// 🔧 修改 - 回归时以当前位置作为行为中心，保证随机移动自然过渡
	Blackboard->SetValueAsVector(BehaviorCenterKey, Dummy->GetActorLocation());

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
	const FName BehaviorDestinationKey = AIConfig.BehaviorDestinationKey.IsNone() ? TEXT("BehaviorDestination") : AIConfig.BehaviorDestinationKey;
	const FName BehaviorCenterKey = AIConfig.BehaviorCenterKey.IsNone() ? TEXT("BehaviorCenter") : AIConfig.BehaviorCenterKey;
	const FName HomeLocationKey = AIConfig.HomeLocationKey.IsNone() ? TEXT("HomeLocation") : AIConfig.HomeLocationKey;
	const FName RouteIndexKey = AIConfig.RouteIndexKey.IsNone() ? TEXT("RoutePointIndex") : AIConfig.RouteIndexKey;

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
			return;
		}

		const FVector BehaviorCenter = Blackboard->GetValueAsVector(BehaviorCenterKey);
		FNavLocation RandomLocation;
		if (NavSystem->GetRandomPointInNavigableRadius(BehaviorCenter, AIConfig.WanderRadius, RandomLocation))
		{
			Blackboard->SetValueAsVector(BehaviorDestinationKey, RandomLocation.Location);
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

	const FXBLeaderAIConfig& AIConfig = Dummy->GetLeaderAIConfig();
	const FName RouteIndexKey = AIConfig.RouteIndexKey.IsNone() ? TEXT("RoutePointIndex") : AIConfig.RouteIndexKey;

	const float InputKey = SplineComp->FindInputKeyClosestToWorldLocation(Dummy->GetActorLocation());
	const int32 ClampedIndex = FMath::Clamp(FMath::RoundToInt(InputKey), 0, SplineComp->GetNumberOfSplinePoints() - 1);
	Blackboard->SetValueAsInt(RouteIndexKey, ClampedIndex);
}
