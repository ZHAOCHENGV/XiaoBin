    // Copyright XiaoBing Project. All Rights Reserved.

#include "GAS/Abilities/XBGameplayAbility.h"
#include "GAS/XBAbilitySystemComponent.h"
#include "GAS/XBAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"

UXBGameplayAbility::UXBGameplayAbility()
{
    // 默认实例化策略
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    
    // 默认网络执行策略（单机游戏，本地执行）
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

void UXBGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
    Super::OnGiveAbility(ActorInfo, Spec);

    if (ActivationPolicy == EXBAbilityActivationPolicy::OnGiven)
    {
        TryActivateOnPolicy(ActorInfo, Spec);
    }
}

void UXBGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
    Super::OnAvatarSet(ActorInfo, Spec);

    if (ActivationPolicy == EXBAbilityActivationPolicy::OnSpawn)
    {
        TryActivateOnPolicy(ActorInfo, Spec);
    }
}

void UXBGameplayAbility::TryActivateOnPolicy(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
    if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
    {
        ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
    }
}

float UXBGameplayAbility::GetOwnerScale() const
{
    if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        if (const UXBAttributeSet* Attributes = ASC->GetSet<UXBAttributeSet>())
        {
            return Attributes->GetScale();
        }
    }
    return 1.0f;
}

float UXBGameplayAbility::GetScaledRadius(float BaseRadius) const
{
    return BaseRadius * GetOwnerScale();
}

void UXBGameplayAbility::NotifyAbilityHit(AActor* HitTarget)
{
    if (!bTriggerCombatOnHit || !HitTarget)
    {
        return;
    }

    // 触发战斗状态
    if (UXBAbilitySystemComponent* ASC = Cast<UXBAbilitySystemComponent>(
        GetAbilitySystemComponentFromActorInfo()))
    {
        ASC->EnterCombat();
    }
}