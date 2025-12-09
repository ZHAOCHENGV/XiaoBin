/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Character/XBCharacterBase.h

/**
 * @file XBCharacterBase.h
 * @brief 角色基类 - 包含阵营、士兵管理、战斗组件、死亡系统等功能
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

    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void AddSoldier(AXBSoldierActor* Soldier);

    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void RemoveSoldier(AXBSoldierActor* Soldier);

    UFUNCTION(BlueprintPure, Category = "士兵")
    int32 GetSoldierCount() const { return Soldiers.Num(); }

    UFUNCTION(BlueprintPure, Category = "士兵")
    const TArray<AXBSoldierActor*>& GetSoldiers() const { return Soldiers; }

    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void OnSoldierDied();

    UFUNCTION(BlueprintCallable, Category = "成长")
    void OnSoldiersAdded(int32 SoldierCount);

    // ============ 战斗组件 ============

    UFUNCTION(BlueprintPure, Category = "战斗")
    UXBCombatComponent* GetCombatComponent() const { return CombatComponent; }

    // ============ 召回系统 ============

    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void RecallAllSoldiers();

    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void SetSoldiersEscaping(bool bEscaping);

    // ============ 死亡系统 ============

    /**
     * @brief 处理角色死亡
     * @note 播放死亡蒙太奇，延迟后销毁
     */
    UFUNCTION(BlueprintCallable, Category = "死亡")
    virtual void HandleDeath();

    /**
     * @brief 检查角色是否已死亡
     */
    UFUNCTION(BlueprintPure, Category = "死亡")
    bool IsDead() const { return bIsDead; }

    /**
     * @brief 死亡事件委托
     */
    UPROPERTY(BlueprintAssignable, Category = "死亡")
    FOnCharacterDeath OnCharacterDeath;

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

    // ============ 配置 ============

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "配置数据表"))
    TObjectPtr<UDataTable> ConfigDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "配置行名"))
    FName ConfigRowName;

    UPROPERTY(BlueprintReadOnly, Category = "配置")
    FXBLeaderTableRow CachedLeaderData;

    UPROPERTY(BlueprintReadOnly, Category = "成长")
    FXBGrowthConfigCache GrowthConfigCache;

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
};
