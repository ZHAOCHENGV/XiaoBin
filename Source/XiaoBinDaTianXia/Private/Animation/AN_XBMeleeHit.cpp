/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Animation/AN_XBMeleeHit.cpp

/**
 * @file AN_XBMeleeHit.cpp
 * @brief 近战命中动画通知实现 - 单次伤害判定
 *
 * @note ✨ 新增文件 - 基于 ANS_XBMeleeDetection 改造
 */

#include "Animation/AN_XBMeleeHit.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/Components/XBCombatComponent.h"
#include "Character/XBCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GAS/XBAttributeSet.h"
#include "Game/XBGameInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Sound/XBSoundManagerSubsystem.h"
#include "Utils/XBBlueprintFunctionLibrary.h"
#include "Utils/XBLogCategories.h"
#include "XBCollisionChannels.h"

UAN_XBMeleeHit::UAN_XBMeleeHit() {
  DamageTag = FGameplayTag::RequestGameplayTag(FName("Damage.Base"), false);
}

void UAN_XBMeleeHit::Notify(USkeletalMeshComponent *MeshComp,
                            UAnimSequenceBase *Animation,
                            const FAnimNotifyEventReference &EventReference) {
  Super::Notify(MeshComp, Animation, EventReference);

  if (!MeshComp) {
    return;
  }

  AActor *OwnerActor = MeshComp->GetOwner();
  if (!OwnerActor) {
    return;
  }

  // 获取伤害值
  float Damage = GetAttackDamage(OwnerActor);
  UE_LOG(LogXBCombat, Log,
         TEXT("[AN_XBMeleeHit] 执行单次伤害检测 - 攻击者: %s, 伤害: %.1f"),
         *OwnerActor->GetName(), Damage);

  // 执行伤害检测
  TArray<FHitResult> HitResults = PerformHitDetection(MeshComp);

  // 应用伤害
  if (HitResults.Num() > 0) {
    ApplyDamageToTargets(HitResults, OwnerActor);
    UE_LOG(LogXBCombat, Log, TEXT("[AN_XBMeleeHit] 命中 %d 个目标"),
           HitResults.Num());
  } else {
    UE_LOG(LogXBCombat, Verbose, TEXT("[AN_XBMeleeHit] 未命中任何目标"));
  }
}

