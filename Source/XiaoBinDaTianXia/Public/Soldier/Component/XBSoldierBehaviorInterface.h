/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Soldier/Component/XBSoldierBehaviorInterface.h

/**
 * @file XBSoldierBehaviorInterface.h
 * @brief 士兵行为接口组件 - 提供统一的行为执行接口
 * 
 * @note ✨ 新增文件
 *       核心职责：
 *       1. 封装所有行为执行逻辑
 *       2. 供行为树 Task 调用
 *       3. 与 Actor 状态解耦
 *       4. 可复用于不同兵种
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Army/XBSoldierTypes.h"
#include "AI/XBSoldierPerceptionSubsystem.h"  // ✨ 新增 - 包含 FXBPerceptionResult 定义
#include "XBSoldierBehaviorInterface.generated.h"

class AXBSoldierCharacter;
class UXBSoldierPerceptionSubsystem;

// ============================================
// 行为执行结果枚举
// ============================================

UENUM(BlueprintType)
enum class EXBBehaviorResult : uint8
{
    Success     UMETA(DisplayName = "成功"),
    Failed      UMETA(DisplayName = "失败"),
    InProgress  UMETA(DisplayName = "进行中"),
    Aborted     UMETA(DisplayName = "中止")
};

// ============================================
// 行为执行委托
// ============================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBehaviorCompleted, FName, BehaviorName, EXBBehaviorResult, Result);

/**
 * @brief 士兵行为接口组件
 * 
 * @note 设计理念：
 *       - 所有行为逻辑封装在此组件
 *       - 行为树 Task 仅调用此组件的方法
 *       - Actor 类不再包含决策/行为逻辑
 *       - 便于扩展新兵种（继承并重写）
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName = "XB 士兵行为接口"))
class XIAOBINDATIANXIA_API UXBSoldierBehaviorInterface : public UActorComponent
{
    GENERATED_BODY()

public:
    UXBSoldierBehaviorInterface();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, 
        FActorComponentTickFunction* ThisTickFunction) override;

    // ==================== 感知行为 ====================

    /**
     * @brief 搜索最近的敌人
     * @param OutEnemy 输出找到的敌人
     * @return 是否找到敌人
     * 
     * @note 通过 PerceptionSubsystem 执行，支持缓存
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Behavior|Perception", meta = (DisplayName = "搜索敌人"))
    bool SearchForEnemy(AActor*& OutEnemy);

    /**
     * @brief 检查是否有敌人在视野内
     * @return 是否有敌人
     */
    UFUNCTION(BlueprintPure, Category = "XB|Behavior|Perception", meta = (DisplayName = "视野内有敌人"))
    bool HasEnemyInSight() const;

    /**
     * @brief 检查目标是否仍然有效
     * @param Target 目标
     * @return 是否有效
     */
    UFUNCTION(BlueprintPure, Category = "XB|Behavior|Perception", meta = (DisplayName = "目标有效"))
    bool IsTargetValid(AActor* Target) const;

    // ==================== 战斗行为 ====================

    /**
     * @brief 执行攻击
     * @param Target 攻击目标
     * @return 执行结果
     * 
     * @note 包含冷却检查、距离检查、伤害计算
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Behavior|Combat", meta = (DisplayName = "执行攻击"))
    EXBBehaviorResult ExecuteAttack(AActor* Target);

    /**
     * @brief 检查是否可以攻击
     * @param Target 目标
     * @return 是否可以攻击
     */
    UFUNCTION(BlueprintPure, Category = "XB|Behavior|Combat", meta = (DisplayName = "可以攻击"))
    bool CanAttack(AActor* Target) const;

    /**
     * @brief 检查是否在攻击范围内
     * @param Target 目标
     * @return 是否在范围内
     */
    UFUNCTION(BlueprintPure, Category = "XB|Behavior|Combat", meta = (DisplayName = "在攻击范围内"))
    bool IsInAttackRange(AActor* Target) const;

    /**
     * @brief 获取攻击冷却剩余时间
     * @return 剩余时间（秒）
     */
    UFUNCTION(BlueprintPure, Category = "XB|Behavior|Combat", meta = (DisplayName = "攻击冷却剩余"))
    float GetAttackCooldownRemaining() const { return AttackCooldownTimer; }

    // ==================== 移动行为 ====================

    /**
     * @brief 移动到目标位置
     * @param TargetLocation 目标位置
     * @param AcceptanceRadius 接受半径
     * @return 执行结果
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Behavior|Movement", meta = (DisplayName = "移动到位置"))
    EXBBehaviorResult MoveToLocation(const FVector& TargetLocation, float AcceptanceRadius = 50.0f);

    /**
     * @brief 移动到目标 Actor
     * @param Target 目标 Actor
     * @param AcceptanceRadius 接受半径
     * @return 执行结果
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Behavior|Movement", meta = (DisplayName = "移动到目标"))
    EXBBehaviorResult MoveToActor(AActor* Target, float AcceptanceRadius = -1.0f);

    /**
     * @brief 返回编队位置
     * @return 执行结果
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Behavior|Movement", meta = (DisplayName = "返回编队"))
    EXBBehaviorResult ReturnToFormation();

    /**
     * @brief 停止移动
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Behavior|Movement", meta = (DisplayName = "停止移动"))
    void StopMovement();

    /**
     * @brief 检查是否到达编队位置
     * @return 是否到达
     */
    UFUNCTION(BlueprintPure, Category = "XB|Behavior|Movement", meta = (DisplayName = "已到达编队位置"))
    bool IsAtFormationPosition() const;

    // ==================== 弓手特殊行为 ====================

    /**
     * @brief 检查是否需要后撤
     * @param Target 当前目标
     * @return 是否需要后撤
     */
    UFUNCTION(BlueprintPure, Category = "XB|Behavior|Archer", meta = (DisplayName = "需要后撤"))
    bool ShouldRetreat(AActor* Target) const;

    /**
     * @brief 执行后撤
     * @param Target 后撤方向的参考目标
     * @return 执行结果
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Behavior|Archer", meta = (DisplayName = "执行后撤"))
    EXBBehaviorResult ExecuteRetreat(AActor* Target);

    // ==================== 决策辅助 ====================

    /**
     * @brief 检查是否应该脱离战斗
     * @return 是否应该脱离
     * 
     * @note 条件：
     *       1. 距离将领过远
     *       2. 长时间无敌人
     */
    UFUNCTION(BlueprintPure, Category = "XB|Behavior|Decision", meta = (DisplayName = "应该脱离战斗"))
    bool ShouldDisengage() const;

    /**
     * @brief 获取到将领的距离
     * @return 距离（如果没有将领返回 MAX_FLT）
     */
    UFUNCTION(BlueprintPure, Category = "XB|Behavior|Decision", meta = (DisplayName = "到将领距离"))
    float GetDistanceToLeader() const;

    /**
     * @brief 更新上次看见敌人的时间
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Behavior|Decision", meta = (DisplayName = "记录看见敌人"))
    void RecordEnemySeen();

    // ==================== 委托 ====================

    /** @brief 行为完成委托 */
    UPROPERTY(BlueprintAssignable, Category = "XB|Behavior")
    FOnBehaviorCompleted OnBehaviorCompleted;

