/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Character/XBCharacterBase.h

/**
 * @file XBCharacterBase.h
 * @brief 角色基类 - 包含阵营、士兵管理、战斗组件等功能
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Data/XBLeaderDataTable.h"
#include "Army/XBSoldierTypes.h"  // ✨ 新增 - 包含 EXBFaction 枚举定义
#include "XBCharacterBase.generated.h"

class UAbilitySystemComponent;
class UXBAbilitySystemComponent;
class UXBAttributeSet;
class UXBCombatComponent;
class UDataTable;
class AXBSoldierActor;

// ❌ 删除 - EXBFaction 枚举已在 XBSoldierTypes.h 中定义
// UENUM(BlueprintType)
// enum class EXBFaction : uint8
// {
//     Neutral     UMETA(DisplayName = "中立"),
//     Player      UMETA(DisplayName = "玩家"),
//     Enemy       UMETA(DisplayName = "敌人"),
//     Ally        UMETA(DisplayName = "友军")
// };

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

    /**
     * @brief 从数据表初始化角色
     * @param DataTable 数据表
     * @param RowName 行名
     */
    UFUNCTION(BlueprintCallable, Category = "初始化")
    virtual void InitializeFromDataTable(UDataTable* DataTable, FName RowName);

    /**
     * @brief 应用属性到ASC
     */
    UFUNCTION(BlueprintCallable, Category = "属性")
    void ApplyInitialAttributes();

    // ============ 阵营系统 ============

    /**
     * @brief 获取阵营
     * @return 当前阵营
     */
    UFUNCTION(BlueprintPure, Category = "阵营")
    EXBFaction GetFaction() const { return Faction; }

    /**
     * @brief 设置阵营
     * @param NewFaction 新阵营
     */
    UFUNCTION(BlueprintCallable, Category = "阵营")
    void SetFaction(EXBFaction NewFaction) { Faction = NewFaction; }

    /**
     * @brief 检查是否对目标敌对
     * @param Other 目标角色
     * @return 是否敌对
     */
    UFUNCTION(BlueprintPure, Category = "阵营")
    bool IsHostileTo(const AXBCharacterBase* Other) const;

    /**
     * @brief 检查是否对目标友好
     * @param Other 目标角色
     * @return 是否友好
     */
    UFUNCTION(BlueprintPure, Category = "阵营")
    bool IsFriendlyTo(const AXBCharacterBase* Other) const;

    // ============ 士兵管理 ============

    /**
     * @brief 添加士兵
     * @param Soldier 士兵Actor
     */
    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void AddSoldier(AXBSoldierActor* Soldier);

    /**
     * @brief 移除士兵
     * @param Soldier 士兵Actor
     */
    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void RemoveSoldier(AXBSoldierActor* Soldier);

    /**
     * @brief 获取士兵数量
     * @return 当前士兵数量
     */
    UFUNCTION(BlueprintPure, Category = "士兵")
    int32 GetSoldierCount() const { return Soldiers.Num(); }

    /**
     * @brief 获取所有士兵
     * @return 士兵数组
     */
    UFUNCTION(BlueprintPure, Category = "士兵")
    const TArray<AXBSoldierActor*>& GetSoldiers() const { return Soldiers; }

    /**
     * @brief 士兵死亡回调
     */
    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void OnSoldierDied();

    /**
     * @brief 添加士兵时更新属性
     * @param SoldierCount 添加的士兵数量
     */
    UFUNCTION(BlueprintCallable, Category = "成长")
    void OnSoldiersAdded(int32 SoldierCount);

    // ============ 战斗组件 ============

    /**
     * @brief 获取战斗组件
     * @return 战斗组件指针
     */
    UFUNCTION(BlueprintPure, Category = "战斗")
    UXBCombatComponent* GetCombatComponent() const { return CombatComponent; }

    // ============ 召回系统 ============

    /**
     * @brief 召回所有士兵
     */
    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void RecallAllSoldiers();

    /**
     * @brief 设置士兵逃跑状态
     * @param bEscaping 是否逃跑
     */
    UFUNCTION(BlueprintCallable, Category = "士兵")
    virtual void SetSoldiersEscaping(bool bEscaping);

protected:
    virtual void BeginPlay() override;
    virtual void PossessedBy(AController* NewController) override;

    /** @brief 初始化ASC */
    virtual void InitializeAbilitySystem();

protected:
    // ============ 组件 ============

    /** @brief 能力系统组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "能力系统组件"))
    TObjectPtr<UXBAbilitySystemComponent> AbilitySystemComponent;

    /** @brief 属性集 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "属性集"))
    TObjectPtr<UXBAttributeSet> AttributeSet;

    /** @brief 战斗组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "战斗组件"))
    TObjectPtr<UXBCombatComponent> CombatComponent;

    // ============ 阵营 ============

    /** @brief 阵营 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "阵营", meta = (DisplayName = "阵营"))
    EXBFaction Faction = EXBFaction::Neutral;

    // ============ 士兵管理 ============

    /** @brief 士兵列表 */
    UPROPERTY(BlueprintReadOnly, Category = "士兵")
    TArray<AXBSoldierActor*> Soldiers;

    /** @brief 当前士兵数量（用于成长计算） */
    UPROPERTY(BlueprintReadOnly, Category = "成长")
    int32 CurrentSoldierCount = 0;

    // ============ 配置 ============

    /** @brief 配置数据表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "配置数据表"))
    TObjectPtr<UDataTable> ConfigDataTable;

    /** @brief 配置行名 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "配置行名"))
    FName ConfigRowName;

    /** @brief 缓存的数据表行数据 */
    UPROPERTY(BlueprintReadOnly, Category = "配置")
    FXBLeaderTableRow CachedLeaderData;

    /** @brief 成长配置缓存 */
    UPROPERTY(BlueprintReadOnly, Category = "成长")
    FXBGrowthConfigCache GrowthConfigCache;
};
