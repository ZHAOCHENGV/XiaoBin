// Copyright XiaoBing Project. All Rights Reserved.

/**
 * @file XBGameplayAbility_Attack.h
 * @brief 将领普攻技能
 * 
 * 功能说明：
 * - 鼠标左键触发
 * - 扇形范围伤害
 * - 命中敌人触发战斗状态
 * - 伤害随将领属性缩放
 */

#pragma once

#include "CoreMinimal.h"
#include "NiagaraSystem.h"
#include "GAS/Abilities/XBGameplayAbility.h"
#include "XBGameplayAbility_Attack.generated.h"

/**
 * @brief 将领普攻技能类
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBGameplayAbility_Attack : public UXBGameplayAbility
{
    GENERATED_BODY()

public:
    UXBGameplayAbility_Attack();

    /** 激活技能 */
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    /** 结束技能 */
    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

protected:
    // ==================== 攻击配置 ====================

    /** 攻击范围 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "攻击配置", meta = (DisplayName = "攻击范围"))
    float AttackRange = 200.0f;

    /** 攻击角度（扇形） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "攻击配置", meta = (DisplayName = "攻击角度"))
    float AttackAngle = 120.0f;

    /** 伤害效果类 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "攻击配置", meta = (DisplayName = "伤害效果"))
    TSubclassOf<UGameplayEffect> DamageEffectClass;

    /** 攻击动画蒙太奇 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "攻击配置", meta = (DisplayName = "攻击动画"))
    TObjectPtr<UAnimMontage> AttackMontage;

    /** 攻击特效 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "攻击配置", meta = (DisplayName = "攻击特效"))
    TObjectPtr<UNiagaraSystem> AttackVFX;

    // ==================== 内部方法 ====================

    /**
     * @brief 执行攻击检测
     * 检测扇形范围内的敌人并造成伤害
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Attack")
    void PerformAttack();

    /**
     * @brief 获取扇形范围内的目标
     * @param OutTargets 输出目标数组
     */
    void GetTargetsInCone(TArray<AActor*>& OutTargets) const;

    /**
     * @brief 对目标应用伤害
     * @param Target 目标Actor
     */
    void ApplyDamageToTarget(AActor* Target);

    /** 动画结束回调 */
    UFUNCTION()
    void OnMontageCompleted(UAnimMontage* Montage, bool bInterrupted);

    UFUNCTION()
    void OnMontageBlendOut();
};
