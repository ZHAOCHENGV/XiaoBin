// Copyright XiaoBing Project. All Rights Reserved.

/**
 * @file XBGameplayAbility_Skill.h
 * @brief 将领特殊技能基类
 * 
 * 功能说明：
 * - 鼠标右键触发
 * - 圆形范围伤害（或其他效果）
 * - 有冷却时间
 * - 可派生不同将领的专属技能
 */

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/XBGameplayAbility.h"
#include "XBGameplayAbility_Skill.generated.h"

class UNiagaraSystem;

/**
 * @brief 将领特殊技能基类
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBGameplayAbility_Skill : public UXBGameplayAbility
{
    GENERATED_BODY()

public:
    UXBGameplayAbility_Skill();

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

protected:
    // ==================== 技能配置 ====================

    /** 技能范围 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "技能配置", meta = (DisplayName = "技能范围"))
    float SkillRange = 400.0f;

    /** 伤害效果类 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "技能配置", meta = (DisplayName = "伤害效果"))
    TSubclassOf<UGameplayEffect> DamageEffectClass;

    /** 伤害倍率 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "技能配置", meta = (DisplayName = "伤害倍率"))
    float DamageMultiplier = 2.0f;

    /** 技能动画蒙太奇 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "技能配置", meta = (DisplayName = "技能动画"))
    TObjectPtr<UAnimMontage> SkillMontage;

    /** 技能特效 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "技能配置", meta = (DisplayName = "技能特效"))
    TObjectPtr<UNiagaraSystem> SkillVFX;

    // ==================== 内部方法 ====================

    /**
     * @brief 执行技能效果
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Skill")
    virtual void PerformSkill();

    /**
     * @brief 获取圆形范围内的目标
     * @param OutTargets 输出目标数组
     */
    void GetTargetsInRadius(TArray<AActor*>& OutTargets) const;

    /**
     * @brief 对目标应用伤害
     * @param Target 目标Actor
     */
    void ApplyDamageToTarget(AActor* Target);
};
