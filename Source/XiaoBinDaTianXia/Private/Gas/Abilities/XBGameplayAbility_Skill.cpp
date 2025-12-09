/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AbilitySystem/Abilities/XBGameplayAbility_Skill.cpp

/**
 * @file XBGameplayAbility_Skill.cpp
 * @brief 技能GA实现 - 简化版
 */

#include "GAS/Abilities/XBGameplayAbility_Skill.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"

UXBGameplayAbility_Skill::UXBGameplayAbility_Skill()
{
    // ✨ 新增 - 使用新API设置标签
    FGameplayTagContainer Tags;
    Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Skill"), false));
    SetAssetTags(Tags);

    // 设置能力的基本属性
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UXBGameplayAbility_Skill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("技能GA激活"));

    // 蒙太奇播放和伤害检测由XBCombatComponent和ANS_XBMeleeDetection处理
    // 这里只负责GA的生命周期管理
}

void UXBGameplayAbility_Skill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    UE_LOG(LogTemp, Log, TEXT("技能GA结束 - 被取消: %s"), bWasCancelled ? TEXT("是") : TEXT("否"));

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
