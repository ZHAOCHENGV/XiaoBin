/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Soldier/XBSoldierActor.h

/**
 * @file XBSoldierActor.h
 * @brief 士兵Actor类 - 支持数据驱动和行为树AI
 * 
 * @note 🔧 修改记录:
 *       1. 重构为数据驱动，从数据表加载配置
 *       2. 新增行为树AI支持
 *       3. 完善战斗系统（寻敌/攻击/撤退）
 *       4. 弓手特殊逻辑（原地攻击）
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Data/XBDataTypes.h"
#include "Data/XBSoldierDataTable.h"
#include "XBSoldierActor.generated.h"

// 前向声明
class USkeletalMeshComponent;
class UCapsuleComponent;
class UXBSoldierFollowComponent;
class UBehaviorTree;
class UBlackboardComponent;
class AAIController;
class AXBSoldierAIController;
class AXBCharacterBase;
class UDataTable;
class UAnimMontage;

// ✨ 新增 - 士兵状态变化委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSoldierStateChanged, EXBSoldierState, OldState, EXBSoldierState, NewState);

// ✨ 新增 - 士兵死亡委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSoldierDied, AXBSoldierActor*, Soldier);

/**
 * @brief 士兵Actor类 - 使用Character基类支持行为树AI
 */
UCLASS()
class XIAOBINDATIANXIA_API AXBSoldierActor : public ACharacter
{
    GENERATED_BODY()

public:
    AXBSoldierActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // ==================== 初始化 ====================

    /**
     * @brief 从数据表初始化士兵
     * @param DataTable 士兵数据表
     * @param RowName 行名称
     * @param InFaction 所属阵营
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "从数据表初始化"))
    void InitializeFromDataTable(UDataTable* DataTable, FName RowName, EXBFaction InFaction);

    /**
     * @brief 初始化士兵（旧接口保持兼容）
     * @param InConfig 士兵配置
     * @param InFaction 所属阵营
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "初始化士兵"))
    void InitializeSoldier(const FXBSoldierConfig& InConfig, EXBFaction InFaction);

    // ==================== 跟随系统 ====================

    /**
     * @brief 设置跟随的将领
     * @param NewLeader 将领Actor
     * @param SlotIndex 编队槽位索引
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置跟随将领"))
    void SetFollowTarget(AActor* NewLeader, int32 SlotIndex);

    /**
     * @brief 获取跟随的将领
     * @return 将领Actor指针
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取跟随将领"))
    AActor* GetFollowTarget() const { return FollowTarget.Get(); }

    /**
     * @brief 获取将领角色（类型转换）
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取将领角色"))
    AXBCharacterBase* GetLeaderCharacter() const;

    /**
     * @brief 获取编队槽位索引
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取槽位索引"))
    int32 GetFormationSlotIndex() const { return FormationSlotIndex; }

    /**
     * @brief 设置编队槽位索引
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置槽位索引"))
    void SetFormationSlotIndex(int32 NewIndex);

    // ==================== 状态管理 ====================

    /**
     * @brief 获取士兵类型
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取士兵类型"))
    EXBSoldierType GetSoldierType() const { return SoldierType; }

    /**
     * @brief 获取士兵状态
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取士兵状态"))
    EXBSoldierState GetSoldierState() const { return CurrentState; }

    /**
     * @brief 设置士兵状态
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置士兵状态"))
    void SetSoldierState(EXBSoldierState NewState);

    /**
     * @brief 获取阵营
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取阵营"))
    EXBFaction GetFaction() const { return Faction; }

    /**
     * @brief 获取士兵配置
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取士兵配置"))
    const FXBSoldierConfig& GetSoldierConfig() const { return SoldierConfig; }

    // ==================== 战斗系统 ====================

    /**
     * @brief 进入战斗状态
     * @note 由将领的OnAttackHit调用
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "进入战斗"))
    void EnterCombat();

    /**
     * @brief 退出战斗状态（返回跟随）
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "退出战斗"))
    void ExitCombat();

    /**
     * @brief 受到伤害
     * @param DamageAmount 伤害量
     * @param DamageSource 伤害来源
     * @return 实际伤害量
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "受到伤害"))
    float TakeSoldierDamage(float DamageAmount, AActor* DamageSource);

    /**
     * @brief 执行攻击
     * @param Target 攻击目标
     * @return 是否成功攻击
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "执行攻击"))
    bool PerformAttack(AActor* Target);

    /**
     * @brief 获取当前血量
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取当前血量"))
    float GetCurrentHealth() const { return CurrentHealth; }

    /**
     * @brief 获取最大血量
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取最大血量"))
    float GetMaxHealth() const { return CachedTableRow.MaxHealth; }

    /**
     * @brief 是否可以攻击
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "是否可以攻击"))
    bool CanAttack() const { return AttackCooldownTimer <= 0.0f && CurrentState != EXBSoldierState::Dead; }

    // ==================== AI系统 ====================

    /**
     * @brief 寻找最近的敌人
     * @return 最近的敌人Actor
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "寻找最近敌人"))
    AActor* FindNearestEnemy() const;

    /**
     * @brief 获取到目标的距离
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "获取到目标距离"))
    float GetDistanceToTarget(AActor* Target) const;

    /**
     * @brief 是否在攻击范围内
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "是否在攻击范围内"))
    bool IsInAttackRange(AActor* Target) const;

    /**
     * @brief 是否应该脱离战斗（超出距离）
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "是否应该脱离战斗"))
    bool ShouldDisengage() const;

    /**
     * @brief 移动到目标位置
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "移动到目标"))
    void MoveToTarget(AActor* Target);

    /**
     * @brief 移动到编队位置
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "移动到编队位置"))
    void MoveToFormationPosition();

    /**
     * @brief 获取编队世界位置
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "获取编队位置"))
    FVector GetFormationWorldPosition() const;

    /**
     * @brief 获取编队世界位置（安全版本）
     * @note 🔧 新增 - 在组件未初始化时返回ZeroVector而非崩溃
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "获取编队位置(安全)"))
    FVector GetFormationWorldPositionSafe() const;

    /**
     * @brief 是否到达编队位置
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "是否到达编队位置"))
    bool IsAtFormationPosition() const;

    /**
     * @brief 是否到达编队位置（安全版本）
     * @note 🔧 新增 - 在组件未初始化时返回true而非崩溃
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "是否到达编队位置(安全)"))
    bool IsAtFormationPositionSafe() const;

    // ==================== 逃跑系统 ====================

    /**
     * @brief 设置逃跑加速状态
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置逃跑状态"))
    void SetEscaping(bool bEscaping);

    /**
     * @brief 是否正在逃跑
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "是否正在逃跑"))
    bool IsEscaping() const { return bIsEscaping; }

    // ==================== 委托事件 ====================

    /** @brief 状态变化事件 */
    UPROPERTY(BlueprintAssignable, Category = "XB|Soldier")
    FOnSoldierStateChanged OnSoldierStateChanged;

