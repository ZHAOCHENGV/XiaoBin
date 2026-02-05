/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/GAS/XBAttributeSet.cpp

// Copyright XiaoBing Project. All Rights Reserved.

#include "GAS/XBAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Character/XBCharacterBase.h"
#include "Soldier/XBSoldierCharacter.h"

UXBAttributeSet::UXBAttributeSet()
{
    // 初始化默认值
    InitHealth(100.0f);
    InitMaxHealth(100.0f);
    InitHealthMultiplier(1.0f);
    // ❌ 删除 - InitBaseDamage
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
    // ❌ 删除 - BaseDamage 的复制
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

    // 处理受到的伤害
    if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
    {
        HandleHealthChanged(Data);
    }

    // 处理受到的治疗
    if (Data.EvaluatedData.Attribute == GetIncomingHealingAttribute())
    {
        const float HealingDone = GetIncomingHealing();
        SetIncomingHealing(0.0f);

        if (HealingDone > 0.0f)
        {
            const float OldHealth = GetHealth();
            const float NewHealth = FMath::Clamp(OldHealth + HealingDone, 0.0f, GetMaxHealth());
            SetHealth(NewHealth);

            UE_LOG(LogTemp, Log, TEXT("===== 治疗事件 ====="));
            UE_LOG(LogTemp, Log, TEXT("目标: %s"), *GetOwningActor()->GetName());
            UE_LOG(LogTemp, Log, TEXT("治疗量: %.2f"), HealingDone);
            UE_LOG(LogTemp, Log, TEXT("生命值: %.2f -> %.2f / %.2f"), OldHealth, NewHealth, GetMaxHealth());
        }
    }

    // 处理缩放变化
    if (Data.EvaluatedData.Attribute == GetScaleAttribute())
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
        const float OldHealth = GetHealth();
        const float NewHealth = FMath::Clamp(OldHealth - DamageDone, 0.0f, GetMaxHealth());
        SetHealth(NewHealth);

        // ✨ 新增 - 详细的伤害日志
        UE_LOG(LogTemp, Log, TEXT(""));
        UE_LOG(LogTemp, Log, TEXT("╔══════════════════════════════════════════╗"));
        UE_LOG(LogTemp, Log, TEXT("║           伤害事件详细日志               ║"));
        UE_LOG(LogTemp, Log, TEXT("╠══════════════════════════════════════════╣"));

        // 获取攻击者信息
        AActor* TargetActor = GetOwningActor();
        AActor* SourceActor = nullptr;
        AController* SourceController = nullptr;

        
        if (Data.EffectSpec.GetContext().GetEffectCauser())
        {
            SourceActor = Data.EffectSpec.GetContext().GetEffectCauser();
        }
        
        if (Data.EffectSpec.GetContext().GetInstigator())
        {
            SourceController = Cast<AController>(Data.EffectSpec.GetContext().GetInstigator());
            if (!SourceActor && SourceController)
            {
                SourceActor = SourceController->GetPawn();
            }
        }

        // 输出攻击者信息
        if (SourceActor)
        {
            UE_LOG(LogTemp, Log, TEXT("║ 攻击者: %s"), *SourceActor->GetName());
            
            // 尝试获取攻击者的阵营
            if (AXBCharacterBase* SourceCharacter = Cast<AXBCharacterBase>(SourceActor))
            {
                FString FactionName;
                switch (SourceCharacter->GetFaction())
                {
                case EXBFaction::Player:
                    FactionName = TEXT("玩家");
                    break;
                case EXBFaction::Enemy:
                    FactionName = TEXT("敌人");
                    break;
                case EXBFaction::Ally:
                    FactionName = TEXT("友军");
                    break;
                default:
                    FactionName = TEXT("中立");
                    break;
                }
                UE_LOG(LogTemp, Log, TEXT("║ 攻击者阵营: %s"), *FactionName);
            }
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("║ 攻击者: 未知"));
        }
        // 🔧 修改 - 将伤害来源记录到目标角色
        if (AXBCharacterBase* TargetCharacter = Cast<AXBCharacterBase>(TargetActor))
        {
            if (SourceActor)
            {
                TargetCharacter->SetLastDamageInstigator(SourceActor);
            }
            // 🔧 修改 - 触发目标的受伤回调，用于AI响应
            TargetCharacter->HandleDamageReceived(SourceActor, DamageDone);
        }
        // ✨ 新增 - 为士兵触发受击白光效果
        else if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(TargetActor))
        {
            TargetSoldier->TriggerHitFlash();
        }
        // 输出目标信息
        if (TargetActor)
        {
            UE_LOG(LogTemp, Log, TEXT("║ 目标: %s"), *TargetActor->GetName());
            
            // 尝试获取目标的阵营
            if (AXBCharacterBase* TargetCharacter = Cast<AXBCharacterBase>(TargetActor))
            {
                FString FactionName;
                switch (TargetCharacter->GetFaction())
                {
                case EXBFaction::Player:
                    FactionName = TEXT("玩家");
                    break;
                case EXBFaction::Enemy:
                    FactionName = TEXT("敌人");
                    break;
                case EXBFaction::Ally:
                    FactionName = TEXT("友军");
                    break;
                default:
                    FactionName = TEXT("中立");
                    break;
                }
                UE_LOG(LogTemp, Log, TEXT("║ 目标阵营: %s"), *FactionName);
            }
        }

        UE_LOG(LogTemp, Log, TEXT("╠══════════════════════════════════════════╣"));
        UE_LOG(LogTemp, Log, TEXT("║ 伤害数值: %.2f"), DamageDone);
        UE_LOG(LogTemp, Log, TEXT("║ 生命值变化: %.2f -> %.2f"), OldHealth, NewHealth);
        UE_LOG(LogTemp, Log, TEXT("║ 最大生命值: %.2f"), GetMaxHealth());
        UE_LOG(LogTemp, Log, TEXT("║ 剩余生命百分比: %.1f%%"), (NewHealth / GetMaxHealth()) * 100.0f);

        // 检查是否死亡
        if (NewHealth <= 0.0f)
        {
            UE_LOG(LogTemp, Log, TEXT("╠══════════════════════════════════════════╣"));
            UE_LOG(LogTemp, Warning, TEXT("║ ★★★ 目标已死亡! ★★★"));
            
            // ✨ 新增 - 调用角色死亡处理
            if (AXBCharacterBase* TargetCharacter = Cast<AXBCharacterBase>(GetOwningActor()))
            {
                TargetCharacter->HandleDeath();
            }
        }

        UE_LOG(LogTemp, Log, TEXT("╚══════════════════════════════════════════╝"));
        UE_LOG(LogTemp, Log, TEXT(""));
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

// ❌ 删除 - OnRep_BaseDamage

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
