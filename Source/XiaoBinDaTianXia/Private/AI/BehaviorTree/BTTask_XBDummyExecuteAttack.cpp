/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTTask_XBDummyExecuteAttack.cpp

/**
 * @file BTTask_XBDummyExecuteAttack.cpp
 * @brief 行为树任务 - 假人执行受击反击
 * 
 * @note ✨ 新增 - 由黑板触发释放技能/普攻
 */

#include "AI/BehaviorTree/BTTask_XBDummyExecuteAttack.h"
#include "AI/XBDummyAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/XBDummyCharacter.h"
#include "Utils/XBLogCategories.h"

UBTTask_XBDummyExecuteAttack::UBTTask_XBDummyExecuteAttack()
{
	NodeName = TEXT("假人执行受击反击");
}

/**
 * @brief 执行任务
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @return 任务执行结果
 * @note   详细流程分析: 获取控制器/假人 -> 执行攻击 -> 重置黑板标记
 *         性能/架构注意事项: 任务仅负责一次攻击，不循环
 */
EBTNodeResult::Type UBTTask_XBDummyExecuteAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 获取AI控制器
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人攻击任务失败：AI控制器无效"));
		return EBTNodeResult::Failed;
	}

	// 获取假人实例
	AXBDummyCharacter* Dummy = Cast<AXBDummyCharacter>(AIController->GetPawn());
	if (!Dummy)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人攻击任务失败：Pawn不是假人"));
		return EBTNodeResult::Failed;
	}

	// 执行攻击逻辑
	const bool bExecuted = Dummy->ExecuteDamageResponseAttack();

	// 🔧 修改 - 执行后重置黑板标记，避免重复触发
	if (UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent())
	{
		if (const AXBDummyAIController* DummyAI = Cast<AXBDummyAIController>(AIController))
		{
			BlackboardComp->SetValueAsBool(DummyAI->GetDamageResponseKey(), false);
		}
	}

	return bExecuted ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}
