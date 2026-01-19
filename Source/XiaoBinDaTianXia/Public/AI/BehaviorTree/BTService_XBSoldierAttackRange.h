/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/BehaviorTree/BTService_XBSoldierAttackRange.h

/**
 * @file BTService_XBSoldierAttackRange.h
 * @brief 行为树服务 - 士兵攻击范围检测
 *
 * @note ✨ 新增 - 根据士兵技能攻击范围判断目标是否在攻击范围内
 *       1. 持续检测目标是否在攻击范围内
 *       2. 将结果写入自定义黑板键（可配置）
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_XBSoldierAttackRange.generated.h"

/**
 * @brief 士兵攻击范围检测服务
 * @note 根据士兵技能攻击范围判断当前目标是否在攻击范围内
 */
UCLASS()
class XIAOBINDATIANXIA_API UBTService_XBSoldierAttackRange : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_XBSoldierAttackRange();

protected:
    /**
     * @brief 服务Tick更新
     * @param OwnerComp 行为树组件
     * @param NodeMemory 节点内存
     * @param DeltaSeconds 帧间隔
     */
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    /** @brief 目标黑板键 */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "目标键"))
    FBlackboardKeySelector TargetKey;

    /** @brief 是否在攻击范围内黑板键（输出） */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "攻击范围内键"))
    FBlackboardKeySelector IsInAttackRangeKey;
};
