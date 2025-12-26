/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/BehaviorTree/BTTask_XBSetSoldierState.h

/**
 * @file BTTask_XBSetSoldierState.h
 * @brief 行为树任务 - 设置士兵状态
 *
 * @note ✨ 新增文件
 *       1. 改变士兵状态枚举
 *       2. 同步更新黑板值
 *       3. 可选清理目标
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Army/XBSoldierTypes.h"
#include "BTTask_XBSetSoldierState.generated.h"

/**
 * @brief 设置士兵状态任务
 */
UCLASS()
class XIAOBINDATIANXIA_API UBTTask_XBSetSoldierState : public UBTTaskNode
{
    GENERATED_BODY()

public:
    // 🔧 修改 - 简单注释: 构造任务
    UBTTask_XBSetSoldierState();

    // 🔧 修改 - 简单注释: 执行任务
    /** @brief 执行任务 */
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    // 🔧 修改 - 简单注释: 获取描述
    /** @brief 获取节点描述 */
    virtual FString GetStaticDescription() const override;

protected:
    // 🔧 修改 - 简单注释: 目标状态
    /** @brief 要设置的状态 */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "目标状态"))
    EXBSoldierState NewState = EXBSoldierState::Following;

    // 🔧 修改 - 简单注释: 清理目标开关
    /** @brief 是否清理当前目标 */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "清理目标"))
    bool bClearTarget = false;

    // 🔧 修改 - 简单注释: 目标键
    /** @brief 目标黑板键（用于清理） */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "目标键", EditCondition = "bClearTarget"))
    FBlackboardKeySelector TargetKey;
};
