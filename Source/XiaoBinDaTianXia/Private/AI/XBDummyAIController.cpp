/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/XBDummyAIController.cpp

/**
 * @file XBDummyAIController.cpp
 * @brief 假人AI控制器实现
 * 
 * @note ✨ 新增 - 行为树启动与受击响应黑板写入
 */

#include "AI/XBDummyAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AI/XBSoldierPerceptionSubsystem.h"
#include "Character/XBDummyCharacter.h"
#include "Character/XBCharacterBase.h"
#include "Character/Components/XBCombatComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "NavigationSystem.h"
#include "Components/SplineComponent.h"
#include "Utils/XBLogCategories.h"

AXBDummyAIController::AXBDummyAIController()
{
	bAttachToPawn = true;
	PrimaryActorTick.bCanEverTick = true;
}

void AXBDummyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 🔧 修改 - 初始化假人主将AI配置
	if (AXBDummyCharacter* Dummy = Cast<AXBDummyCharacter>(InPawn))
	{
		InitializeLeaderAI(Dummy);
	}

	// 🔧 修改 - 启动行为树，确保假人AI逻辑可运行
	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
		UE_LOG(LogXBAI, Log, TEXT("假人AI控制器启动行为树: %s"), *BehaviorTreeAsset->GetName());
	}
	else
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人AI控制器未配置行为树资源"));
	}
}

void AXBDummyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TickLeaderAI(DeltaSeconds);
}

/**
 * @brief  初始化假人主将AI配置
 * @param  Dummy 假人主将
 * @return 无
 * @note   详细流程分析: 缓存配置 -> 初始化状态 -> 记录初始位置
 *         性能/架构注意事项: 仅在首次Possess时调用
 */
void AXBDummyAIController::InitializeLeaderAI(AXBDummyCharacter* Dummy)
{
	if (!Dummy)
	{
		return;
	}

	// 🔧 修改 - 缓存配置，避免每帧读取数据表
	CachedAIConfig = Dummy->GetLeaderAIConfig();
	HomeLocation = Dummy->GetActorLocation();
	BehaviorCenterLocation = HomeLocation;
	CurrentRoutePointIndex = 0;
	CurrentTargetLeader = nullptr;
	CurrentState = EXBDummyLeaderAIState::Behavior;
	bLeaderAIInitialized = true;

	// ✨ 新增 - 缓存巡逻路线样条
	CachedPatrolSpline = Dummy->GetPatrolSplineComponent();

	if (CachedAIConfig.MoveMode == EXBLeaderAIMoveMode::Route && !CachedPatrolSpline.IsValid())
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人AI未找到巡逻路线样条，回退为原地站立: %s"), *Dummy->GetName());
		CachedAIConfig.MoveMode = EXBLeaderAIMoveMode::Stand;
	}
}

/**
 * @brief  更新假人主将AI主循环
 * @param  DeltaSeconds 帧间隔
 * @return 无
 * @note   详细流程分析: 目标侦测 -> 战斗逻辑 -> 行为模式
 *         性能/架构注意事项: 使用时间间隔降低感知开销
 */
void AXBDummyAIController::TickLeaderAI(float DeltaSeconds)
{
	AXBDummyCharacter* Dummy = GetDummyPawn();
	if (!Dummy || Dummy->IsDead() || !bLeaderAIInitialized || !CachedAIConfig.bEnableAI)
	{
		return;
	}

	UpdateCombatTarget();

	if (CurrentState == EXBDummyLeaderAIState::Combat)
	{
		if (AXBCharacterBase* TargetLeader = CurrentTargetLeader.Get())
		{
			MoveToActor(TargetLeader, CachedAIConfig.WanderAcceptanceRadius, true);

			// 🔧 修改 - 目标进入草丛后立即丢失追踪并回归行为
			if (TargetLeader->IsHiddenInBush())
			{
				ExitCombatState();
				return;
			}

			if (UXBCombatComponent* CombatComp = Dummy->GetCombatComponent())
			{
				if (CombatComp->IsTargetInRange(TargetLeader))
				{
					// 🔧 修改 - 优先技能，冷却后释放普攻
					if (!CombatComp->IsSkillOnCooldown())
					{
						CombatComp->PerformSpecialSkill();
					}
					else if (!CombatComp->IsBasicAttackOnCooldown())
					{
						CombatComp->PerformBasicAttack();
					}
				}
			}
		}
		return;
	}

	UpdateBehaviorMovement(DeltaSeconds);
}

