/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Soldier/XBSoldierCharacter.h

/**
 * @file XBSoldierCharacter.h
 * @brief 士兵Actor类 - 支持数据驱动和行为树AI
 * 
 * @note 🔧 修改记录:
 *       1. 使用球形检测替代全量Actor搜索
 *       2. 从数据表读取视野范围和战斗配置
 *       3. 增强空指针检查
 *       4. 使用项目专用日志类别
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
// 在现有的前向声明区域添加:
class UXBSoldierDebugComponent;  // ✨ 新增

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSoldierStateChanged, EXBSoldierState, OldState, EXBSoldierState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSoldierDied, AXBSoldierCharacter*, Soldier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSoldierRecruited, AXBSoldierCharacter*, Soldier, AActor*, Leader);

UCLASS()
class XIAOBINDATIANXIA_API AXBSoldierCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AXBSoldierCharacter();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void PostInitializeComponents() override;

    UPROPERTY(BlueprintReadOnly, Category = "状态")
    bool bComponentsInitialized = false;

    void EnableMovementAndTick();

    // ==================== 初始化 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "从数据表初始化"))
    void InitializeFromDataTable(UDataTable* DataTable, FName RowName, EXBFaction InFaction);

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "初始化士兵"))
    void InitializeSoldier(const FXBSoldierConfig& InConfig, EXBFaction InFaction);

    // ✨ 新增 - 检查是否从数据表初始化
    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "是否从数据表初始化"))
    bool IsInitializedFromDataTable() const { return bInitializedFromDataTable; }

    // ✨ 新增 - 获取视野范围
    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "获取视野范围"))
    float GetVisionRange() const;

    // ✨ 新增 - 获取脱离距离
    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "获取脱离距离"))
    float GetDisengageDistance() const;

    // ✨ 新增 - 获取返回延迟
    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "获取返回延迟"))
    float GetReturnDelay() const;

    // ✨ 新增 - 获取到达阈值
    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "获取到达阈值"))
    float GetArrivalThreshold() const;

    // ==================== 招募系统 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "被招募"))
    void OnRecruited(AActor* NewLeader, int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "是否已招募"))
    bool IsRecruited() const { return bIsRecruited; }

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

    // ==================== AI系统（🔧 修改 - 使用球形检测） ====================

    /**
     * @brief 寻找最近的敌人
     * @return 最近的敌人Actor
     * @note 🔧 修改 - 使用球形检测替代全量Actor搜索
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "寻找最近敌人"))
    AActor* FindNearestEnemy() const;

    /**
     * @brief 检查指定范围内是否有敌人
     * @param Radius 检测半径
     * @return 是否有敌人
     * @note 🔧 修改 - 使用球形检测
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "范围内有敌人"))
    bool HasEnemiesInRadius(float Radius) const;

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "获取到目标距离"))
    float GetDistanceToTarget(AActor* Target) const;

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "是否在攻击范围内"))
    bool IsInAttackRange(AActor* Target) const;

    void UpdateCombat(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "移动到目标"))
    void MoveToTarget(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "是否应该脱离战斗"))
    bool ShouldDisengage() const;

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "返回队列"))
    void ReturnToFormation();

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "是否应该后撤"))
    bool ShouldRetreat() const;

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "后撤"))
    void RetreatFromTarget(AActor* Target);

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

    UPROPERTY(BlueprintAssignable, Category = "XB|Soldier")
    FOnSoldierRecruited OnSoldierRecruited;

    // ==================== 组件 ====================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "跟随组件"))
    TObjectPtr<UXBSoldierFollowComponent> FollowComponent;

    // ✨ 新增 - 调试组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "调试组件"))
    TObjectPtr<UXBSoldierDebugComponent> DebugComponent;
    
protected:


    

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
    float LastEnemySeenTime = 0.0f;

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

protected:
    FVector CalculateAvoidanceDirection(const FVector& DesiredDirection);

private:
    void SpawnAndPossessAIController();
    void InitializeAI();
    FTimerHandle DelayedAIStartTimerHandle;
};
