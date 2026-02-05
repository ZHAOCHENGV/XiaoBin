/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Combat/XBProjectile.cpp

/**
 * @file XBProjectile.cpp
 * @brief 远程投射物基类实现 - 支持直线与抛射模式
 *
 * @note ✨ 新增文件
 */

#include "Combat/XBProjectile.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/XBCharacterBase.h"
#include "Combat/XBProjectilePoolSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "GAS/XBAttributeSet.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "XBCollisionChannels.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Sound/XBSoundManagerSubsystem.h"
#include "Utils/XBBlueprintFunctionLibrary.h"
#include "Utils/XBLogCategories.h"

AXBProjectile::AXBProjectile() {
  PrimaryActorTick.bCanEverTick = false;

  // 以静态网格作为根组件，便于朝向与视觉对齐
  MeshComponent =
      CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
  MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  RootComponent = MeshComponent;

  // 创建胶囊碰撞体（碰撞预设在蓝图中配置）
  CapsuleCollision =
      CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleCollision"));
  CapsuleCollision->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
  CapsuleCollision->SetGenerateOverlapEvents(true);
  CapsuleCollision->SetupAttachment(MeshComponent);

  // 创建盒体碰撞体（碰撞预设在蓝图中配置）
  BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
  BoxCollision->SetBoxExtent(BoxExtent);
  BoxCollision->SetGenerateOverlapEvents(true);
  BoxCollision->SetupAttachment(MeshComponent);
  BoxCollision->SetVisibility(false);

  // 创建拖尾 Niagara 组件
  TrailNiagaraComponent =
      CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailNiagaraComponent"));
  TrailNiagaraComponent->SetupAttachment(MeshComponent);
  TrailNiagaraComponent->bAutoActivate = false;

  ProjectileMovementComponent =
      CreateDefaultSubobject<UProjectileMovementComponent>(
          TEXT("ProjectileMovementComponent"));
  ProjectileMovementComponent->InitialSpeed = LinearSpeed;
  ProjectileMovementComponent->MaxSpeed = LinearSpeed;
  ProjectileMovementComponent->bRotationFollowsVelocity = true;
  ProjectileMovementComponent->ProjectileGravityScale = 0.0f;

  DamageTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage"), false);

  // ✨ 新增 - 设置爆炸检测默认对象类型（士兵和主将通道）
  ExplosionObjectTypes.Add(UEngineTypes::ConvertToObjectType(XBCollision::Soldier));
  ExplosionObjectTypes.Add(UEngineTypes::ConvertToObjectType(XBCollision::Leader));
}

void AXBProjectile::BeginPlay() {
  Super::BeginPlay();

  // 根据碰撞体类型更新组件状态
  UpdateCollisionType();

  // 绑定碰撞事件（Overlap 用于角色命中，Hit 用于场景碰撞）
  // 注：场景碰撞由碰撞体的碰撞通道配置决定，无需额外参数控制
  if (CapsuleCollision) {
    CapsuleCollision->OnComponentBeginOverlap.AddDynamic(
        this, &AXBProjectile::OnProjectileOverlap);
    CapsuleCollision->OnComponentHit.AddDynamic(
        this, &AXBProjectile::OnProjectileHit);
  }
  if (BoxCollision) {
    BoxCollision->OnComponentBeginOverlap.AddDynamic(
        this, &AXBProjectile::OnProjectileOverlap);
    BoxCollision->OnComponentHit.AddDynamic(this,
                                            &AXBProjectile::OnProjectileHit);
  }

  // 应用网格缩放
  if (MeshComponent) {
    MeshComponent->SetWorldScale3D(MeshScale);

    if (!MeshComponent->GetStaticMesh()) {
      UE_LOG(LogXBCombat, Warning,
             TEXT("投射物 %s 未配置StaticMesh，可能导致不可见"), *GetName());
    }
  }

  // 启动拖尾特效（如果组件上已配置 Niagara 系统）
  if (TrailNiagaraComponent && TrailNiagaraComponent->GetAsset()) {
    TrailNiagaraComponent->Activate(true);
  }

  // 播放生成音效
  if (SpawnSoundTag.IsValid()) {
    if (UGameInstance *GameInstance = GetGameInstance()) {
      if (UXBSoundManagerSubsystem *SoundMgr =
              GameInstance->GetSubsystem<UXBSoundManagerSubsystem>()) {
        SoundMgr->PlaySoundAtLocation(GetWorld(), SpawnSoundTag,
                                      GetActorLocation());
      }
    }
  }
}

