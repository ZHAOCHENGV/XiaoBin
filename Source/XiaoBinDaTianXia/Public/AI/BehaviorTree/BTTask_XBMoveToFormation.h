/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/BehaviorTree/BTTask_XBMoveToFormation.h

/**
 * @file BTTask_XBMoveToFormation.h
 * @brief 行为树任务 - 移动到编队位置
 * 
 * @note ✨ 新增文件
 *       1. 让士兵移动到编队中的指定位置
 *       2. 支持到达检测
 *       3. 使用AI移动系统
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_XBMoveToFormation.generated.h"

/**
 * @brief 移动到编队位置任务
 * 
 * @note 功能说明:
 *       - 计算编队中的目标位置
 *       - 使用AI移动系统导航
 *       - 到达后返回成功
 */
UCLASS()
class XIAOBINDATIANXIA_API UBTTask_XBMoveToFormation : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_XBMoveToFormation();

    /** @brief 执行任务 */
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    /** @brief Tick更新 */
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    /** @brief 获取节点描述 */
    virtual FString GetStaticDescription() const override;

protected:
    /** @brief 编队位置黑板键 */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "编队位置键"))
    FBlackboardKeySelector FormationPositionKey;

    /** @brief 到达阈值（距离目标多近算到达） */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "到达阈值", ClampMin = "10.0"))
    float AcceptableRadius = 50.0f;

    /** @brief 是否使用直接移动（不使用寻路） */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "直接移动"))
    bool bUseDirectMove = false;
};
