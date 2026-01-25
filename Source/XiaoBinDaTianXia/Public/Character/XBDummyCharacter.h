// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "XBCharacterBase.h"
#include "XBDummyCharacter.generated.h"

class AXBDummyAIController;
class USplineComponent;
class AActor;
class UXBDummyAIDebugComponent;

UCLASS()
class XIAOBINDATIANXIA_API AXBDummyCharacter : public AXBCharacterBase {
  GENERATED_BODY()

public:
  AXBDummyCharacter();

protected:
  virtual void BeginPlay() override;

public:
  virtual void Tick(float DeltaTime) override;

  /**
   * @brief  处理假人受击逻辑
   * @param  DamageSource 伤害来源
   * @param  DamageAmount 伤害数值
   * @return 无
   * @note   详细流程分析: 触发延迟响应 -> 由行为树任务执行攻击
   *         性能/架构注意事项: 通过随机延迟削峰，避免同时释放导致性能抖动
   */
  virtual void HandleDamageReceived(AActor *DamageSource,
                                    float DamageAmount) override;

  /**
   * @brief  初始化主将数据（假人）
   * @return 无
   * @note   详细流程分析: 使用父类通用初始化，默认从 Actor 内部与数据表读取
   */
  virtual void InitializeLeaderData() override;

  /**
   * @brief  从配置界面初始化角色名称
   * @param  InDisplayName 显示名称
   * @return 无
   * @note   专用于 UXBLeaderSpawnConfigWidget 放置时初始化角色名称
   */
  UFUNCTION(BlueprintCallable, Category = "初始化",
            meta = (DisplayName = "初始化角色名称"))
  void InitializeCharacterNameFromConfig(const FString &InDisplayName);

  /**
   * @brief  执行假人受击后的攻击逻辑
   * @return 是否成功执行攻击
   * @note   详细流程分析: 检查战斗组件 -> 判断冷却 -> 优先释放技能或普攻
   *         性能/架构注意事项: 仅负责释放，不处理目标选择
   */
  bool ExecuteDamageResponseAttack();

  /**
   * @brief  获取最近造成伤害的主将
   * @return 主将指针（可能为空）
   * @note   详细流程分析: 返回缓存的伤害来源主将
   *         性能/架构注意事项: 仅用于AI反击目标判断
   */
  AXBCharacterBase *GetLastDamageLeader() const;

  /**
   * @brief  清理最近伤害来源主将记录
   * @return 无
   * @note   详细流程分析: 反击成功锁定后清理，避免重复锁定
   *         性能/架构注意事项: 仅在AI服务中调用
   */
  void ClearLastDamageLeader();

  /**
   * @brief  获取巡逻路线样条组件
   * @return 样条组件指针（可能为空）
   * @note   详细流程分析: 读取路线Actor -> 查找第一个Spline组件
   *         性能/架构注意事项: 仅在AI初始化时调用，避免每帧查找
   */
  USplineComponent *GetPatrolSplineComponent() const;

  /**
   * @brief  获取假人当前移动模式
   * @return 移动模式枚举
   */
  UFUNCTION(BlueprintPure, Category = "AI|移动",
            meta = (DisplayName = "获取移动模式"))
  EXBLeaderAIMoveMode GetDummyMoveMode() const { return DummyMoveMode; }

  /**
   * @brief  设置假人移动模式
   * @param  InMoveMode 移动模式枚举
   */
  UFUNCTION(BlueprintCallable, Category = "AI|移动",
            meta = (DisplayName = "设置移动模式"))
  void SetDummyMoveMode(EXBLeaderAIMoveMode InMoveMode);

private:
  // ✨ 新增 - 巡逻路线Actor（需包含Spline组件）
  /**
   * @brief 巡逻路线Actor
   * @note  用于固定路线行走模式
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI",
            meta = (DisplayName = "巡逻路线Actor", ExposeOnSpawn = "true",
                    AllowPrivateAccess = "true"))
  TObjectPtr<AActor> PatrolSplineActor;

  /**
   * @brief 假人默认移动方式
   * @note  覆盖数据表中的 MoveMode，避免依赖表配置
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|移动",
            meta = (DisplayName = "默认移动方式", AllowPrivateAccess = "true"))
  EXBLeaderAIMoveMode DummyMoveMode = EXBLeaderAIMoveMode::Stand;

  /** @brief 出生点缓存（用于 Stand 模式回归） */
  FVector SpawnLocation;

  /** @brief 出生朝向缓存（用于 Stand 模式回归） */
  FRotator SpawnRotation;

public:
  /**
   * @brief  获取回归位置（按模式决定）
   * @return 回归目标坐标
   * @note   Stand=出生点, Wander=当前位置, Route=巡逻路线最近点
   */
  UFUNCTION(BlueprintPure, Category = "AI|移动",
            meta = (DisplayName = "获取回归位置"))
  FVector GetReturnLocation() const;

  /** @brief 获取出生点 */
  UFUNCTION(BlueprintPure, Category = "AI|移动",
            meta = (DisplayName = "获取出生点"))
  FVector GetSpawnLocation() const { return SpawnLocation; }

  /** @brief 获取出生朝向 */
  UFUNCTION(BlueprintPure, Category = "AI|移动",
            meta = (DisplayName = "获取出生朝向"))
  FRotator GetSpawnRotation() const { return SpawnRotation; }

private:
  // ✨ 新增 - 假人受击响应延迟
  /**
   * @brief 受击响应延迟最小值
   * @note  用于随机范围下限，避免所有假人同步出手
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI",
            meta = (DisplayName = "受击响应延迟最小值", ClampMin = "0.0",
                    AllowPrivateAccess = "true"))
  float DamageResponseDelayMin = 0.2f;

  /**
   * @brief 受击响应延迟最大值
   * @note  用于随机范围上限
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI",
            meta = (DisplayName = "受击响应延迟最大值", ClampMin = "0.0",
                    AllowPrivateAccess = "true"))
  float DamageResponseDelayMax = 0.6f;

  // ✨ 新增 - 最近伤害来源主将缓存
  /**
   * @brief 最近伤害来源主将
   * @note  用于中立/无敌人时的反击目标
   */
  TWeakObjectPtr<AXBCharacterBase> LastDamageLeader;

  // ✨ 新增 - 受击响应定时器
  FTimerHandle DamageResponseTimerHandle;

  // ✨ 新增 - AI调试组件
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "调试",
            meta = (DisplayName = "AI调试组件", AllowPrivateAccess = "true"))
  TObjectPtr<UXBDummyAIDebugComponent> AIDebugComponent;

  // ✨ 新增 - 延迟后触发攻击请求
  /**
   * @brief 延迟回调：通知AI执行攻击
   * @return 无
   * @note   详细流程分析: 获取AI控制器 -> 写入黑板触发攻击任务
   *         性能/架构注意事项: 若AI控制器缺失则直接返回
   */
  void TriggerDamageResponse();
};
