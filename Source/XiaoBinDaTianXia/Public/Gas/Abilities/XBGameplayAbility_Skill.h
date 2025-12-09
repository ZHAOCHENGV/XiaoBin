/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AbilitySystem/Abilities/XBGameplayAbility_Skill.h

/**
 * @file XBGameplayAbility_Skill.h
 * @brief 技能GA - 简化版，配置由数据表和动画通知处理
 */

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/XBGameplayAbility.h"
#include "XBGameplayAbility_Skill.generated.h"

/**
 * @brief 技能能力
 * @note 伤害检测由ANS_XBMeleeDetection动画通知处理
 *       技能配置由XBLeaderDataTable数据表处理
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBGameplayAbility_Skill : public UXBGameplayAbility
{
    GENERATED_BODY()

public:
    UXBGameplayAbility_Skill();
    /** 激活技能 */
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    /** 结束技能 */
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
};