void AXBProjectile::InitializeProjectile(AActor *InSourceActor, float InDamage,
                                         const FVector &ShootDirection,
                                         float InSpeed, bool bInUseArc) {
  // 🔧 修改 - 兼容蓝图调用的初始化入口
  InitializeProjectileWithTarget(InSourceActor, InDamage, ShootDirection,
                                 InSpeed, bInUseArc, FVector::ZeroVector);
}

void AXBProjectile::InitializeProjectileWithTarget(
    AActor *InSourceActor, float InDamage, const FVector &ShootDirection,
    float InSpeed, bool bInUseArc, const FVector &TargetLocation) {
  SourceActor = InSourceActor;
  Damage = InDamage;

  // 判断发射模式
  const bool bIsArcMode =
      (LaunchMode == EXBProjectileLaunchMode::Arc) || bInUseArc;

  // 根据模式选择速度
  float FinalSpeed = 0.0f;
  if (bIsArcMode) {
    FinalSpeed = InSpeed > 0.0f ? InSpeed : ArcSpeed;
  } else {
    FinalSpeed = InSpeed > 0.0f ? InSpeed : LinearSpeed;
  }

  ProjectileMovementComponent->InitialSpeed = FinalSpeed;
  ProjectileMovementComponent->MaxSpeed = FinalSpeed;

  FVector Velocity = ShootDirection.GetSafeNormal() * FinalSpeed;

  if (bIsArcMode) {
    // 抛物线模式
    ProjectileMovementComponent->ProjectileGravityScale = ArcGravityScale;

    // 优先使用目标位置计算抛物线速度
    FVector ActualTarget = TargetLocation;
    if (ActualTarget.IsZero() && ArcDistance > 0.0f) {
      // 没有目标位置时，根据飞行距离计算目标点
      ActualTarget =
          GetActorLocation() + ShootDirection.GetSafeNormal() * ArcDistance;
    }

    if (!ActualTarget.IsZero()) {
      FVector SuggestedVelocity = FVector::ZeroVector;
      const FVector StartLocation = GetActorLocation();
      const float OverrideGravityZ =
          GetWorld() ? GetWorld()->GetGravityZ() * ArcGravityScale : 0.0f;

      const bool bHasSolution = UGameplayStatics::SuggestProjectileVelocity(
          this, SuggestedVelocity, StartLocation, ActualTarget, FinalSpeed,
          false, 0.0f, OverrideGravityZ,
          ESuggestProjVelocityTraceOption::DoNotTrace);

      if (bHasSolution) {
        Velocity = SuggestedVelocity;
      } else {
        // 无解时使用默认上抛角度
        Velocity = ShootDirection.GetSafeNormal() * FinalSpeed * 0.707f;
        Velocity.Z = FinalSpeed * 0.707f;
      }
    } else {
      // 无目标时默认45度上抛
      Velocity = ShootDirection.GetSafeNormal() * FinalSpeed * 0.707f;
      Velocity.Z = FinalSpeed * 0.707f;
    }
  } else {
    // 直线模式
    ProjectileMovementComponent->ProjectileGravityScale = 0.0f;
  }

  ProjectileMovementComponent->Velocity = Velocity;

  // 以飞行方向更新Actor旋转
  SetActorRotation(Velocity.Rotation());

  // 启动存活计时
  if (LifeSeconds > 0.0f) {
    GetWorldTimerManager().ClearTimer(LifeTimerHandle);
    GetWorldTimerManager().SetTimer(LifeTimerHandle, this,
                                    &AXBProjectile::ResetForPooling,
                                    LifeSeconds, false);
  }

  UE_LOG(LogXBCombat, Log,
         TEXT("投射物初始化: 来源=%s 伤害=%.1f 模式=%s 速度=%.1f"),
         InSourceActor ? *InSourceActor->GetName() : TEXT("无"), Damage,
         bIsArcMode ? TEXT("抛物线") : TEXT("直线"), FinalSpeed);
}

