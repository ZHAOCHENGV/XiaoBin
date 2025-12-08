#pragma once

#include "CoreMinimal.h"
#include "Army/XBSoldierTypes.h"  // 包含共享类型定义
#include "XBDataTypes.generated.h"

// ============================================
// 仅定义 XBDataTypes 独有的类型
// 不要重复定义 XBSoldierTypes.h 中已有的类型
// ============================================

// 主将配置（仅在此文件定义）
USTRUCT(BlueprintType)
struct FXBLeaderConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FName LeaderId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TObjectPtr<USkeletalMesh> LeaderMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSubclassOf<UAnimInstance> AnimClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float BaseHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float BaseDamage = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AttackRange = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MoveSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float DashSpeed = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Growth")
    float HealthPerSoldier = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Growth")
    float DamagePerSoldier = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Growth")
    float ScalePerSoldier = 0.02f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    TSubclassOf<class UGameplayAbility> BasicAttackAbility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    TSubclassOf<class UGameplayAbility> SpecialSkillAbility;
};

// 战斗配置（仅在此文件定义）
USTRUCT(BlueprintType)
struct FXBCombatConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection")
    float EnemyDetectionRange = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection")
    float AllyDetectionRange = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float CombatEngageDistance = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float CombatDisengageDistance = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Return")
    float ReturnToLeaderDistance = 600.0f;
};
