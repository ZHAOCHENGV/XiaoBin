/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Character/XBCharacterBase.h

/**
 * @file XBCharacterBase.h
 * @brief 角色基类 - 包含阵营、士兵管理、战斗组件、死亡系统等功能
 * 
 * @note 🔧 修改记录:
 *       1. 新增战斗状态系统 - 用于触发士兵进入战斗
 *       2. 新增士兵掉落系统 - 将领死亡后掉落士兵
 *       3. 完善血量成长逻辑 - 区分回复和溢出提升上限
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
class AXBSoldierActor;
class UAnimMontage;

// ✨ 新增 - 死亡事件委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDeath, AXBCharacterBase*, DeadCharacter);

// ✨ 新增 - 战斗状态变化委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatStateChanged, bool, bInCombat);

// ✨ 新增 - 士兵数量变化委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSoldierCountChanged, int32, OldCount, int32, NewCount);

/**
 * @brief 成长配置缓存结构体
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBGrowthConfigCache
{
    GENERATED_BODY()

    /** @brief 每个士兵提供的生命值 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长", meta = (DisplayName = "每士兵生命加成"))
    float HealthPerSoldier = 5.0f;

    /** @brief 每个士兵提供的体型缩放 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长", meta = (DisplayName = "每士兵体型加成"))
    float ScalePerSoldier = 0.01f;

    /** @brief 最大体型缩放 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长", meta = (DisplayName = "最大体型缩放"))
    float MaxScale = 2.0f;
};

// ✨ 新增 - 士兵掉落配置
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBSoldierDropConfig
{
    GENERATED_BODY()

    /** @brief 死亡时掉落士兵数量 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "掉落", meta = (DisplayName = "掉落数量", ClampMin = "0"))
    int32 DropCount = 5;

    /** @brief 掉落半径范围 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "掉落", meta = (DisplayName = "掉落半径", ClampMin = "50.0"))
    float DropRadius = 300.0f;

    /** @brief 掉落士兵类 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "掉落", meta = (DisplayName = "掉落士兵类"))
    TSubclassOf<AXBSoldierActor> DropSoldierClass;

    /** @brief 掉落动画时长 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "掉落", meta = (DisplayName = "掉落动画时长", ClampMin = "0.1"))
    float DropAnimDuration = 0.5f;
};

/**
 * @brief 角色基类
 */
UCLASS()
class XIAOBINDATIANXIA_API AXBCharacterBase : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AXBCharacterBase();

    // ============ IAbilitySystemInterface ============
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

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

    /**
     * @brief 添加士兵到队列
     * @param Soldier 士兵Actor
     * @note 会自动分配槽位并触发成长效果
     */
    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void AddSoldier(AXBSoldierActor* Soldier);

    /**
     * @brief 从队列移除士兵
     * @param Soldier 士兵Actor
     * @note 🔧 修改 - 实现补位逻辑，后面的士兵向前补
     */
    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void RemoveSoldier(AXBSoldierActor* Soldier);

    UFUNCTION(BlueprintPure, Category = "士兵")
    int32 GetSoldierCount() const { return Soldiers.Num(); }

    UFUNCTION(BlueprintPure, Category = "士兵")
    const TArray<AXBSoldierActor*>& GetSoldiers() const { return Soldiers; }

    /**
     * @brief 士兵死亡回调
     * @note 触发补位逻辑和缩放更新
     */
    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void OnSoldierDied(AXBSoldierActor* DeadSoldier);

    /**
     * @brief 士兵添加后的成长处理
     * @param SoldierCount 新增士兵数量
     * @note 🔧 修改 - 实现设计文档的血量回复逻辑:
     *       1. 优先回复当前血量
     *       2. 溢出部分才提升最大血量
     */
    UFUNCTION(BlueprintCallable, Category = "成长")
    void OnSoldiersAdded(int32 SoldierCount);

    // ============ 战斗组件 ============

    UFUNCTION(BlueprintPure, Category = "战斗")
    UXBCombatComponent* GetCombatComponent() const { return CombatComponent; }

    // ============ 战斗状态系统 ============

    /**
     * @brief 进入战斗状态
     * @note ✨ 新增 - 玩家攻击命中时调用，通知所有士兵进入战斗
     */
    UFUNCTION(BlueprintCallable, Category = "战斗")
    virtual void EnterCombat();

    /**
     * @brief 退出战斗状态
     * @note ✨ 新增 - 周围无敌人时调用，士兵返回队列
     */
    UFUNCTION(BlueprintCallable, Category = "战斗")
    virtual void ExitCombat();

    /**
     * @brief 检查是否在战斗中
     */
    UFUNCTION(BlueprintPure, Category = "战斗")
    bool IsInCombat() const { return bIsInCombat; }

    /**
     * @brief 攻击命中目标时调用
     * @param HitTarget 命中的目标
     * @note ✨ 新增 - 用于触发士兵进入战斗
     */
    UFUNCTION(BlueprintCallable, Category = "战斗")
    virtual void OnAttackHit(AActor* HitTarget);

    // ============ 召回系统 ============

    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void RecallAllSoldiers();

    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void SetSoldiersEscaping(bool bEscaping);

    // ============ 死亡系统 ============

    /**
     * @brief 处理角色死亡
     * @note 🔧 修改 - 增加士兵掉落逻辑
     */
    UFUNCTION(BlueprintCallable, Category = "死亡")
    virtual void HandleDeath();

    /**
     * @brief 检查角色是否已死亡
     */
    UFUNCTION(BlueprintPure, Category = "死亡")
    bool IsDead() const { return bIsDead; }

    // ============ 委托事件 ============

    /** @brief 死亡事件委托 */
    UPROPERTY(BlueprintAssignable, Category = "死亡")
    FOnCharacterDeath OnCharacterDeath;

    /** @brief 战斗状态变化事件 */
    UPROPERTY(BlueprintAssignable, Category = "战斗")
    FOnCombatStateChanged OnCombatStateChanged;

    /** @brief 士兵数量变化事件 */
    UPROPERTY(BlueprintAssignable, Category = "士兵")
    FOnSoldierCountChanged OnSoldierCountChanged;

