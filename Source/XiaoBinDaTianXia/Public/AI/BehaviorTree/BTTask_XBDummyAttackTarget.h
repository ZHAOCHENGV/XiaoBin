/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/BehaviorTree/BTTask_XBDummyAttackTarget.h

/**
 * @file BTTask_XBDummyAttackTarget.h
 * @brief 行为树任务 - 假人主将攻击目标
 *
 * @note ✨ 新增 - 将假人主将战斗逻辑迁移到行为树任务
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_XBDummyAttackTarget.generated.h"

/**
 * @brief 假人主将攻击目标任务
 */
UCLASS()
class XIAOBINDATIANXIA_API UBTTask_XBDummyAttackTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_XBDummyAttackTarget();

protected:
	/**
	 * @brief 执行任务
	 * @param OwnerComp 行为树组件
	 * @param NodeMemory 节点内存
	 * @return 任务执行结果
	 * @note   详细流程分析: 获取目标 -> 检查距离 -> 优先技能后普攻
	 *         性能/架构注意事项: 任务仅执行一次，不循环
	 */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** @brief 目标黑板键（可覆盖数据表默认值） */
	UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "目标键"))
	FBlackboardKeySelector TargetKey;
};