/**
 * @brief  侦测并维护目标主将
 * @return 无
 * @note   详细流程分析: 目标有效性检测 -> 搜索敌方主将 -> 状态切换
 *         性能/架构注意事项: 仅按间隔查询感知子系统
 */
void AXBDummyAIController::UpdateCombatTarget()
{
	AXBDummyCharacter* Dummy = GetDummyPawn();
	if (!Dummy)
	{
		return;
	}

	if (AXBCharacterBase* TargetLeader = CurrentTargetLeader.Get())
	{
		// 🔧 修改 - 目标进入草丛或全灭后回归行为
		if (TargetLeader->IsHiddenInBush() || IsLeaderArmyEliminated(TargetLeader))
		{
			ExitCombatState();
		}
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < NextSearchTime)
	{
		return;
	}
	NextSearchTime = CurrentTime + CachedAIConfig.TargetSearchInterval;

	AXBCharacterBase* FoundLeader = nullptr;
	if (FindEnemyLeaderInRange(FoundLeader))
	{
		EnterCombatState(FoundLeader);
	}
}

/**
 * @brief  尝试获取视野内敌方主将
 * @param  OutLeader 输出主将
 * @return 是否找到
 * @note   详细流程分析: 感知查询 -> 过滤阵营/死亡/草丛 -> 选择最近主将
 *         性能/架构注意事项: 只遍历查询结果而非全局列表
 */
bool AXBDummyAIController::FindEnemyLeaderInRange(AXBCharacterBase*& OutLeader)
{
	OutLeader = nullptr;

	AXBDummyCharacter* Dummy = GetDummyPawn();
	if (!Dummy)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UXBSoldierPerceptionSubsystem* Perception = World->GetSubsystem<UXBSoldierPerceptionSubsystem>();
	if (!Perception)
	{
		return false;
	}

	FXBPerceptionResult Result;
	if (!Perception->QueryEnemiesInRadius(Dummy, Dummy->GetActorLocation(), CachedAIConfig.VisionRange, Dummy->GetFaction(), Result))
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
bool AXBDummyAIController::IsLeaderArmyEliminated(AXBCharacterBase* Leader) const
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
 * @brief  进入战斗状态
 * @param  TargetLeader 目标主将
 * @return 无
 * @note   详细流程分析: 缓存目标 -> 触发EnterCombat
 *         性能/架构注意事项: 仅在目标有效时调用
 */
void AXBDummyAIController::EnterCombatState(AXBCharacterBase* TargetLeader)
{
	AXBDummyCharacter* Dummy = GetDummyPawn();
	if (!Dummy || !TargetLeader)
	{
		return;
	}

	CurrentTargetLeader = TargetLeader;
	CurrentState = EXBDummyLeaderAIState::Combat;
	Dummy->EnterCombat();

	UE_LOG(LogXBAI, Log, TEXT("假人主将 %s 发现敌方主将并进入战斗: %s"),
		*Dummy->GetName(), *TargetLeader->GetName());
}

/**
 * @brief  退出战斗状态并回归行为
 * @return 无
 * @note   详细流程分析: 清理目标 -> 触发ExitCombat -> 重置行为中心
 *         性能/架构注意事项: 退出时停止移动，防止路径残留
 */
void AXBDummyAIController::ExitCombatState()
{
	AXBDummyCharacter* Dummy = GetDummyPawn();
	if (!Dummy)
	{
		return;
	}

	CurrentTargetLeader = nullptr;
	CurrentState = EXBDummyLeaderAIState::Behavior;
	Dummy->ExitCombat();
	StopMovement();

	// 🔧 修改 - 范围内随机移动回归时以当前位置为中心
	BehaviorCenterLocation = Dummy->GetActorLocation();

	// ✨ 新增 - 路线模式下回归最近路线点
	if (CachedAIConfig.MoveMode == EXBLeaderAIMoveMode::Route)
	{
		if (USplineComponent* SplineComp = CachedPatrolSpline.Get())
		{
			const float InputKey = SplineComp->FindInputKeyClosestToWorldLocation(Dummy->GetActorLocation());
			CurrentRoutePointIndex = FMath::Clamp(FMath::RoundToInt(InputKey), 0, SplineComp->GetNumberOfSplinePoints() - 1);
		}
	}

	UE_LOG(LogXBAI, Log, TEXT("假人主将 %s 退出战斗并回归行为"), *Dummy->GetName());
}

/**
 * @brief  更新行为模式移动
 * @param  DeltaSeconds 帧间隔
 * @return 无
 * @note   详细流程分析: 根据配置选择站立/随机/路线
 *         性能/架构注意事项: 通过间隔控制随机移动频率
 */
void AXBDummyAIController::UpdateBehaviorMovement(float DeltaSeconds)
{
	switch (CachedAIConfig.MoveMode)
	{
	case EXBLeaderAIMoveMode::Stand:
		UpdateStandBehavior();
		break;
	case EXBLeaderAIMoveMode::Wander:
		UpdateWanderBehavior();
		break;
	case EXBLeaderAIMoveMode::Route:
		UpdateRouteBehavior();
		break;
	default:
		break;
	}
}

/**
 * @brief  更新原地站立行为
 * @return 无
 * @note   详细流程分析: 回到初始位置 -> 到位后停止移动
 *         性能/架构注意事项: 使用接受半径避免频繁微调
 */
void AXBDummyAIController::UpdateStandBehavior()
{
	AXBDummyCharacter* Dummy = GetDummyPawn();
	if (!Dummy)
	{
		return;
	}

	const float Distance = FVector::Dist(Dummy->GetActorLocation(), HomeLocation);
	if (Distance <= CachedAIConfig.StandReturnRadius)
	{
		StopMovement();
		return;
	}

	MoveToLocation(HomeLocation, CachedAIConfig.StandReturnRadius);
}

/**
 * @brief  更新范围内随机移动行为
 * @return 无
 * @note   详细流程分析: 按间隔取随机点 -> MoveToLocation
 *         性能/架构注意事项: 仅在间隔到达时计算随机点
 */
void AXBDummyAIController::UpdateWanderBehavior()
{
	AXBDummyCharacter* Dummy = GetDummyPawn();
	if (!Dummy)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < NextWanderTime)
	{
		return;
	}

	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSystem)
	{
		return;
	}

	FNavLocation RandomLocation;
	if (NavSystem->GetRandomPointInNavigableRadius(BehaviorCenterLocation, CachedAIConfig.WanderRadius, RandomLocation))
	{
		MoveToLocation(RandomLocation.Location, CachedAIConfig.WanderAcceptanceRadius);
		NextWanderTime = CurrentTime + CachedAIConfig.WanderInterval;
	}
}

