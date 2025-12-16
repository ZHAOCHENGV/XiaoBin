/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Data/XBLeaderDataTable.h

/**
 * @file XBLeaderDataTable.h
 * @brief 主将数据表结构定义 - 与XBAttributeSet属性对应
 * 
 * @note 🔧 修改记录:
 *       1. ❌ 删除 FXBAbilityConfig 的 DamageMultiplier（倍率统一由 AttributeSet 管理）
 *       2. ✨ 新增 FXBAbilityConfig 的 BaseDamage（每个技能独立配置伤害）
 *       3. ❌ 删除 FXBLeaderTableRow 的 BaseDamage（移到技能配置中）
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "XBLeaderDataTable.generated.h"

class UGameplayAbility;
class UAnimMontage;
class UGameplayEffect;

/**
 * @brief 技能配置结构体
 * @note 🔧 修改 - BaseDamage 现在在此配置，而非 FXBLeaderTableRow
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBAbilityConfig
{
    GENERATED_BODY()

    /** @brief 技能GA类 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "技能", meta = (DisplayName = "技能GA类"))
    TSubclassOf<UGameplayAbility> AbilityClass;

    /** @brief 技能蒙太奇 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "技能", meta = (DisplayName = "技能蒙太奇"))
    TSoftObjectPtr<UAnimMontage> AbilityMontage;

    // ✨ 新增 - 技能基础伤害（从 FXBLeaderTableRow 移动过来）
    /**
     * @brief 技能基础伤害
     * @note 实际伤害 = BaseDamage * DamageMultiplier（来自AttributeSet）
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "技能", meta = (DisplayName = "基础伤害", ClampMin = "0.0"))
    float BaseDamage = 10.0f;

    // ❌ 删除 - DamageMultiplier（倍率统一由 UXBAttributeSet::DamageMultiplier 管理）

    /** @brief 冷却时间（秒），0表示无冷却 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "技能", meta = (DisplayName = "冷却时间", ClampMin = "0.0"))
    float Cooldown = 0.0f;
};

/**
 * @brief 主将数据表行 - 属性与XBAttributeSet对应
 * @note 🔧 修改 - 移除 BaseDamage，现在由各技能独立配置
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBLeaderTableRow : public FTableRowBase
{
    GENERATED_BODY()

    // ============ 基础信息 ============

    /** @brief 主将名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础信息", meta = (DisplayName = "主将名称"))
    FText LeaderName;

    /** @brief 主将描述 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础信息", meta = (DisplayName = "主将描述"))
    FText Description;

    // ============ 核心属性 (对应XBAttributeSet) ============

    /** @brief 最大生命值 - 对应 MaxHealth */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "核心属性", meta = (DisplayName = "最大生命值", ClampMin = "1.0"))
    float MaxHealth = 100.0f;

    /** @brief 生命值倍率 - 对应 HealthMultiplier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "核心属性", meta = (DisplayName = "生命值倍率", ClampMin = "0.1"))
    float HealthMultiplier = 1.0f;

    // ❌ 删除 - BaseDamage（移到 FXBAbilityConfig 中）

    /** @brief 伤害倍率 - 对应 DamageMultiplier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "核心属性", meta = (DisplayName = "伤害倍率", ClampMin = "0.1"))
    float DamageMultiplier = 1.0f;

    /** @brief 移动速度 - 对应 MoveSpeed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "核心属性", meta = (DisplayName = "移动速度", ClampMin = "0.0"))
    float MoveSpeed = 600.0f;

    /** @brief 初始缩放 - 对应 Scale */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "核心属性", meta = (DisplayName = "初始缩放", ClampMin = "0.1"))
    float Scale = 1.0f;

    // ============ 成长配置 ============

    /** @brief 每个士兵提供的生命值 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长配置", meta = (DisplayName = "每士兵生命加成"))
    float HealthPerSoldier = 5.0f;

    /** @brief 每个士兵提供的体型缩放 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长配置", meta = (DisplayName = "每士兵体型加成"))
    float ScalePerSoldier = 0.01f;

    /** @brief 最大体型缩放 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长配置", meta = (DisplayName = "最大体型缩放", ClampMin = "1.0"))
    float MaxScale = 2.0f;

    // ============ 战斗配置 ============

    /** @brief 普攻配置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗配置", meta = (DisplayName = "普攻配置"))
    FXBAbilityConfig BasicAttackConfig;

    /** @brief 技能配置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗配置", meta = (DisplayName = "技能配置"))
    FXBAbilityConfig SpecialSkillConfig;
};
