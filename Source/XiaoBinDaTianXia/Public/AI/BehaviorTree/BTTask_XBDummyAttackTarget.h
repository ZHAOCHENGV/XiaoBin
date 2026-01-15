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
	 * @note   详细流程分析: 获取目标 -> 设置焦点转向 -> 返回InProgress等待转向
	 *         性能/架构注意事项: 任务会Tick直到转向完成后攻击
	 */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/**
	 * @brief Tick更新任务
	 * @param OwnerComp 行为树组件
	 * @param NodeMemory 节点内存
	 * @param DeltaSeconds 帧间隔
	 * @note   检查转向是否完成，完成后执行攻击
	 */
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/**
	 * @brief 中止任务
	 * @param OwnerComp 行为树组件
	 * @param NodeMemory 节点内存
	 * @return 中止结果
	 */
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** @brief 目标黑板键（可覆盖数据表默认值） */
	UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "目标键"))
	FBlackboardKeySelector TargetKey;

	/** @brief 选择的能力类型黑板键 */
	UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "能力类型键"))
	FBlackboardKeySelector AbilityTypeKey;

	/** @brief 转向角度阈值（度），小于此角度视为已朝向目标 */
	UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "转向角度阈值", ClampMin = "1.0", ClampMax = "45.0"))
	float FacingAngleThreshold = 15.0f;

	/** @brief 转向速度（度/秒），用于匀速插值到目标朝向 */
	UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "转向速度", ClampMin = "90.0", ClampMax = "1080.0"))
	float FacingRotationSpeed = 720.0f;

	/** @brief 最大等待转向时间（秒），超时后放弃本次攻击 */
	UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "最大转向等待时间", ClampMin = "0.1", ClampMax = "2.0"))
	float MaxRotationWaitTime = 0.5f;

private:
	/** @brief 转向等待计时器 */
	float RotationTimer = 0.0f;
};