void AXBProjectile::ActivateFromPool(const FVector &SpawnLocation,
                                     const FRotator &SpawnRotation) {
  SetActorHiddenInGame(false);
  SetActorEnableCollision(true);
  SetActorLocation(SpawnLocation);
  SetActorRotation(SpawnRotation);

  if (ProjectileMovementComponent) {
    ProjectileMovementComponent->StopMovementImmediately();
  }
}

void AXBProjectile::ResetForPooling() {
  if (ProjectileMovementComponent) {
    ProjectileMovementComponent->StopMovementImmediately();
  }

  GetWorldTimerManager().ClearTimer(LifeTimerHandle);

  // 🔧 修改 - 若启用对象池则回收，否则允许直接销毁
  if (bUsePooling) {
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
  } else {
    Destroy();
    return;
  }

  SourceActor = nullptr;

  UE_LOG(LogXBCombat, Verbose, TEXT("投射物 %s 已重置并进入池化休眠"),
         *GetName());
}

void AXBProjectile::OnProjectileOverlap(
    UPrimitiveComponent *OverlappedComponent, AActor *OtherActor,
    UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult &SweepResult) {
  if (!OtherActor || OtherActor == this) {
    return;
  }

  if (SourceActor.IsValid() && OtherActor == SourceActor.Get()) {
    return;
  }

  // 🔧 修复 - 如果 SourceActor 未设置，尝试从 Owner/Instigator 获取
  AActor *EffectiveSource = SourceActor.Get();
  if (!EffectiveSource) {
    if (GetOwner()) {
      EffectiveSource = GetOwner();
      SourceActor = EffectiveSource; // 缓存以供后续使用
      UE_LOG(LogXBCombat, Verbose,
             TEXT("投射物 %s: SourceActor 未设置，使用 Owner: %s"), *GetName(),
             *EffectiveSource->GetName());
    } else if (GetInstigator()) {
      EffectiveSource = GetInstigator();
      SourceActor = EffectiveSource;
      UE_LOG(LogXBCombat, Verbose,
             TEXT("投射物 %s: SourceActor 未设置，使用 Instigator: %s"),
             *GetName(), *EffectiveSource->GetName());
    }
  }

  // 🔧 修改 - 统一友军判定（包含同阵营、同主将、休眠无敌士兵检查）
  if (UXBBlueprintFunctionLibrary::IsFriendlyTarget(EffectiveSource,
                                                    OtherActor)) {
    UE_LOG(LogXBCombat, Verbose, TEXT("投射物穿透友军或无敌单位: %s -> %s"),
           EffectiveSource ? *EffectiveSource->GetName() : TEXT("None"),
           *OtherActor->GetName());
    return; // 友军或无敌士兵，穿透
  }

  FVector HitLocation = GetActorLocation();
  if (!SweepResult.ImpactPoint.IsZero()) {
    HitLocation = FVector(SweepResult.ImpactPoint);
  }

  bool bDidApplyFlightDamage = false;

  // 飞行伤害（仅 FlightOnly 或 Both 模式）
  if (DamageType == EXBProjectileDamageType::FlightOnly ||
      DamageType == EXBProjectileDamageType::Both) {
    bDidApplyFlightDamage = ApplyDamageToTarget(OtherActor, SweepResult);

    // 播放命中效果
    if (bDidApplyFlightDamage) {
      // 优先使用 Tag 音效
      if (HitSoundTag.IsValid()) {
        if (UGameInstance *GameInstance = GetGameInstance()) {
          if (UXBSoundManagerSubsystem *SoundMgr =
                  GameInstance->GetSubsystem<UXBSoundManagerSubsystem>()) {
            SoundMgr->PlaySoundAtLocation(GetWorld(), HitSoundTag, HitLocation);
          }
        }
      } else if (HitSound) {
        // 🔧 修复 - 使用 GetWorld() 而不是 this，避免发射物销毁时音效被中断
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound,
                                              HitLocation);
      }

      // 播放击中特效（根据类型选择 Niagara 或 Cascade）
      if (HitEffectType == EXBHitEffectType::Niagara && HitEffect) {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this, HitEffect, HitLocation, FRotator::ZeroRotator,
            FVector(HitEffectScale), true, true, ENCPoolMethod::None, true);
      } else if (HitEffectType == EXBHitEffectType::Cascade && HitEffectCascade) {
        UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(), HitEffectCascade, HitLocation, FRotator::ZeroRotator,
            FVector(HitEffectScale), true, EPSCPoolMethod::None, true);
      }
    }
  }

  // 爆炸伤害（仅 ExplosionOnly 或 Both 模式）
  if (DamageType == EXBProjectileDamageType::ExplosionOnly ||
      DamageType == EXBProjectileDamageType::Both) {
    PerformExplosionDamage(HitLocation);
  }

  // 命中后销毁/回收
  if (bDestroyOnHit) {
    DeactivateTrailEffect();

    if (bUsePooling) {
      if (UWorld *World = GetWorld()) {
        if (UXBProjectilePoolSubsystem *PoolSubsystem =
                World->GetSubsystem<UXBProjectilePoolSubsystem>()) {
          PoolSubsystem->ReleaseProjectile(this);
          return;
        }
      }
    }

    Destroy();
  }
}

