/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/BehaviorTree/BTTask_XBSetSoldierState.h

/**
 * @file BTTask_XBSetSoldierState.h
 * @brief 行为树任务 - 设置士兵状态
 * 
 * @note ✨ 新增文件
 *       1. 改变士兵的状态枚举
 *       2. 同步更新黑板值
 *       3. 触发状态变化委托
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Army/XBSoldierTypes.h"
#include "BTTask_XBSetSoldierState.generated.h"

/**
 * @brief 设置士兵状态任务
 * 
 * @note 功能说明:
 *       - 设置士兵的状态枚举值
 *       - 自动更新黑板
 *       - 可选触发状态进入逻辑
 */
UCLASS()
class XIAOBINDATIANXIA_API UBTTask_XBSetSoldierState : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_XBSetSoldierState();

    /** @brief 执行任务 */
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    /** @brief 获取节点描述 */
    virtual FString GetStaticDescription() const override;

protected:
    /** @brief 要设置的状态 */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "目标状态"))
    EXBSoldierState NewState = EXBSoldierState::Following;

    /** @brief 是否清除当前目标 */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "清除目标"))
    bool bClearTarget = false;

    /** @brief 目标黑板键（用于清除） */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "目标键", EditCondition = "bClearTarget"))
    FBlackboardKeySelector TargetKey;
};
