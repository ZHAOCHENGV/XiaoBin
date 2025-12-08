
#include "GAS/XBAbilitySystemComponent.h"
#include "Utils/XBGameplayTags.h"

UXBAbilitySystemComponent::UXBAbilitySystemComponent()
{
    // 设置复制模式
    ReplicationMode = EGameplayEffectReplicationMode::Mixed;
}

bool UXBAbilitySystemComponent::TryActivateAbilityByTag(const FGameplayTag& AbilityTag, bool bAllowRemoteActivation)
{
    FGameplayTagContainer TagContainer;
    TagContainer.AddTag(AbilityTag);
    return TryActivateAbilitiesByTag(TagContainer, bAllowRemoteActivation);
}

bool UXBAbilitySystemComponent::IsAbilityActiveByTag(const FGameplayTag& AbilityTag) const
{
    FGameplayTagContainer TagContainer;
    TagContainer.AddTag(AbilityTag);
    
    TArray<FGameplayAbilitySpec*> MatchingSpecs;
    GetActivatableGameplayAbilitySpecsByAllMatchingTags(TagContainer, MatchingSpecs, false);

    for (const FGameplayAbilitySpec* Spec : MatchingSpecs)
    {
        if (Spec && Spec->IsActive())
        {
            return true;
        }
    }
    return false;
}

void UXBAbilitySystemComponent::CancelAbilityByTag(const FGameplayTag& AbilityTag)
{
    FGameplayTagContainer TagContainer;
    TagContainer.AddTag(AbilityTag);
    CancelAbilities(&TagContainer);
}

void UXBAbilitySystemComponent::AddStartupAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities)
{
    if (bStartupAbilitiesGiven)
    {
        return;
    }

    for (const TSubclassOf<UGameplayAbility>& AbilityClass : StartupAbilities)
    {
        if (AbilityClass)
        {
            FGameplayAbilitySpec AbilitySpec(AbilityClass, 1);
            GiveAbility(AbilitySpec);
        }
    }

    bStartupAbilitiesGiven = true;
}

void UXBAbilitySystemComponent::EnterCombat()
{
    const FXBGameplayTags& Tags = FXBGameplayTags::Get();
    
    if (!HasMatchingGameplayTag(Tags.State_Combat))
    {
        AddLooseGameplayTag(Tags.State_Combat);
        
        // 发送进入战斗事件
        FGameplayEventData EventData;
        HandleGameplayEvent(Tags.Event_Combat_Enter, &EventData);
    }
}

void UXBAbilitySystemComponent::ExitCombat()
{
    const FXBGameplayTags& Tags = FXBGameplayTags::Get();
    
    if (HasMatchingGameplayTag(Tags.State_Combat))
    {
        RemoveLooseGameplayTag(Tags.State_Combat);
        
        // 发送退出战斗事件
        FGameplayEventData EventData;
        HandleGameplayEvent(Tags.Event_Combat_Exit, &EventData);
    }
}

bool UXBAbilitySystemComponent::IsInCombat() const
{
    return HasMatchingGameplayTag(FXBGameplayTags::Get().State_Combat);
}