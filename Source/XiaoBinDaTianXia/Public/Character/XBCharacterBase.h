/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Character/XBCharacterBase.h

/**
 * @file XBCharacterBase.h
 * @brief 角色基类 - 包含所有将领共用的组件和功能
 * 
 * @note 🔧 修改记录:
 *       1. 新增 GetSoldierActorClass() 公开访问器
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
class AXBSoldierActor;
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

    /** @brief 每个士兵增加的生命值 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长", meta = (DisplayName = "每士兵生命加成"))
    float HealthPerSoldier = 20.0f;

    /** @brief 每个士兵增加的体型缩放（累加方式：1 + 0.1 + 0.1...） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长", meta = (DisplayName = "每士兵体型加成"))
    float ScalePerSoldier = 0.01f;

    /** @brief 最大体型缩放 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长", meta = (DisplayName = "最大体型缩放"))
    float MaxScale = 2.f;

    // ✨ 新增 - 技能特效缩放配置
    /** @brief 是否启用技能特效缩放 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长", meta = (DisplayName = "启用技能特效缩放"))
    bool bEnableSkillEffectScaling = true;

    /** @brief 技能特效缩放倍率（相对于角色缩放） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长", meta = (DisplayName = "技能特效缩放倍率", ClampMin = "0.1"))
    float SkillEffectScaleMultiplier = 1.0f;

    // ✨ 新增 - 攻击范围缩放配置
    /** @brief 是否启用攻击范围缩放 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长", meta = (DisplayName = "启用攻击范围缩放"))
    bool bEnableAttackRangeScaling = true;

    /** @brief 攻击范围缩放倍率（相对于角色缩放） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长", meta = (DisplayName = "攻击范围缩放倍率", ClampMin = "0.1"))
    float AttackRangeScaleMultiplier = 1.0f;
};

USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBSoldierDropConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "掉落", meta = (DisplayName = "掉落数量", ClampMin = "0"))
    int32 DropCount = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "掉落", meta = (DisplayName = "掉落半径", ClampMin = "50.0"))
    float DropRadius = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "掉落", meta = (DisplayName = "掉落士兵类"))
    TSubclassOf<AXBSoldierActor> DropSoldierClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "掉落", meta = (DisplayName = "掉落动画时长", ClampMin = "0.1"))
    float DropAnimDuration = 0.5f;
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
    virtual void AddSoldier(AXBSoldierActor* Soldier);

    UFUNCTION(BlueprintCallable, Category = "士兵")
    FName GetRecruitSoldierRowName() const { return RecruitSoldierRowName; }

    UFUNCTION(BlueprintCallable, Category = "士兵")
    UDataTable* GetSoldierDataTable() const { return SoldierDataTable; }

    // ✨ 新增 - 公开访问器，修复 protected 访问问题
    /**
     * @brief 获取士兵Actor类
     * @return 士兵Actor类引用
     * @note 用于磁场组件生成士兵实例
     */
    UFUNCTION(BlueprintCallable, Category = "士兵", meta = (DisplayName = "获取士兵Actor类"))
    TSubclassOf<AXBSoldierActor> GetSoldierActorClass() const { return SoldierActorClass; }

    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void RemoveSoldier(AXBSoldierActor* Soldier);

    UFUNCTION(BlueprintPure, Category = "士兵")
    int32 GetSoldierCount() const { return Soldiers.Num(); }

    UFUNCTION(BlueprintPure, Category = "士兵")
    const TArray<AXBSoldierActor*>& GetSoldiers() const { return Soldiers; }


    /**
     * @brief 士兵增加时的成长逻辑
     * @param SoldierCount 增加的士兵数量
     * @note 功能：
     *       1. 增加体型（累加方式）
     *       2. 增加血量（溢出时提升最大值）
     *       3. 缩放技能特效和攻击范围
     */
    UFUNCTION(BlueprintCallable, Category = "成长")
    void OnSoldiersAdded(int32 SoldierCount);

    /**
     * @brief 士兵死亡时的缩减逻辑
     * @param DeadSoldier 死亡的士兵
     * @note 功能：
     *       1. 缩小体型
     *       2. 不减少血量（只减体型）
     *       3. 重新缩放技能特效和攻击范围
     */
    UFUNCTION(BlueprintCallable, Category = "成长")
    virtual void OnSoldierDied(AXBSoldierActor* DeadSoldier);

    // ✨ 新增 - 获取当前缩放倍率
    /**
     * @brief 获取当前角色的实际缩放倍率
     * @return 缩放倍率（如 1.5 表示放大到 150%）
     */
    UFUNCTION(BlueprintPure, Category = "成长", meta = (DisplayName = "获取当前缩放倍率"))
    float GetCurrentScale() const;

    // ✨ 新增 - 获取当前攻击范围
    /**
     * @brief 获取经过缩放后的攻击范围
     * @return 实际攻击范围
     */
    UFUNCTION(BlueprintPure, Category = "成长", meta = (DisplayName = "获取当前攻击范围"))
    float GetScaledAttackRange() const;

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

    /**
    * @brief 脱离战斗（逃跑）
    * @note ✨ 新增方法 - 玩家按键触发
    *       功能：
    *       1. 立即退出战斗状态
    *       2. 所有士兵停止追击，返回队列
    *       3. 清除所有战斗目标
    *       4. 进入冲刺状态加速逃离
    */
    UFUNCTION(BlueprintCallable, Category = "战斗", meta = (DisplayName = "脱离战斗（逃跑）"))
    virtual void DisengageFromCombat();


