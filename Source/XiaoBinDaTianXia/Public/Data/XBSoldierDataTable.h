/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Data/XBSoldierDataTable.h

/**
 * @file XBSoldierDataTable.h
 * @brief 士兵配置数据表 - 统一数据源架构
 * 
 * @note 🔧 重构记录:
 *       1. ❌ 删除 FXBSoldierConfig 冗余结构
 *       2. ❌ 删除 ToSoldierConfig() 手动转换方法
 *       3. ✨ 新增 智能访问器模式
 *       4. ✨ 新增 运行时数据校验
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Army/XBSoldierTypes.h"
#include "Data/XBLeaderDataTable.h"
#include "Combat/XBProjectile.h"
#include "XBSoldierDataTable.generated.h"

class UBehaviorTree;
class USkeletalMesh;
class UAnimInstance;
class UAnimMontage;
class UGameplayEffect;

// ============================================
// 配置子结构（保持不变，增强注释）
// ============================================

/**
 * @brief 士兵AI配置
 * @note 所有AI参数集中管理，便于行为树读取
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBSoldierAIConfig
{
    GENERATED_BODY()

    /** @brief 行为树资源 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "行为树"))
    TSoftObjectPtr<UBehaviorTree> BehaviorTree;

    /** @brief 视野范围（检测敌人的最大距离） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|检测", meta = (DisplayName = "视野范围", ClampMin = "100.0"))
    float VisionRange = 800.0f;

    // 🔧 修改 - 追击距离用于限制离队追击，避免士兵过远脱离主将
    /** @brief 追击距离（目标非战斗状态时，超过此距离退出战斗并回归跟随） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|战斗", meta = (DisplayName = "追击距离", ClampMin = "100.0"))
    float DisengageDistance = 1000.0f;

    /** @brief 无敌人后返回将领的延迟时间（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|战斗", meta = (DisplayName = "返回延迟", ClampMin = "0.0"))
    float ReturnDelay = 2.0f;

    /** @brief 寻敌间隔（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|检测", meta = (DisplayName = "寻敌间隔", ClampMin = "0.1"))
    float TargetSearchInterval = 0.5f;

    /** @brief 避让半径（避免扎堆） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|移动", meta = (DisplayName = "避让半径", ClampMin = "0.0"))
    float AvoidanceRadius = 50.0f;

    /** @brief 避让权重 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|移动", meta = (DisplayName = "避让权重", ClampMin = "0.0", ClampMax = "1.0"))
    float AvoidanceWeight = 0.3f;

    /** @brief 到达编队位置的判定阈值 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|移动", meta = (DisplayName = "到达阈值", ClampMin = "10.0"))
    float ArrivalThreshold = 50.0f;

    /** @brief 黑板数据更新间隔（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "黑板更新间隔", ClampMin = "0.05"))
    float BlackboardUpdateInterval = 0.1f;
};

/**
 * @brief 士兵视觉配置
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBSoldierVisualConfig
{
    GENERATED_BODY()

    /** @brief 骨骼网格体 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "视觉", meta = (DisplayName = "骨骼网格"))
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

    /** @brief 动画蓝图类 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "视觉", meta = (DisplayName = "动画蓝图"))
    TSoftClassPtr<UAnimInstance> AnimClass;

    /** @brief 模型缩放 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "视觉", meta = (DisplayName = "模型缩放", ClampMin = "0.1"))
    float MeshScale = 1.0f;

    /** @brief 死亡蒙太奇 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "视觉", meta = (DisplayName = "死亡蒙太奇"))
    TSoftObjectPtr<UAnimMontage> DeathMontage;
};

/**
 * @brief 士兵投射物配置（弓手专用）
 * @note 仅在 SoldierType 为 Archer 时启用
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBProjectileConfig
{
    GENERATED_BODY()

    /** @brief 投射物类 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "投射物", meta = (DisplayName = "投射物类"))
    TSubclassOf<AXBProjectile> ProjectileClass;

    /** @brief 发射速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "投射物", meta = (DisplayName = "发射速度", ClampMin = "0.0"))
    float Speed = 1200.0f;

    /** @brief 是否使用抛射 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "投射物", meta = (DisplayName = "抛射模式"))
    bool bUseArc = false;

    /** @brief 上抛速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "投射物", meta = (DisplayName = "上抛速度"))
    float ArcLaunchSpeed = 600.0f;

    /** @brief 重力缩放 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "投射物", meta = (DisplayName = "重力缩放"))
    float ArcGravityScale = 1.0f;

    /** @brief 预加载数量 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "投射物", meta = (DisplayName = "预加载数量", ClampMin = "0"))
    int32 PreloadCount = 5;

    /** @brief 最大存活时间 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "投射物", meta = (DisplayName = "最大存活时间", ClampMin = "0.0"))
    float LifeSeconds = 5.0f;

    /** @brief 伤害效果（GAS） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "投射物", meta = (DisplayName = "伤害效果"))
    TSubclassOf<UGameplayEffect> DamageEffectClass;
};

// ============================================
// ✨ 新增：数据访问器前向声明
// ============================================
class UXBSoldierDataAccessor;

// ============================================
// 士兵配置数据表行 - 唯一数据源
// ============================================

/**
 * @brief 士兵配置数据表行 - 项目唯一数据源
 * @note 🔧 架构变更:
 *       - 所有运行时代码直接从此结构读取数据
 *       - 通过 UXBSoldierDataAccessor 提供类型安全的访问接口
 *       - 资源加载由访问器统一管理（支持异步/同步/缓存）
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBSoldierTableRow : public FTableRowBase
{
    GENERATED_BODY()

    // ==================== 基础信息 ====================

    /** @brief 士兵类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础", meta = (DisplayName = "士兵类型"))
    EXBSoldierType SoldierType = EXBSoldierType::Infantry;

    /** @brief 显示名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础", meta = (DisplayName = "显示名称"))
    FText DisplayName;

    /** @brief 士兵描述 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础", meta = (DisplayName = "描述"))
    FText Description;

    /** @brief 士兵标签（用于技能系统等）*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础", meta = (DisplayName = "士兵标签"))
    FGameplayTagContainer SoldierTags;

    // ==================== 战斗配置 ====================

    /** @brief 普通攻击配置（包含基础伤害、攻击范围、攻击间隔等） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "普通攻击"))
    FXBAbilityConfig BasicAttack;

    /** @brief 最大血量 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "最大血量", ClampMin = "1.0"))
    float MaxHealth = 100.0f;

    // ==================== 远程配置（弓手专用） ====================

    /** @brief 投射物配置（仅弓手显示） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗|远程", meta = (DisplayName = "发射物配置", EditCondition = "SoldierType == EXBSoldierType::Archer", EditConditionHides))
    FXBProjectileConfig ProjectileConfig;

    // ==================== 移动配置 ====================

    /** @brief 移动速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "移动", meta = (DisplayName = "移动速度", ClampMin = "0.0"))
    float MoveSpeed = 400.0f;

    /** @brief 冲刺速度倍率（将领冲刺时士兵加速） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "移动", meta = (DisplayName = "冲刺速度倍率", ClampMin = "1.0"))
    float SprintSpeedMultiplier = 2.0f;

    /** @brief 跟随插值速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "移动", meta = (DisplayName = "跟随插值", ClampMin = "1.0"))
    float FollowInterpSpeed = 5.0f;

    /** @brief 旋转速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "移动", meta = (DisplayName = "旋转速度", ClampMin = "0.0"))
    float RotationSpeed = 360.0f;

    
    // ==================== AI配置 ====================

    /** @brief AI行为配置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "AI配置"))
    FXBSoldierAIConfig AIConfig;

    // ==================== 视觉配置 ====================

    /** @brief 视觉资源配置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "视觉", meta = (DisplayName = "视觉配置"))
    FXBSoldierVisualConfig VisualConfig;

    // ==================== ✨ 新增：便捷访问方法 ====================

    /**
     * @brief 获取视野范围
     * @return 视野范围值
     * @note 提供向后兼容的快捷访问
     */
    FORCEINLINE float GetVisionRange() const
    {
        return AIConfig.VisionRange;
    }

    /**
     * @brief 获取脱离距离
     * @return 脱离战斗距离
     */
    FORCEINLINE float GetDisengageDistance() const
    {
        return AIConfig.DisengageDistance;
    }

    /**
     * @brief 获取返回延迟
     * @return 返回延迟时间（秒）
     */
    FORCEINLINE float GetReturnDelay() const
    {
        return AIConfig.ReturnDelay;
    }

    /**
     * @brief 获取到达阈值
     * @return 到达判定阈值
     */
    FORCEINLINE float GetArrivalThreshold() const
    {
        return AIConfig.ArrivalThreshold;
    }

    /**
     * @brief 数据校验
     * @return 数据是否有效
     * @note 用于编辑器验证和运行时检查
     */
    bool Validate(FText& OutError) const
    {
        if (MaxHealth <= 0.0f)
        {
            OutError = FText::FromString(TEXT("最大血量必须大于0"));
            return false;
        }

        if (BasicAttack.AttackRange < 10.0f)
        {
            OutError = FText::FromString(TEXT("攻击范围过小"));
            return false;
        }

        if (MoveSpeed <= 0.0f)
        {
            OutError = FText::FromString(TEXT("移动速度必须大于0"));
            return false;
        }

        return true;
    }
};
