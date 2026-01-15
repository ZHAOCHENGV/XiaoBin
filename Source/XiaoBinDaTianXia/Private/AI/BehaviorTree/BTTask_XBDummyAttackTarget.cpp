/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTTask_XBDummyAttackTarget.cpp

/**
 * @file BTTask_XBDummyAttackTarget.cpp
 * @brief 行为树任务 - 假人主将攻击目标
 *
 * @note ✨ 新增 - 将假人主将战斗逻辑迁移到行为树任务
 */

#include "AI/BehaviorTree/BTTask_XBDummyAttackTarget.h"
#include "AI/XBDummyAIType.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/Components/XBCombatComponent.h"
#include "Character/XBDummyCharacter.h"
#include "Character/XBCharacterBase.h"
#include "Utils/XBLogCategories.h"

UBTTask_XBDummyAttackTarget::UBTTask_XBDummyAttackTarget()
{
	NodeName = TEXT("假人主将攻击目标");
	TargetKey.SelectedKeyName = TEXT("TargetLeader");
	// ✨ 新增 - 绑定能力类型黑板键，保证攻击与选择一致
	AbilityTypeKey.SelectedKeyName = TEXT("SelectedAbilityType");
	AbilityTypeKey.AddIntFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBDummyAttackTarget, AbilityTypeKey));
	
	// 启用Tick更新，以便等待转向完成
	bNotifyTick = true;
	bNotifyTaskFinished = true;
}

/**
 * @brief 执行任务
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @return 任务执行结果
 * @note   详细流程分析: 获取目标 -> 设置焦点开始转向 -> 返回InProgress等待Tick
 *         性能/架构注意事项: 使用Tick等待转向完成
 */
EBTNodeResult::Type UBTTask_XBDummyAttackTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!AIController || !Blackboard)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人攻击任务失败：AI控制器或黑板无效"));
		return EBTNodeResult::Failed;
	}

	AXBDummyCharacter* Dummy = Cast<AXBDummyCharacter>(AIController->GetPawn());
	if (!Dummy)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人攻击任务失败：Pawn不是假人"));
		return EBTNodeResult::Failed;
	}

	// 🔧 修改 - 黑板键使用默认固定名称，避免依赖数据表配置
	static const FName DefaultTargetLeaderKey(TEXT("TargetLeader"));
	const FName TargetLeaderKey = TargetKey.SelectedKeyName.IsNone()
		? DefaultTargetLeaderKey
		: TargetKey.SelectedKeyName;

	AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Blackboard->GetValueAsObject(TargetLeaderKey));
	if (!TargetLeader || TargetLeader->IsDead() || TargetLeader->IsHiddenInBush())
	{
		UE_LOG(LogXBAI, Verbose, TEXT("假人攻击任务中目标无效，取消执行"));
		return EBTNodeResult::Failed;
	}

	// 🔧 修改 - 读取当前选择的能力类型，确保攻击与移动使用同一能力
	static const FName DefaultAbilityTypeKey(TEXT("SelectedAbilityType"));
	const FName AbilityTypeKeyName = AbilityTypeKey.SelectedKeyName.IsNone()
		? DefaultAbilityTypeKey
		: AbilityTypeKey.SelectedKeyName;
	const EXBDummyLeaderAbilityType SelectedAbilityType =
		static_cast<EXBDummyLeaderAbilityType>(Blackboard->GetValueAsInt(AbilityTypeKeyName));

	if (SelectedAbilityType == EXBDummyLeaderAbilityType::None)
	{
		UE_LOG(LogXBAI, Verbose, TEXT("假人攻击任务失败：未选择可用能力，等待重新选择"));
		return EBTNodeResult::Failed;
	}

	UXBCombatComponent* CombatComp = Dummy->GetCombatComponent();
	if (!CombatComp)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人攻击任务失败：战斗组件无效"));
		return EBTNodeResult::Failed;
	}

	// 检查攻击范围和冷却状态
	const bool bSkillOnCooldown = CombatComp->IsSkillOnCooldown();
	const bool bBasicOnCooldown = CombatComp->IsBasicAttackOnCooldown();
	const bool bInSkillRange = CombatComp->IsTargetInSkillRange(TargetLeader);
	const bool bInBasicRange = CombatComp->IsTargetInBasicAttackRange(TargetLeader);

	// 🔧 修改 - 按已选择能力判断范围与冷却，避免未到对应范围就停下
	const bool bSelectedSkillReady = SelectedAbilityType == EXBDummyLeaderAbilityType::SpecialSkill && !bSkillOnCooldown && bInSkillRange;
	const bool bSelectedBasicReady = SelectedAbilityType == EXBDummyLeaderAbilityType::BasicAttack && !bBasicOnCooldown && bInBasicRange;
	if (!bSelectedSkillReady && !bSelectedBasicReady)
	{
		return EBTNodeResult::Failed;
	}

	// 🔧 关键修改 - 使用SetFocus开始平滑转向
	AIController->SetFocus(TargetLeader);
	
	// 重置转向计时器
	RotationTimer = 0.0f;
	
	UE_LOG(LogXBAI, Log, TEXT("假人 %s 开始转向目标"), *Dummy->GetName());
	
	// 返回InProgress，让Tick检查转向并执行攻击
	return EBTNodeResult::InProgress;
}

/**
 * @brief Tick更新任务 - 检查转向完成后执行攻击
 */
