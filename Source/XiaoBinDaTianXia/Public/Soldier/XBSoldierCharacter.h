/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Soldier/XBSoldierCharacter.h

/**
 * @file XBSoldierCharacter.h
 * @brief 士兵Actor类 - 重构为纯数据容器 + 状态持有者
 * 
 * @note 🔧 架构重构记录:
 *       1. ❌ 删除 FXBSoldierConfig 冗余结构
 *       2. ❌ 删除 FXBSoldierTableRow CachedTableRow
 *       3. ❌ 删除 ToSoldierConfig() 转换方法
 *       4. ❌ 删除 bInitializedFromDataTable 标记
 *       5. ✨ 新增 UXBSoldierDataAccessor 数据访问器组件
 *       6. 🔧 所有配置数据访问委托给 DataAccessor
 *       7. 🔧 保留运行时状态（CurrentHealth, CurrentState等）
 *       8. ✨ 新增 bIsDead 死亡状态变量（蓝图可读）
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Army/XBSoldierTypes.h"
#include "XBSoldierCharacter.generated.h"

// ============================================
// 前向声明
// ============================================

class UXBSoldierFollowComponent;
class UXBSoldierDebugComponent;
class UXBSoldierDataAccessor;
class UXBSoldierBehaviorInterface;
class UBehaviorTree;
class AAIController;
class AXBSoldierAIController;
class AXBCharacterBase;
class UDataTable;
class UAnimMontage;

// ============================================
// 委托声明
// ============================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSoldierStateChanged, EXBSoldierState, OldState, EXBSoldierState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSoldierDied, AXBSoldierCharacter*, Soldier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSoldierRecruited, AXBSoldierCharacter*, Soldier, AActor*, Leader);

// ============================================
// 士兵Actor类
// ============================================

/**
 * @brief 士兵Actor - 数据驱动架构
 * @note 🔧 新架构职责:
 *       - 持有运行时状态（血量、位置、目标等）
 *       - 管理组件生命周期
 *       - 响应游戏事件
 *       - 委托数据访问给 DataAccessor
 *       - AI逻辑由行为树和AIController处理
 */
