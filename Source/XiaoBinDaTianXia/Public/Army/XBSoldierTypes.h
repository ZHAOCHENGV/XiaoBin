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
// 小兵配置 - 运行时使用的配置结构
// ============================================

/**
 * @brief 士兵运行时配置
 * @note 🔧 修改 - 添加从数据表初始化的方法
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBSoldierConfig
{
    GENERATED_BODY()

    // ==================== 基础属性 ====================

    /** @brief 士兵类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础", meta = (DisplayName = "士兵类型"))
    EXBSoldierType SoldierType = EXBSoldierType::Infantry;

    /** @brief 士兵唯一标识（对应数据表行名） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础", meta = (DisplayName = "士兵ID"))
    FName SoldierId;

    /** @brief 显示名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础", meta = (DisplayName = "显示名称"))
    FText DisplayName;

    // ==================== 视觉资源 ====================

    /** @brief 骨骼网格体 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "视觉", meta = (DisplayName = "骨骼网格"))
    TObjectPtr<USkeletalMesh> SoldierMesh;

    /** @brief 动画蓝图类 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "视觉", meta = (DisplayName = "动画蓝图"))
    TSubclassOf<UAnimInstance> AnimClass;

    /** @brief 模型缩放 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "视觉", meta = (DisplayName = "模型缩放", ClampMin = "0.1"))
    float MeshScale = 1.0f;

    // ==================== 战斗属性 ====================

    /** @brief 最大血量 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "最大血量", ClampMin = "1.0"))
    float MaxHealth = 100.0f;

    /** @brief 基础伤害 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "基础伤害", ClampMin = "0.0"))
    float BaseDamage = 10.0f;

    /** @brief 攻击范围 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "攻击范围", ClampMin = "10.0"))
    float AttackRange = 150.0f;

    /** @brief 攻击间隔（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "攻击间隔", ClampMin = "0.1"))
    float AttackInterval = 1.0f;

    // ==================== 移动属性 ====================

    /** @brief 移动速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "移动", meta = (DisplayName = "移动速度", ClampMin = "0.0"))
    float MoveSpeed = 400.0f;

    /** @brief 跟随插值速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "移动", meta = (DisplayName = "跟随插值", ClampMin = "1.0"))
    float FollowInterpSpeed = 5.0f;

    /** @brief 冲刺速度倍率 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "移动", meta = (DisplayName = "冲刺倍率", ClampMin = "1.0"))
    float SprintSpeedMultiplier = 1.5f;

    /** @brief 旋转速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "移动", meta = (DisplayName = "旋转速度", ClampMin = "0.0"))
    float RotationSpeed = 360.0f;

    // ==================== AI属性（✨ 新增） ====================

    /** @brief 视野范围 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "视野范围", ClampMin = "100.0"))
    float VisionRange = 800.0f;

    /** @brief 脱离战斗距离 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "脱离距离", ClampMin = "100.0"))
    float DisengageDistance = 1000.0f;

    /** @brief 返回延迟（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "返回延迟", ClampMin = "0.0"))
    float ReturnDelay = 2.0f;

    /** @brief 到达阈值 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "到达阈值", ClampMin = "10.0"))
    float ArrivalThreshold = 50.0f;

    // ==================== 给主将的加成 ====================

    /** @brief 给将领的血量加成 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "加成", meta = (DisplayName = "血量加成", ClampMin = "0.0"))
    float HealthBonusToLeader = 20.0f;

    /** @brief 给将领的伤害加成 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "加成", meta = (DisplayName = "伤害加成", ClampMin = "0.0"))
    float DamageBonusToLeader = 2.0f;

    /** @brief 给将领的缩放加成 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "加成", meta = (DisplayName = "缩放加成", ClampMin = "0.0"))
    float ScaleBonusToLeader = 0.01f;

    // ==================== 标签 ====================

    /** @brief 士兵标签 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "标签", meta = (DisplayName = "士兵标签"))
    FGameplayTagContainer SoldierTags;

    // ==================== 辅助方法 ====================

    /** @brief 检查配置是否有效 */
    bool IsValid() const
    {
        return SoldierType != EXBSoldierType::None && MaxHealth > 0.0f;
    }

    /** @brief 重置为默认值 */
    void Reset()
    {
        *this = FXBSoldierConfig();
    }
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

    // 辅助方法
    bool IsAlive() const { return State != EXBSoldierState::Dead && CurrentHealth > 0.0f; }
    bool IsInCombat() const { return State == EXBSoldierState::Combat; }
    bool HasLeader() const { return LeaderId >= 0; }
};
