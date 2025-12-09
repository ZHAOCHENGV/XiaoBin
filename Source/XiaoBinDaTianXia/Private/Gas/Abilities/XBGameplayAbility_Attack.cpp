/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AbilitySystem/Abilities/XBGameplayAbility_Attack.cpp

/**
 * @file XBGameplayAbility_Attack.cpp
 * @brief 普通攻击GA实现 - 简化版
 */

#include "GAS/Abilities/XBGameplayAbility_Attack.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"

UXBGameplayAbility_Attack::UXBGameplayAbility_Attack()
{
    // ✨ 新增 - 使用新API设置标签
    FGameplayTagContainer Tags;
    Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Attack"), false));
    SetAssetTags(Tags);

    // 设置能力的基本属性
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UXBGameplayAbility_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("普攻GA激活"));

    // 蒙太奇播放和伤害检测由XBCombatComponent和ANS_XBMeleeDetection处理
    // 这里只负责GA的生命周期管理
}

void UXBGameplayAbility_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    UE_LOG(LogTemp, Log, TEXT("普攻GA结束 - 被取消: %s"), bWasCancelled ? TEXT("是") : TEXT("否"));

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