TArray<FHitResult>
UAN_XBMeleeHit::PerformHitDetection(USkeletalMeshComponent *MeshComp) {
  TArray<FHitResult> OutHitResults;

  if (!MeshComp) {
    return OutHitResults;
  }

  AActor *OwnerActor = MeshComp->GetOwner();
  if (!OwnerActor) {
    return OutHitResults;
  }

  // 获取缩放
  float OwnerScale = GetOwnerScale(OwnerActor);

  // 获取角色朝向（用于旋转检测区域）
  FRotator CharacterRotation = FRotator::ZeroRotator;
  if (HitConfig.bRotateWithCharacter) {
    CharacterRotation = OwnerActor->GetActorRotation();
  }

  // 获取起始和结束位置
  FVector StartLocation;
  FVector EndLocation;

  if (!HitConfig.StartSocketName.IsNone() &&
      MeshComp->DoesSocketExist(HitConfig.StartSocketName)) {
    FTransform StartTransform = MeshComp->GetSocketTransform(
        HitConfig.StartSocketName, ERelativeTransformSpace::RTS_World);
    // 应用旋转偏移
    FQuat StartRotationQuat =
        (CharacterRotation + HitConfig.StartRotationOffset).Quaternion();
    FVector RotatedStartOffset =
        StartRotationQuat.RotateVector(HitConfig.StartLocationOffset);
    StartLocation = StartTransform.GetLocation() + RotatedStartOffset;
  } else {
    // 无插槽时，使用角色位置并应用旋转偏移
    FQuat StartRotationQuat =
        (CharacterRotation + HitConfig.StartRotationOffset).Quaternion();
    FVector RotatedStartOffset =
        StartRotationQuat.RotateVector(HitConfig.StartLocationOffset);
    StartLocation = MeshComp->GetComponentLocation() + RotatedStartOffset;
  }

  if (!HitConfig.EndSocketName.IsNone() &&
      MeshComp->DoesSocketExist(HitConfig.EndSocketName)) {
    FTransform EndTransform = MeshComp->GetSocketTransform(
        HitConfig.EndSocketName, ERelativeTransformSpace::RTS_World);
    // 应用旋转偏移
    FQuat EndRotationQuat =
        (CharacterRotation + HitConfig.EndRotationOffset).Quaternion();
    FVector RotatedEndOffset =
        EndRotationQuat.RotateVector(HitConfig.EndLocationOffset);
    EndLocation = EndTransform.GetLocation() + RotatedEndOffset;
  } else {
    // 无插槽时，使用起始位置+角色朝向前方
    FQuat EndRotationQuat =
        (CharacterRotation + HitConfig.EndRotationOffset).Quaternion();
    FVector DefaultDirection =
        EndRotationQuat.RotateVector(FVector(100.0f, 0, 0)); // 朝前方向
    FVector RotatedEndOffset =
        EndRotationQuat.RotateVector(HitConfig.EndLocationOffset);
    EndLocation = StartLocation + DefaultDirection + RotatedEndOffset;
  }

  // 计算检测半径
  float ScaledRadius = HitConfig.DetectionRadius;
  if (HitConfig.bEnableRangeScaling) {
    ScaledRadius *= OwnerScale * HitConfig.ScaleMultiplier;
  }

  // 构建碰撞通道
  TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
  ObjectTypes.Add(
      UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

  if (HitConfig.bDetectSoldierChannel) {
    EObjectTypeQuery SoldierQuery =
        UEngineTypes::ConvertToObjectType(XBCollision::Soldier);
    ObjectTypes.AddUnique(SoldierQuery);
  }

  if (HitConfig.bDetectLeaderChannel) {
    EObjectTypeQuery LeaderQuery =
        UEngineTypes::ConvertToObjectType(XBCollision::Leader);
    ObjectTypes.AddUnique(LeaderQuery);
  }

  // 忽略自身
  TArray<AActor *> IgnoreActors;
  IgnoreActors.Add(OwnerActor);

  // 执行扫掠检测
  TArray<FHitResult> HitResults;
  bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
      MeshComp->GetWorld(), StartLocation, EndLocation, ScaledRadius,
      ObjectTypes, false, IgnoreActors,
      HitConfig.bEnableDebugDraw ? EDrawDebugTrace::ForDuration
                                 : EDrawDebugTrace::None,
      HitResults, true, FLinearColor::Red, FLinearColor::Green,
      HitConfig.DebugDrawDuration);

  // 调试绘制
  if (HitConfig.bEnableDebugDraw) {
    if (UWorld *World = MeshComp->GetWorld()) {
      DrawDebugSphere(World, StartLocation, ScaledRadius, 12, FColor::Blue,
                      false, HitConfig.DebugDrawDuration);
      DrawDebugSphere(World, EndLocation, ScaledRadius, 12, FColor::Cyan, false,
                      HitConfig.DebugDrawDuration);
      DrawDebugLine(World, StartLocation, EndLocation,
                    bHit ? FColor::Red : FColor::Green, false,
                    HitConfig.DebugDrawDuration, 0, 3.0f);

      FVector TextLocation = (StartLocation + EndLocation) * 0.5f +
                             FVector(0, 0, ScaledRadius + 20.0f);
      DrawDebugString(World, TextLocation,
                      FString::Printf(TEXT("单次检测 缩放: %.2fx, 命中: %d"),
                                      OwnerScale, HitResults.Num()),
                      nullptr, FColor::White, HitConfig.DebugDrawDuration);
    }
  }

  return HitResults;
}

float UAN_XBMeleeHit::GetOwnerScale(AActor *OwnerActor) const {
  if (!OwnerActor) {
    return 1.0f;
  }

  if (AXBCharacterBase *Character = Cast<AXBCharacterBase>(OwnerActor)) {
    float CharScale = Character->GetCurrentScale();
    if (CharScale > 0.0f) {
      return CharScale;
    }
  }

  if (UAbilitySystemComponent *ASC =
          UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(
              OwnerActor)) {
    float ScaleAttribute =
        ASC->GetNumericAttribute(UXBAttributeSet::GetScaleAttribute());
    if (ScaleAttribute > 0.0f) {
      return ScaleAttribute;
    }
  }

  FVector ActorScale = OwnerActor->GetActorScale3D();
  if (ActorScale.X > 0.0f) {
    return ActorScale.X;
  }

  return 1.0f;
}

EXBFaction UAN_XBMeleeHit::GetOwnerFaction(AActor *OwnerActor) const {
  if (!OwnerActor) {
    return EXBFaction::Neutral;
  }

  if (AXBCharacterBase *Character = Cast<AXBCharacterBase>(OwnerActor)) {
    return Character->GetFaction();
  }

  if (AXBSoldierCharacter *Soldier = Cast<AXBSoldierCharacter>(OwnerActor)) {
    return Soldier->GetFaction();
  }

  return EXBFaction::Neutral;
}

bool UAN_XBMeleeHit::ShouldDamageTarget(AActor *OwnerActor,
                                        AActor *TargetActor) const {
  if (!OwnerActor || !TargetActor) {
    return false;
  }

  if (OwnerActor == TargetActor) {
    return false;
  }

  if (!HitConfig.bEnableFactionFilter) {
    return true;
  }

  EXBFaction OwnerFaction = GetOwnerFaction(OwnerActor);
  EXBFaction TargetFaction = EXBFaction::Neutral;

  if (AXBCharacterBase *TargetCharacter = Cast<AXBCharacterBase>(TargetActor)) {
    if (TargetCharacter->IsHiddenInBush()) {
      return false;
    }
    TargetFaction = TargetCharacter->GetFaction();
  } else if (AXBSoldierCharacter *TargetSoldier =
                 Cast<AXBSoldierCharacter>(TargetActor)) {
    if (TargetSoldier->IsHiddenInBush()) {
      return false;
    }
    if (!TargetSoldier->IsRecruited() || !TargetSoldier->GetLeaderCharacter()) {
      return false;
    }
    TargetFaction = TargetSoldier->GetFaction();
  } else {
    return true;
  }

  return UXBBlueprintFunctionLibrary::AreFactionsHostile(OwnerFaction,
                                                         TargetFaction);
}

