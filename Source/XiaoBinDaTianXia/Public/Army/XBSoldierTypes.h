/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Army/XBSoldierTypes.h

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "XBSoldierTypes.generated.h"

// ============================================
// 枚举定义
// ============================================

UENUM(BlueprintType)
enum class EXBSoldierType : uint8
{
    None        UMETA(DisplayName = "无"),
    Infantry    UMETA(DisplayName = "步兵"),
    Archer      UMETA(DisplayName = "弓箭手"),
    Cavalry     UMETA(DisplayName = "骑兵")
};

UENUM(BlueprintType)
enum class EXBFaction : uint8
{
    Neutral     UMETA(DisplayName = "中立"),
    Player      UMETA(DisplayName = "玩家"),
    Enemy       UMETA(DisplayName = "敌人"),
    Ally        UMETA(DisplayName = "友军")
};

UENUM(BlueprintType)
enum class EXBSoldierState : uint8
{
    Dormant     UMETA(DisplayName = "休眠"), 
    Idle        UMETA(DisplayName = "待机"),
    Following   UMETA(DisplayName = "跟随"),
    Combat      UMETA(DisplayName = "战斗"),
    Seeking     UMETA(DisplayName = "搜索"),
    Returning   UMETA(DisplayName = "返回"),
    Dead        UMETA(DisplayName = "死亡")
};

// ============================================
// 阵型槽位
// ============================================

USTRUCT(BlueprintType)
struct FXBFormationSlot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SlotIndex = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D LocalOffset = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bOccupied = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 OccupantSoldierId = -1;
};

// ============================================
// 阵型配置
// ============================================

USTRUCT(BlueprintType)
struct FXBFormationConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation", meta = (DisplayName = "横向间距"))
    float HorizontalSpacing = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation", meta = (DisplayName = "纵向间距"))
    float VerticalSpacing = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation", meta = (DisplayName = "离将领距离"))
    float MinDistanceToLeader = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation", meta = (DisplayName = "最大列数"))
    int32 MaxColumns = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation", meta = (DisplayName = "跟随距离"))
    float FollowDistance = 200.0f;
};


// ============================================
// ✨ 新增：休眠类型枚举
// ============================================

/**
 * @brief 休眠类型枚举
 * @note 用于配置未招募士兵的外观表现
 */
UENUM(BlueprintType)
enum class EXBDormantType : uint8
{
    Sleeping    UMETA(DisplayName = "睡眠"),
    Standing    UMETA(DisplayName = "站立"),
    Hidden      UMETA(DisplayName = "隐藏")
};

// ============================================
// ✨ 新增：休眠配置结构体
// ============================================

/**
 * @brief 休眠态视觉配置
 * @note 用于配置未招募时的外观
 */
USTRUCT(BlueprintType)
struct FXBDormantVisualConfig
{
    GENERATED_BODY()

    /** @brief 休眠动画类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "休眠", meta = (DisplayName = "休眠类型"))
    EXBDormantType DormantType = EXBDormantType::Sleeping;

    /** @brief 是否显示 Zzz 特效 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "休眠", meta = (DisplayName = "显示Zzz特效"))
    bool bShowZzzEffect = true;

    /** @brief Zzz 特效位置偏移 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "休眠", meta = (DisplayName = "Zzz特效偏移"))
    FVector ZzzEffectOffset = FVector(0.0f, 0.0f, 100.0f);

    /** @brief 睡眠动画序列 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "休眠", meta = (DisplayName = "睡眠动画"))
    TSoftObjectPtr<UAnimSequence> SleepingAnimation;

    /** @brief 站立待机动画序列 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "休眠", meta = (DisplayName = "站立动画"))
    TSoftObjectPtr<UAnimSequence> StandingAnimation;
};

// ============================================
// 轻量级小兵数据（用于集群系统）
// ============================================

USTRUCT(BlueprintType)
struct FXBSoldierData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SoldierId = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EXBFaction Faction = EXBFaction::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EXBSoldierType SoldierType = EXBSoldierType::Infantry;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EXBSoldierState State = EXBSoldierState::Idle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Position = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CurrentHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 LeaderId = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 FormationSlotIndex = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector TargetPosition = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsSprinting = false;

    bool IsAlive() const { return State != EXBSoldierState::Dead && CurrentHealth > 0.0f; }
    bool IsInCombat() const { return State == EXBSoldierState::Combat; }
    bool HasLeader() const { return LeaderId >= 0; }
};