protected:
    // ✨ 新增 - 逃跑配置
    /** @brief 逃跑时是否自动开启冲刺 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "逃跑时自动冲刺"))
    bool bSprintWhenDisengaging = true;

    /** @brief 脱离战斗后的冲刺持续时间（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "逃跑冲刺时长", ClampMin = "0.0"))
    float DisengageSprintDuration = 3.0f;

    /** @brief 脱离战斗的冷却时间（防止频繁切换） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "脱离冷却时间", ClampMin = "0.0"))
    float DisengageCooldown = 2.0f;

    /** @brief 上次脱离战斗的时间 */
    float LastDisengageTime = 0.0f;

    /** @brief 脱离战斗计时器句柄 */
    FTimerHandle DisengageSprintTimerHandle;
    

    // ✨ 新增 - 应用缩放到技能特效
    /**
     * @brief 更新技能特效的缩放
     * @note 遍历所有激活的技能实例，应用缩放
     */
    void UpdateSkillEffectScaling();

    // ✨ 新增 - 应用缩放到攻击范围
    /**
     * @brief 更新攻击范围（用于碰撞检测）
     */
    void UpdateAttackRangeScaling();

    // ✨ 新增 - 更新角色体型
    /**
     * @brief 根据当前士兵数量更新体型
     * @note 使用累加方式：BaseScale + (Count * ScalePerSoldier)
     */
    void UpdateLeaderScale();

    // ✨ 新增 - 更新角色血量
    /**
     * @brief 增加血量，支持溢出提升最大值
     * @param HealthToAdd 要增加的血量
     */
    void AddHealthWithOverflow(float HealthToAdd);
    
    
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



    UFUNCTION(BlueprintPure, Category = "移动", meta = (DisplayName = "是否正在冲刺"))
    bool IsSprinting() const { return bIsSprinting; }

    UFUNCTION(BlueprintPure, Category = "移动", meta = (DisplayName = "获取当前移动速度"))
    float GetCurrentMoveSpeed() const;



    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void SetSoldiersEscaping(bool bEscaping);



    // ============ 委托事件 ============

    UPROPERTY(BlueprintAssignable, Category = "事件")
    FOnCharacterDeath OnCharacterDeath;

    UPROPERTY(BlueprintAssignable, Category = "事件")
    FOnCombatStateChanged OnCombatStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "事件")
    FOnSoldierCountChanged OnSoldierCountChanged;

    UPROPERTY(BlueprintAssignable, Category = "事件", meta = (DisplayName = "冲刺状态变化"))
    FOnSprintStateChanged OnSprintStateChanged;

protected:
    virtual void BeginPlay() override;
    virtual void PossessedBy(AController* NewController) override;

    virtual void InitializeAbilitySystem();

    UFUNCTION()
    void OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    UFUNCTION()
    void OnDestroyTimerExpired();

    virtual void PreDestroyCleanup();

    virtual void SpawnDroppedSoldiers();

    void ReassignSoldierSlots(int32 StartIndex);
    

    virtual void UpdateSprint(float DeltaTime);

    virtual void SetupMovementComponent();

    UFUNCTION()
    virtual void OnMagnetFieldActorEntered(AActor* EnteredActor);


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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "编队组件"))
    TObjectPtr<UXBFormationComponent> FormationComponent;

    // ==================== 阵营 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "阵营", meta = (DisplayName = "阵营"))
    EXBFaction Faction = EXBFaction::Neutral;

    // ==================== 士兵管理 ====================

    UPROPERTY(BlueprintReadOnly, Category = "士兵")
    TArray<AXBSoldierActor*> Soldiers;

    UPROPERTY(BlueprintReadOnly, Category = "成长")
    int32 CurrentSoldierCount = 0;

    // ✨ 新增 - 基础缩放（从数据表读取）
    UPROPERTY(BlueprintReadOnly, Category = "成长")
    float BaseScale = 1.0f;

    // ✨ 新增 - 基础攻击范围（从数据表读取）
    UPROPERTY(BlueprintReadOnly, Category = "成长")
    float BaseAttackRange = 150.0f;

    // ==================== 战斗状态 ====================

    UPROPERTY(BlueprintReadOnly, Category = "战斗")
    bool bIsInCombat = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "战斗超时时间"))
    float CombatTimeoutDuration = 5.0f;

    FTimerHandle CombatTimeoutHandle;

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

    // 🔧 修改 - 将访问权限改为 public，或添加公开访问器（已选择后者）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "招募", meta = (DisplayName = "士兵Actor类"))
    TSubclassOf<AXBSoldierActor> SoldierActorClass;

    // ==================== 死亡系统 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "死亡", meta = (DisplayName = "死亡蒙太奇"))
    TObjectPtr<UAnimMontage> DeathMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "死亡", meta = (DisplayName = "死亡后消失延迟", ClampMin = "0.0"))
    float DeathDestroyDelay = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "死亡", meta = (DisplayName = "蒙太奇结束后开始计时"))
    bool bDelayAfterMontage = true;

    UPROPERTY(BlueprintReadOnly, Category = "死亡")
    bool bIsDead = false;

    FTimerHandle DeathDestroyTimerHandle;

private:
    UFUNCTION()
    void OnCombatTimeout();

};
