/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Data/XBSoldierDataTable.h

/**
 * @file XBSoldierDataTable.h
 * @brief 士兵配置数据表结构
 * 
 * @note 🔧 修改记录:
 *       1. 增强数据表结构支持完整的士兵配置
 *       2. 新增行为树配置
 *       3. 新增视觉资源配置
 *       4. 新增弓手特殊配置
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Army/XBSoldierTypes.h"
#include "Data/XBLeaderDataTable.h"
#include "XBSoldierDataTable.generated.h"

class UBehaviorTree;
class USkeletalMesh;
class UAnimInstance;
class UAnimMontage;

/**
 * @brief 士兵AI配置
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBSoldierAIConfig
{
    GENERATED_BODY()

    /** @brief 行为树资源 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "行为树"))
    TSoftObjectPtr<UBehaviorTree> BehaviorTree;

    /** @brief 敌人检测范围 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "检测范围", ClampMin = "100.0"))
    float DetectionRange = 800.0f;

    /** @brief 脱离战斗距离（超过此距离自动返回） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "脱离距离", ClampMin = "100.0"))
    float DisengageDistance = 1000.0f;

    /** @brief 寻敌间隔（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "寻敌间隔", ClampMin = "0.1"))
    float TargetSearchInterval = 0.5f;

    /** @brief 避让半径（避免扎堆） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "避让半径", ClampMin = "0.0"))
    float AvoidanceRadius = 50.0f;

    /** @brief 避让权重 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "避让权重", ClampMin = "0.0", ClampMax = "1.0"))
    float AvoidanceWeight = 0.3f;
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
    TSubclassOf<UAnimInstance> AnimClass;

    /** @brief 模型缩放 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "视觉", meta = (DisplayName = "模型缩放", ClampMin = "0.1"))
    float MeshScale = 1.0f;

    /** @brief 死亡蒙太奇 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "视觉", meta = (DisplayName = "死亡蒙太奇"))
    TSoftObjectPtr<UAnimMontage> DeathMontage;
};

/**
 * @brief 弓手特殊配置
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBArcherConfig
{
    GENERATED_BODY()

    /** @brief 是否启用原地攻击（弓手特性：在攻击范围内不追踪） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "弓手", meta = (DisplayName = "启用原地攻击"))
    bool bStationaryAttack = true;

    /** @brief 最小攻击距离（过近时后撤） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "弓手", meta = (DisplayName = "最小攻击距离", ClampMin = "0.0"))
    float MinAttackDistance = 100.0f;

    /** @brief 后撤距离 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "弓手", meta = (DisplayName = "后撤距离", ClampMin = "0.0"))
    float RetreatDistance = 150.0f;
};

/**
 * @brief 士兵配置数据表行
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

    // ==================== 战斗配置 ====================

    /** @brief 普通攻击配置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "普通攻击"))
    FXBAbilityConfig BasicAttack;

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

    // ==================== 加成配置 ====================

    /** @brief 给将领的血量加成 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "加成", meta = (DisplayName = "血量加成", ClampMin = "0.0"))
    float HealthBonusToLeader = 20.0f;

    /** @brief 给将领的伤害加成 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "加成", meta = (DisplayName = "伤害加成", ClampMin = "0.0"))
    float DamageBonusToLeader = 2.0f;

    /** @brief 给将领的缩放加成 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "加成", meta = (DisplayName = "缩放加成", ClampMin = "0.0"))
    float ScaleBonusToLeader = 0.01f;

    // ==================== AI配置 ====================

    /** @brief AI行为配置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "AI配置"))
    FXBSoldierAIConfig AIConfig;

    // ==================== 视觉配置 ====================

    /** @brief 视觉资源配置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "视觉", meta = (DisplayName = "视觉配置"))
    FXBSoldierVisualConfig VisualConfig;

    // ==================== 弓手特殊配置 ====================

    /** @brief 弓手配置（仅对弓手类型生效） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "弓手", meta = (DisplayName = "弓手配置", EditCondition = "SoldierType == EXBSoldierType::Archer"))
    FXBArcherConfig ArcherConfig;
};
