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

	// 计算最优停止距离（纯攻击范围，边缘到边缘）
	const float OptimalStopDistance = CalculateOptimalStopDistance(CombatComp, Dummy, Target, SelectedAbilityType);
	
	// 🔧 关键修复 - 计算边缘距离（中心距离 - 双方碰撞半径）
	// AttackRange 是从自己边缘到目标边缘的距离，所以比较时也要用边缘距离
	const float CenterDistance = FVector::Dist(Dummy->GetActorLocation(), Target->GetActorLocation());
	const float DummyRadius = Dummy->GetSimpleCollisionRadius();
	const float TargetRadius = Target->GetSimpleCollisionRadius();
	const float EdgeDistance = CenterDistance - DummyRadius - TargetRadius;

	// 如果已在范围内（边缘距离 <= 攻击范围），直接成功
	if (EdgeDistance <= OptimalStopDistance)
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		UE_LOG(LogXBAI, Verbose, TEXT("假人 %s 已在攻击范围内(边缘距离=%.1f <= 攻击范围=%.1f)"), 
			*Dummy->GetName(), EdgeDistance, OptimalStopDistance);
		return EBTNodeResult::Succeeded;
	}

	// 发起移动请求
	// 说明：MoveToActor 用中心距离判定，StopOnOverlap 会自行考虑自身碰撞半径
	// 说明：OptimalStopDistance 是边缘距离，不再额外叠加目标半径
	const float AcceptanceRadius = OptimalStopDistance;
	
	EPathFollowingRequestResult::Type MoveResult = AIController->MoveToActor(
		Target,
        AcceptanceRadius,  // 说明：使用边缘距离，StopOnOverlap 会自行考虑自身碰撞半径
		true,  // StopOnOverlap
		true,  // UsePathfinding
		true,  // CanStrafe
		nullptr,
		true   // AllowPartialPath
	);

	if (MoveResult == EPathFollowingRequestResult::RequestSuccessful)
	{
		TargetUpdateTimer = 0.0f;
		UE_LOG(LogXBAI, Log, TEXT("假人 %s 开始移动到目标，攻击范围=%.1f, 导航停止距离=%.1f"), 
			*Dummy->GetName(), OptimalStopDistance, AcceptanceRadius);
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
	
	// 🔧 关键修复 - 计算边缘距离（中心距离 - 双方碰撞半径）
	const float CenterDistance = FVector::Dist(Dummy->GetActorLocation(), Target->GetActorLocation());
	const float DummyRadius = Dummy->GetSimpleCollisionRadius();
	const float TargetRadius = Target->GetSimpleCollisionRadius();
	const float EdgeDistance = CenterDistance - DummyRadius - TargetRadius;

	// 如果已到达范围（边缘距离 <= 攻击范围），停止移动并成功
	if (EdgeDistance <= OptimalStopDistance)
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		UE_LOG(LogXBAI, Log, TEXT("假人 %s 到达攻击范围(边缘距离=%.1f <= 攻击范围=%.1f)，停止移动"), 
			*Dummy->GetName(), EdgeDistance, OptimalStopDistance);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// 定期更新移动请求（目标可能移动）
	TargetUpdateTimer += DeltaSeconds;
	if (TargetUpdateTimer >= TargetUpdateInterval)
	{
		TargetUpdateTimer = 0.0f;
// 说明：MoveToActor 用中心距离判定，StopOnOverlap 会自行考虑自身碰撞半径
		const float AcceptanceRadius = OptimalStopDistance;
		AIController->MoveToActor(Target, AcceptanceRadius, true, true, true, nullptr, true);
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

	// 🔧 关键修复 - 不再加碰撞半径，只使用纯攻击范围
// 说明：OptimalStopDistance 是边缘距离，不再叠加碰撞半径

	// 获取技能和普攻的范围与冷却状态
	const float SkillRange = CombatComp->GetSkillAttackRange();
	const float BasicRange = CombatComp->GetBasicAttackRange();
	const bool bSkillOnCooldown = CombatComp->IsSkillOnCooldown();
	const bool bBasicOnCooldown = CombatComp->IsBasicAttackOnCooldown();

	UE_LOG(LogXBAI, Verbose, TEXT("假人AI技能范围=%.1f, 普攻范围=%.1f"), SkillRange, BasicRange);

	// 🔧 修改 - 直接使用攻击范围作为停止距离，不加碰撞半径
	if (SelectedAbilityType == EXBDummyLeaderAbilityType::SpecialSkill && !bSkillOnCooldown)
	{
		UE_LOG(LogXBAI, Verbose, TEXT("假人移动：已选择技能，停止距离=%.1f"), SkillRange);
		return SkillRange;
	}

	if (SelectedAbilityType == EXBDummyLeaderAbilityType::BasicAttack && !bBasicOnCooldown)
	{
		UE_LOG(LogXBAI, Verbose, TEXT("假人移动：已选择普攻，停止距离=%.1f"), BasicRange);
		return BasicRange;
	}

	// 优先级1：技能就绪，使用技能范围
	if (!bSkillOnCooldown)
	{
		UE_LOG(LogXBAI, Verbose, TEXT("假人移动：技能就绪，停止距离=%.1f"), SkillRange);
		return SkillRange;
	}

	// 优先级2：普攻就绪，使用普攻范围
	if (!bBasicOnCooldown)
	{
		UE_LOG(LogXBAI, Verbose, TEXT("假人移动：普攻就绪，停止距离=%.1f"), BasicRange);
		return BasicRange;
	}

	// 优先级3：都在冷却，使用最大范围等待
	const float MaxRange = FMath::Max(SkillRange, BasicRange);
	UE_LOG(LogXBAI, Verbose, TEXT("假人移动：都在冷却，停止距离=%.1f"), MaxRange);
	return MaxRange;
}
