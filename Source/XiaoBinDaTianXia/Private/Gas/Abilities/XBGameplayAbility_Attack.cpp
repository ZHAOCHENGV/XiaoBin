/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AbilitySystem/Abilities/XBGameplayAbility_Attack.cpp

/**
 * @file XBGameplayAbility_Attack.cpp
 * @brief 普通攻击GA实现 - 简化版
 */

#include "GAS/Abilities/XBGameplayAbility_Attack.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemGlobals.h"
#include "GAS/XBAttributeSet.h"
#include "Utils/XBGameplayTags.h"
#include "Utils/XBBlueprintFunctionLibrary.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Character/XBCharacterBase.h"
#include "Character/Components/XBCombatComponent.h"

UXBGameplayAbility_Attack::UXBGameplayAbility_Attack()
{
    // ✨ 新增 - 使用新API设置标签
    FGameplayTagContainer Tags;
    Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Attack"), false));
    SetAssetTags(Tags);

    // ✨ 新增 - 近战命中事件触发
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = FXBGameplayTags::Get().Event_Attack_MeleeHit;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);

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

    UE_LOG(LogTemp, Log, TEXT("普攻GA激活 - 触发Tag: %s"),
        TriggerEventData ? *TriggerEventData->EventTag.ToString() : TEXT("无"));

    // 🔧 修改 - 仅处理近战命中事件，避免误伤
    if (!TriggerEventData || TriggerEventData->EventTag != FXBGameplayTags::Get().Event_Attack_MeleeHit)
    {
        UE_LOG(LogTemp, Warning, TEXT("普攻GA触发失败：事件Tag不匹配"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    AActor* SourceActor = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
    if (!SourceActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("普攻GA触发失败：SourceActor无效"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 🔧 修改 - 弓手不使用近战命中Tag
    if (AXBSoldierCharacter* SourceSoldier = Cast<AXBSoldierCharacter>(SourceActor))
    {
        if (SourceSoldier->GetSoldierType() == EXBSoldierType::Archer)
        {
            UE_LOG(LogTemp, Verbose, TEXT("弓手跳过近战命中GA: %s"), *SourceSoldier->GetName());
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }
    }

    // 🔧 修改 - 通过 Get() 获取指针，避免直接对 TObjectPtr 做 const_cast
    const AActor* TargetActorConst = TriggerEventData->Target.Get();
    AActor* TargetActor = const_cast<AActor*>(TargetActorConst);
    if (!TargetActor)
    {
        if (AXBSoldierCharacter* SourceSoldier = Cast<AXBSoldierCharacter>(SourceActor))
        {
            TargetActor = SourceSoldier->CurrentAttackTarget.Get();
        }
    }

    if (!TargetActor || !IsValid(TargetActor))
    {
        UE_LOG(LogTemp, Warning, TEXT("普攻GA触发失败：目标无效"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 🔧 修改 - 解析阵营并过滤非敌对目标
    EXBFaction SourceFaction = EXBFaction::Neutral;
    if (AXBSoldierCharacter* SourceSoldier = Cast<AXBSoldierCharacter>(SourceActor))
    {
        SourceFaction = SourceSoldier->GetFaction();
    }
    else if (AXBCharacterBase* SourceLeader = Cast<AXBCharacterBase>(SourceActor))
    {
        SourceFaction = SourceLeader->GetFaction();
    }

    EXBFaction TargetFaction = EXBFaction::Neutral;
    if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(TargetActor))
    {
        TargetFaction = TargetSoldier->GetFaction();
    }
    else if (AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(TargetActor))
    {
        TargetFaction = TargetLeader->GetFaction();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("普攻GA触发失败：目标类型不支持"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (!UXBBlueprintFunctionLibrary::AreFactionsHostile(SourceFaction, TargetFaction))
    {
        UE_LOG(LogTemp, Verbose, TEXT("普攻GA跳过：非敌对阵营"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 🔧 修改 - 获取伤害数值（士兵用基础伤害，将领用战斗组件）
    float AttackDamage = 0.0f;
    if (AXBSoldierCharacter* SourceSoldier = Cast<AXBSoldierCharacter>(SourceActor))
    {
        AttackDamage = SourceSoldier->GetBaseDamage();
    }
    else if (AXBCharacterBase* SourceLeader = Cast<AXBCharacterBase>(SourceActor))
    {
        if (UXBCombatComponent* CombatComponent = SourceLeader->FindComponentByClass<UXBCombatComponent>())
        {
            AttackDamage = CombatComponent->GetCurrentAttackFinalDamage();
        }
    }

    if (AttackDamage <= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("普攻GA触发失败：伤害为0"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 🔧 修改 - 对士兵目标直接扣血，对将领目标使用ASC输入伤害
    if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(TargetActor))
    {
        TargetSoldier->TakeSoldierDamage(AttackDamage, SourceActor);
        UE_LOG(LogTemp, Log, TEXT("普攻GA命中士兵: %s, 伤害: %.1f"),
            *TargetSoldier->GetName(), AttackDamage);
    }
    else if (AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(TargetActor))
    {
        if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetLeader))
        {
            TargetASC->SetNumericAttributeBase(UXBAttributeSet::GetIncomingDamageAttribute(), AttackDamage);
            UE_LOG(LogTemp, Log, TEXT("普攻GA命中将领: %s, 伤害: %.1f"),
                *TargetLeader->GetName(), AttackDamage);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("普攻GA命中将领失败：目标ASC无效"));
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UXBGameplayAbility_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    UE_LOG(LogTemp, Log, TEXT("普攻GA结束 - 被取消: %s"), bWasCancelled ? TEXT("是") : TEXT("否"));

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
