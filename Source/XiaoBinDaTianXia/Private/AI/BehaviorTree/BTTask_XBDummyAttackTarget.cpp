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
 * @note   详细流程分析: 获取目标 -> 检查距离 -> 优先技能后普攻
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

	const FXBLeaderAIConfig& AIConfig = Dummy->GetLeaderAIConfig();
	const FName TargetLeaderKey = TargetKey.SelectedKeyName.IsNone()
		? (AIConfig.TargetLeaderKey.IsNone() ? TEXT("TargetLeader") : AIConfig.TargetLeaderKey)
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

	// 🔧 修改 - 仅在攻击范围内触发技能/普攻
	if (!CombatComp->IsTargetInRange(TargetLeader))
	{
		return EBTNodeResult::Failed;
	}

	// 🔧 修改 - 优先技能，冷却后释放普攻
	if (!CombatComp->IsSkillOnCooldown())
	{
		CombatComp->PerformSpecialSkill();
		return EBTNodeResult::Succeeded;
	}

	if (!CombatComp->IsBasicAttackOnCooldown())
	{
		CombatComp->PerformBasicAttack();
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}
