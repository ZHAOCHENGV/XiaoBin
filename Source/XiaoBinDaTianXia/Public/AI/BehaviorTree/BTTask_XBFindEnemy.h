/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/BehaviorTree/BTTask_XBFindEnemy.h

/**
 * @file BTTask_XBFindEnemy.h
 * @brief 行为树任务 - 寻找敌人
 * 
 * @note 🔧 修改记录:
 *       1. 使用球形检测替代全量Actor搜索
 *       2. 支持从数据表读取视野范围
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_XBFindEnemy.generated.h"

UCLASS()
class XIAOBINDATIANXIA_API UBTTask_XBFindEnemy : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_XBFindEnemy();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual FString GetStaticDescription() const override;

protected:
    /** @brief 目标黑板键 - 存储找到的敌人 */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "目标键"))
    FBlackboardKeySelector TargetKey;

    /** @brief 检测范围黑板键 */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "检测范围键"))
    FBlackboardKeySelector DetectionRangeKey;

    /** @brief 默认检测范围（如果黑板键无效且数据表未配置） */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "默认检测范围", ClampMin = "100.0"))
    float DefaultDetectionRange = 800.0f;

    /** @brief 是否忽略已死亡的目标 */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "忽略死亡目标"))
    bool bIgnoreDeadTargets = true;
};
