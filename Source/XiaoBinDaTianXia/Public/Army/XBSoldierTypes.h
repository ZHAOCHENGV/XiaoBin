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

    // 🔧 修改 - 明确使用 FVector2D
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D LocalOffset = FVector2D::ZeroVector;

    // ✨ 新增 - 修复 cpp 中 bOccupied 访问错误
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bOccupied = false;

    // ✨ 新增 - 修复 cpp 中 OccupantSoldierId 访问错误
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

    // ✨ 新增 - 横向间距
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation", meta = (DisplayName = "横向间距"))
    float HorizontalSpacing = 100.0f;

    // ✨ 新增 - 纵向间距
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation", meta = (DisplayName = "纵向间距"))
    float VerticalSpacing = 100.0f;

    // ✨ 新增 - 离将领的最小距离
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation", meta = (DisplayName = "离将领距离"))
    float MinDistanceToLeader = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation", meta = (DisplayName = "最大列数"))
    int32 MaxColumns = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation", meta = (DisplayName = "跟随距离"))
    float FollowDistance = 200.0f;
};

// ============================================
// 小兵配置 - 包含所有必要成员
// ============================================

USTRUCT(BlueprintType)
struct FXBSoldierConfig
{
    GENERATED_BODY()

    // ---- 基础属性 ----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    EXBSoldierType SoldierType = EXBSoldierType::Infantry;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FName SoldierId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText DisplayName;

    // ---- 视觉资源 ----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TObjectPtr<USkeletalMesh> SoldierMesh;  // 小兵骨骼网格体

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSubclassOf<UAnimInstance> AnimClass;   // 动画蓝图类

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    float MeshScale = 1.0f;

    // ---- 战斗属性 ----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float MaxHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float BaseDamage = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AttackRange = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AttackInterval = 1.0f;

    // ---- 移动属性 ----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MoveSpeed = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float FollowInterpSpeed = 5.0f;  // 跟随插值速度

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float SprintSpeedMultiplier = 1.5f;  // 冲刺速度倍率

    // ---- 给主将的加成 ----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus")
    float HealthBonusToLeader = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus")
    float DamageBonusToLeader = 2.0f;

    // ---- 标签 ----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
    FGameplayTagContainer SoldierTags;
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

    // ✨ 新增 - 修复 ArmySubsystem 中的 'bIsSprinting' 错误
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsSprinting = false;

    // 辅助方法
    bool IsAlive() const { return State != EXBSoldierState::Dead && CurrentHealth > 0.0f; }
    bool IsInCombat() const { return State == EXBSoldierState::Combat; }
    bool HasLeader() const { return LeaderId >= 0; }
};
