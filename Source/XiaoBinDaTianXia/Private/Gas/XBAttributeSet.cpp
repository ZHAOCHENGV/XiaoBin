// Copyright XiaoBing Project. All Rights Reserved.

#include "GAS/XBAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystemBlueprintLibrary.h"

UXBAttributeSet::UXBAttributeSet()
{
    // 初始化默认值
    InitHealth(100.0f);
    InitMaxHealth(100.0f);
    InitHealthMultiplier(1.0f);
    InitBaseDamage(10.0f);
    InitDamageMultiplier(1.0f);
    InitMoveSpeed(600.0f);
    InitScale(1.0f);
}

void UXBAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UXBAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UXBAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UXBAttributeSet, HealthMultiplier, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UXBAttributeSet, BaseDamage, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UXBAttributeSet, DamageMultiplier, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UXBAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UXBAttributeSet, Scale, COND_None, REPNOTIFY_Always);
}

void UXBAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void UXBAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
    if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
    }
    else if (Attribute == GetMaxHealthAttribute())
    {
        NewValue = FMath::Max(NewValue, 1.0f);
    }
    else if (Attribute == GetScaleAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.5f, 5.0f);
    }
    else if (Attribute == GetMoveSpeedAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
}

void UXBAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    // 处理受到伤害
    if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
    {
        HandleHealthChanged(Data);
    }
    // 处理受到治疗
    else if (Data.EvaluatedData.Attribute == GetIncomingHealingAttribute())
    {
        const float HealingDone = GetIncomingHealing();
        SetIncomingHealing(0.0f);

        if (HealingDone > 0.0f)
        {
            const float NewHealth = FMath::Min(GetHealth() + HealingDone, GetMaxHealth());
            SetHealth(NewHealth);
        }
    }
    // 处理缩放变化
    else if (Data.EvaluatedData.Attribute == GetScaleAttribute())
    {
        HandleScaleChanged(Data);
    }
}

void UXBAttributeSet::HandleHealthChanged(const FGameplayEffectModCallbackData& Data)
{
    const float DamageDone = GetIncomingDamage();
    SetIncomingDamage(0.0f);

    if (DamageDone > 0.0f)
    {
        const float NewHealth = FMath::Max(GetHealth() - DamageDone, 0.0f);
        SetHealth(NewHealth);

        // 检查是否死亡
        if (NewHealth <= 0.0f)
        {
            // 通知 ASC 触发死亡事件
            if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
            {
                FGameplayEventData EventData;
                EventData.EventMagnitude = DamageDone;
                ASC->HandleGameplayEvent(FGameplayTag::RequestGameplayTag(FName("Event.Leader.Died")), &EventData);
            }
        }
    }
}

void UXBAttributeSet::HandleScaleChanged(const FGameplayEffectModCallbackData& Data)
{
    // 通知角色更新缩放
    if (AActor* OwningActor = GetOwningActor())
    {
        const float NewScale = GetScale();
        OwningActor->SetActorScale3D(FVector(NewScale));
    }
}

// ============ 复制回调 ============

void UXBAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UXBAttributeSet, Health, OldValue);
}

void UXBAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UXBAttributeSet, MaxHealth, OldValue);
}

void UXBAttributeSet::OnRep_HealthMultiplier(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UXBAttributeSet, HealthMultiplier, OldValue);
}

void UXBAttributeSet::OnRep_BaseDamage(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UXBAttributeSet, BaseDamage, OldValue);
}

void UXBAttributeSet::OnRep_DamageMultiplier(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UXBAttributeSet, DamageMultiplier, OldValue);
}

void UXBAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UXBAttributeSet, MoveSpeed, OldValue);
}

void UXBAttributeSet::OnRep_Scale(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UXBAttributeSet, Scale, OldValue);
}