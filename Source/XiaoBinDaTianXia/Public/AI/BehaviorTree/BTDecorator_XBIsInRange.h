/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/BehaviorTree/BTDecorator_XBIsInRange.h

/**
 * @file BTDecorator_XBIsInRange.h
 * @brief 行为树装饰器 - 检查是否在范围内
 *
 * @note ✨ 新增文件
 *       1. 检查士兵与目标的距离
 *       2. 支持攻击范围与脱离范围判断
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
    // 🔧 修改 - 简单注释: 构造装饰器
    UBTDecorator_XBIsInRange();

protected:
    // 🔧 修改 - 简单注释: 计算范围条件
    /** @brief 计算条件结果 */
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

    // 🔧 修改 - 简单注释: 获取节点描述
    /** @brief 获取节点描述 */
    virtual FString GetStaticDescription() const override;

protected:
    // 🔧 修改 - 简单注释: 目标键
    /** @brief 目标黑板键 */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "目标键"))
    FBlackboardKeySelector TargetKey;

    // 🔧 修改 - 简单注释: 范围键
    /** @brief 范围黑板键（可选，如不设置使用默认值） */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "范围键"))
    FBlackboardKeySelector RangeKey;

    // 🔧 修改 - 简单注释: 检测类型
    /** @brief 检查类型 */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "检查类型"))
    EXBRangeCheckType CheckType = EXBRangeCheckType::InRange;

    // 🔧 修改 - 简单注释: 默认范围
    /** @brief 默认范围（黑板键无效时使用） */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "默认范围", ClampMin = "0.0"))
    float DefaultRange = 150.0f;
};
