/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Soldier/XBSoldierCharacter.h

/**
 * @file XBSoldierCharacter.h
 * @brief 士兵Actor类 - 统一角色系统（休眠态 + 激活态）
 * 
 * @note 🔧 架构重构记录:
 *       1. ✨ 新增 休眠态系统（替代 XBVillagerActor）
 *       2. ✨ 新增 组件启用/禁用管理
 *       3. ✨ 新增 Zzz 特效系统
 *       4. ✨ 新增 休眠可视化调试
 *       5. 🔧 修改 状态机支持 Dormant 状态
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
class UAnimSequence;
class UNiagaraComponent;
class UNiagaraSystem;

// ============================================
// 委托声明
// ============================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSoldierStateChanged, EXBSoldierState, OldState, EXBSoldierState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSoldierDied, AXBSoldierCharacter*, Soldier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSoldierRecruited, AXBSoldierCharacter*, Soldier, AActor*, Leader);
// ✨ 新增 - 休眠状态变化委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDormantStateChanged, AXBSoldierCharacter*, Soldier, bool, bIsDormant);



// ============================================
// 士兵Actor类
// ============================================

/**
 * @brief 士兵Actor - 统一角色系统
 * @note 🔧 新架构职责:
 *       - 休眠态：作为可招募的中立单位（原村民功能）
 *       - 激活态：作为战斗士兵
 *       - 组件按需启用/禁用
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

    // ==================== 数据访问器接口 ====================

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Data", meta = (DisplayName = "获取数据访问器"))
    UXBSoldierDataAccessor* GetDataAccessor() const { return DataAccessor; }

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Data", meta = (DisplayName = "数据访问器有效"))
    bool IsDataAccessorValid() const;

    // ==================== 初始化方法 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "从数据表初始化"))
    void InitializeFromDataTable(UDataTable* DataTable, FName RowName, EXBFaction InFaction);

    // ==================== ✨ 新增：休眠系统接口 ====================

    /**
     * @brief 进入休眠态
     * @param DormantType 休眠类型
     * @note 禁用所有非必要组件，显示休眠视觉效果
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|Dormant", meta = (DisplayName = "进入休眠态"))
    void EnterDormantState(EXBDormantType DormantType = EXBDormantType::Sleeping);

    /**
     * @brief 退出休眠态（激活）
     * @note 启用所有组件，准备进入战斗
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|Dormant", meta = (DisplayName = "退出休眠态"))
    void ExitDormantState();

    /**
     * @brief 检查是否处于休眠态
     */
    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Dormant", meta = (DisplayName = "是否休眠中"))
    bool IsDormant() const { return CurrentState == EXBSoldierState::Dormant; }

    /**
     * @brief 设置休眠视觉配置
     * @param NewConfig 新配置
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|Dormant", meta = (DisplayName = "设置休眠配置"))
    void SetDormantVisualConfig(const FXBDormantVisualConfig& NewConfig);

    /**
     * @brief 获取休眠视觉配置
     */
    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Dormant", meta = (DisplayName = "获取休眠配置"))
    const FXBDormantVisualConfig& GetDormantVisualConfig() const { return DormantConfig; }

    /**
     * @brief 设置 Zzz 特效启用状态
     * @param bEnabled 是否启用
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|Dormant", meta = (DisplayName = "设置Zzz特效"))
    void SetZzzEffectEnabled(bool bEnabled);

    /**
     * @brief 切换休眠类型（不改变休眠状态）
     * @param NewType 新的休眠类型
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|Dormant", meta = (DisplayName = "切换休眠类型"))
    void SetDormantType(EXBDormantType NewType);

    /**
     * @brief 获取当前休眠类型
     */
    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Dormant", meta = (DisplayName = "获取休眠类型"))
    EXBDormantType GetDormantType() const { return CurrentDormantType; }

    // ==================== 配置属性访问方法 ====================

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Data", meta = (DisplayName = "获取士兵类型"))
    EXBSoldierType GetSoldierType() const { return SoldierType; }

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Data", meta = (DisplayName = "获取显示名称"))
    FText GetDisplayName() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Data", meta = (DisplayName = "获取士兵标签"))
    FGameplayTagContainer GetSoldierTags() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Combat", meta = (DisplayName = "获取最大血量"))
    float GetMaxHealth() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Combat", meta = (DisplayName = "获取基础伤害"))
    float GetBaseDamage() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Combat", meta = (DisplayName = "获取攻击范围"))
    float GetAttackRange() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Combat", meta = (DisplayName = "获取攻击间隔"))
    float GetAttackInterval() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Movement", meta = (DisplayName = "获取移动速度"))
    float GetMoveSpeed() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Movement", meta = (DisplayName = "获取冲刺倍率"))
    float GetSprintSpeedMultiplier() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Movement", meta = (DisplayName = "获取跟随插值速度"))
    float GetFollowInterpSpeed() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Movement", meta = (DisplayName = "获取旋转速度"))
    float GetRotationSpeed() const;

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

    // ==================== 对象池支持 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|Pool", meta = (DisplayName = "重置状态"))
    void ResetForPooling();

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Pool", meta = (DisplayName = "是否池化士兵"))
    bool IsPooledSoldier() const { return bIsPooledSoldier; }

    void MarkAsPooledSoldier() { bIsPooledSoldier = true; }

    // ==================== 委托事件 ====================

    UPROPERTY(BlueprintAssignable, Category = "XB|Soldier")
    FOnSoldierStateChanged OnSoldierStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "XB|Soldier")
    FOnSoldierDied OnSoldierDied;

    UPROPERTY(BlueprintAssignable, Category = "XB|Soldier")
    FOnSoldierRecruited OnSoldierRecruited;

    // ✨ 新增 - 休眠状态变化委托
    UPROPERTY(BlueprintAssignable, Category = "XB|Soldier|Dormant", meta = (DisplayName = "休眠状态变化"))
    FOnDormantStateChanged OnDormantStateChanged;

    // ==================== 组件访问 ====================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "跟随组件"))
    TObjectPtr<UXBSoldierFollowComponent> FollowComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "调试组件"))
    TObjectPtr<UXBSoldierDebugComponent> DebugComponent;

    // ✨ 新增 - Zzz 特效组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "Zzz特效"))
    TObjectPtr<UNiagaraComponent> ZzzEffectComponent;

    // ==================== 公开访问的战斗状态 ====================

    UPROPERTY(BlueprintReadOnly, Category = "状态")
    TWeakObjectPtr<AActor> CurrentAttackTarget;

    // ==================== AI系统友元 ====================

    friend class AXBSoldierAIController;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Behavior", meta = (DisplayName = "获取行为接口"))
    UXBSoldierBehaviorInterface* GetBehaviorInterface() const { return BehaviorInterface; }