bool AXBProjectile::ApplyDamageToTarget(AActor *TargetActor,
                                        const FHitResult &HitResult) {
  if (!TargetActor) {
    return false;
  }

  AActor *Source = SourceActor.Get();
  if (!Source) {
    UE_LOG(LogXBCombat, Warning, TEXT("投射物命中 %s，但没有有效的来源"),
           *TargetActor->GetName());
    return false;
  }

  EXBFaction SourceFaction = EXBFaction::Neutral;
  if (AXBSoldierCharacter *SourceSoldier = Cast<AXBSoldierCharacter>(Source)) {
    SourceFaction = SourceSoldier->GetFaction();
  } else if (AXBCharacterBase *SourceLeader = Cast<AXBCharacterBase>(Source)) {
    SourceFaction = SourceLeader->GetFaction();
  }

  EXBFaction TargetFaction = EXBFaction::Neutral;
  if (!GetTargetFaction(TargetActor, TargetFaction)) {
    return false;
  }

  if (!UXBBlueprintFunctionLibrary::AreFactionsHostile(SourceFaction,
                                                       TargetFaction)) {
    // 🔧 修改 - 只对敌人生效，友军直接忽略
    UE_LOG(LogXBCombat, Verbose, TEXT("投射物忽略友军: %s -> %s"),
           *Source->GetName(), *TargetActor->GetName());
    return false;
  }

  if (AXBSoldierCharacter *TargetSoldier =
          Cast<AXBSoldierCharacter>(TargetActor)) {
    // 🔧 修改 - 草丛隐身士兵不可被命中
    if (TargetSoldier->IsHiddenInBush()) {
      return false;
    }
    float ActualDamage = TargetSoldier->TakeSoldierDamage(Damage, Source);
    UE_LOG(LogXBCombat, Log, TEXT("投射物命中士兵: %s, 伤害: %.1f, 实际: %.1f"),
           *TargetActor->GetName(), Damage, ActualDamage);
    return true;
  }

  AXBCharacterBase *TargetLeader = Cast<AXBCharacterBase>(TargetActor);
  if (!TargetLeader) {
    return false;
  }
  // 🔧 修改 - 草丛隐身主将不可被命中
  if (TargetLeader->IsHiddenInBush()) {
    return false;
  }

  if (AXBCharacterBase *SourceLeader = Cast<AXBCharacterBase>(Source)) {
    SourceLeader->OnAttackHit(TargetLeader);
  }

  UAbilitySystemComponent *SourceASC =
      UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Source);
  UAbilitySystemComponent *TargetASC =
      UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);

  if (!TargetASC) {
    UE_LOG(LogXBCombat, Warning, TEXT("投射物命中 %s，但目标没有ASC"),
           *TargetActor->GetName());
    return false;
  }

  if (DamageEffectClass && SourceASC) {
    FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
    ContextHandle.AddSourceObject(Source);
    ContextHandle.AddHitResult(HitResult);

    FGameplayEffectSpecHandle SpecHandle =
        SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
    if (SpecHandle.IsValid()) {
      if (DamageTag.IsValid()) {
        SpecHandle.Data->SetSetByCallerMagnitude(DamageTag, Damage);
      } else {
        UE_LOG(LogXBCombat, Warning,
               TEXT("投射物伤害Tag无效(Data.Damage)，目标=%s"),
               *TargetActor->GetName());
      }

      SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data, TargetASC);

      UE_LOG(LogXBCombat, Log, TEXT("投射物命中将领: %s, 伤害: %.1f (GAS)"),
             *TargetActor->GetName(), Damage);
    }
  } else {
    TargetASC->SetNumericAttributeBase(
        UXBAttributeSet::GetIncomingDamageAttribute(), Damage);
    UE_LOG(LogXBCombat, Log, TEXT("投射物命中将领: %s, 伤害: %.1f (直接属性)"),
           *TargetActor->GetName(), Damage);
  }

  return true;
}

