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
#include "Kismet/KismetSystemLibrary.h"
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
  bScaleLocationOffset = true;
  bSpawnOnGround = false;
  GroundTraceDistance = 500.0f;
  // 默认检测 WorldStatic 和 WorldDynamic
  GroundTraceTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
  GroundTraceTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));

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

  AActor *OwnerActor = MeshComp ? MeshComp->GetOwner() : nullptr;

  // ✨ 核心功能 - 计算最终缩放
  FVector FinalScale = Scale;
  float CharacterScale = 1.0f;

  if (bScaleWithCharacter && OwnerActor) {
    CharacterScale = GetOwnerScale(OwnerActor);

    // 在基础Scale上应用角色缩放
    FinalScale = Scale * CharacterScale * CharacterScaleMultiplier;

    UE_LOG(LogXBCombat, Verbose,
           TEXT("[AN_XBPlayNiagaraEffect] 角色缩放: %.2f, 最终特效缩放: (%.2f, "
                "%.2f, %.2f)"),
           CharacterScale, FinalScale.X, FinalScale.Y, FinalScale.Z);
  }

  // ✨ 计算最终位置偏移（可选随角色缩放）
  FVector FinalLocationOffset = LocationOffset;
  if (bScaleWithCharacter && bScaleLocationOffset) {
    FinalLocationOffset = LocationOffset * CharacterScale;
  }

  if (Attached) {
    // 附着模式：附着到插槽，跟随角色移动
    ReturnComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
        Template, MeshComp, SocketName, FinalLocationOffset, RotationOffset,
        EAttachLocation::KeepRelativeOffset, true);
  } else {
    // ✨ 非附着模式：在角色脚底生成，不跟随角色移动
    FVector SpawnLocation;
    FRotator SpawnRotation;

    if (OwnerActor) {
      // 默认使用角色朝向作为特效旋转 + 旋转偏移
      SpawnRotation = OwnerActor->GetActorRotation() + RotationOffset;

      // 计算基础位置（角色位置 + 旋转后的偏移）
      FVector BaseLocation =
          OwnerActor->GetActorLocation() +
          OwnerActor->GetActorRotation().RotateVector(FinalLocationOffset);

      // ✨ 地面检测功能
      if (bSpawnOnGround && GroundTraceTypes.Num() > 0) {
        FHitResult GroundHit;
        FVector TraceStart = BaseLocation;
        FVector TraceEnd = BaseLocation - FVector(0, 0, GroundTraceDistance);

        TArray<AActor *> IgnoreActors;
        IgnoreActors.Add(OwnerActor);

        bool bHitGround = UKismetSystemLibrary::LineTraceSingleForObjects(
            MeshComp->GetWorld(), TraceStart, TraceEnd, GroundTraceTypes,
            false, // bTraceComplex
            IgnoreActors, EDrawDebugTrace::None, GroundHit,
            true // bIgnoreSelf
        );

        if (bHitGround) {
          SpawnLocation = GroundHit.ImpactPoint;
          UE_LOG(LogXBCombat, Verbose,
                 TEXT("[地面检测] 命中地面: %s, 位置: (%.1f, %.1f, %.1f)"),
                 *GroundHit.GetActor()->GetName(), SpawnLocation.X,
                 SpawnLocation.Y, SpawnLocation.Z);
        } else {
          // 未命中地面，使用基础位置
          SpawnLocation = BaseLocation;
          UE_LOG(LogXBCombat, Verbose,
                 TEXT("[地面检测] 未命中地面，使用基础位置"));
        }
      } else {
        SpawnLocation = BaseLocation;
      }
    } else {
      // 回退到插槽位置
      const FTransform MeshTransform = MeshComp->GetSocketTransform(SocketName);
      SpawnLocation = MeshTransform.TransformPosition(FinalLocationOffset);
      SpawnRotation =
          (MeshTransform.GetRotation() * RotationOffsetQuat).Rotator();
    }

    ReturnComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        MeshComp->GetWorld(), Template, SpawnLocation, SpawnRotation,
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
