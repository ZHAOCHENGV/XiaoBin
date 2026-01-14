/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTTask_XBDummyAttackTarget.cpp

/**
 * @file BTTask_XBDummyAttackTarget.cpp
 * @brief 行为树任务 - 假人主将攻击目标
 *
 * @note ✨ 新增 - 将假人主将战斗逻辑迁移到行为树任务
 */

#include "AI/BehaviorTree/BTTask_XBDummyAttackTarget.h"
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
}

/**
 * @brief 执行任务
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @return 任务执行结果
 * @note   详细流程分析: 获取目标 -> 检查技能范围/冷却 -> 检查普攻范围/冷却 -> 释放能力
 *         性能/架构注意事项: 任务仅执行一次，不循环
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

	UXBCombatComponent* CombatComp = Dummy->GetCombatComponent();
	if (!CombatComp)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人攻击任务失败：战斗组件无效"));
		return EBTNodeResult::Failed;
	}

	// ✨ 新增 - 获取攻击范围和冷却状态
	const bool bSkillOnCooldown = CombatComp->IsSkillOnCooldown();
	const bool bBasicOnCooldown = CombatComp->IsBasicAttackOnCooldown();
	const bool bInSkillRange = CombatComp->IsTargetInSkillRange(TargetLeader);
	const bool bInBasicRange = CombatComp->IsTargetInBasicAttackRange(TargetLeader);

	// 🔧 修改 - 直接设置旋转朝向目标，而非使用异步的SetFocus
	// SetFocus是渐变转向，无法保证攻击前完成转向
	const FVector ToTarget = TargetLeader->GetActorLocation() - Dummy->GetActorLocation();
	if (!ToTarget.IsNearlyZero())
	{
		// 只旋转Yaw轴（水平方向），保持Pitch和Roll为0
		const FRotator TargetRotation = FRotationMatrix::MakeFromX(ToTarget).Rotator();
		const FRotator NewRotation(0.0f, TargetRotation.Yaw, 0.0f);
		Dummy->SetActorRotation(NewRotation);
		
		UE_LOG(LogXBAI, Verbose, TEXT("假人 %s 转向目标，Yaw=%.1f"), 
			*Dummy->GetName(), NewRotation.Yaw);
	}

	// ✨ 新增 - 优先检查技能：在范围内且不冷却
	if (bInSkillRange && !bSkillOnCooldown)
	{
		CombatComp->PerformSpecialSkill();
		UE_LOG(LogXBAI, Log, TEXT("假人 %s 释放技能"), *Dummy->GetName());
		return EBTNodeResult::Succeeded;
	}

	// ✨ 新增 - 检查普攻：在范围内且不冷却
	if (bInBasicRange && !bBasicOnCooldown)
	{
		CombatComp->PerformBasicAttack();
		UE_LOG(LogXBAI, Log, TEXT("假人 %s 释放普攻"), *Dummy->GetName());
		return EBTNodeResult::Succeeded;
	}

	// ✨ 新增 - 两者都在冷却或不在范围内，返回失败让行为树继续靠近
	UE_LOG(LogXBAI, Verbose, TEXT("假人 %s 无法攻击: 技能冷却=%s 普攻冷却=%s 技能范围=%s 普攻范围=%s"),
		*Dummy->GetName(),
		bSkillOnCooldown ? TEXT("是") : TEXT("否"),
		bBasicOnCooldown ? TEXT("是") : TEXT("否"),
		bInSkillRange ? TEXT("是") : TEXT("否"),
		bInBasicRange ? TEXT("是") : TEXT("否"));

	return EBTNodeResult::Failed;
}

