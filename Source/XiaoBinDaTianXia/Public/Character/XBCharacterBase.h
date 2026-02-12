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
#include "GameplayTagContainer.h"
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
class USkeletalMesh;
class UXBWorldHealthBarComponent;
class UXBMagnetFieldComponent;
class UXBFormationComponent;
class UMaterialInterface;
struct FXBGameConfigData;
class UAudioComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class UParticleSystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDeath, AXBCharacterBase*, DeadCharacter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatStateChanged, bool, bInCombat);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSoldierCountChanged, int32, OldCount, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSprintStateChanged, bool, bIsSprinting);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnterCombatDelegate, AXBCharacterBase*, Leader);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAssignTargetDelegate, AXBSoldierCharacter*, Soldier, AActor*, Target);

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

    /**
     * @brief  应用运行时配置到主将
     * @param  GameConfig 游戏配置数据
     * @param  bApplyInitialSoldiers 是否应用初始士兵数量
     * @return 无
     * @note   详细流程分析: 覆盖主将数据 -> 刷新属性 -> 应用磁场/掉落/招募配置
     */
    UFUNCTION(BlueprintCallable, Category = "配置", meta = (DisplayName = "应用运行时配置"))
    void ApplyRuntimeConfig(const FXBGameConfigData& GameConfig, bool bApplyInitialSoldiers = true);

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

    /**
     * @brief  获取主将AI配置
     * @return AI配置
     * @note   详细流程分析: 直接返回数据表缓存
     *         性能/架构注意事项: 仅供读取，不应在外部修改
     */
    UFUNCTION(BlueprintPure, Category = "AI", meta = (DisplayName = "获取主将AI配置"))
    const FXBLeaderAIConfig& GetLeaderAIConfig() const { return CachedLeaderData.AIConfig; }

    // ============ 冲刺系统（共用） ============

    /**
     * @brief  触发冲刺（按键触发）
     * @return 无
     * @note   详细流程分析: 检查死亡/冲刺状态 -> 启动冲刺 -> 根据配置持续时间安排结束
     *         性能/架构注意事项: 冲刺期间重复触发无效，避免反复创建计时器
     */
    UFUNCTION(BlueprintCallable, Category = "移动", meta = (DisplayName = "触发冲刺"))
    void TriggerSprint();

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

    /** @brief 主将开始攻击时通知士兵进入战斗 */
    void NotifyAttackStarted();

    UFUNCTION(BlueprintPure, Category = "战斗")
    bool IsInCombat() const { return bIsInCombat; }

    UFUNCTION(BlueprintPure, Category = "战斗", meta = (DisplayName = "战斗中有敌人"))
    bool HasEnemiesInCombat() const { return bHasEnemiesInCombat; }

    UFUNCTION(BlueprintCallable, Category = "战斗", meta = (DisplayName = "设置战斗敌人状态"))
    void SetHasEnemiesInCombat(bool bInCombat);

    UFUNCTION(BlueprintCallable, Category = "战斗")
    virtual void OnAttackHit(AActor* HitTarget);

    UFUNCTION(BlueprintPure, Category = "战斗", meta = (DisplayName = "获取最近攻击的敌方主将"))
    AXBCharacterBase* GetLastAttackedEnemyLeader() const { return LastAttackedEnemyLeader.Get(); }

    /**
     * @brief  为麾下士兵分配敌方目标
     * @param  EnemyLeader 敌方主将
     * @return 无
     * 功能说明: 依据兵种与距离进行目标分配，并在无敌兵时改为攻击敌方主将
     * 详细流程: 校验敌方主将 -> 收集存活士兵 -> 收集存活敌兵 -> 负载与距离择优 -> 通知士兵接收
     * 注意事项: 仅分配存活单位，避免无效目标
     */
    void AssignTargetsToSoldiers(AXBCharacterBase* EnemyLeader);

    /**
     * @brief  为单个士兵分配目标（士兵申请时调用）
     * @param  RequestingSoldier 申请目标的士兵
     * @return 分配的目标（可能为空）
     * 功能说明: 根据负载均衡与距离选择目标
     * 详细流程: 校验申请者 -> 获取敌方主将 -> 收集敌兵 -> 负载统计 -> 选择目标
     * 注意事项: 敌方无士兵时返回敌方主将
     */
    AActor* AssignTargetToSoldier(AXBSoldierCharacter* RequestingSoldier);

    // 🔧 修改 - 记录主将最近攻击到的敌方阵营，用于士兵优先选敌
    UFUNCTION(BlueprintPure, Category = "战斗", meta = (DisplayName = "获取最近攻击的敌方阵营"))
    bool GetLastAttackedEnemyFaction(EXBFaction& OutFaction) const;

    /**
     * @brief  设置草丛隐身状态
     * @param  bHidden 是否隐身
     * @note   详细流程分析: 更新自身隐身状态 -> 设置半透明 -> 调整碰撞 -> 同步所有士兵
     *         性能/架构注意事项: 仅在状态变化时执行，避免重复刷新材质
     */
    UFUNCTION(BlueprintCallable, Category = "草丛", meta = (DisplayName = "设置草丛隐身"))
    void SetHiddenInBush(bool bEnableHidden);

    /**
     * @brief  是否处于草丛隐身
     * @return 是否隐身
     */
    UFUNCTION(BlueprintPure, Category = "草丛", meta = (DisplayName = "是否草丛隐身"))
    bool IsHiddenInBush() const { return bIsHiddenInBush; }

    /**
     * @brief  增加草丛重叠计数（进入草丛时调用）
     * @note   当计数从 0 变为 1 时开启隐身
     */
    UFUNCTION(BlueprintCallable, Category = "草丛", meta = (DisplayName = "增加草丛计数"))
    void IncrementBushOverlapCount();

    /**
     * @brief  减少草丛重叠计数（离开草丛时调用）
     * @note   当计数从 1 变为 0 时关闭隐身
     */
    UFUNCTION(BlueprintCallable, Category = "草丛", meta = (DisplayName = "减少草丛计数"))
    void DecrementBushOverlapCount();

    UFUNCTION(BlueprintPure, Category = "移动", meta = (DisplayName = "是否正在冲刺"))
    bool IsSprinting() const { return bIsSprinting; }

    UFUNCTION(BlueprintPure, Category = "移动", meta = (DisplayName = "获取当前移动速度"))
    float GetCurrentMoveSpeed() const;

    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void SetSoldiersEscaping(bool bEscaping);

    UFUNCTION(BlueprintCallable, Category = "死亡", meta = (DisplayName = "设置伤害来源"))
    void SetLastDamageInstigator(AActor* InInstigator) { LastDamageInstigator = InInstigator; }

    /**
     * @brief  处理受到伤害的回调（可被子类覆写）
     * @param  DamageSource 伤害来源
     * @param  DamageAmount 伤害数值
     * @return 无
     * @note   详细流程分析: 基类默认不处理 -> 子类可接入AI/技能逻辑
     *         性能/架构注意事项: 仅在真实伤害发生时调用，避免无意义事件
     */
    virtual void HandleDamageReceived(AActor* DamageSource, float DamageAmount);

    // ==================== 受击白光效果 ====================

    /**
     * @brief  触发受击白光闪烁效果
     * @return 无
     * @note   详细流程分析: 设置材质参数 WhiteLight 为 1 -> 延迟后恢复为 0
     */
    UFUNCTION(BlueprintCallable, Category = "视觉", meta = (DisplayName = "触发受击白光"))
    void TriggerHitFlash();

    /**
     * @brief  设置白光参数值
     * @param  Value 0-1，0为原色，1为白色
     */
    UFUNCTION(BlueprintCallable, Category = "视觉", meta = (DisplayName = "设置白光参数"))
    void SetHitFlashValue(float Value);

    // ============ 委托事件 ============

    UPROPERTY(BlueprintAssignable, Category = "事件")
    FOnCharacterDeath OnCharacterDeath;

    UPROPERTY(BlueprintAssignable, Category = "事件")
    FOnCombatStateChanged OnCombatStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "事件")
    FOnSoldierCountChanged OnSoldierCountChanged;

    UPROPERTY(BlueprintAssignable, Category = "事件", meta = (DisplayName = "冲刺状态变化"))
    FOnSprintStateChanged OnSprintStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "事件", meta = (DisplayName = "主将进入战斗"))
    FOnEnterCombatDelegate OnEnterCombatDelegate;

    UPROPERTY(BlueprintAssignable, Category = "事件", meta = (DisplayName = "主将分配目标"))
    FOnAssignTargetDelegate OnAssignTargetDelegate;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "编队组件"))
    TObjectPtr<UXBFormationComponent> FormationComponent;

