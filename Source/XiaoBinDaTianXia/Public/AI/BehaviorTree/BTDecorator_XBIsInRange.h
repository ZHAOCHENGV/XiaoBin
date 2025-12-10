/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/BehaviorTree/BTDecorator_XBIsInRange.h

/**
 * @file BTDecorator_XBIsInRange.h
 * @brief 行为树装饰器 - 检查是否在范围内
 * 
 * @note ✨ 新增文件
 *       1. 检查士兵与目标的距离
 *       2. 支持攻击范围和脱离范围检查
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_XBIsInRange.generated.h"

/**
 * @brief 距离检查类型
 */
UENUM(BlueprintType)
enum class EXBRangeCheckType : uint8
{
    /** @brief 在范围内 */
    InRange     UMETA(DisplayName = "在范围内"),
    /** @brief 超出范围 */
    OutOfRange  UMETA(DisplayName = "超出范围")
};

/**
 * @brief 检查是否在范围内的装饰器
 */
UCLASS()
class XIAOBINDATIANXIA_API UBTDecorator_XBIsInRange : public UBTDecorator
{
    GENERATED_BODY()

public:
    UBTDecorator_XBIsInRange();

protected:
    /** @brief 计算条件结果 */
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

    /** @brief 获取节点描述 */
    virtual FString GetStaticDescription() const override;

protected:
    /** @brief 目标黑板键 */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "目标键"))
    FBlackboardKeySelector TargetKey;

    /** @brief 范围黑板键（可选，如果不设置使用默认值） */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "范围键"))
    FBlackboardKeySelector RangeKey;

    /** @brief 检查类型 */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "检查类型"))
    EXBRangeCheckType CheckType = EXBRangeCheckType::InRange;

    /** @brief 默认范围（如果黑板键无效） */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "默认范围", ClampMin = "0.0"))
    float DefaultRange = 150.0f;
};