UCLASS()
class XIAOBINDATIANXIA_API AXBSoldierCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AXBSoldierCharacter();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void PostInitializeComponents() override;

    // ==================== 组件状态 ====================

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "组件已初始化"))
    bool bComponentsInitialized = false;

    void EnableMovementAndTick();

    // ==================== ✨ 新增：数据访问器接口 ====================

    /**
     * @brief 获取数据访问器组件
     * @return 数据访问器引用
     * @note 所有配置数据读取必须通过此组件
     */
    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Data", meta = (DisplayName = "获取数据访问器"))
    UXBSoldierDataAccessor* GetDataAccessor() const { return DataAccessor; }

    /**
     * @brief 检查数据访问器是否有效
     * @return 是否有效且已初始化
     */
    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Data", meta = (DisplayName = "数据访问器有效"))
    bool IsDataAccessorValid() const;

    // ==================== 🔧 重构：初始化方法 ====================

    /**
     * @brief 从数据表初始化
     * @param DataTable 数据表资源
     * @param RowName 行名
     * @param InFaction 阵营
     * @note 🔧 重构 - 简化为直接初始化 DataAccessor
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "从数据表初始化"))
    void InitializeFromDataTable(UDataTable* DataTable, FName RowName, EXBFaction InFaction);


    // ==================== 🔧 重构：配置属性访问方法 ====================
    // 所有配置数据访问都委托给 DataAccessor

    // --- 基础属性 ---

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Data", meta = (DisplayName = "获取士兵类型"))
    EXBSoldierType GetSoldierType() const { return SoldierType; }

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Data", meta = (DisplayName = "获取显示名称"))
    FText GetDisplayName() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Data", meta = (DisplayName = "获取士兵标签"))
    FGameplayTagContainer GetSoldierTags() const;

    // --- 战斗配置 ---

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Combat", meta = (DisplayName = "获取最大血量"))
    float GetMaxHealth() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Combat", meta = (DisplayName = "获取基础伤害"))
    float GetBaseDamage() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Combat", meta = (DisplayName = "获取攻击范围"))
    float GetAttackRange() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Combat", meta = (DisplayName = "获取攻击间隔"))
    float GetAttackInterval() const;

    // --- 移动配置 ---

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Movement", meta = (DisplayName = "获取移动速度"))
    float GetMoveSpeed() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Movement", meta = (DisplayName = "获取冲刺倍率"))
    float GetSprintSpeedMultiplier() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Movement", meta = (DisplayName = "获取跟随插值速度"))
    float GetFollowInterpSpeed() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Movement", meta = (DisplayName = "获取旋转速度"))
    float GetRotationSpeed() const;

    // --- AI配置 ---

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "获取视野范围"))
    float GetVisionRange() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "获取脱离距离"))
    float GetDisengageDistance() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "获取返回延迟"))
    float GetReturnDelay() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "获取到达阈值"))
    float GetArrivalThreshold() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "获取避让半径"))
    float GetAvoidanceRadius() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "获取避让权重"))
    float GetAvoidanceWeight() const;

    // --- 加成配置 ---

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Bonus", meta = (DisplayName = "获取血量加成"))
    float GetHealthBonusToLeader() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Bonus", meta = (DisplayName = "获取伤害加成"))
    float GetDamageBonusToLeader() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Bonus", meta = (DisplayName = "获取缩放加成"))
    float GetScaleBonusToLeader() const;

    // ==================== 招募系统 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "被招募"))
    void OnRecruited(AActor* NewLeader, int32 SlotIndex);

    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "是否已招募"))
    bool IsRecruited() const { return bIsRecruited; }

    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "是否可招募"))
    bool CanBeRecruited() const;

    // ==================== 跟随系统 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置跟随将领"))
    void SetFollowTarget(AActor* NewLeader, int32 SlotIndex);

    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "获取跟随将领"))
    AActor* GetFollowTarget() const { return FollowTarget.Get(); }

    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "获取将领角色"))
    AXBCharacterBase* GetLeaderCharacter() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "获取槽位索引"))
    int32 GetFormationSlotIndex() const { return FormationSlotIndex; }

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置槽位索引"))
    void SetFormationSlotIndex(int32 NewIndex);

    // ==================== 状态管理 ====================

    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "获取士兵状态"))
    EXBSoldierState GetSoldierState() const { return CurrentState; }

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置士兵状态"))
    void SetSoldierState(EXBSoldierState NewState);

    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "获取阵营"))
    EXBFaction GetFaction() const { return Faction; }

    // ✨ 新增 - 死亡状态检查
    /**
     * @brief 检查士兵是否已死亡
     * @return 是否已死亡
     * @note 蓝图可读，用于 UI 和逻辑判断
     */
    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "是否已死亡"))
    bool IsDead() const { return bIsDead; }

    // ==================== 战斗系统 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "进入战斗"))
    void EnterCombat();

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "退出战斗"))
    void ExitCombat();

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "受到伤害"))
    float TakeSoldierDamage(float DamageAmount, AActor* DamageSource);

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "执行攻击"))
    bool PerformAttack(AActor* Target);

    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "获取当前血量"))
    float GetCurrentHealth() const { return CurrentHealth; }

    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "是否可以攻击"))
    bool CanAttack() const;

    // ==================== AI系统 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "范围内有敌人"))
    bool HasEnemiesInRadius(float Radius) const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "获取到目标距离"))
    float GetDistanceToTarget(AActor* Target) const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "是否在攻击范围内"))
    bool IsInAttackRange(AActor* Target) const;

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "返回队列"))
    void ReturnToFormation();
    

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "移动到编队位置"))
    void MoveToFormationPosition();

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "获取编队位置"))
    FVector GetFormationWorldPosition() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "获取编队位置(安全)"))
    FVector GetFormationWorldPositionSafe() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "是否到达编队位置"))
    bool IsAtFormationPosition() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "是否到达编队位置(安全)"))
    bool IsAtFormationPositionSafe() const;

    // ==================== 逃跑系统 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置逃跑状态"))
    void SetEscaping(bool bEscaping);

    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "是否正在逃跑"))
    bool IsEscaping() const { return bIsEscaping; }

    // ==================== 委托事件 ====================

    UPROPERTY(BlueprintAssignable, Category = "XB|Soldier")
    FOnSoldierStateChanged OnSoldierStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "XB|Soldier")
    FOnSoldierDied OnSoldierDied;

    UPROPERTY(BlueprintAssignable, Category = "XB|Soldier")
    FOnSoldierRecruited OnSoldierRecruited;

    // ==================== 组件访问 ====================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "跟随组件"))
    TObjectPtr<UXBSoldierFollowComponent> FollowComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "调试组件"))
    TObjectPtr<UXBSoldierDebugComponent> DebugComponent;

    // ==================== 公开访问的战斗状态 ====================

    UPROPERTY(BlueprintReadOnly, Category = "状态")
    TWeakObjectPtr<AActor> CurrentAttackTarget;

    // ==================== AI系统友元 ====================

    friend class AXBSoldierAIController;

    // ==================== ✨ 新增：行为接口组件 ====================
    /**
     * @brief 获取行为接口组件
     * @return 行为接口组件
     * @note 所有 AI 行为执行通过此组件
     */
    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Behavior", meta = (DisplayName = "获取行为接口"))
    UXBSoldierBehaviorInterface* GetBehaviorInterface() const { return BehaviorInterface; }

