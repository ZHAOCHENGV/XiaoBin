/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/BehaviorTree/BTTask_XBFindEnemy.h

/**
 * @file BTTask_XBFindEnemy.h
 * @brief 行为树任务 - 寻找敌人
 *
 * @note 🔧 修改记录:
 *       1. 使用感知接口搜索敌人
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
    // 🔧 修改 - 简单注释: 构造任务
    UBTTask_XBFindEnemy();

    // 🔧 修改 - 简单注释: 执行任务
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    // 🔧 修改 - 简单注释: 获取描述
    virtual FString GetStaticDescription() const override;

protected:
    // 🔧 修改 - 简单注释: 目标键
    /** @brief 目标黑板键 - 存储找到的敌人 */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "目标键"))
    FBlackboardKeySelector TargetKey;

    // 🔧 修改 - 简单注释: 检测范围键
    /** @brief 检测范围黑板键 */
    UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "检测范围键"))
    FBlackboardKeySelector DetectionRangeKey;

    // 🔧 修改 - 简单注释: 默认检测范围
    /** @brief 默认检测范围（黑板无效且数据表未配置时使用） */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "默认检测范围", ClampMin = "100.0"))
    float DefaultDetectionRange = 800.0f;

    // 🔧 修改 - 简单注释: 忽略死亡目标
    /** @brief 是否忽略已死亡的目标 */
    UPROPERTY(EditAnywhere, Category = "配置", meta = (DisplayName = "忽略死亡目标"))
    bool bIgnoreDeadTargets = true;
};
