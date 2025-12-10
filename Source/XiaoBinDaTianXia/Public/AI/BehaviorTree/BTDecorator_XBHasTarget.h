/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/BehaviorTree/BTDecorator_XBHasTarget.h

/**
 * @file BTDecorator_XBHasTarget.h
 * @brief 行为树装饰器 - 检查是否有目标
 * 
 * @note ✨ 新增文件
 *       1. 用于行为树条件判断
 *       2. 检查黑板中是否存在有效目标
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_XBHasTarget.generated.h"

/**
 * @brief 检查是否有目标的装饰器
 */
UCLASS()
class XIAOBINDATIANXIA_API UBTDecorator_XBHasTarget : public UBTDecorator
{
    GENERATED_BODY()

public:
    UBTDecorator_XBHasTarget();

protected:
    /** @brief 计算条件结果 */
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

    /** @brief 获取节点描述 */
    virtual FString GetStaticDescription() const override;

protected:
    /** @brief 目标黑板键 */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "目标键"))
    FBlackboardKeySelector TargetKey;
};
