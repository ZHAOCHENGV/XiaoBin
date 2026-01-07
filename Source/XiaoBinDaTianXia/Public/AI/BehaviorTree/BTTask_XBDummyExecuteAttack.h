/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/BehaviorTree/BTTask_XBDummyExecuteAttack.h

/**
 * @file BTTask_XBDummyExecuteAttack.h
 * @brief 行为树任务 - 假人执行受击反击
 * 
 * @note ✨ 新增 - 由黑板触发释放技能/普攻
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_XBDummyExecuteAttack.generated.h"

/**
 * @brief 假人执行受击反击任务
 * @note 读取假人实例并触发攻击执行
 */
UCLASS()
class XIAOBINDATIANXIA_API UBTTask_XBDummyExecuteAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_XBDummyExecuteAttack();

protected:
	/**
	 * @brief 执行任务
	 * @param OwnerComp 行为树组件
	 * @param NodeMemory 节点内存
	 * @return 任务执行结果
	 * @note   详细流程分析: 获取控制器/假人 -> 执行攻击 -> 重置黑板标记
	 *         性能/架构注意事项: 任务仅负责一次攻击，不循环
	 */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
