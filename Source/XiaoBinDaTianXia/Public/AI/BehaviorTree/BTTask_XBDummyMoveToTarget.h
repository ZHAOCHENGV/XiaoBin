/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/BehaviorTree/BTTask_XBDummyMoveToTarget.h

/**
 * @file BTTask_XBDummyMoveToTarget.h
 * @brief 行为树任务 - 假人主将智能移动到目标
 *
 * @note ✨ 新增文件
 *       核心功能：
 *       1. 根据技能/普攻范围动态选择停止距离
 *       2. 优先使用技能范围（更远），技能冷却时用普攻范围
 *       3. 到达范围后停止，交给攻击任务执行
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_XBDummyMoveToTarget.generated.h"

class UXBCombatComponent;

/**
 * @brief 假人主将智能移动任务
 * @note 与士兵版本的区别：根据技能/普攻的独立范围动态调整停止距离
 */
UCLASS()
class XIAOBINDATIANXIA_API UBTTask_XBDummyMoveToTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	/**
	 * @brief 构造函数
	 */
	UBTTask_XBDummyMoveToTarget();

	/**
	 * @brief 执行任务
	 * @param OwnerComp 行为树组件
	 * @param NodeMemory 节点内存
	 * @return 任务执行结果
	 * @note 详细流程：获取目标 -> 计算最优停止距离 -> 判断是否已到达 -> 发起移动
	 */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/**
	 * @brief Tick更新任务
	 * @param OwnerComp 行为树组件
	 * @param NodeMemory 节点内存
	 * @param DeltaSeconds 帧间隔
	 * @note 详细流程：检查目标有效性 -> 检查距离 -> 定期更新移动请求
	 */
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/**
	 * @brief 中止任务
	 * @param OwnerComp 行为树组件
	 * @param NodeMemory 节点内存
	 * @return 中止结果
	 * @note 详细流程：停止移动 -> 清理焦点
	 */
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/**
	 * @brief 获取节点描述
	 * @return 描述字符串
	 */
	virtual FString GetStaticDescription() const override;

protected:
	/** @brief 目标黑板键 */
	UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "目标键"))
	FBlackboardKeySelector TargetKey;

	/** @brief 停止距离缩放（留出误差缓冲，避免边界抖动）*/
	UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "停止距离缩放", ClampMin = "0.5", ClampMax = "0.95"))
	float StopDistanceScale = 0.85f;

	/** @brief 目标位置更新间隔 */
	UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "位置更新间隔", ClampMin = "0.1"))
	float TargetUpdateInterval = 0.3f;

private:
	/**
	 * @brief 计算最优停止距离
	 * @param CombatComp 战斗组件
	 * @param Dummy 假人主将
	 * @param Target 目标Actor
	 * @return 最优停止距离（考虑技能/普攻范围和冷却状态）
	 * @note 优先级：技能就绪 > 普攻就绪 > 最大范围等待
	 */
	float CalculateOptimalStopDistance(UXBCombatComponent* CombatComp, AActor* Dummy, AActor* Target) const;

	/** @brief 目标位置更新计时器 */
	float TargetUpdateTimer = 0.0f;
};
