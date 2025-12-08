// Source/XiaoBinDaTianXia/Private/Gas/Abilities/XBGameplayAbility_Attack.cpp

#include "GAS/Abilities/XBGameplayAbility_Attack.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Character/XBCharacterBase.h"
#include "GAS/XBAttributeSet.h"
#include "NiagaraFunctionLibrary.h"

UXBGameplayAbility_Attack::UXBGameplayAbility_Attack()
{
    // 设置技能标签
    // 注意：AbilityTags 在新版本中建议通过 GetAssetTags() 访问，但在构造函数中直接添加仍是常用做法
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Attack")));
    
    // 设置激活策略
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

void UXBGameplayAbility_Attack::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    // 调用父类
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

    // 播放攻击动画
    if (AttackMontage)
    {
        // 播放蒙太奇
        UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            // 绑定结束回调 (匹配 FOnMontageEnded 签名: UAnimMontage*, bool)
            FOnMontageEnded EndDelegate;
            EndDelegate.BindUObject(this, &UXBGameplayAbility_Attack::OnMontageCompleted);
            
            // 绑定混出回调 (如果是 void 无参版本)
            // 注意：通常混出也建议使用带参数版本，这里根据头文件声明适配无参
            // 假设头文件中 OnMontageBlendOut 是 void 类型，这里可能无法直接绑定到 Montage 委托
            // 如果需要绑定，通常使用 UAbilityTask_PlayMontageAndWait
            
            AnimInstance->Montage_Play(AttackMontage);
            AnimInstance->Montage_SetEndDelegate(EndDelegate, AttackMontage);
        }
    }
    else
    {
        // 没有动画，直接执行攻击
        PerformAttack();
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }

    // 播放攻击特效
    if (AttackVFX)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            Character->GetWorld(),
            AttackVFX,
            Character->GetActorLocation(),
            Character->GetActorRotation());
    }
}

void UXBGameplayAbility_Attack::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UXBGameplayAbility_Attack::PerformAttack()
{
    // 获取扇形范围内的目标
    TArray<AActor*> Targets;
    GetTargetsInCone(Targets);

    // 对每个目标应用伤害
    for (AActor* Target : Targets)
    {
        ApplyDamageToTarget(Target);
    }

    // 如果命中了目标，通知进入战斗状态
    if (Targets.Num() > 0)
    {
        // 通知技能命中
        NotifyAbilityHit(Targets[0]);
    }

    UE_LOG(LogTemp, Log, TEXT("Attack hit %d targets"), Targets.Num());
}

void UXBGameplayAbility_Attack::GetTargetsInCone(TArray<AActor*>& OutTargets) const
{
    // 获取施法者
    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    if (!AvatarActor)
    {
        return;
    }

    // 获取施法者位置和朝向
    FVector Origin = AvatarActor->GetActorLocation();
    FVector Forward = AvatarActor->GetActorForwardVector();

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
    
    // 使用缩放后的攻击范围
    float ScaledRange = GetScaledRadius(AttackRange);
    
    UKismetSystemLibrary::SphereOverlapActors(
        AvatarActor->GetWorld(),
        Origin,
        ScaledRange,
        ObjectTypes,
        AXBCharacterBase::StaticClass(),
        ActorsToIgnore,
        OverlappedActors);

    // 过滤：检查是否在扇形范围内 + 是否是敌对阵营
    float HalfAngleRad = FMath::DegreesToRadians(AttackAngle * 0.5f);

    for (AActor* Actor : OverlappedActors)
    {
        if (!Actor)
        {
            continue;
        }

        // 检查阵营
        if (AXBCharacterBase* TargetCharacter = Cast<AXBCharacterBase>(Actor))
        {
            // 跳过同阵营
            if (!TargetCharacter->IsHostileTo(Cast<AXBCharacterBase>(AvatarActor)))
            {
                continue;
            }
        }

        // 检查角度
        FVector ToTarget = (Actor->GetActorLocation() - Origin).GetSafeNormal();
        float DotProduct = FVector::DotProduct(Forward, ToTarget);
        float AngleToTarget = FMath::Acos(DotProduct);

        if (AngleToTarget <= HalfAngleRad)
        {
            OutTargets.Add(Actor);
        }
    }
}

void UXBGameplayAbility_Attack::ApplyDamageToTarget(AActor* Target)
{
    if (!Target || !DamageEffectClass)
    {
        return;
    }

    // 获取 ASC
    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
    UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();

    if (!TargetASC || !SourceASC)
    {
        return;
    }

    // 创建效果上下文
    FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
    ContextHandle.AddSourceObject(GetAvatarActorFromActorInfo());

    // 创建效果规格
    FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), ContextHandle);

    if (SpecHandle.IsValid())
    {
        // 获取伤害值（从属性集读取基础伤害 * 缩放）
        float BaseDamage = 50.0f; // 默认值
        if (const UXBAttributeSet* AttributeSet = SourceASC->GetSet<UXBAttributeSet>())
        {
            BaseDamage = AttributeSet->GetBaseDamage() * AttributeSet->GetDamageMultiplier();
        }

        // 根据缩放调整伤害
        float ScaledDamage = BaseDamage * GetOwnerScale();

        // 设置伤害值到效果规格
        SpecHandle.Data->SetSetByCallerMagnitude(
            FGameplayTag::RequestGameplayTag(FName("Data.Damage")), 
            ScaledDamage);

        // 应用效果
        SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

        UE_LOG(LogTemp, Log, TEXT("Applied %.1f damage to %s"), ScaledDamage, *Target->GetName());
    }
}

void UXBGameplayAbility_Attack::OnMontageCompleted(UAnimMontage* Montage, bool bInterrupted)
{
    if (!bInterrupted)
    {
        // 动画播放到攻击点时执行攻击（或在动画结束时结算）
        PerformAttack();
    }
    
    // 结束技能
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UXBGameplayAbility_Attack::OnMontageBlendOut()
{
    // 混出时也结束技能（如果有绑定）
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}