protected:
    // ==================== 数据访问器组件 ====================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "数据访问器"))
    TObjectPtr<UXBSoldierDataAccessor> DataAccessor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "行为接口"))
    TObjectPtr<UXBSoldierBehaviorInterface> BehaviorInterface;

    // ==================== ✨ 新增：休眠配置 ====================

    /** @brief 休眠态视觉配置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Soldier|Dormant", meta = (DisplayName = "休眠配置"))
    FXBDormantVisualConfig DormantConfig;

    /** @brief Zzz 特效资源 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Soldier|Dormant", meta = (DisplayName = "Zzz特效资源"))
    TSoftObjectPtr<UNiagaraSystem> ZzzEffectAsset;

    /** @brief 当前休眠类型 */
    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "休眠类型"))
    EXBDormantType CurrentDormantType = EXBDormantType::Sleeping;

    /** @brief 是否以休眠态开始 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Soldier|Dormant", meta = (DisplayName = "初始休眠态"))
    bool bStartAsDormant = false;

    // ==================== 运行时状态 ====================

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "士兵类型"))
    EXBSoldierType SoldierType = EXBSoldierType::Infantry;

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "阵营"))
    EXBFaction Faction = EXBFaction::Neutral;

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

    float TargetSearchTimer = 0.0f;
    float LastEnemySeenTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "是否已招募"))
    bool bIsRecruited = false;

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "是否已死亡"))
    bool bIsDead = false;

    UPROPERTY(BlueprintReadOnly, Category = "状态")
    bool bIsPooledSoldier = false;

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

    // ✨ 新增 - 休眠系统内部方法
    
    /**
     * @brief 启用激活态组件
     * @note 启用 AI、跟随、行为接口等
     */
    void EnableActiveComponents();

    /**
     * @brief 禁用激活态组件（进入休眠）
     * @note 禁用 AI、跟随、行为接口等，保留基础碰撞
     */
    void DisableActiveComponents();

    /**
     * @brief 更新休眠动画
     */
    void UpdateDormantAnimation();

    /**
     * @brief 更新 Zzz 特效
     */
    void UpdateZzzEffect();

    /**
     * @brief 播放指定动画序列
     * @param Animation 动画序列
     * @param bLoop 是否循环
     */
    void PlayAnimationSequence(UAnimSequence* Animation, bool bLoop = true);

    /**
     * @brief 加载休眠动画资源
     */
    void LoadDormantAnimations();

private:
    void SpawnAndPossessAIController();
    void InitializeAI();
    FTimerHandle DelayedAIStartTimerHandle;

    // ✨ 新增 - 缓存的动画资源
    UPROPERTY()
    TObjectPtr<UAnimSequence> LoadedSleepingAnimation;
    UPROPERTY()
    TObjectPtr<UAnimSequence> LoadedStandingAnimation;

};