protected:
    // ==================== ✨ 新增：数据访问器组件 ====================

    /**
     * @brief 数据访问器组件 - 唯一数据源入口
     * @note 所有配置数据必须通过此组件访问
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "数据访问器"))
    TObjectPtr<UXBSoldierDataAccessor> DataAccessor;

    
    /**
     * @brief 行为接口组件
     * @note 封装所有 AI 行为执行逻辑
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "行为接口"))
    TObjectPtr<UXBSoldierBehaviorInterface> BehaviorInterface;

    // ==================== 保留：运行时状态（非配置数据） ====================

    /** @brief 士兵类型（缓存以提高访问速度） */
    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "士兵类型"))
    EXBSoldierType SoldierType = EXBSoldierType::Infantry;

    /** @brief 阵营 */
    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "阵营"))
    EXBFaction Faction = EXBFaction::Neutral;

    /** @brief 当前状态 */
    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "当前状态"))
    EXBSoldierState CurrentState = EXBSoldierState::Idle;

    /** @brief 跟随目标 */
    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "跟随目标"))
    TWeakObjectPtr<AActor> FollowTarget;

    /** @brief 编队槽位索引 */
    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "槽位索引"))
    int32 FormationSlotIndex = INDEX_NONE;

    /** @brief 当前血量 */
    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "当前血量"))
    float CurrentHealth = 100.0f;

    /** @brief 是否正在逃跑 */
    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "正在逃跑"))
    bool bIsEscaping = false;

    /** @brief 攻击冷却计时器 */
    UPROPERTY(BlueprintReadOnly, Category = "状态")
    float AttackCooldownTimer = 0.0f;

    /** @brief 目标搜索计时器 */
    float TargetSearchTimer = 0.0f;

    /** @brief 上次看见敌人的时间 */
    float LastEnemySeenTime = 0.0f;

    /** @brief 是否已招募 */
    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "是否已招募"))
    bool bIsRecruited = false;

    // ✨ 新增 - 死亡状态变量
    /**
     * @brief 是否已死亡
     * @note 蓝图可读，用于 UI 显示和逻辑判断
     *       在 HandleDeath() 中设置为 true
     */
    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "是否已死亡"))
    bool bIsDead = false;

    // ==================== AI配置 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "行为树"))
    TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "AI控制器类"))
    TSubclassOf<AXBSoldierAIController> SoldierAIControllerClass;

    // ==================== 内部方法 ====================

    void HandleDeath();
    bool PlayAttackMontage();
    void ApplyVisualConfig();
    void FaceTarget(AActor* Target, float DeltaTime);
    FVector CalculateAvoidanceDirection(const FVector& DesiredDirection);

private:
    void SpawnAndPossessAIController();
    void InitializeAI();
    FTimerHandle DelayedAIStartTimerHandle;
};
