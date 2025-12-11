/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Soldier/XBSoldierActor.h

/**
 * @file XBSoldierActor.h
 * @brief 士兵Actor类 - 支持数据驱动和行为树AI
 * 
 * @note 🔧 修改记录:
 *       1. 移除自动 Possess，改为招募时触发
 *       2. 新增招募状态管理
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Data/XBSoldierDataTable.h"
#include "XBSoldierCharacter.generated.h"

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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSoldierStateChanged, EXBSoldierState, OldState, EXBSoldierState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSoldierDied, AXBSoldierCharacter*, Soldier);

// ✨ 新增 - 士兵招募委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSoldierRecruited, AXBSoldierCharacter*, Soldier, AActor*, Leader);

/**
 * @brief 士兵Actor类
 */
UCLASS()
class XIAOBINDATIANXIA_API AXBSoldierCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AXBSoldierCharacter();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // ✨ 新增 - 重写组件初始化完成回调
    virtual void PostInitializeComponents() override;

    // ✨ 新增 - 组件初始化完成标记
    UPROPERTY(BlueprintReadOnly, Category = "状态")
    bool bComponentsInitialized = false;

    // ✨ 新增 - 启用移动和Tick
    void EnableMovementAndTick();

    // ==================== 初始化 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "从数据表初始化"))
    void InitializeFromDataTable(UDataTable* DataTable, FName RowName, EXBFaction InFaction);

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "初始化士兵"))
    void InitializeSoldier(const FXBSoldierConfig& InConfig, EXBFaction InFaction);

    // ==================== 招募系统 ====================

    /**
     * @brief 被将领招募
     * @param NewLeader 招募的将领
     * @param SlotIndex 分配的编队槽位
     * @note ✨ 新增 - 招募时才启动AI控制器
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "被招募"))
    void OnRecruited(AActor* NewLeader, int32 SlotIndex);

    /**
     * @brief 检查是否已被招募
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "是否已招募"))
    bool IsRecruited() const { return bIsRecruited; }

    /**
     * @brief 检查是否可以被招募
     * @note 中立阵营且处于待机状态才可招募
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "是否可招募"))
    bool CanBeRecruited() const;

    // ==================== 跟随系统 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置跟随将领"))
    void SetFollowTarget(AActor* NewLeader, int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取跟随将领"))
    AActor* GetFollowTarget() const { return FollowTarget.Get(); }

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取将领角色"))
    AXBCharacterBase* GetLeaderCharacter() const;

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取槽位索引"))
    int32 GetFormationSlotIndex() const { return FormationSlotIndex; }

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置槽位索引"))
    void SetFormationSlotIndex(int32 NewIndex);

    // ==================== 状态管理 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取士兵类型"))
    EXBSoldierType GetSoldierType() const { return SoldierType; }

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取士兵状态"))
    EXBSoldierState GetSoldierState() const { return CurrentState; }

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置士兵状态"))
    void SetSoldierState(EXBSoldierState NewState);

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取阵营"))
    EXBFaction GetFaction() const { return Faction; }

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取士兵配置"))
    const FXBSoldierConfig& GetSoldierConfig() const { return SoldierConfig; }

    // ==================== 战斗系统 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "进入战斗"))
    void EnterCombat();

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "退出战斗"))
    void ExitCombat();

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "受到伤害"))
    float TakeSoldierDamage(float DamageAmount, AActor* DamageSource);

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "执行攻击"))
    bool PerformAttack(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取当前血量"))
    float GetCurrentHealth() const { return CurrentHealth; }

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取最大血量"))
    float GetMaxHealth() const { return CachedTableRow.MaxHealth > 0 ? CachedTableRow.MaxHealth : SoldierConfig.MaxHealth; }

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "是否可以攻击"))
    bool CanAttack() const { return AttackCooldownTimer <= 0.0f && CurrentState != EXBSoldierState::Dead; }

    // ==================== AI系统 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "寻找最近敌人"))
    AActor* FindNearestEnemy() const;

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "获取到目标距离"))
    float GetDistanceToTarget(AActor* Target) const;

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "是否在攻击范围内"))
    bool IsInAttackRange(AActor* Target) const;



    // ============ 战斗追踪系统（✨ 新增/增强）============

    /**
     * @brief 更新战斗逻辑
     * @param DeltaTime 帧时间
     * @note 功能：
     *       1. 搜索最近敌人
     *       2. 移动到目标（带避障）
     *       3. 攻击目标
     *       4. 检测脱离范围
     */
    void UpdateCombat(float DeltaTime);

    /**
      * @brief 移动到目标（带避障）
      * @param Target 目标Actor
      * @note 使用导航系统自动绕障
      */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "移动到目标"))
    void MoveToTarget(AActor* Target);

    /**
     * @brief 检查是否应该返回队列
     * @return true表示应该返回
     * @note 条件：
     *       1. 距离将领超过脱离距离
     *       2. 周边无敌人
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "是否应该脱离战斗"))
    bool ShouldDisengage() const;

    /**
     * @brief 自动返回队列
     * @note 退出战斗状态，移动到编队位置
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "返回队列"))
    void ReturnToFormation();

    // ✨ 新增 - 弓手专用逻辑
    /**
     * @brief 检查是否应该后撤（弓手专用）
     * @return true表示敌人过近，需要后撤
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "是否应该后撤"))
    bool ShouldRetreat() const;

    
    /**
     * @brief 后撤到安全距离（弓手专用）
     * @param Target 威胁目标
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "后撤"))
    void RetreatFromTarget(AActor* Target);

    
protected:
    // ✨ 新增 - 避障计算
    /**
     * @brief 计算避障后的移动方向
     * @param DesiredDirection 期望方向
     * @return 修正后的方向
     */
    FVector CalculateAvoidanceDirection(const FVector& DesiredDirection);

    // ✨ 新增 - 检查周边是否有敌人
    /**
     * @brief 在指定范围内检测敌人
     * @param Radius 检测半径
     * @return 是否有敌人
     */
    bool HasEnemiesInRadius(float Radius) const;
    
    // ✨ 新增 - 战斗配置
    /** @brief 脱离战斗距离（超过此距离自动返回） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "脱离距离", ClampMin = "100.0"))
    float DisengageDistance = 1000.0f;

    /** @brief 无敌人后返回延迟（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "返回延迟", ClampMin = "0.0"))
    float ReturnDelay = 2.0f;

    /** @brief 避障检测半径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "避障半径", ClampMin = "0.0"))
    float AvoidanceRadius = 100.0f;

    /** @brief 上次检测到敌人的时间 */
    float LastEnemySeenTime = 0.0f;