bool AXBProjectile::GetTargetFaction(AActor *TargetActor,
                                     EXBFaction &OutFaction) const {
  if (AXBSoldierCharacter *TargetSoldier =
          Cast<AXBSoldierCharacter>(TargetActor)) {
    OutFaction = TargetSoldier->GetFaction();
    return true;
  }

  if (AXBCharacterBase *TargetLeader = Cast<AXBCharacterBase>(TargetActor)) {
    OutFaction = TargetLeader->GetFaction();
    return true;
  }

  return false;
}

void AXBProjectile::UpdateCollisionType() {
  const bool bUseCapsule =
      (CollisionType == EXBProjectileCollisionType::Capsule);

  if (CapsuleCollision) {
    CapsuleCollision->SetCollisionEnabled(bUseCapsule
                                              ? ECollisionEnabled::QueryOnly
                                              : ECollisionEnabled::NoCollision);
    CapsuleCollision->SetVisibility(bUseCapsule);
    CapsuleCollision->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
  }

  if (BoxCollision) {
    BoxCollision->SetCollisionEnabled(bUseCapsule
                                          ? ECollisionEnabled::NoCollision
                                          : ECollisionEnabled::QueryOnly);
    BoxCollision->SetVisibility(!bUseCapsule);
    BoxCollision->SetBoxExtent(BoxExtent);
  }

  if (MeshComponent) {
    MeshComponent->SetWorldScale3D(MeshScale);
  }
}

#if WITH_EDITOR
void AXBProjectile::PostEditChangeProperty(
    FPropertyChangedEvent &PropertyChangedEvent) {
  Super::PostEditChangeProperty(PropertyChangedEvent);

  const FName PropertyName = PropertyChangedEvent.GetPropertyName();

  // 碰撞体类型或尺寸变更时更新组件
  if (PropertyName == GET_MEMBER_NAME_CHECKED(AXBProjectile, CollisionType) ||
      PropertyName == GET_MEMBER_NAME_CHECKED(AXBProjectile, CapsuleRadius) ||
      PropertyName ==
          GET_MEMBER_NAME_CHECKED(AXBProjectile, CapsuleHalfHeight) ||
      PropertyName == GET_MEMBER_NAME_CHECKED(AXBProjectile, BoxExtent) ||
      PropertyName == GET_MEMBER_NAME_CHECKED(AXBProjectile, MeshScale)) {
    UpdateCollisionType();
  }
}
#endif

void AXBProjectile::DeactivateTrailEffect() {
  if (TrailNiagaraComponent && TrailNiagaraComponent->IsActive()) {
    TrailNiagaraComponent->Deactivate();
  }
}

void AXBProjectile::OnProjectileHit(UPrimitiveComponent *HitComponent,
                                    AActor *OtherActor,
                                    UPrimitiveComponent *OtherComp,
                                    FVector NormalImpulse,
                                    const FHitResult &Hit) {
  // 忽略自身和来源
  if (OtherActor == this || OtherActor == SourceActor.Get()) {
    return;
  }

  // 播放命中音效
  if (HitSound) {
    // 🔧 修复 - 使用 GetWorld() 而不是 this，避免发射物销毁时音效被中断
    UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound,
                                          Hit.ImpactPoint);
  }

  // 播放命中特效（根据类型选择 Niagara 或 Cascade）
  if (HitEffectType == EXBHitEffectType::Niagara && HitEffect) {
    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        this, HitEffect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation(),
        FVector(HitEffectScale), true, true, ENCPoolMethod::None, true);
  } else if (HitEffectType == EXBHitEffectType::Cascade && HitEffectCascade) {
    UGameplayStatics::SpawnEmitterAtLocation(
        GetWorld(), HitEffectCascade, Hit.ImpactPoint, Hit.ImpactNormal.Rotation(),
        FVector(HitEffectScale), true, EPSCPoolMethod::None, true);
  }

  // 命中场景时触发爆炸伤害（仅 ExplosionOnly 或 Both 模式）
  if (DamageType == EXBProjectileDamageType::ExplosionOnly ||
      DamageType == EXBProjectileDamageType::Both) {
    PerformExplosionDamage(Hit.ImpactPoint);
  }

  UE_LOG(LogXBCombat, Verbose, TEXT("投射物 %s 命中场景: %s"), *GetName(),
         *OtherActor->GetName());

  if (bDestroyOnHit) {
    DeactivateTrailEffect();

    if (bUsePooling) {
      if (UWorld *World = GetWorld()) {
        if (UXBProjectilePoolSubsystem *PoolSubsystem =
                World->GetSubsystem<UXBProjectilePoolSubsystem>()) {
          PoolSubsystem->ReleaseProjectile(this);
          return;
        }
      }
    }

    Destroy();
  }
}