    /** @brief 死亡事件 */
    UPROPERTY(BlueprintAssignable, Category = "XB|Soldier")
    FOnSoldierDied OnSoldierDied;

protected:
    // ==================== 组件 ====================

    /** @brief 跟随组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "跟随组件"))
    TObjectPtr<UXBSoldierFollowComponent> FollowComponent;

    // ==================== 配置数据 ====================

    /** @brief 士兵类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "士兵类型"))
    EXBSoldierType SoldierType = EXBSoldierType::Infantry;

    /** @brief 阵营 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "阵营"))
    EXBFaction Faction = EXBFaction::Neutral;

    /** @brief 士兵配置（旧式，保持兼容） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "士兵配置"))
    FXBSoldierConfig SoldierConfig;

    /** @brief 缓存的数据表行 */
    UPROPERTY(BlueprintReadOnly, Category = "配置")
    FXBSoldierTableRow CachedTableRow;

    /** @brief 是否已从数据表初始化 */
    UPROPERTY(BlueprintReadOnly, Category = "配置")
    bool bInitializedFromDataTable = false;

    // ==================== 状态数据 ====================

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

    /** @brief 当前攻击目标 */
    UPROPERTY(BlueprintReadOnly, Category = "状态")
    TWeakObjectPtr<AActor> CurrentAttackTarget;

    /** @brief 寻敌计时器 */
    float TargetSearchTimer = 0.0f;

    // ==================== AI控制器 ====================

    /** @brief 行为树 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "行为树"))
    TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

    /** @brief AI控制器类（使用专门的士兵AI控制器） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "AI控制器类"))
    TSubclassOf<AXBSoldierAIController> SoldierAIControllerClass;

public:
    // ==================== 友元声明 ====================
    friend class AXBSoldierAIController;

    // ==================== 内部方法 ====================

    /** @brief 更新跟随逻辑 */
    void UpdateFollowing(float DeltaTime);

    /** @brief 更新战斗逻辑 */
    void UpdateCombat(float DeltaTime);

    /** @brief 更新返回逻辑 */
    void UpdateReturning(float DeltaTime);

    /** @brief 处理死亡 */
    void HandleDeath();

    /** @brief 播放攻击蒙太奇 */
    bool PlayAttackMontage();

    /** @brief 应用视觉配置 */
    void ApplyVisualConfig();

    /** @brief 初始化AI */
    void InitializeAI();

    /** @brief 面向目标 */
    void FaceTarget(AActor* Target, float DeltaTime);
};
