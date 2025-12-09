// Source/XiaoBinDaTianXia/Public/Data/XBSoldierDataTable.h
/* --- 完整文件代码 --- */

// Copyright XiaoBing Project. All Rights Reserved.

/**
 * @file XBSoldierDataTable.h
 * @brief 士兵配置数据表结构
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Army/XBSoldierTypes.h"
#include "Data/XBLeaderDataTable.h"
#include "XBSoldierDataTable.generated.h"

/**
 * @brief 士兵配置数据表行
 */
USTRUCT(BlueprintType)
struct FXBSoldierTableRow : public FTableRowBase
{
    GENERATED_BODY()

    /** 士兵类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础", meta = (DisplayName = "士兵类型"))
    EXBSoldierType SoldierType = EXBSoldierType::Infantry;

    /** 显示名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础", meta = (DisplayName = "显示名称"))
    FText DisplayName;

    // ==================== 战斗配置 ====================

    /** 普通攻击配置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "普通攻击"))
    FXBAbilityConfig BasicAttack;

    // ❌ 删除 - MeleeDetectionConfigs 已移至动画通知

    /** 最大血量 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "最大血量"))
    float MaxHealth = 100.0f;

    /** 基础伤害 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "基础伤害"))
    float BaseDamage = 10.0f;

    /** 攻击范围 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "攻击范围"))
    float AttackRange = 150.0f;

    /** 攻击间隔 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "攻击间隔"))
    float AttackInterval = 1.0f;

    // ==================== 移动配置 ====================

    /** 移动速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "移动", meta = (DisplayName = "移动速度"))
    float MoveSpeed = 400.0f;

    /** 跟随插值速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "移动", meta = (DisplayName = "跟随插值"))
    float FollowInterpSpeed = 5.0f;

    // ==================== 加成配置 ====================

    /** 给将领的血量加成 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "加成", meta = (DisplayName = "血量加成"))
    float HealthBonusToLeader = 20.0f;

    /** 给将领的伤害加成 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "加成", meta = (DisplayName = "伤害加成"))
    float DamageBonusToLeader = 2.0f;
};