void UBTTask_XBDummyAttackTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!AIController || !Blackboard)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	AXBDummyCharacter* Dummy = Cast<AXBDummyCharacter>(AIController->GetPawn());
	if (!Dummy)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	static const FName DefaultTargetLeaderKey(TEXT("TargetLeader"));
	const FName TargetLeaderKey = TargetKey.SelectedKeyName.IsNone()
		? DefaultTargetLeaderKey
		: TargetKey.SelectedKeyName;

	AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Blackboard->GetValueAsObject(TargetLeaderKey));
	if (!TargetLeader || TargetLeader->IsDead() || TargetLeader->IsHiddenInBush())
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	static const FName DefaultAbilityTypeKey(TEXT("SelectedAbilityType"));
	const FName AbilityTypeKeyName = AbilityTypeKey.SelectedKeyName.IsNone()
		? DefaultAbilityTypeKey
		: AbilityTypeKey.SelectedKeyName;
	const EXBDummyLeaderAbilityType SelectedAbilityType =
		static_cast<EXBDummyLeaderAbilityType>(Blackboard->GetValueAsInt(AbilityTypeKeyName));
	if (SelectedAbilityType == EXBDummyLeaderAbilityType::None)
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// ?? 修改 - 仅使用平面方向计算朝向，避免高度差导致角度无法收敛
	FVector ToTarget = TargetLeader->GetActorLocation() - Dummy->GetActorLocation();
	ToTarget.Z = 0.0f;
	if (!ToTarget.IsNearlyZero())
	{
		// ?? 修改 - 使用匀速插值转向，保证未移动时也能持续转头
		// 公式思路: 以固定角速度逼近目标Yaw，避免移动组件不更新导致转向停滞
		const FRotator DesiredRotation = ToTarget.Rotation();
		const FRotator CurrentRotation = Dummy->GetActorRotation();
		const FRotator TargetRotation(0.0f, DesiredRotation.Yaw, 0.0f);
		const FRotator NewRotation = FMath::RInterpConstantTo(CurrentRotation, TargetRotation, DeltaSeconds, FacingRotationSpeed);
		Dummy->SetActorRotation(NewRotation);
		AIController->SetControlRotation(NewRotation);
	}

	FVector Forward = Dummy->GetActorForwardVector();
	Forward.Z = 0.0f;
	Forward = Forward.GetSafeNormal();
	const FVector ToTargetDir = ToTarget.IsNearlyZero() ? Forward : ToTarget.GetSafeNormal();
	// 点积公式: cosθ = (A·B)/(|A||B|)，用于计算朝向夹角
	const float DotProduct = FVector::DotProduct(Forward, ToTargetDir);
	// 避免数值误差导致Acos无效
	const float ClampedDotProduct = FMath::Clamp(DotProduct, -1.0f, 1.0f);
	const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(ClampedDotProduct));

	// 更新计时器
	RotationTimer += DeltaSeconds;

	// 判断是否已转向目标（角度足够小）
	const bool bIsFacingTarget = AngleDegrees <= FacingAngleThreshold;
	const bool bTimeout = RotationTimer >= MaxRotationWaitTime;

	if (bIsFacingTarget)
	{
		// 转向完成，执行攻击
		UXBCombatComponent* CombatComp = Dummy->GetCombatComponent();
		if (!CombatComp)
		{
			AIController->ClearFocus(EAIFocusPriority::Gameplay);
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}

		const bool bSkillOnCooldown = CombatComp->IsSkillOnCooldown();
		const bool bBasicOnCooldown = CombatComp->IsBasicAttackOnCooldown();
		const bool bInSkillRange = CombatComp->IsTargetInSkillRange(TargetLeader);
		const bool bInBasicRange = CombatComp->IsTargetInBasicAttackRange(TargetLeader);

		// 清除焦点
		AIController->ClearFocus(EAIFocusPriority::Gameplay);

		// 🔧 修改 - 仅释放已选择的能力，确保与移动范围一致
		if (SelectedAbilityType == EXBDummyLeaderAbilityType::SpecialSkill && bInSkillRange && !bSkillOnCooldown)
		{
			CombatComp->PerformSpecialSkill();
			UE_LOG(LogXBAI, Log, TEXT("假人 %s 转向完成(%.1f度)，释放技能"), *Dummy->GetName(), AngleDegrees);
			Blackboard->SetValueAsInt(AbilityTypeKeyName, static_cast<int32>(EXBDummyLeaderAbilityType::None));
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}

		if (SelectedAbilityType == EXBDummyLeaderAbilityType::BasicAttack && bInBasicRange && !bBasicOnCooldown)
		{
			CombatComp->PerformBasicAttack();
			UE_LOG(LogXBAI, Log, TEXT("假人 %s 转向完成(%.1f度)，释放普攻"), *Dummy->GetName(), AngleDegrees);
			Blackboard->SetValueAsInt(AbilityTypeKeyName, static_cast<int32>(EXBDummyLeaderAbilityType::None));
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}

		// 无法攻击
		UE_LOG(LogXBAI, Verbose, TEXT("假人 %s 转向完成但无法攻击"), *Dummy->GetName());
		Blackboard->SetValueAsInt(AbilityTypeKeyName, static_cast<int32>(EXBDummyLeaderAbilityType::None));
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (bTimeout)
	{
		// 超时仍未转向则放弃本次攻击，避免背对出手
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		UE_LOG(LogXBAI, Verbose, TEXT("假人 %s 转向超时，取消本次攻击"), *Dummy->GetName());
		Blackboard->SetValueAsInt(AbilityTypeKeyName, static_cast<int32>(EXBDummyLeaderAbilityType::None));
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	// 否则继续等待转向
}

/**
 * @brief 中止任务
 */
EBTNodeResult::Type UBTTask_XBDummyAttackTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
	
	return EBTNodeResult::Aborted;
}
