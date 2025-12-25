/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Character/XBCharacterBase.h

/**
 * @file XBCharacterBase.h
 * @brief 角色基类 - 包含所有将领共用的组件和功能
 * 
 * @note 🔧 修改记录:
 *       1. 修复士兵计数同步问题
 *       2. 添加将领死亡标记防止循环回调
 *       3. 优化代码结构
 *       4. ✨ 新增 掉落抛物线配置和逻辑
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Data/XBLeaderDataTable.h"
#include "Army/XBSoldierTypes.h"
#include "XBCharacterBase.generated.h"

class UAbilitySystemComponent;
class UXBAbilitySystemComponent;
class UXBAttributeSet;
class UXBCombatComponent;
class UDataTable;
struct FXBSoldierTableRow;
class AXBSoldierCharacter;
class UAnimMontage;
class UXBWorldHealthBarComponent;
class UXBMagnetFieldComponent;
class UXBFormationComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDeath, AXBCharacterBase*, DeadCharacter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatStateChanged, bool, bInCombat);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSoldierCountChanged, int32, OldCount, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSprintStateChanged, bool, bIsSprinting);

USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBGrowthConfigCache
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长", meta = (DisplayName = "每士兵生命加成"))
    float HealthPerSoldier = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长", meta = (DisplayName = "每士兵体型加成"))
    float ScalePerSoldier = 0.01f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长", meta = (DisplayName = "最大体型缩放"))
    float MaxScale = 2.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长", meta = (DisplayName = "每士兵伤害倍率加成"))
    float DamageMultiplierPerSoldier = 0.01f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长", meta = (DisplayName = "最大伤害倍率", ClampMin = "1.0"))
    float MaxDamageMultiplier = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长", meta = (DisplayName = "启用技能特效缩放"))
    bool bEnableSkillEffectScaling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长", meta = (DisplayName = "技能特效缩放倍率", ClampMin = "0.1"))
    float SkillEffectScaleMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长", meta = (DisplayName = "启用攻击范围缩放"))
    bool bEnableAttackRangeScaling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长", meta = (DisplayName = "攻击范围缩放倍率", ClampMin = "0.1"))
    float AttackRangeScaleMultiplier = 1.0f;
};

// 🔧 修改 - 更新掉落配置结构体，整合抛物线配置
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBSoldierDropConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "掉落", meta = (DisplayName = "掉落数量", ClampMin = "0"))
    int32 DropCount = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "掉落", meta = (DisplayName = "掉落士兵类"))
    TSubclassOf<AXBSoldierCharacter> DropSoldierClass;

    // ✨ 新增 - 抛物线配置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "掉落", meta = (DisplayName = "抛物线配置"))
    FXBDropArcConfig ArcConfig;

    // ❌ 删除 - 移除旧的 DropRadius 和 DropAnimDuration，由 ArcConfig 替代
};

UCLASS()
class XIAOBINDATIANXIA_API AXBCharacterBase : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AXBCharacterBase();

    virtual void Tick(float DeltaTime) override;

    // ============ IAbilitySystemInterface ============
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    // ============ 基础信息 ============

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "角色名称"))
    FString CharacterName;

    // ============ 初始化 ============

    UFUNCTION(BlueprintCallable, Category = "初始化")
    virtual void InitializeFromDataTable(UDataTable* DataTable, FName RowName);

    UFUNCTION(BlueprintCallable, Category = "属性")
    void ApplyInitialAttributes();

    // ============ 阵营系统 ============

    UFUNCTION(BlueprintPure, Category = "阵营")
    EXBFaction GetFaction() const { return Faction; }

    UFUNCTION(BlueprintCallable, Category = "阵营")
    void SetFaction(EXBFaction NewFaction) { Faction = NewFaction; }

    UFUNCTION(BlueprintPure, Category = "阵营")
    bool IsHostileTo(const AXBCharacterBase* Other) const;

    UFUNCTION(BlueprintPure, Category = "阵营")
    bool IsFriendlyTo(const AXBCharacterBase* Other) const;

    // ============ 士兵管理 ============

    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void AddSoldier(AXBSoldierCharacter* Soldier);

    UFUNCTION(BlueprintCallable, Category = "士兵")
    FName GetRecruitSoldierRowName() const { return RecruitSoldierRowName; }

    UFUNCTION(BlueprintCallable, Category = "士兵")
    UDataTable* GetSoldierDataTable() const { return SoldierDataTable; }

    UFUNCTION(BlueprintCallable, Category = "士兵", meta = (DisplayName = "获取士兵Actor类"))
    TSubclassOf<AXBSoldierCharacter> GetSoldierActorClass() const { return SoldierActorClass; }

    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void RemoveSoldier(AXBSoldierCharacter* Soldier);

    UFUNCTION(BlueprintPure, Category = "士兵")
    int32 GetSoldierCount() const { return Soldiers.Num(); }

    UFUNCTION(BlueprintPure, Category = "士兵")
    const TArray<AXBSoldierCharacter*>& GetSoldiers() const { return Soldiers; }

    UFUNCTION(BlueprintCallable, Category = "成长")
    virtual void OnSoldierDied(AXBSoldierCharacter* DeadSoldier);

    UFUNCTION(BlueprintPure, Category = "成长", meta = (DisplayName = "获取当前缩放倍率"))
    float GetCurrentScale() const;

    UFUNCTION(BlueprintPure, Category = "成长", meta = (DisplayName = "获取当前攻击范围"))
    float GetScaledAttackRange() const;

    UFUNCTION(BlueprintPure, Category = "成长", meta = (DisplayName = "获取当前伤害倍率"))
    float GetCurrentDamageMultiplier() const;

    UFUNCTION()
    void OnCombatAttackStateChanged(bool bIsAttacking);

    void BindCombatEvents();
    
    // ============ 死亡系统 ============

    UFUNCTION(BlueprintCallable, Category = "死亡")
    virtual void HandleDeath();
    
    UFUNCTION(BlueprintPure, Category = "死亡")
    bool IsDead() const { return bIsDead; }
    
    UFUNCTION(BlueprintPure, Category = "组件")
    UXBCombatComponent* GetCombatComponent() const { return CombatComponent; }

    // ============ 冲刺系统（共用） ============

    UFUNCTION(BlueprintCallable, Category = "移动", meta = (DisplayName = "开始冲刺"))
    virtual void StartSprint();

    UFUNCTION(BlueprintCallable, Category = "移动", meta = (DisplayName = "停止冲刺"))
    virtual void StopSprint();

    // ============ 召回系统 ============
    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void RecallAllSoldiers();

    UFUNCTION(BlueprintCallable, Category = "战斗", meta = (DisplayName = "脱离战斗（逃跑）"))
    virtual void DisengageFromCombat();

    // ============ 组件访问 ============

    UFUNCTION(BlueprintCallable, Category = "组件", meta = (DisplayName = "获取磁场组件"))
    UXBMagnetFieldComponent* GetMagnetFieldComponent() const { return MagnetFieldComponent; }

    UFUNCTION(BlueprintCallable, Category = "组件", meta = (DisplayName = "获取编队组件"))
    UXBFormationComponent* GetFormationComponent() const { return FormationComponent; }

    UFUNCTION(BlueprintCallable, Category = "组件", meta = (DisplayName = "获取血条组件"))
    UXBWorldHealthBarComponent* GetHealthBarComponent() const { return HealthBarComponent; }

    // ============ 战斗状态系统 ============

    UFUNCTION(BlueprintCallable, Category = "战斗")
    virtual void EnterCombat();

    UFUNCTION(BlueprintCallable, Category = "战斗")
    virtual void ExitCombat();

    UFUNCTION(BlueprintPure, Category = "战斗")
    bool IsInCombat() const { return bIsInCombat; }

    UFUNCTION(BlueprintCallable, Category = "战斗")
    virtual void OnAttackHit(AActor* HitTarget);

    UFUNCTION(BlueprintPure, Category = "战斗", meta = (DisplayName = "获取最近攻击的敌方主将"))
    AXBCharacterBase* GetLastAttackedEnemyLeader() const { return LastAttackedEnemyLeader.Get(); }

    UFUNCTION(BlueprintPure, Category = "移动", meta = (DisplayName = "是否正在冲刺"))
    bool IsSprinting() const { return bIsSprinting; }

    UFUNCTION(BlueprintPure, Category = "移动", meta = (DisplayName = "获取当前移动速度"))
    float GetCurrentMoveSpeed() const;

    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void SetSoldiersEscaping(bool bEscaping);

    UFUNCTION(BlueprintCallable, Category = "死亡", meta = (DisplayName = "设置伤害来源"))
    void SetLastDamageInstigator(AActor* InInstigator) { LastDamageInstigator = InInstigator; }

    // ============ 委托事件 ============

    UPROPERTY(BlueprintAssignable, Category = "事件")
    FOnCharacterDeath OnCharacterDeath;

    UPROPERTY(BlueprintAssignable, Category = "事件")
    FOnCombatStateChanged OnCombatStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "事件")
    FOnSoldierCountChanged OnSoldierCountChanged;

    UPROPERTY(BlueprintAssignable, Category = "事件", meta = (DisplayName = "冲刺状态变化"))
    FOnSprintStateChanged OnSprintStateChanged;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "编队组件"))
    TObjectPtr<UXBFormationComponent> FormationComponent;

protected:
    virtual void BeginPlay() override;
    virtual void PossessedBy(AController* NewController) override;

    virtual void InitializeAbilitySystem();

    UFUNCTION()
    void OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    UFUNCTION()
    void OnDestroyTimerExpired();

    virtual void PreDestroyCleanup();

    // 🔧 修改 - 重构掉落士兵方法
    virtual void SpawnDroppedSoldiers();

    void ReassignSoldierSlots(int32 StartIndex);

    virtual void UpdateSprint(float DeltaTime);

    virtual void SetupMovementComponent();

    UFUNCTION()
    virtual void OnMagnetFieldActorEntered(AActor* EnteredActor);

    bool Internal_AddSoldierToArray(AXBSoldierCharacter* Soldier);
    bool Internal_RemoveSoldierFromArray(AXBSoldierCharacter* Soldier);
    void UpdateSoldierCount(int32 OldCount);

    void ApplyGrowthOnSoldiersAdded(int32 SoldierCount);
    void ApplyGrowthOnSoldiersRemoved(int32 SoldierCount);

    void UpdateSkillEffectScaling();
    void UpdateAttackRangeScaling();
    void UpdateLeaderScale();
    void AddHealthWithOverflow(float HealthToAdd);
    void UpdateDamageMultiplier();

    // ==================== 核心组件 ====================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "能力系统组件"))
    TObjectPtr<UXBAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "属性集"))
    TObjectPtr<UXBAttributeSet> AttributeSet;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "战斗组件"))
    TObjectPtr<UXBCombatComponent> CombatComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "头顶血条"))
    TObjectPtr<UXBWorldHealthBarComponent> HealthBarComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "磁场组件"))
    TObjectPtr<UXBMagnetFieldComponent> MagnetFieldComponent;

    // ==================== 阵营 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "阵营", meta = (DisplayName = "阵营"))
    EXBFaction Faction = EXBFaction::Neutral;

    // ==================== 士兵管理 ====================

    UPROPERTY(BlueprintReadOnly, Category = "士兵")
    TArray<AXBSoldierCharacter*> Soldiers;

    UPROPERTY(BlueprintReadOnly, Category = "成长")
    float BaseScale = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "成长")
    float BaseAttackRange = 150.0f;

    // ==================== 战斗状态 ====================

    UPROPERTY(BlueprintReadOnly, Category = "战斗")
    bool bIsInCombat = false;

    UPROPERTY(BlueprintReadOnly, Category = "战斗")
    TWeakObjectPtr<AXBCharacterBase> LastAttackedEnemyLeader;



    // ==================== 移动配置（共用） ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "移动", meta = (DisplayName = "基础移动速度", ClampMin = "0.0"))
    float BaseMoveSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "移动", meta = (DisplayName = "冲刺速度倍率", ClampMin = "1.0", ClampMax = "5.0"))
    float SprintSpeedMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "移动", meta = (DisplayName = "速度变化平滑度", ClampMin = "1.0"))
    float SpeedInterpRate = 15.0f;

    UPROPERTY(BlueprintReadOnly, Category = "移动")
    bool bIsSprinting = false;

    UPROPERTY(BlueprintReadOnly, Category = "移动")
    float TargetMoveSpeed = 0.0f;

    // ==================== 配置 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "配置数据表"))
    TObjectPtr<UDataTable> ConfigDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "配置行名"))
    FName ConfigRowName;

    UPROPERTY(BlueprintReadOnly, Category = "配置")
    FXBLeaderTableRow CachedLeaderData;

    UPROPERTY(BlueprintReadOnly, Category = "成长")
    FXBGrowthConfigCache GrowthConfigCache;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "掉落", meta = (DisplayName = "士兵掉落配置"))
    FXBSoldierDropConfig SoldierDropConfig;

    // ==================== 招募配置 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "招募", meta = (DisplayName = "士兵数据表"))
    TObjectPtr<UDataTable> SoldierDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "招募", meta = (DisplayName = "招募士兵行名"))
    FName RecruitSoldierRowName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "招募", meta = (DisplayName = "士兵Actor类"))
    TSubclassOf<AXBSoldierCharacter> SoldierActorClass;

    // ==================== 死亡系统 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "死亡", meta = (DisplayName = "死亡蒙太奇"))
    TObjectPtr<UAnimMontage> DeathMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "死亡", meta = (DisplayName = "死亡后消失延迟", ClampMin = "0.0"))
    float DeathDestroyDelay = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "死亡", meta = (DisplayName = "蒙太奇结束后开始计时"))
    bool bDelayAfterMontage = true;

    UPROPERTY(BlueprintReadOnly, Category = "死亡")
    bool bIsDead = false;

    UPROPERTY(BlueprintReadOnly, Category = "死亡")
    bool bIsCleaningUpSoldiers = false;

    UPROPERTY(BlueprintReadOnly, Category = "死亡", meta = (DisplayName = "最后伤害来源"))
    TWeakObjectPtr<AActor> LastDamageInstigator;

    FTimerHandle DeathDestroyTimerHandle;

    void KillAllSoldiers();

    // ==================== AI配置 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "逃跑时自动冲刺"))
    bool bSprintWhenDisengaging = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "逃跑冲刺时长", ClampMin = "0.0"))
    float DisengageSprintDuration = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "脱离冷却时间", ClampMin = "0.0"))
    float DisengageCooldown = 2.0f;

    float LastDisengageTime = 0.0f;
    
    FTimerHandle DisengageSprintTimerHandle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "战斗超时时间"))
    float CombatTimeoutDuration = 999.0f;

    FTimerHandle CombatTimeoutHandle;

private:
    UFUNCTION()
    void OnCombatTimeout();
};
