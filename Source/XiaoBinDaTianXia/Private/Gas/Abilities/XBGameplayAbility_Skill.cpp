// Copyright XiaoBing Project. All Rights Reserved.

/**
 * @file XBGameplayAbility_Skill.cpp
 * @brief 将领特殊技能实现
 */

#include "GAS/Abilities/XBGameplayAbility_Skill.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Character/XBCharacterBase.h"
#include "GAS/XBAttributeSet.h"
#include "NiagaraFunctionLibrary.h"

UXBGameplayAbility_Skill::UXBGameplayAbility_Skill()
{
    // 设置技能标签
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Skill")));
    
    // 设置激活策略
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;

    // ✨ 新增 - 使用新API
    FGameplayTagContainer Tags;
    Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Skill")));
    SetAssetTags(Tags);
}

void UXBGameplayAbility_Skill::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    // 检查是否可以提交技能消耗
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 获取角色
    ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
    if (!Character)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 播放技能动画
    if (SkillMontage)
    {
        UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            AnimInstance->Montage_Play(SkillMontage);
        }
    }

    // 播放技能特效
    if (SkillVFX)
    {
        float ScaledRange = GetScaledRadius(SkillRange);
        FVector SpawnLocation = Character->GetActorLocation();
        
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            Character->GetWorld(),
            SkillVFX,
            SpawnLocation,
            FRotator::ZeroRotator,
            FVector(ScaledRange / SkillRange)); // 缩放特效
    }

    // 执行技能效果
    PerformSkill();

    // 结束技能
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UXBGameplayAbility_Skill::PerformSkill()
{
    // 获取范围内的目标
    TArray<AActor*> Targets;
    GetTargetsInRadius(Targets);

    // 对每个目标应用伤害
    for (AActor* Target : Targets)
    {
        ApplyDamageToTarget(Target);
    }

    // 如果命中了目标，通知进入战斗状态
    if (Targets.Num() > 0)
    {
        NotifyAbilityHit(Targets[0]);
    }

    UE_LOG(LogTemp, Log, TEXT("Skill hit %d targets"), Targets.Num());
}

void UXBGameplayAbility_Skill::GetTargetsInRadius(TArray<AActor*>& OutTargets) const
{
    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    if (!AvatarActor)
    {
        return;
    }

    FVector Origin = AvatarActor->GetActorLocation();

    // 获取施法者阵营
    EXBFaction MyFaction = EXBFaction::Neutral;
    if (AXBCharacterBase* CharacterBase = Cast<AXBCharacterBase>(AvatarActor))
    {
        MyFaction = CharacterBase->GetFaction();
    }

    // 球形检测
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(AvatarActor);

    TArray<AActor*> OverlappedActors;

    // 使用缩放后的技能范围
    float ScaledRange = GetScaledRadius(SkillRange);

    UKismetSystemLibrary::SphereOverlapActors(
        AvatarActor->GetWorld(),
        Origin,
        ScaledRange,
        ObjectTypes,
        AXBCharacterBase::StaticClass(),
        ActorsToIgnore,
        OverlappedActors);

    // 过滤敌对阵营
    for (AActor* Actor : OverlappedActors)
    {
        if (AXBCharacterBase* TargetCharacter = Cast<AXBCharacterBase>(Actor))
        {
            if (TargetCharacter->IsHostileTo(Cast<AXBCharacterBase>(AvatarActor)))
            {
                OutTargets.Add(Actor);
            }
        }
    }
}

void UXBGameplayAbility_Skill::ApplyDamageToTarget(AActor* Target)
{
    if (!Target || !DamageEffectClass)
    {
        return;
    }

    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
    UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();

    if (!TargetASC || !SourceASC)
    {
        return;
    }

    FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
    ContextHandle.AddSourceObject(GetAvatarActorFromActorInfo());

    FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), ContextHandle);

    if (SpecHandle.IsValid())
    {
        // ✨ 新增 - 技能使用倍率计算额外伤害
        // 从属性集读取基础伤害，乘以技能倍率作为额外伤害
        float BaseDamage = 50.0f;
        if (const UXBAttributeSet* AttributeSet = SourceASC->GetSet<UXBAttributeSet>())
        {
            BaseDamage = AttributeSet->GetBaseDamage();
        }
        
        // 技能额外伤害 = 基础伤害 * (倍率 - 1)，因为 Execution 已经计算了一次基础伤害
        float SkillBonusDamage = BaseDamage * (DamageMultiplier - 1.0f) * GetOwnerScale();
        
        SpecHandle.Data->SetSetByCallerMagnitude(
            FGameplayTag::RequestGameplayTag(FName("Data.Damage")),
            SkillBonusDamage);

        SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

        UE_LOG(LogTemp, Log, TEXT("技能对 %s 造成额外伤害: %.1f"), *Target->GetName(), SkillBonusDamage);
    }
}