float UAN_XBMeleeHit::GetAttackDamage(AActor *OwnerActor) const {
  if (!OwnerActor) {
    return 0.0f;
  }

  // 从将领战斗组件获取
  if (AXBCharacterBase *Character = Cast<AXBCharacterBase>(OwnerActor)) {
    if (UXBCombatComponent *CombatComp = Character->GetCombatComponent()) {
      float FinalDamage = CombatComp->GetCurrentAttackFinalDamage();
      if (FinalDamage > 0.0f) {
        return FinalDamage;
      }
    }
  }

  // 从士兵获取
  if (AXBSoldierCharacter *Soldier = Cast<AXBSoldierCharacter>(OwnerActor)) {
    return Soldier->GetBaseDamage();
  }

  return 10.0f; // 默认伤害
}

void UAN_XBMeleeHit::ApplyDamageToTargets(const TArray<FHitResult> &HitResults,
                                          AActor *OwnerActor) {
  if (!OwnerActor) {
    return;
  }

  float AttackDamage = GetAttackDamage(OwnerActor);
  if (AttackDamage <= 0.0f) {
    UE_LOG(LogXBCombat, Warning, TEXT("[AN_XBMeleeHit] 攻击伤害为 0，跳过"));
    return;
  }

  UAbilitySystemComponent *SourceASC =
      UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor);
  TSet<AActor *> ProcessedActors;
  bool bPlayedHitSound = false;

  for (const FHitResult &Hit : HitResults) {
    AActor *HitActor = Hit.GetActor();
    if (!HitActor || ProcessedActors.Contains(HitActor)) {
      continue;
    }

    if (!ShouldDamageTarget(OwnerActor, HitActor)) {
      continue;
    }

    ProcessedActors.Add(HitActor);

    // 播放命中音效（只播放一次）
    if (HitSoundTag.IsValid() && !bPlayedHitSound) {
      if (UWorld *World = OwnerActor->GetWorld()) {
        if (UGameInstance *GameInstance =
                World->GetGameInstance<UGameInstance>()) {
          if (UXBSoundManagerSubsystem *SoundMgr =
                  GameInstance->GetSubsystem<UXBSoundManagerSubsystem>()) {
            SoundMgr->PlaySoundAtLocation(World, HitSoundTag, Hit.ImpactPoint);
            bPlayedHitSound = true;
          }
        }
      }
    }

    // 对士兵造成伤害
    if (AXBSoldierCharacter *TargetSoldier =
            Cast<AXBSoldierCharacter>(HitActor)) {
      float ActualDamage =
          TargetSoldier->TakeSoldierDamage(AttackDamage, OwnerActor);
      UE_LOG(LogXBCombat, Log, TEXT("[AN_XBMeleeHit] 命中士兵: %s, 伤害: %.1f"),
             *HitActor->GetName(), ActualDamage);
      continue;
    }

    // 主将命中主将时，触发士兵进入战斗
    if (AXBCharacterBase *OwnerLeader = Cast<AXBCharacterBase>(OwnerActor)) {
      if (AXBCharacterBase *TargetLeader = Cast<AXBCharacterBase>(HitActor)) {
        OwnerLeader->OnAttackHit(TargetLeader);
      }
    }

    // 对将领使用 GAS 伤害
    UAbilitySystemComponent *TargetASC =
        UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor);
    if (!TargetASC) {
      continue;
    }

    if (DamageEffectClass && SourceASC) {
      FGameplayEffectContextHandle ContextHandle =
          SourceASC->MakeEffectContext();
      ContextHandle.AddSourceObject(OwnerActor);
      ContextHandle.AddHitResult(Hit);

      FGameplayEffectSpecHandle SpecHandle =
          SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
      if (SpecHandle.IsValid()) {
        if (DamageTag.IsValid()) {
          SpecHandle.Data->SetSetByCallerMagnitude(DamageTag, AttackDamage);
        }
        SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data, TargetASC);
        UE_LOG(LogXBCombat, Log,
               TEXT("[AN_XBMeleeHit] 命中将领: %s, 伤害: %.1f (GAS)"),
               *HitActor->GetName(), AttackDamage);
      }
    } else if (TargetASC) {
      TargetASC->SetNumericAttributeBase(
          UXBAttributeSet::GetIncomingDamageAttribute(), AttackDamage);
      UE_LOG(LogXBCombat, Log,
             TEXT("[AN_XBMeleeHit] 命中: %s, 伤害: %.1f (直接属性)"),
             *HitActor->GetName(), AttackDamage);
    }
  }
}
