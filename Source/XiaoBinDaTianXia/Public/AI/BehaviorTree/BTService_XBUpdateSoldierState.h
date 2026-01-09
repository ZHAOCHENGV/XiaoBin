/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/BehaviorTree/BTService_XBUpdateSoldierState.h

/**
 * @file BTService_XBUpdateSoldierState.h
 * @brief 行为树服务 - 更新士兵状态
 *
 * @note ✨ 新增文件
 *       1. 定期更新黑板数据
 *       2. 监控战斗状态变化
 *       3. 计算距离与目标有效性
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_XBUpdateSoldierState.generated.h"

/**
 * @brief 士兵状态更新服务
 */
UCLASS()
class XIAOBINDATIANXIA_API UBTService_XBUpdateSoldierState : public UBTService
{
    GENERATED_BODY()

public:
    // 🔧 修改 - 简单注释: 构造服务
    UBTService_XBUpdateSoldierState();

protected:
    // 🔧 修改 - 简单注释: 服务激活
    /** @brief 服务激活时调用 */
    virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    // 🔧 修改 - 简单注释: 定期Tick
    /** @brief 定期Tick调用 */
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    // 🔧 修改 - 简单注释: 获取描述
    /** @brief 获取节点描述 */
    virtual FString GetStaticDescription() const override;

protected:
    // 🔧 修改 - 简单注释: 目标键
    /** @brief 目标黑板键 */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "目标键"))
    FBlackboardKeySelector TargetKey;

    // 🔧 修改 - 简单注释: 主将键
    /** @brief 主将黑板键 */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "主将键"))
    FBlackboardKeySelector LeaderKey;

    // 🔧 修改 - 简单注释: 自动寻敌
    /** @brief 是否自动寻找新目标 */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "自动寻找目标"))
    bool bAutoFindTarget = true;

    // 🔧 修改 - 简单注释: 目标有效性检查
    /** @brief 目标有效性检测（死亡/销毁） */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "检测目标失效"))
    bool bCheckTargetValidity = true;

    // 🔧 修改 - 简单注释: 追击距离
    /** @brief 追击距离（目标非战斗状态时，超过距离回归主将） */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "追击距离", ClampMin = "100.0"))
    float DisengageDistance = 1000.0f;
};