/**
 * @brief  更新路线巡逻行为
 * @return 无
 * @note   详细流程分析: 移动至路线点 -> 到达后切换下一个点
 *         性能/架构注意事项: 路线为空时回退为站立
 */
void AXBDummyAIController::UpdateRouteBehavior()
{
	AXBDummyCharacter* Dummy = GetDummyPawn();
	if (!Dummy)
	{
		return;
	}

	USplineComponent* SplineComp = CachedPatrolSpline.Get();
	if (!SplineComp || SplineComp->GetNumberOfSplinePoints() <= 0)
	{
		UpdateStandBehavior();
		return;
	}

	const int32 PointCount = SplineComp->GetNumberOfSplinePoints();
	CurrentRoutePointIndex = FMath::Clamp(CurrentRoutePointIndex, 0, PointCount - 1);

	const FVector TargetLocation = SplineComp->GetLocationAtSplinePoint(CurrentRoutePointIndex, ESplineCoordinateSpace::World);
	const float Distance = FVector::Dist(Dummy->GetActorLocation(), TargetLocation);
	if (Distance <= CachedAIConfig.RouteAcceptanceRadius)
	{
		// 🔧 修改 - 到达后切换到下一点
		CurrentRoutePointIndex = (CurrentRoutePointIndex + 1) % PointCount;
	}

	const FVector NextLocation = SplineComp->GetLocationAtSplinePoint(CurrentRoutePointIndex, ESplineCoordinateSpace::World);
	MoveToLocation(NextLocation, CachedAIConfig.RouteAcceptanceRadius);
}

/**
 * @brief  获取假人主将
 * @return 假人主将指针
 * @note   详细流程分析: 从控制器Pawn安全转换
 *         性能/架构注意事项: 每帧访问可接受
 */
AXBDummyCharacter* AXBDummyAIController::GetDummyPawn() const
{
	return Cast<AXBDummyCharacter>(GetPawn());
}

void AXBDummyAIController::SetDamageResponseReady(bool bReady)
{
	// 🔧 修改 - 使用黑板同步受击响应状态
	if (UBlackboardComponent* BlackboardComp = GetBlackboardComponent())
	{
		BlackboardComp->SetValueAsBool(DamageResponseKey, bReady);
		UE_LOG(LogXBAI, Log, TEXT("假人AI黑板受击响应标记=%s"), bReady ? TEXT("true") : TEXT("false"));
	}
	else
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人AI控制器黑板无效，无法写入受击响应标记"));
	}
}
