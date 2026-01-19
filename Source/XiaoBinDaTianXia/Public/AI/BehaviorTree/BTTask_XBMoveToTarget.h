/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/BehaviorTree/BTTask_XBMoveToTarget.h

/**
 * @file BTTask_XBMoveToTarget.h
 * @brief 行为树任务 - 移动到目标
 *
 * @note ✨ 新增文件
 *       1. 移动到黑板中指定的目标Actor
 *       2. 支持动态更新目标位置
 */

#pragma once

#include "BTTask_XBMoveToTarget.generated.h"
#include "BehaviorTree/BTTaskNode.h"
#include "CoreMinimal.h"

/**
 * @brief 移动到目标任务
 */
UCLASS()
class XIAOBINDATIANXIA_API UBTTask_XBMoveToTarget : public UBTTaskNode {
  GENERATED_BODY()

public:
  // 🔧 修改 - 简单注释: 构造任务
  UBTTask_XBMoveToTarget();

  // 🔧 修改 - 简单注释: 执行任务
  /** @brief 执行任务 */
  virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent &OwnerComp,
                                          uint8 *NodeMemory) override;

  // 🔧 修改 - 简单注释: Tick更新
  /** @brief Tick更新 */
  virtual void TickTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory,
                        float DeltaSeconds) override;

  // 🔧 修改 - 简单注释: 中止任务
  /** @brief 任务中止时调用 */
  virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent &OwnerComp,
                                        uint8 *NodeMemory) override;

  // 🔧 修改 - 简单注释: 获取描述
  /** @brief 获取节点描述 */
  virtual FString GetStaticDescription() const override;

protected:
  // 🔧 新增: 停止距离的缩放因子 (0.9 表示走到攻击范围的 90% 处就停下，留出 10%
  // 的误差缓冲)
  UPROPERTY(EditAnywhere, Category = "配置",
            meta = (DisplayName = "停止距离缩放", ClampMin = "0.5",
                    ClampMax = "0.95"))
  float StopDistanceScale = 0.8f;

  // 🔧 修改 - 简单注释: 目标键
  /** @brief 目标黑板键 */
  UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "目标键"))
  FBlackboardKeySelector TargetKey;

  // 🔧 修改 - 简单注释: 攻击范围键
  /** @brief 攻击范围黑板键 */
  UPROPERTY(EditAnywhere, Category = "黑板",
            meta = (DisplayName = "攻击范围键"))
  FBlackboardKeySelector AttackRangeKey;

  // 🔧 修改 - 简单注释: 默认停止距离
  /** @brief 默认停止距离（攻击范围） */
  UPROPERTY(EditAnywhere, Category = "配置",
            meta = (DisplayName = "默认停止距离", ClampMin = "10.0"))
  float DefaultStopDistance = 150.0f;

  // 🔧 修改 - 简单注释: 目标更新间隔
  /** @brief 目标位置更新间隔 */
  UPROPERTY(EditAnywhere, Category = "配置",
            meta = (DisplayName = "位置更新间隔", ClampMin = "0.1"))
  float TargetUpdateInterval = 0.3f;

  // 🔧 新增 - 判定卡住的最小移动速度
  /** @brief 判定卡住的最小速度 */
  UPROPERTY(EditAnywhere, Category = "配置",
            meta = (DisplayName = "最小移动速度", ClampMin = "0.0"))
  float MinMoveSpeed = 5.0f;

  // 🔧 新增 - 卡住持续时间阈值
  /** @brief 卡住持续时间阈值 */
  UPROPERTY(EditAnywhere, Category = "配置",
            meta = (DisplayName = "卡住判定时间", ClampMin = "0.1"))
  float StuckTimeThreshold = 2.0f;

private:
  // 🔧 修改 - 简单注释: 更新计时器
  /** @brief 目标位置更新计时器 */
  float TargetUpdateTimer = 0.0f;

  // 🔧 新增 - 卡住检测计时器
  /** @brief 卡住检测计时器 */
  float StuckTimer = 0.0f;
};