protected:
    /**
     * @brief  刷新已招募士兵的跟随状态
     * @return 无
     * @note   详细流程分析: 主将数据完成初始化后，补齐编队槽位并驱动已招募士兵重新进入跟随逻辑
     *         性能/架构注意事项: 仅在需要时触发，避免在 Tick 中频繁调用
     */
    void RefreshRecruitedSoldiersAfterLeaderInit();
    virtual void BeginPlay() override;
    // 🔧 修改 - 退出时注销感知子系统注册
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
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

    /**
     * @brief  初始化主将数据
     * @return 无
     * @note   详细流程分析: 优先读取外部配置 -> 否则使用 Actor 内部配置 -> 再读取数据表初始化
     *         性能注意: 仅在 BeginPlay 调用一次，避免重复初始化
     */
    virtual void InitializeLeaderData();

    /**
     * @brief  获取外部初始化配置
     * @param  OutConfig 输出配置
     * @return 是否存在外部配置
     * @note   详细流程分析: 默认返回 false，子类可重写提供外部配置
     */
    virtual bool GetExternalInitConfig(FXBGameConfigData& OutConfig) const;

    bool Internal_AddSoldierToArray(AXBSoldierCharacter* Soldier);
    bool Internal_RemoveSoldierFromArray(AXBSoldierCharacter* Soldier);
    void UpdateSoldierCount(int32 OldCount);

    void ApplyGrowthOnSoldiersAdded(int32 SoldierCount);
    void ApplyGrowthOnSoldiersRemoved(int32 SoldierCount);

    void UpdateSkillEffectScaling();
    void UpdateAttackRangeScaling();
    void UpdateLeaderScale();
    void SmoothLeaderScale(float DeltaTime);
    void ApplyLeaderScale(float NewScale);
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长", meta = (DisplayName = "主将缩放插值速度", ClampMin = "0.0"))
    float LeaderScaleInterpSpeed = 6.0f;

    UPROPERTY(VisibleAnywhere, Category = "成长", meta = (DisplayName = "目标主将缩放"))
    float TargetLeaderScale = 1.0f;

    UPROPERTY(VisibleAnywhere, Category = "成长", meta = (DisplayName = "是否存在缩放目标"))
    bool bHasTargetLeaderScale = false;

    UPROPERTY(BlueprintReadOnly, Category = "成长")
    float BaseAttackRange = 150.0f;

    // ==================== 战斗状态 ====================

    UPROPERTY(BlueprintReadOnly, Category = "战斗")
    bool bIsInCombat = false;

    UPROPERTY(BlueprintReadOnly, Category = "战斗")
    TWeakObjectPtr<AXBCharacterBase> LastAttackedEnemyLeader;

    // 🔧 修改 - 记录主将最近攻击到的敌方阵营
    UPROPERTY(BlueprintReadOnly, Category = "战斗")
    bool bHasLastAttackedEnemyFaction = false;

    // 🔧 修改 - 保存最近攻击到的敌方阵营
    UPROPERTY(BlueprintReadOnly, Category = "战斗")
    EXBFaction LastAttackedEnemyFaction = EXBFaction::Neutral;



    // ==================== 移动配置（共用） ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "移动", meta = (DisplayName = "基础移动速度", ClampMin = "0.0"))
    float BaseMoveSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "移动", meta = (DisplayName = "冲刺速度倍率", ClampMin = "1.0", ClampMax = "5.0"))
    float SprintSpeedMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "移动", meta = (DisplayName = "冲刺持续时间", ClampMin = "0.0"))
    float SprintDuration = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "移动", meta = (DisplayName = "速度变化平滑度", ClampMin = "1.0"))
    float SpeedInterpRate = 15.0f;

    UPROPERTY(BlueprintReadOnly, Category = "移动")
    bool bIsSprinting = false;

    UPROPERTY(BlueprintReadOnly, Category = "移动")
    float TargetMoveSpeed = 0.0f;

    // ✨ 新增 - 按键冲刺持续时间计时器
    FTimerHandle SprintDurationTimerHandle;

    // ✨ 新增 - 无输入触发时的自动冲刺移动开关
    bool bAutoSprintMove = false;

    // ==================== 配置 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "配置数据表"))
    TObjectPtr<UDataTable> ConfigDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "配置行名"))
    FName ConfigRowName;

    UPROPERTY(BlueprintReadOnly, Category = "配置")
    FXBLeaderTableRow CachedLeaderData;

    UPROPERTY(BlueprintReadOnly, Category = "成长")
    FXBGrowthConfigCache GrowthConfigCache;

    UPROPERTY(BlueprintReadOnly, Category = "战斗", meta = (DisplayName = "战斗中有敌人"))
    bool bHasEnemiesInCombat = false;

    // ==================== 草丛隐身 ====================

    /** 草丛隐身时修改的材质参数名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "草丛", 
              meta = (DisplayName = "隐身材质参数名"))
    FName BushHiddenParameterName = FName("HiddenAlpha");

    /** 隐身状态时的参数值（通常为半透明） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "草丛", 
              meta = (DisplayName = "隐身参数值", ClampMin = "0.0", ClampMax = "1.0"))
    float BushHiddenParameterValue = 0.5f;

    /** 正常状态时的参数值 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "草丛", 
              meta = (DisplayName = "正常参数值", ClampMin = "0.0", ClampMax = "1.0"))
    float BushVisibleParameterValue = 1.0f;

    /** 草丛隐身时修改的亮度材质参数名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "草丛", 
              meta = (DisplayName = "亮度材质参数名"))
    FName BushHeightParameterName = FName("Height");

    /** 隐身状态时的亮度参数值 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "草丛", 
              meta = (DisplayName = "隐身亮度值", ClampMin = "0.0", ClampMax = "1.0"))
    float BushHiddenHeightValue = 0.2f;

    /** 正常状态时的亮度参数值 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "草丛", 
              meta = (DisplayName = "正常亮度值", ClampMin = "0.0", ClampMax = "1.0"))
    float BushVisibleHeightValue = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "草丛", meta = (DisplayName = "是否草丛隐身"))
    bool bIsHiddenInBush = false;

    /** 草丛重叠计数（支持同时在多个草丛中） */
    UPROPERTY(BlueprintReadOnly, Category = "草丛", meta = (DisplayName = "草丛重叠计数"))
    int32 BushOverlapCount = 0;

    UPROPERTY()
    bool bCachedBushCollisionResponse = false;

    UPROPERTY()
    TEnumAsByte<ECollisionResponse> CachedLeaderCollisionResponse = ECR_Block;

    UPROPERTY()
    TEnumAsByte<ECollisionResponse> CachedSoldierCollisionResponse = ECR_Block;

    /** 动态材质实例数组（隐身时创建） */
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInstanceDynamic>> BushDynamicMaterials;

    /** 原始材质数组（用于恢复） */
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> CachedOriginalMaterials;

    // ==================== 受击白光效果 ====================

    /** 白光材质参数名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "受击白光", 
              meta = (DisplayName = "白光参数名"))
    FName HitFlashParameterName = FName("WhiteLight");

    /** 白光持续时间（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "受击白光", 
              meta = (DisplayName = "白光持续时间", ClampMin = "0.01", ClampMax = "2.0"))
    float HitFlashDuration = 0.1f;

    /** 是否启用受击白光 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "受击白光", 
              meta = (DisplayName = "启用受击白光"))
    bool bEnableHitFlash = true;

    /** 白光动态材质实例数组 */
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInstanceDynamic>> HitFlashDynamicMaterials;

    /** 白光恢复计时器 */
    FTimerHandle HitFlashTimerHandle;

    /** 初始化白光动态材质 */
    void InitializeHitFlashMaterials();

    /** 重置白光参数 */
    void ResetHitFlash();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "掉落", meta = (DisplayName = "士兵掉落配置"))
    FXBSoldierDropConfig SoldierDropConfig;

    // ==================== 招募配置 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "招募", meta = (DisplayName = "士兵数据表"))
    TObjectPtr<UDataTable> SoldierDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "招募", meta = (DisplayName = "招募士兵行名"))
    FName RecruitSoldierRowName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "招募", meta = (DisplayName = "士兵Actor类"))
    TSubclassOf<AXBSoldierCharacter> SoldierActorClass;

    // ==================== 音效配置 ====================

    /** 冲刺音效标签（循环播放） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Sound",
              meta = (DisplayName = "冲刺音效", Categories = "Sound"))
    FGameplayTag SprintSoundTag;

    /** 士兵飞出音效标签（将领死亡时） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Sound",
              meta = (DisplayName = "士兵飞出音效", Categories = "Sound"))
    FGameplayTag SoldierDropSoundTag;

    /** 招募士兵音效标签 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Sound",
              meta = (DisplayName = "招募士兵音效", Categories = "Sound"))
    FGameplayTag RecruitSoundTag;

    /** 当前冲刺音效组件（用于停止循环音效） */
    UPROPERTY()
    TObjectPtr<UAudioComponent> SprintAudioComponent;

    /** 开始播放冲刺音效 */
    void PlaySprintSound();

    /** 停止冲刺音效 */
    void StopSprintSound();

    /** 播放招募士兵音效 */
    void PlayRecruitSound();

    /** 播放士兵飞出音效 */
    void PlaySoldierDropSound();

    /** 主将死亡音效标签 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Sound",
              meta = (DisplayName = "死亡音效", Categories = "Sound"))
    FGameplayTag DeathSoundTag;

    /** 播放死亡音效 */
    void PlayDeathSound();

    // ==================== 冲刺特效配置 ====================

    /** 冲刺特效组件（默认不激活，冲刺时激活） */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "XB|VFX",
              meta = (DisplayName = "冲刺特效组件"))
    TObjectPtr<UNiagaraComponent> SprintNiagaraComponent;

    /** 冲刺拖尾特效组件（Cascade 粒子，默认不激活，冲刺时激活） */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "XB|VFX",
              meta = (DisplayName = "冲刺拖尾特效组件"))
    TObjectPtr<UParticleSystemComponent> SprintTrailParticleComponent;

    /** 拖尾特效延迟启动时间（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|VFX",
              meta = (DisplayName = "拖尾延迟启动时间", ClampMin = "0.0"))
    float SprintTrailDelayTime = 0.0f;

    /** 特效消失时间（秒，0 表示立即停止） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|VFX",
              meta = (DisplayName = "特效消失时间", ClampMin = "0.0"))
    float SprintVFXFadeOutTime = 0.0f;

    /** 冲刺特效提前停用时间（秒，0 表示不提前停用，跟随冲刺结束） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|VFX",
              meta = (DisplayName = "冲刺特效提前停用时间", ClampMin = "0.0"))
    float SprintVFXEarlyStopTime = 0.0f;

    /** 开始播放冲刺特效 */
    void PlaySprintVFX();

    /** 停止冲刺特效 */
    void StopSprintVFX();

    /** 拖尾延迟启动计时器 */
    FTimerHandle SprintTrailDelayTimerHandle;

    /** 冲刺特效提前停用计时器 */
    FTimerHandle SprintVFXEarlyStopTimerHandle;

    // ==================== 死亡系统 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "视觉", meta = (DisplayName = "动画蓝图类"))
    TSubclassOf<UAnimInstance> AnimClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "死亡", meta = (DisplayName = "死亡蒙太奇"))
    TObjectPtr<UAnimMontage> DeathMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "死亡", meta = (DisplayName = "死亡后消失延迟", ClampMin = "0.0"))
    float DeathDestroyDelay = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "死亡", meta = (DisplayName = "蒙太奇结束后开始计时"))
    bool bDelayAfterMontage = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "死亡", meta = (DisplayName = "死亡缩放比例", ClampMin = "0.1"))
    float DeathScale = 0.2f;

    UPROPERTY(BlueprintReadOnly, Category = "死亡")
    bool bIsDead = false;

    UPROPERTY(BlueprintReadOnly, Category = "死亡")
    bool bIsCleaningUpSoldiers = false;

    UPROPERTY(BlueprintReadOnly, Category = "死亡", meta = (DisplayName = "最后伤害来源"))
    TWeakObjectPtr<AActor> LastDamageInstigator;

    FTimerHandle DeathDestroyTimerHandle;

    /** @brief 主将死亡时是否杀死麾下士兵（设为 false 时，士兵保持存活并解除绑定） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "死亡", meta = (DisplayName = "死亡时杀死士兵"))
    bool bKillSoldiersOnDeath = true;

    // ==================== 死亡渐隐效果 ====================

    /** 启用死亡渐隐（通过材质参数控制透明度） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "死亡|渐隐效果",
              meta = (DisplayName = "启用死亡渐隐"))
    bool bEnableDeathFade = false;

    /** 死亡渐隐材质参数名称（Scalar参数） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "死亡|渐隐效果",
              meta = (DisplayName = "渐隐参数名称",
                      EditCondition = "bEnableDeathFade", EditConditionHides))
    FName DeathFadeParameterName = FName("Opacity");

    /** 死亡渐隐时间（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "死亡|渐隐效果",
              meta = (DisplayName = "渐隐时间", ClampMin = "0.05", ClampMax = "5.0",
                      EditCondition = "bEnableDeathFade", EditConditionHides))
    float DeathFadeDuration = 1.0f;

    /** 死亡渐隐延迟（动画播放后再开始渐隐） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "死亡|渐隐效果",
              meta = (DisplayName = "渐隐延迟", ClampMin = "0.0", ClampMax = "10.0",
                      EditCondition = "bEnableDeathFade", EditConditionHides))
    float DeathFadeDelay = 0.0f;

    /** 开始死亡渐隐动画 */
    void StartDeathFade();

    /** 更新死亡渐隐进度 */
    void UpdateDeathFade();

    /** 死亡渐隐动态材质实例 */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> DeathFadeMaterials;

    /** 死亡渐隐计时器 */
    FTimerHandle DeathFadeTimerHandle;

    /** 死亡渐隐当前进度（0~1） */
    float DeathFadeProgress = 0.0f;

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
    

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "无敌人脱战延迟", ClampMin = "0.0"))
    float NoEnemyDisengageDelay = 3.0f;

    FTimerHandle CombatTimeoutHandle;

    FTimerHandle NoEnemyDisengageHandle;

public:
    void ScheduleNoEnemyDisengage();

    void CancelNoEnemyDisengage();

private:
    UFUNCTION()
    void OnCombatTimeout();

    /**
     * @brief  生成初始士兵
     * @param  DesiredCount 期望士兵数量
     * @return 无
     * @note   详细流程分析: 计算缺口 -> 从对象池或新建 -> 初始化并招募
     */
    void SpawnInitialSoldiers(int32 DesiredCount);
};
