/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTTask_XBDummyMoveToTarget.cpp

/**
 * @file BTTask_XBDummyMoveToTarget.cpp
 * @brief 假人主将智能移动任务实现
 */

#include "AI/BehaviorTree/BTTask_XBDummyMoveToTarget.h"
#include "AI/XBDummyAIType.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/XBDummyCharacter.h"
#include "Character/XBCharacterBase.h"
#include "Character/Components/XBCombatComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Utils/XBLogCategories.h"

/**
 * @brief 构造函数
 */
UBTTask_XBDummyMoveToTarget::UBTTask_XBDummyMoveToTarget()
{
	// 设置任务名称
	NodeName = TEXT("假人主将智能移动");
	
	// 开启Tick更新
	bNotifyTick = true;
	bNotifyTaskFinished = true;
	
	// 配置默认目标键
	TargetKey.SelectedKeyName = TEXT("TargetLeader");
	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBDummyMoveToTarget, TargetKey), AActor::StaticClass());

	// ✨ 新增 - 配置能力类型键，用于决定移动到哪个攻击范围
	AbilityTypeKey.SelectedKeyName = TEXT("SelectedAbilityType");
	AbilityTypeKey.AddIntFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBDummyMoveToTarget, AbilityTypeKey));
}

/**
 * @brief 执行任务
 */
EBTNodeResult::Type UBTTask_XBDummyMoveToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 获取AI控制器
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人移动任务失败：AI控制器无效"));
		return EBTNodeResult::Failed;
	}

	// 获取假人主将
	AXBDummyCharacter* Dummy = Cast<AXBDummyCharacter>(AIController->GetPawn());
	if (!Dummy)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人移动任务失败：Pawn不是假人主将"));
		return EBTNodeResult::Failed;
	}

	// 获取黑板组件
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人移动任务失败：黑板无效"));
		return EBTNodeResult::Failed;
	}

	// 获取目标
	AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(TargetKey.SelectedKeyName));
	if (!Target)
	{
		return EBTNodeResult::Failed;
	}

	// 🔧 修改 - 获取当前选择的能力类型，确保移动范围与能力一致
	static const FName DefaultAbilityTypeKey(TEXT("SelectedAbilityType"));
	const FName AbilityTypeKeyName = AbilityTypeKey.SelectedKeyName.IsNone()
		? DefaultAbilityTypeKey
		: AbilityTypeKey.SelectedKeyName;
	const EXBDummyLeaderAbilityType SelectedAbilityType =
		static_cast<EXBDummyLeaderAbilityType>(Blackboard->GetValueAsInt(AbilityTypeKeyName));

	// 检查目标有效性
	if (AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Target))
	{
		if (TargetLeader->IsDead() || TargetLeader->IsHiddenInBush())
		{
			UE_LOG(LogXBAI, Verbose, TEXT("假人移动任务：目标无效（死亡/草丛）"));
			return EBTNodeResult::Failed;
		}
	}

	// 获取战斗组件
	UXBCombatComponent* CombatComp = Dummy->GetCombatComponent();
	if (!CombatComp)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人移动任务失败：战斗组件无效"));
		return EBTNodeResult::Failed;
	}

	// 设置焦点
	AIController->SetFocus(Target);

	// 计算最优停止距离
	const float OptimalStopDistance = CalculateOptimalStopDistance(CombatComp, Dummy, Target, SelectedAbilityType);
	
	// 计算当前距离
	const float CurrentDistance = FVector::Dist(Dummy->GetActorLocation(), Target->GetActorLocation());

	// 如果已在范围内，直接成功
	if (CurrentDistance <= OptimalStopDistance)
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		UE_LOG(LogXBAI, Verbose, TEXT("假人 %s 已在攻击范围内(%.1f <= %.1f)"), 
			*Dummy->GetName(), CurrentDistance, OptimalStopDistance);
		return EBTNodeResult::Succeeded;
	}

	// 发起移动请求
	// 🔧 修复 - 直接使用攻击范围作为停止距离，不再缩放
	// 之前使用 StopDistanceScale 导致提前停止，假人无法到达攻击范围
	const float NavStopDistance = FMath::Max(0.0f, OptimalStopDistance - CollisionBuffer);
	
	EPathFollowingRequestResult::Type MoveResult = AIController->MoveToActor(
		Target,
		NavStopDistance,
		true,  // StopOnOverlap
		true,  // UsePathfinding
		true,  // CanStrafe
		nullptr,
		true   // AllowPartialPath
	);

	if (MoveResult == EPathFollowingRequestResult::RequestSuccessful)
	{
		TargetUpdateTimer = 0.0f;
		UE_LOG(LogXBAI, Log, TEXT("假人 %s 开始移动到目标，停止距离=%.1f"), 
			*Dummy->GetName(), NavStopDistance);
		return EBTNodeResult::InProgress;
	}
	else if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		return EBTNodeResult::Succeeded;
	}

	// 无法寻路
	UE_LOG(LogXBAI, Warning, TEXT("假人 %s 无法寻路到目标"), *Dummy->GetName());
	return EBTNodeResult::Failed;
}

/**
 * @brief Tick更新任务
 */
