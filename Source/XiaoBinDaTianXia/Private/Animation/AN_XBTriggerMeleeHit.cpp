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
#include "Utils/XBGameplayTags.h"
#include "Utils/XBLogCategories.h"
#include "Game/XBGameInstance.h"
#include "Sound/XBSoundManagerSubsystem.h"

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

        ASC->HandleGameplayEvent(EventTag, &EventData);
        UE_LOG(LogXBCombat, Verbose, TEXT("近战命中Tag触发GA事件: %s, Owner=%s, Target=%s"),
            *EventTag.ToString(),
            *OwnerActor->GetName(),
            EventData.Target ? *EventData.Target->GetName() : TEXT("无"));
        return;
    }

    UE_LOG(LogXBCombat, Warning, TEXT("近战命中Tag触发失败：ASC无效，Owner=%s"), *OwnerActor->GetName());
}
