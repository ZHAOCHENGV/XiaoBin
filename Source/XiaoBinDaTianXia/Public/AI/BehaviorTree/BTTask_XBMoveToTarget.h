/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/BehaviorTree/BTTask_XBMoveToTarget.h

/**
 * @file BTTask_XBMoveToTarget.h
 * @brief 行为树任务 - 移动到目标
 * 
 * @note ✨ 新增文件
 *       1. 移动到黑板中指定的目标Actor
 *       2. 支持动态更新目标位置
 *       3. 弓手特殊处理（保持距离）
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_XBMoveToTarget.generated.h"

/**
 * @brief 移动到目标任务
 * 
 * @note 功能说明:
 *       - 追踪并移动到指定目标
 *       - 到达攻击范围后停止
 */
UCLASS()
class XIAOBINDATIANXIA_API UBTTask_XBMoveToTarget : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_XBMoveToTarget();

    /** @brief 执行任务 */
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    /** @brief Tick更新 */
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    /** @brief 任务中止时调用 */
    virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    /** @brief 获取节点描述 */
    virtual FString GetStaticDescription() const override;

protected:
    /** @brief 目标黑板键 */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "目标键"))
    FBlackboardKeySelector TargetKey;

    /** @brief 攻击范围黑板键 */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "攻击范围键"))
    FBlackboardKeySelector AttackRangeKey;

    /** @brief 默认停止距离（攻击范围） */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "默认停止距离", ClampMin = "10.0"))
    float DefaultStopDistance = 150.0f;

    /** @brief 目标位置更新间隔 */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "位置更新间隔", ClampMin = "0.1"))
    float TargetUpdateInterval = 0.3f;

private:
    /** @brief 目标位置更新计时器 */
    float TargetUpdateTimer = 0.0f;
};