void AXBProjectile::PerformExplosionDamage(const FVector &ExplosionLocation) {
  UWorld *World = GetWorld();
  if (!World) {
    return;
  }

  // 播放爆炸特效
  if (ExplosionEffect) {
    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        this, ExplosionEffect, ExplosionLocation, FRotator::ZeroRotator,
        FVector(ExplosionEffectScale), true, true, ENCPoolMethod::None, true);
  }

  // 播放爆炸音效(优先使用Tag)
  if (ExplosionSoundTag.IsValid()) {
    if (UGameInstance *GameInstance = GetGameInstance()) {
      if (UXBSoundManagerSubsystem *SoundMgr =
              GameInstance->GetSubsystem<UXBSoundManagerSubsystem>()) {
        SoundMgr->PlaySoundAtLocation(GetWorld(), ExplosionSoundTag,
                                      ExplosionLocation);
      }
    }
  } else if (ExplosionSound) {
    // 🔧 修复 - 使用 GetWorld() 而不是 this，避免发射物销毁时音效被中断
    UGameplayStatics::PlaySoundAtLocation(GetWorld(), ExplosionSound,
                                          ExplosionLocation);
  }

  // ✨ 新增 - 播放命中特效（在爆炸位置播放）
  if (HitEffectType == EXBHitEffectType::Niagara && HitEffect) {
    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        this, HitEffect, ExplosionLocation, FRotator::ZeroRotator,
        FVector(HitEffectScale), true, true, ENCPoolMethod::None, true);
  } else if (HitEffectType == EXBHitEffectType::Cascade && HitEffectCascade) {
    UGameplayStatics::SpawnEmitterAtLocation(
        GetWorld(), HitEffectCascade, ExplosionLocation, FRotator::ZeroRotator,
        FVector(HitEffectScale), true, EPSCPoolMethod::None, true);
  }

  // 调试可视化：绘制爆炸半径球体
  if (bDebugExplosionRadius) {
    DrawDebugSphere(World, ExplosionLocation, ExplosionRadius, 16, FColor::Red,
                    false, 2.0f, 0, 2.0f);
  }

  // 获取来源阵营
  AActor *Source = SourceActor.Get();
  if (!Source) {
    UE_LOG(LogXBCombat, Warning, TEXT("爆炸伤害执行失败：无有效来源Actor"));
    return;
  }

  EXBFaction SourceFaction = EXBFaction::Neutral;
  if (AXBSoldierCharacter *SourceSoldier = Cast<AXBSoldierCharacter>(Source)) {
    SourceFaction = SourceSoldier->GetFaction();
  } else if (AXBCharacterBase *SourceLeader = Cast<AXBCharacterBase>(Source)) {
    SourceFaction = SourceLeader->GetFaction();
  }

  // 确定实际爆炸伤害（若未设置则使用基础伤害）
  const float ActualExplosionDamage =
      (ExplosionDamage > 0.0f) ? ExplosionDamage : Damage;

  // 球形范围检测
  TArray<FOverlapResult> OverlapResults;
  FCollisionQueryParams QueryParams;
  QueryParams.AddIgnoredActor(this);
  if (Source) {
    QueryParams.AddIgnoredActor(Source);
  }

  // ✨ 新增 - 使用配置的对象类型进行检测（默认为 Soldier 和 Leader 通道）
  FCollisionObjectQueryParams ObjectQueryParams;
  for (const TEnumAsByte<EObjectTypeQuery>& ObjectType : ExplosionObjectTypes) {
    ObjectQueryParams.AddObjectTypesToQuery(
        UEngineTypes::ConvertToCollisionChannel(ObjectType));
  }

  const bool bHasOverlaps = World->OverlapMultiByObjectType(
      OverlapResults, ExplosionLocation, FQuat::Identity, ObjectQueryParams,
      FCollisionShape::MakeSphere(ExplosionRadius), QueryParams);

  if (!bHasOverlaps) {
    UE_LOG(LogXBCombat, Verbose, TEXT("爆炸范围内无目标: 位置=%s 半径=%.1f"),
           *ExplosionLocation.ToString(), ExplosionRadius);
    return;
  }

  // 记录已处理的Actor，避免重复伤害
  TSet<AActor *> ProcessedActors;
  int32 HitCount = 0;

  for (const FOverlapResult &Result : OverlapResults) {
    AActor *HitActor = Result.GetActor();
    if (!HitActor || ProcessedActors.Contains(HitActor)) {
      continue;
    }
    ProcessedActors.Add(HitActor);

    // 获取目标阵营
    EXBFaction TargetFaction = EXBFaction::Neutral;
    if (!GetTargetFaction(HitActor, TargetFaction)) {
      continue;
    }

    // 阵营敌对检查
    if (!UXBBlueprintFunctionLibrary::AreFactionsHostile(SourceFaction,
                                                         TargetFaction)) {
      continue;
    }

    // 对士兵造成伤害
    if (AXBSoldierCharacter *TargetSoldier =
            Cast<AXBSoldierCharacter>(HitActor)) {
      // 草丛隐身检查
      if (TargetSoldier->IsHiddenInBush()) {
        continue;
      }

      float ActualDamage =
          TargetSoldier->TakeSoldierDamage(ActualExplosionDamage, Source);
      HitCount++;

      UE_LOG(LogXBCombat, Log,
             TEXT("爆炸伤害命中士兵: %s, 伤害: %.1f, 实际: %.1f"),
             *HitActor->GetName(), ActualExplosionDamage, ActualDamage);
      continue;
    }

    // 对将领造成伤害
    AXBCharacterBase *TargetLeader = Cast<AXBCharacterBase>(HitActor);
    if (!TargetLeader) {
      continue;
    }

    // 草丛隐身检查
    if (TargetLeader->IsHiddenInBush()) {
      continue;
    }

    // 通知攻击命中
    if (AXBCharacterBase *SourceLeader = Cast<AXBCharacterBase>(Source)) {
      SourceLeader->OnAttackHit(TargetLeader);
    }

    // 使用GAS应用伤害
    UAbilitySystemComponent *SourceASC =
        UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Source);
    UAbilitySystemComponent *TargetASC =
        UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor);

    if (!TargetASC) {
      UE_LOG(LogXBCombat, Warning, TEXT("爆炸伤害目标 %s 无ASC"),
             *HitActor->GetName());
      continue;
    }

    if (DamageEffectClass && SourceASC) {
      FGameplayEffectContextHandle ContextHandle =
          SourceASC->MakeEffectContext();
      ContextHandle.AddSourceObject(Source);

      FGameplayEffectSpecHandle SpecHandle =
          SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
      if (SpecHandle.IsValid()) {
        if (DamageTag.IsValid()) {
          SpecHandle.Data->SetSetByCallerMagnitude(DamageTag,
                                                   ActualExplosionDamage);
        }

        SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data, TargetASC);
        HitCount++;

        UE_LOG(LogXBCombat, Log, TEXT("爆炸伤害命中将领: %s, 伤害: %.1f (GAS)"),
               *HitActor->GetName(), ActualExplosionDamage);
      }
    } else {
      TargetASC->SetNumericAttributeBase(
          UXBAttributeSet::GetIncomingDamageAttribute(), ActualExplosionDamage);
      HitCount++;

      UE_LOG(LogXBCombat, Log,
             TEXT("爆炸伤害命中将领: %s, 伤害: %.1f (直接属性)"),
             *HitActor->GetName(), ActualExplosionDamage);
    }
  }

  UE_LOG(LogXBCombat, Log, TEXT("爆炸伤害完成: 位置=%s 半径=%.1f 命中=%d"),
         *ExplosionLocation.ToString(), ExplosionRadius, HitCount);
}
