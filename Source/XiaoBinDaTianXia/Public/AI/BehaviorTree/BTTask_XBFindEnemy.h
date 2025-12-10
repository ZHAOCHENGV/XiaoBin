/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/BehaviorTree/BTTask_XBFindEnemy.h

/**
 * @file BTTask_XBFindEnemy.h
 * @brief 行为树任务 - 寻找敌人
 * 
 * @note ✨ 新增文件
 *       1. 搜索范围内的敌对目标
 *       2. 选择最近的敌人作为目标
 *       3. 更新黑板中的目标值
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_XBFindEnemy.generated.h"

/**
 * @brief 寻找敌人任务
 * 
 * @note 功能说明:
 *       - 在检测范围内搜索敌对单位
 *       - 选择最近的敌人
 *       - 将找到的目标写入黑板
 */
UCLASS()
class XIAOBINDATIANXIA_API UBTTask_XBFindEnemy : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_XBFindEnemy();

    /** @brief 执行任务 */
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    /** @brief 获取节点描述 */
    virtual FString GetStaticDescription() const override;

protected:
    /** @brief 目标黑板键 - 存储找到的敌人 */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "目标键"))
    FBlackboardKeySelector TargetKey;

    /** @brief 检测范围黑板键 */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "检测范围键"))
    FBlackboardKeySelector DetectionRangeKey;

    /** @brief 默认检测范围（如果黑板键无效） */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "默认检测范围", ClampMin = "100.0"))
    float DefaultDetectionRange = 800.0f;

    /** @brief 是否忽略已死亡的目标 */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "忽略死亡目标"))
    bool bIgnoreDeadTargets = true;
};
