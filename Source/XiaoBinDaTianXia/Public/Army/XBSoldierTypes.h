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
    Ally        UMETA(DisplayName = "友军"),
    FreeForAll  UMETA(DisplayName = "各自为战")
};

UENUM(BlueprintType)
enum class EXBSoldierState : uint8
{
    Dormant     UMETA(DisplayName = "休眠"), 
    Dropping    UMETA(DisplayName = "掉落中"),
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
// 休眠类型枚举
// ============================================

UENUM(BlueprintType)
enum class EXBDormantType : uint8
{
    Sleeping    UMETA(DisplayName = "睡眠"),
    Standing    UMETA(DisplayName = "站立"),
    Hidden      UMETA(DisplayName = "隐藏")
};

// ============================================
// 休眠配置结构体
// ============================================

USTRUCT(BlueprintType)
struct FXBDormantVisualConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "休眠", meta = (DisplayName = "休眠类型"))
    EXBDormantType DormantType = EXBDormantType::Sleeping;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "休眠", meta = (DisplayName = "显示Zzz特效"))
    bool bShowZzzEffect = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "休眠", meta = (DisplayName = "Zzz特效偏移"))
    FVector ZzzEffectOffset = FVector(0.0f, 0.0f, 100.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "休眠", meta = (DisplayName = "睡眠动画"))
    TSoftObjectPtr<UAnimSequence> SleepingAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "休眠", meta = (DisplayName = "站立动画"))
    TSoftObjectPtr<UAnimSequence> StandingAnimation;
};

// ============================================
// 掉落抛物线配置
// ============================================

USTRUCT(BlueprintType)
struct FXBDropArcConfig
{
    GENERATED_BODY()

    /** @brief 抛出的初始高度（相对于起始点） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "抛物线", meta = (DisplayName = "抛出高度", ClampMin = "50.0"))
    float ArcHeight = 200.0f;

    /** @brief 抛出的水平距离范围（最小值） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "抛物线", meta = (DisplayName = "最小抛出距离", ClampMin = "50.0"))
    float MinDropDistance = 150.0f;

    /** @brief 抛出的水平距离范围（最大值） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "抛物线", meta = (DisplayName = "最大抛出距离", ClampMin = "100.0"))
    float MaxDropDistance = 400.0f;

    /** @brief 抛物线飞行时间 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "抛物线", meta = (DisplayName = "飞行时间", ClampMin = "0.2"))
    float FlightDuration = 0.6f;

    /** @brief 地面检测向上距离 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "抛物线", meta = (DisplayName = "地面检测向上距离", ClampMin = "0.0"))
    float GroundTraceUpDistance = 500.0f;

    /** @brief 地面检测向下距离 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "抛物线", meta = (DisplayName = "地面检测向下距离", ClampMin = "0.0"))
    float GroundTraceDownDistance = 1200.0f;

    /** @brief 落地点额外Z偏移 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "抛物线", meta = (DisplayName = "落地点额外Z偏移"))
    float LandingExtraZOffset = 0.0f;

    /** @brief 是否播放落地特效 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "抛物线", meta = (DisplayName = "播放落地特效"))
    bool bPlayLandingEffect = true;

    /** @brief 落地特效资源 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "抛物线", meta = (DisplayName = "落地特效", EditCondition = "bPlayLandingEffect"))
    TSoftObjectPtr<class UNiagaraSystem> LandingEffect;

    // ✨ 新增 - 落地后自动入列配置
    /** @brief 落地后是否自动入列 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "抛物线", meta = (DisplayName = "落地自动入列"))
    bool bAutoRecruitOnLanding = true;

    /** @brief 落地后自动入列延迟 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "抛物线", meta = (DisplayName = "落地自动入列延迟", ClampMin = "0.0"))
    float AutoRecruitDelay = 0.1f;

    /** @brief 启用抛物线调试绘制 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "抛物线", meta = (DisplayName = "启用调试绘制"))
    bool bEnableDebugDraw = false;

    /** @brief 抛物线调试段数 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "抛物线", meta = (DisplayName = "调试段数", ClampMin = "2"))
    int32 DebugArcSegments = 16;

    /** @brief 抛物线调试持续时间 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "抛物线", meta = (DisplayName = "调试持续时间", ClampMin = "0.0"))
    float DebugDrawDuration = 2.0f;

    /** @brief 抛物线调试颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "抛物线", meta = (DisplayName = "调试颜色"))
    FLinearColor DebugArcColor = FLinearColor::Green;

    /** @brief 调试点尺寸 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "抛物线", meta = (DisplayName = "调试点尺寸", ClampMin = "0.0"))
    float DebugPointSize = 8.0f;
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
