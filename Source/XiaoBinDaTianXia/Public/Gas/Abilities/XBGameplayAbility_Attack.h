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
  
};
