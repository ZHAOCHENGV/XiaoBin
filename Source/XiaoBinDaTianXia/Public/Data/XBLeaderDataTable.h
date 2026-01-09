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
class UBehaviorTree;
class USplineComponent;

/**
 * @brief 假人主将移动方式
 */
UENUM(BlueprintType)
enum class EXBLeaderAIMoveMode : uint8
{
    Stand UMETA(DisplayName = "原地站立"),
    Wander UMETA(DisplayName = "范围内移动"),
    Route UMETA(DisplayName = "固定路线")
};

/**
 * @brief 主将AI配置（假人主将）
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBLeaderAIConfig
{
    GENERATED_BODY()

    /** @brief 是否启用AI */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "启用AI"))
    bool bEnableAI = true;

    /** @brief 行为树资源 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|行为", meta = (DisplayName = "行为树资源"))
    TSoftObjectPtr<UBehaviorTree> BehaviorTree;

    /** @brief 移动方式 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|移动", meta = (DisplayName = "移动方式"))
    EXBLeaderAIMoveMode MoveMode = EXBLeaderAIMoveMode::Stand;

    /** @brief 视野范围 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|感知", meta = (DisplayName = "视野范围", ClampMin = "100.0"))
    float VisionRange = 2000.0f;

    /** @brief 目标检索间隔 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|感知", meta = (DisplayName = "目标检索间隔", ClampMin = "0.1"))
    float TargetSearchInterval = 0.5f;

    /** @brief 随机移动半径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|移动", meta = (DisplayName = "随机移动半径", ClampMin = "100.0"))
    float WanderRadius = 800.0f;

    /** @brief 随机移动间隔 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|移动", meta = (DisplayName = "随机移动间隔", ClampMin = "0.1"))
    float WanderInterval = 2.0f;

    /** @brief 随机移动到达半径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|移动", meta = (DisplayName = "随机移动到达半径", ClampMin = "10.0"))
    float WanderAcceptanceRadius = 120.0f;

    /** @brief 路线到达半径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|移动", meta = (DisplayName = "路线到达半径", ClampMin = "10.0"))
    float RouteAcceptanceRadius = 120.0f;

    /** @brief 原地站立回位半径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|移动", meta = (DisplayName = "站立回位半径", ClampMin = "10.0"))
    float StandReturnRadius = 150.0f;

    // ==================== 黑板键配置 ====================

    /** @brief 目标主将黑板键 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|黑板", meta = (DisplayName = "目标主将键"))
    FName TargetLeaderKey = TEXT("TargetLeader");

    /** @brief 行为目的地黑板键 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|黑板", meta = (DisplayName = "行为目的地键"))
    FName BehaviorDestinationKey = TEXT("BehaviorDestination");

    /** @brief 行为中心黑板键 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|黑板", meta = (DisplayName = "行为中心键"))
    FName BehaviorCenterKey = TEXT("BehaviorCenter");

    /** @brief 初始位置黑板键 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|黑板", meta = (DisplayName = "初始位置键"))
    FName HomeLocationKey = TEXT("HomeLocation");

    /** @brief 巡逻路线索引黑板键 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|黑板", meta = (DisplayName = "路线索引键"))
    FName RouteIndexKey = TEXT("RoutePointIndex");

    /** @brief 行为模式黑板键 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|黑板", meta = (DisplayName = "行为模式键"))
    FName MoveModeKey = TEXT("MoveMode");

    /** @brief 是否战斗黑板键 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|黑板", meta = (DisplayName = "战斗状态键"))
    FName InCombatKey = TEXT("IsInCombat");

    /** @brief 受击响应黑板键 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|黑板", meta = (DisplayName = "受击响应键"))
    FName DamageResponseKey = TEXT("DamageResponseReady");
};

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
    float ScalePerSoldier = 0.1f;

    /** @brief 最大体型缩放 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长配置", meta = (DisplayName = "最大体型缩放", ClampMin = "1.0"))
    float MaxScale = 3.0f;
    
    /** @brief 每个士兵增加的伤害倍率 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长配置", meta = (DisplayName = "每士兵伤害倍率加成"))
    float DamageMultiplierPerSoldier = 0.f;

    /** @brief 最大伤害倍率 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "成长配置", meta = (DisplayName = "最大伤害倍率", ClampMin = "1.0"))
    float MaxDamageMultiplier = 3.0f;

    // ============ 战斗配置 ============

    /** @brief 普攻配置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗配置", meta = (DisplayName = "普攻配置"))
    FXBAbilityConfig BasicAttackConfig;

    /** @brief 技能配置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗配置", meta = (DisplayName = "技能配置"))
    FXBAbilityConfig SpecialSkillConfig;

    // ============ 视觉配置 ============

    /** @brief 动画蓝图类 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "视觉配置", meta = (DisplayName = "动画蓝图类"))
    TSoftClassPtr<UAnimInstance> AnimClass;

    /** @brief 死亡蒙太奇 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "视觉配置", meta = (DisplayName = "死亡蒙太奇"))
    TSoftObjectPtr<UAnimMontage> DeathMontage;

    /** @brief 主将骨骼网格 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "视觉配置", meta = (DisplayName = "主将骨骼网格"))
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

    // ============ AI配置 ============

    /** @brief 主将AI配置（假人） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI配置", meta = (DisplayName = "主将AI配置"))
    FXBLeaderAIConfig AIConfig;
};
