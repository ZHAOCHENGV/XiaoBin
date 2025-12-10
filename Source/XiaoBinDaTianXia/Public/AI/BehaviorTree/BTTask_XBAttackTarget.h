/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/BehaviorTree/BTTask_XBAttackTarget.h

/**
 * @file BTTask_XBAttackTarget.h
 * @brief 行为树任务 - 攻击目标
 * 
 * @note ✨ 新增文件
 *       1. 执行对目标的攻击
 *       2. 支持攻击冷却检查
 *       3. 处理攻击动画和伤害
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_XBAttackTarget.generated.h"

/**
 * @brief 攻击目标任务
 * 
 * @note 功能说明:
 *       - 检查攻击冷却
 *       - 执行攻击动作
 *       - 对目标造成伤害
 */
UCLASS()
class XIAOBINDATIANXIA_API UBTTask_XBAttackTarget : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_XBAttackTarget();

    /** @brief 执行任务 */
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    /** @brief 获取节点描述 */
    virtual FString GetStaticDescription() const override;

protected:
    /** @brief 目标黑板键 */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "目标键"))
    FBlackboardKeySelector TargetKey;

    /** @brief 是否在冷却中仍然返回成功 */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "冷却时成功返回"))
    bool bSucceedOnCooldown = true;
};