protected:
    virtual void BeginPlay() override;
    virtual void PossessedBy(AController* NewController) override;

    /** @brief 初始化ASC */
    virtual void InitializeAbilitySystem();

    /**
     * @brief 死亡蒙太奇播放结束回调
     */
    UFUNCTION()
    void OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    /**
     * @brief 延迟销毁定时器回调
     */
    UFUNCTION()
    void OnDestroyTimerExpired();

    /**
     * @brief 执行角色销毁前的清理
     */
    virtual void PreDestroyCleanup();

    // ✨ 新增 - 士兵掉落相关

    /**
     * @brief 生成掉落的士兵
     * @note 将领死亡时调用，从中心向四周掉落士兵
     */
    virtual void SpawnDroppedSoldiers();

    /**
     * @brief 更新士兵槽位（补位逻辑）
     * @param StartIndex 从哪个索引开始重新分配
     */
    void ReassignSoldierSlots(int32 StartIndex);

    /**
     * @brief 更新将领缩放（不更新血量）
     * @note 士兵死亡时只缩小不扣血
     */
    void UpdateLeaderScale();

protected:
    // ============ 组件 ============

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "能力系统组件"))
    TObjectPtr<UXBAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "属性集"))
    TObjectPtr<UXBAttributeSet> AttributeSet;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "战斗组件"))
    TObjectPtr<UXBCombatComponent> CombatComponent;

    // ============ 阵营 ============

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "阵营", meta = (DisplayName = "阵营"))
    EXBFaction Faction = EXBFaction::Neutral;

    // ============ 士兵管理 ============

    UPROPERTY(BlueprintReadOnly, Category = "士兵")
    TArray<AXBSoldierActor*> Soldiers;

    UPROPERTY(BlueprintReadOnly, Category = "成长")
    int32 CurrentSoldierCount = 0;

    // ============ 战斗状态 ============

    /** @brief 是否处于战斗状态 */
    UPROPERTY(BlueprintReadOnly, Category = "战斗")
    bool bIsInCombat = false;

    /** @brief 战斗超时时间（秒） - 无攻击后自动退出战斗 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "战斗超时时间"))
    float CombatTimeoutDuration = 5.0f;

    /** @brief 战斗超时计时器 */
    FTimerHandle CombatTimeoutHandle;

    // ============ 配置 ============

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "配置数据表"))
    TObjectPtr<UDataTable> ConfigDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "配置行名"))
    FName ConfigRowName;

    UPROPERTY(BlueprintReadOnly, Category = "配置")
    FXBLeaderTableRow CachedLeaderData;

    UPROPERTY(BlueprintReadOnly, Category = "成长")
    FXBGrowthConfigCache GrowthConfigCache;

    // ✨ 新增 - 士兵掉落配置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "掉落", meta = (DisplayName = "士兵掉落配置"))
    FXBSoldierDropConfig SoldierDropConfig;

    // ============ 死亡系统 ============

    /** @brief 死亡蒙太奇 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "死亡", meta = (DisplayName = "死亡蒙太奇"))
    TObjectPtr<UAnimMontage> DeathMontage;

    /** @brief 死亡后延迟消失时间（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "死亡", meta = (DisplayName = "死亡后消失延迟", ClampMin = "0.0"))
    float DeathDestroyDelay = 3.0f;

    /** @brief 是否在死亡蒙太奇播放完后才开始计时 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "死亡", meta = (DisplayName = "蒙太奇结束后开始计时"))
    bool bDelayAfterMontage = true;

    /** @brief 是否已死亡 */
    UPROPERTY(BlueprintReadOnly, Category = "死亡")
    bool bIsDead = false;

    /** @brief 死亡销毁定时器句柄 */
    FTimerHandle DeathDestroyTimerHandle;

private:
    /** @brief 战斗超时回调 */
    UFUNCTION()
    void OnCombatTimeout();
};