protected:
    // ==================== 内部方法 ====================

    /** @brief 获取拥有者士兵 */
    AXBSoldierCharacter* GetOwnerSoldier() const;

    /** @brief 获取感知子系统 */
    UXBSoldierPerceptionSubsystem* GetPerceptionSubsystem() const;

    /** @brief 播放攻击蒙太奇 */
    bool PlayAttackMontage();

    /** @brief 应用伤害到目标 */
    void ApplyDamageToTarget(AActor* Target, float Damage);

    /** @brief 更新攻击冷却 */
    void UpdateAttackCooldown(float DeltaTime);

    /** @brief 面向目标 */
    void FaceTarget(AActor* Target, float DeltaTime);

protected:
    // ==================== 缓存引用 ====================

    /** @brief 缓存的士兵引用 */
    UPROPERTY()
    TWeakObjectPtr<AXBSoldierCharacter> CachedSoldier;

    /** @brief 缓存的感知子系统 */
    UPROPERTY()
    TWeakObjectPtr<UXBSoldierPerceptionSubsystem> CachedPerceptionSubsystem;

    // ==================== 运行时状态 ====================

    /** @brief 攻击冷却计时器 */
    UPROPERTY(BlueprintReadOnly, Category = "状态")
    float AttackCooldownTimer = 0.0f;

    /** @brief 上次看见敌人的时间 */
    UPROPERTY(BlueprintReadOnly, Category = "状态")
    float LastEnemySeenTime = 0.0f;

    /** @brief 缓存的感知结果 */
    UPROPERTY()
    FXBPerceptionResult CachedPerceptionResult;

    /** @brief 感知结果缓存时间 */
    float PerceptionCacheTime = 0.0f;

    /** @brief 感知缓存有效期 */
    static constexpr float PerceptionCacheValidity = 0.1f;
};
