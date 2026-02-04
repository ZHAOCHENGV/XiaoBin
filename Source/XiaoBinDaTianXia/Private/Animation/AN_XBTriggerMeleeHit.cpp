// Source/XiaoBinDaTianXia/Private/Animation/AN_XBTriggerMeleeHit.cpp

/**
 * @file AN_XBTriggerMeleeHit.cpp
 * @brief 近战命中AnimNotify实现
 */

#include "Animation/AN_XBTriggerMeleeHit.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Character/XBCharacterBase.h"
#include "Utils/XBLogCategories.h"
#include "Sound/XBSoundManagerSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"

UAN_XBTriggerMeleeHit::UAN_XBTriggerMeleeHit()
{
    // 🔧 修改 - 使用显式请求Tag，避免初始化顺序导致Tag无效
    EventTag = FGameplayTag::RequestGameplayTag(FName("Event.Attack.MeleeHit"), false);
}

FString UAN_XBTriggerMeleeHit::GetNotifyName_Implementation() const
{
    if (EventTag.IsValid())
    {
        return FString::Printf(TEXT("XB近战命中: %s"), *EventTag.ToString());
    }
    return TEXT("XB近战命中");
}

void UAN_XBTriggerMeleeHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    if (!MeshComp)
    {
        return; 
    }

    AActor* OwnerActor = MeshComp->GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    // 🔧 修改 - 弓手不适用此Tag（弓手依赖投射物伤害）
    if (AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(OwnerActor))
    {
        if (Soldier->GetSoldierType() == EXBSoldierType::Archer)
        {
            UE_LOG(LogXBCombat, Verbose, TEXT("弓手跳过近战命中Tag: %s"), *Soldier->GetName());
            return;
        }
    }

    if (!EventTag.IsValid())
    {
        // 🔧 修改 - 再次尝试请求Tag，避免运行时未初始化导致无效
        EventTag = FGameplayTag::RequestGameplayTag(FName("Event.Attack.MeleeHit"), false);
        if (!EventTag.IsValid())
        {
            UE_LOG(LogXBCombat, Warning, TEXT("近战命中Tag无效，跳过触发"));
            return;
        }
    }

    // 🔧 修改 - 构建GameplayEvent数据，优先填入当前目标
    FGameplayEventData EventData;
    EventData.EventTag = EventTag;
    EventData.Instigator = OwnerActor;
    EventData.Target = nullptr;

    if (AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(OwnerActor))
    {
        EventData.Target = Soldier->CurrentAttackTarget.Get();
    }

    // 🔧 修改 - 通过ASC派发事件，触发对应GA
    if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor))
    {
        // ✨ 新增 - 播放命中音效
        if (HitSoundTag.IsValid())
        {
            if (UWorld* World = OwnerActor->GetWorld())
            {
                if (UGameInstance* GameInstance = World->GetGameInstance<UGameInstance>())
                {
                    if (UXBSoundManagerSubsystem* SoundMgr = GameInstance->GetSubsystem<UXBSoundManagerSubsystem>())
                    {
                        SoundMgr->PlaySoundAtLocation(World, HitSoundTag, OwnerActor->GetActorLocation());
                    }
                }
            }
        }

        // ✨ 新增 - 在攻击者与被击者之间的命中位置播放特效
        if (HitEffect && EventData.Target)
        {
            // 计算攻击者指向被击者的方向
            FVector AttackerLocation = OwnerActor->GetActorLocation();
            FVector TargetLocation = EventData.Target->GetActorLocation();
            FVector DirectionToTarget = (TargetLocation - AttackerLocation).GetSafeNormal();
            
            // 命中位置：被击者朝向攻击者方向偏移（模拟近战武器接触点）
            // 偏移距离约为被击者半径，在被击者前方生成特效
            constexpr float HitOffsetDistance = 50.0f;
            FVector HitLocation = TargetLocation - DirectionToTarget * HitOffsetDistance;
            
            // 特效朝向：面向攻击者到被击者的方向
            FRotator HitRotation = DirectionToTarget.Rotation();
            
            UGameplayStatics::SpawnEmitterAtLocation(
                OwnerActor->GetWorld(),
                HitEffect,
                HitLocation,
                HitRotation,
                FVector::OneVector,
                true,
                EPSCPoolMethod::AutoRelease);
            UE_LOG(LogXBCombat, Verbose, TEXT("播放近战命中特效于 %s 前方"), *EventData.Target->GetName());
        }

        ASC->HandleGameplayEvent(EventTag, &EventData);
        UE_LOG(LogXBCombat, Verbose, TEXT("近战命中Tag触发GA事件: %s, Owner=%s, Target=%s"),
            *EventTag.ToString(),
            *OwnerActor->GetName(),
            EventData.Target ? *EventData.Target->GetName() : TEXT("无"));
        return;
    }

    UE_LOG(LogXBCombat, Warning, TEXT("近战命中Tag触发失败：ASC无效，Owner=%s"), *OwnerActor->GetName());
}