void UBTTask_XBDummyMoveToTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	// 获取AI控制器
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 获取假人主将
	AXBDummyCharacter* Dummy = Cast<AXBDummyCharacter>(AIController->GetPawn());
	if (!Dummy)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 获取黑板
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 获取目标
	AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(TargetKey.SelectedKeyName));
	if (!Target)
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 检查目标有效性
	if (AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Target))
	{
		if (TargetLeader->IsDead() || TargetLeader->IsHiddenInBush())
		{
			AIController->StopMovement();
			AIController->ClearFocus(EAIFocusPriority::Gameplay);
			UE_LOG(LogXBAI, Log, TEXT("假人移动任务中止：目标丢失"));
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}
	}

	// 保持焦点
	AIController->SetFocus(Target);

	// 获取战斗组件
	UXBCombatComponent* CombatComp = Dummy->GetCombatComponent();
	if (!CombatComp)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 计算最优停止距离
	static const FName DefaultAbilityTypeKey(TEXT("SelectedAbilityType"));
	const FName AbilityTypeKeyName = AbilityTypeKey.SelectedKeyName.IsNone()
		? DefaultAbilityTypeKey
		: AbilityTypeKey.SelectedKeyName;
	const EXBDummyLeaderAbilityType SelectedAbilityType =
		static_cast<EXBDummyLeaderAbilityType>(Blackboard->GetValueAsInt(AbilityTypeKeyName));
	const float OptimalStopDistance = CalculateOptimalStopDistance(CombatComp, Dummy, Target, SelectedAbilityType);
	
	// 计算当前距离
	const float CurrentDistance = FVector::Dist(Dummy->GetActorLocation(), Target->GetActorLocation());

	// 如果已到达范围，停止移动并成功
	if (CurrentDistance <= OptimalStopDistance)
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		UE_LOG(LogXBAI, Log, TEXT("假人 %s 到达攻击范围，停止移动"), *Dummy->GetName());
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// 定期更新移动请求（目标可能移动）
	TargetUpdateTimer += DeltaSeconds;
	if (TargetUpdateTimer >= TargetUpdateInterval)
	{
		TargetUpdateTimer = 0.0f;
		
		// 🔧 修复 - 直接使用攻击范围，不缩放
		const float NavStopDistance = FMath::Max(0.0f, OptimalStopDistance - CollisionBuffer);
		AIController->MoveToActor(Target, NavStopDistance, true, true, true, nullptr, true);
	}
}

/**
 * @brief 中止任务
 */
EBTNodeResult::Type UBTTask_XBDummyMoveToTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
	
	return EBTNodeResult::Aborted;
}

/**
 * @brief 获取节点描述
 */
FString UBTTask_XBDummyMoveToTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("智能移动到目标\n目标键: %s"),
		*TargetKey.SelectedKeyName.ToString());
}

/**
 * @brief 计算最优停止距离
 * @note 优先级：已选择能力 > 技能就绪（用技能范围）> 普攻就绪（用普攻范围）> 都冷却（用最大范围等待）
 */
float UBTTask_XBDummyMoveToTarget::CalculateOptimalStopDistance(
	UXBCombatComponent* CombatComp,
	AActor* Dummy,
	AActor* Target,
	EXBDummyLeaderAbilityType SelectedAbilityType) const
{
	if (!CombatComp || !Dummy || !Target)
	{
		return 100.0f; // 默认值
	}

	// 获取碰撞半径
	const float DummyRadius = Dummy->GetSimpleCollisionRadius();
	const float TargetRadius = Target->GetSimpleCollisionRadius();
	const float CollisionRadii = DummyRadius + TargetRadius;

	// 获取技能和普攻的范围与冷却状态
	const float SkillRange = CombatComp->GetSkillAttackRange();
	const float BasicRange = CombatComp->GetBasicAttackRange();
	const bool bSkillOnCooldown = CombatComp->IsSkillOnCooldown();
	const bool bBasicOnCooldown = CombatComp->IsBasicAttackOnCooldown();

	// 🔧 修改 - 优先使用已选择能力的范围，保证移动目标与能力一致
	if (SelectedAbilityType == EXBDummyLeaderAbilityType::SpecialSkill && !bSkillOnCooldown)
	{
		const float StopDistance = SkillRange + CollisionRadii;
		UE_LOG(LogXBAI, Verbose, TEXT("假人移动：已选择技能，使用技能范围=%.1f"), StopDistance);
		return StopDistance;
	}

	if (SelectedAbilityType == EXBDummyLeaderAbilityType::BasicAttack && !bBasicOnCooldown)
	{
		const float StopDistance = BasicRange + CollisionRadii;
		UE_LOG(LogXBAI, Verbose, TEXT("假人移动：已选择普攻，使用普攻范围=%.1f"), StopDistance);
		return StopDistance;
	}

	// 优先级1：技能就绪，使用技能范围（更远）
	if (!bSkillOnCooldown)
	{
		const float StopDistance = SkillRange + CollisionRadii;
		UE_LOG(LogXBAI, Verbose, TEXT("假人移动：技能就绪，选择技能范围=%.1f"), StopDistance);
		return StopDistance;
	}

	// 优先级2：普攻就绪，使用普攻范围
	if (!bBasicOnCooldown)
	{
		const float StopDistance = BasicRange + CollisionRadii;
		UE_LOG(LogXBAI, Verbose, TEXT("假人移动：普攻就绪，选择普攻范围=%.1f"), StopDistance);
		return StopDistance;
	}

	// 优先级3：都在冷却，使用最大范围等待（靠近到技能范围边缘）
	const float MaxRange = FMath::Max(SkillRange, BasicRange);
	const float StopDistance = MaxRange + CollisionRadii;
	UE_LOG(LogXBAI, Verbose, TEXT("假人移动：都在冷却，选择最大范围=%.1f 等待"), StopDistance);
	return StopDistance;
}
