/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/BehaviorTree/BTService_XBUpdateSoldierState.h

/**
 * @file BTService_XBUpdateSoldierState.h
 * @brief 行为树服务 - 更新士兵状态
 * 
 * @note ✨ 新增文件
 *       1. 定期更新士兵的黑板数据
 *       2. 监控战斗状态变化
 *       3. 计算各种距离值
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_XBUpdateSoldierState.generated.h"

/**
 * @brief 士兵状态更新服务
 * 
 * @note 功能说明:
 *       - 定期刷新黑板中的状态数据
 *       - 更新目标有效性
 *       - 更新距离和位置信息
 */
UCLASS()
class XIAOBINDATIANXIA_API UBTService_XBUpdateSoldierState : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_XBUpdateSoldierState();

protected:
    /** @brief 服务激活时调用 */
    virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    /** @brief 定期Tick调用 */
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    /** @brief 获取节点描述 */
    virtual FString GetStaticDescription() const override;

protected:
    /** @brief 目标黑板键 */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "目标键"))
    FBlackboardKeySelector TargetKey;

    /** @brief 将领黑板键 */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "将领键"))
    FBlackboardKeySelector LeaderKey;

    /** @brief 是否自动寻找新目标（当当前目标失效时） */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "自动寻找目标"))
    bool bAutoFindTarget = true;

    /** @brief 目标失效检测（死亡/销毁） */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "检测目标失效"))
    bool bCheckTargetValidity = true;

    /** @brief 脱离战斗距离（超过此距离返回将领） */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "脱离距离", ClampMin = "100.0"))
    float DisengageDistance = 1000.0f;
};