public:

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "移动到编队位置"))
    void MoveToFormationPosition();

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "获取编队位置"))
    FVector GetFormationWorldPosition() const;

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "获取编队位置(安全)"))
    FVector GetFormationWorldPositionSafe() const;

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "是否到达编队位置"))
    bool IsAtFormationPosition() const;

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "是否到达编队位置(安全)"))
    bool IsAtFormationPositionSafe() const;

    // ==================== 逃跑系统 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置逃跑状态"))
    void SetEscaping(bool bEscaping);

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "是否正在逃跑"))
    bool IsEscaping() const { return bIsEscaping; }

    // ==================== 委托事件 ====================

    UPROPERTY(BlueprintAssignable, Category = "XB|Soldier")
    FOnSoldierStateChanged OnSoldierStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "XB|Soldier")
    FOnSoldierDied OnSoldierDied;

    // ✨ 新增 - 招募事件
    UPROPERTY(BlueprintAssignable, Category = "XB|Soldier")
    FOnSoldierRecruited OnSoldierRecruited;

protected:
    // ==================== 组件 ====================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "跟随组件"))
    TObjectPtr<UXBSoldierFollowComponent> FollowComponent;

    // ==================== 配置数据 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "士兵类型"))
    EXBSoldierType SoldierType = EXBSoldierType::Infantry;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "阵营"))
    EXBFaction Faction = EXBFaction::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "士兵配置"))
    FXBSoldierConfig SoldierConfig;

    UPROPERTY(BlueprintReadOnly, Category = "配置")
    FXBSoldierTableRow CachedTableRow;

    UPROPERTY(BlueprintReadOnly, Category = "配置")
    bool bInitializedFromDataTable = false;

    // ==================== 状态数据 ====================

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "当前状态"))
    EXBSoldierState CurrentState = EXBSoldierState::Idle;

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "跟随目标"))
    TWeakObjectPtr<AActor> FollowTarget;

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "槽位索引"))
    int32 FormationSlotIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "当前血量"))
    float CurrentHealth = 100.0f;

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "正在逃跑"))
    bool bIsEscaping = false;

    UPROPERTY(BlueprintReadOnly, Category = "状态")
    float AttackCooldownTimer = 0.0f;
    
public:
    UPROPERTY(BlueprintReadOnly, Category = "状态")
    TWeakObjectPtr<AActor> CurrentAttackTarget;
    
protected:
    float TargetSearchTimer = 0.0f;

    // ✨ 新增 - 招募状态
    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "是否已招募"))
    bool bIsRecruited = false;

    // ==================== AI配置 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "行为树"))
    TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "AI控制器类"))
    TSubclassOf<AXBSoldierAIController> SoldierAIControllerClass;

public:
    friend class AXBSoldierAIController;

    void UpdateFollowing(float DeltaTime);
    void UpdateReturning(float DeltaTime);
    void HandleDeath();
    bool PlayAttackMontage();
    void ApplyVisualConfig();
    void FaceTarget(AActor* Target, float DeltaTime);

private:
    // ✨ 新增 - 启动AI控制器
    /**
     * @brief 生成并启动AI控制器
     * @note 只在招募时调用，确保组件已完全初始化
     */
    void SpawnAndPossessAIController();

    // ✨ 新增 - 初始化AI（行为树等）
    void InitializeAI();

    // ✨ 新增 - 延迟启动AI的定时器句柄
    FTimerHandle DelayedAIStartTimerHandle;
};
