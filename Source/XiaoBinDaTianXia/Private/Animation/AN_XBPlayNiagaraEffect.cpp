// Source/XiaoBinDaTianXia/Private/Animation/AN_XBPlayNiagaraEffect.cpp

/**
 * @file AN_XBPlayNiagaraEffect.cpp
 * @brief 自定义Niagara特效动画通知实现 - 支持角色缩放
 *
 * @note ✨ 新增文件 - 基于 UAnimNotify_PlayNiagaraEffect 改造
 */

#include "Animation/AN_XBPlayNiagaraEffect.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/XBCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GAS/XBAttributeSet.h"
#include "Utils/XBLogCategories.h"


#if WITH_EDITOR
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#endif

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"


UAN_XBPlayNiagaraEffect::UAN_XBPlayNiagaraEffect() : Super() {
  Attached = true;
  Scale = FVector(1.f);
  bAbsoluteScale = false;
  bScaleWithCharacter = true;
  CharacterScaleMultiplier = 1.0f;

#if WITH_EDITORONLY_DATA
  NotifyColor = FColor(192, 255, 99, 255);
#endif
}

void UAN_XBPlayNiagaraEffect::PostLoad() {
  Super::PostLoad();
  RotationOffsetQuat = FQuat(RotationOffset);
}

#if WITH_EDITOR
void UAN_XBPlayNiagaraEffect::PostEditChangeProperty(
    FPropertyChangedEvent &PropertyChangedEvent) {
  Super::PostEditChangeProperty(PropertyChangedEvent);

  if (PropertyChangedEvent.MemberProperty &&
      PropertyChangedEvent.MemberProperty->GetFName() ==
          GET_MEMBER_NAME_CHECKED(UAN_XBPlayNiagaraEffect, RotationOffset)) {
    RotationOffsetQuat = FQuat(RotationOffset);
  }
}

void UAN_XBPlayNiagaraEffect::ValidateAssociatedAssets() {
  static const FName NAME_AssetCheck("AssetCheck");

  if ((Template != nullptr) && (Template->IsLooping())) {
    UObject *ContainingAsset = GetContainingAsset();

    FMessageLog AssetCheckLog(NAME_AssetCheck);

    const FText MessageLooping = FText::Format(
        NSLOCTEXT("AnimNotify", "NiagaraSystem_ShouldNotLoop",
                  "Niagara系统 {0} 在动画通知 {1} "
                  "中被设置为循环，但该插槽是一次性的（为避免每次通知泄漏组件，"
                  "将不会播放）。"),
        FText::AsCultureInvariant(Template->GetPathName()),
        FText::AsCultureInvariant(ContainingAsset->GetPathName()));
    AssetCheckLog.Warning()
        ->AddToken(FUObjectToken::Create(ContainingAsset))
        ->AddToken(FTextToken::Create(MessageLooping));

    if (GIsEditor) {
      AssetCheckLog.Notify(MessageLooping, EMessageSeverity::Warning, true);
    }
  }
}
#endif

void UAN_XBPlayNiagaraEffect::Notify(
    USkeletalMeshComponent *MeshComp, UAnimSequenceBase *Animation,
    const FAnimNotifyEventReference &EventReference) {
  SpawnedEffect = SpawnEffect(MeshComp, Animation);
  Super::Notify(MeshComp, Animation, EventReference);
}

FString UAN_XBPlayNiagaraEffect::GetNotifyName_Implementation() const {
  if (Template) {
    return Template->GetName();
  }
  return Super::GetNotifyName_Implementation();
}

UFXSystemComponent *
UAN_XBPlayNiagaraEffect::SpawnEffect(USkeletalMeshComponent *MeshComp,
                                     UAnimSequenceBase *Animation) {
  UFXSystemComponent *ReturnComp = nullptr;

  if (!Template) {
    return ReturnComp;
  }

  if (Template->IsLooping()) {
    UE_LOG(LogXBCombat, Warning,
           TEXT("[AN_XBPlayNiagaraEffect] Niagara系统 %s 是循环的，跳过生成"),
           *Template->GetName());
    return ReturnComp;
  }

  // ✨ 核心功能 - 计算最终缩放
  FVector FinalScale = Scale;

  if (bScaleWithCharacter && MeshComp) {
    AActor *OwnerActor = MeshComp->GetOwner();
    float CharacterScale = GetOwnerScale(OwnerActor);

    // 在基础Scale上应用角色缩放
    FinalScale = Scale * CharacterScale * CharacterScaleMultiplier;

    UE_LOG(LogXBCombat, Verbose,
           TEXT("[AN_XBPlayNiagaraEffect] 角色缩放: %.2f, 最终特效缩放: (%.2f, "
                "%.2f, %.2f)"),
           CharacterScale, FinalScale.X, FinalScale.Y, FinalScale.Z);
  }

  if (Attached) {
    ReturnComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
        Template, MeshComp, SocketName, LocationOffset, RotationOffset,
        EAttachLocation::KeepRelativeOffset, true);
  } else {
    const FTransform MeshTransform = MeshComp->GetSocketTransform(SocketName);
    ReturnComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        MeshComp->GetWorld(), Template,
        MeshTransform.TransformPosition(LocationOffset),
        (MeshTransform.GetRotation() * RotationOffsetQuat).Rotator(),
        FVector(1.0f), true);
  }

  if (ReturnComp != nullptr) {
    ReturnComp->SetUsingAbsoluteScale(bAbsoluteScale);
    ReturnComp->SetRelativeScale3D_Direct(FinalScale);
  }

  return ReturnComp;
}

float UAN_XBPlayNiagaraEffect::GetOwnerScale(AActor *OwnerActor) const {
  if (!OwnerActor) {
    return 1.0f;
  }

  // 优先从 XBCharacterBase 获取缩放
  if (AXBCharacterBase *Character = Cast<AXBCharacterBase>(OwnerActor)) {
    float CharScale = Character->GetCurrentScale();
    if (CharScale > 0.0f) {
      return CharScale;
    }
  }

  // 从 GAS 属性获取缩放
  if (UAbilitySystemComponent *ASC =
          UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(
              OwnerActor)) {
    float ScaleAttribute =
        ASC->GetNumericAttribute(UXBAttributeSet::GetScaleAttribute());
    if (ScaleAttribute > 0.0f) {
      return ScaleAttribute;
    }
  }

  // 从 Actor 缩放获取
  FVector ActorScale = OwnerActor->GetActorScale3D();
  if (ActorScale.X > 0.0f) {
    return ActorScale.X;
  }

  return 1.0f;
}

UFXSystemComponent *UAN_XBPlayNiagaraEffect::GetSpawnedEffect() {
  return SpawnedEffect;
}
