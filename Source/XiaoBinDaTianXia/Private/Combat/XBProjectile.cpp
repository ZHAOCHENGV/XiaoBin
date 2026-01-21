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
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Combat/XBProjectilePoolSubsystem.h"
#include "GAS/XBAttributeSet.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Character/XBCharacterBase.h"
#include "Utils/XBBlueprintFunctionLibrary.h"
#include "Utils/XBLogCategories.h"
#include "XBCollisionChannels.h"
#include "Components/BoxComponent.h"

AXBProjectile::AXBProjectile()
{
    PrimaryActorTick.bCanEverTick = false;

    // 以静态网格作为根组件，便于朝向与视觉对齐
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RootComponent = MeshComponent;

    // 创建胶囊碰撞体
    CapsuleCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleCollision"));
    CapsuleCollision->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
    CapsuleCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CapsuleCollision->SetCollisionObjectType(ECC_WorldDynamic);
    CapsuleCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    CapsuleCollision->SetCollisionResponseToChannel(XBCollision::Soldier, ECR_Overlap);
    CapsuleCollision->SetCollisionResponseToChannel(XBCollision::Leader, ECR_Overlap);
    CapsuleCollision->SetGenerateOverlapEvents(true);
    CapsuleCollision->SetupAttachment(MeshComponent);

    // 创建盒体碰撞体
    BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
    BoxCollision->SetBoxExtent(BoxExtent);
    BoxCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BoxCollision->SetCollisionObjectType(ECC_WorldDynamic);
    BoxCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    BoxCollision->SetCollisionResponseToChannel(XBCollision::Soldier, ECR_Overlap);
    BoxCollision->SetCollisionResponseToChannel(XBCollision::Leader, ECR_Overlap);
    BoxCollision->SetGenerateOverlapEvents(true);
    BoxCollision->SetupAttachment(MeshComponent);
    BoxCollision->SetVisibility(false);

    ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
    ProjectileMovementComponent->InitialSpeed = LinearSpeed;
    ProjectileMovementComponent->MaxSpeed = LinearSpeed;
    ProjectileMovementComponent->bRotationFollowsVelocity = true;
    ProjectileMovementComponent->ProjectileGravityScale = 0.0f;

    DamageTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage"), false);
}

void AXBProjectile::BeginPlay()
{
    Super::BeginPlay();

    // 根据碰撞体类型更新组件状态
    UpdateCollisionType();

    // 绑定碰撞事件
    if (CapsuleCollision)
    {
        CapsuleCollision->OnComponentBeginOverlap.AddDynamic(this, &AXBProjectile::OnProjectileOverlap);
    }
    if (BoxCollision)
    {
        BoxCollision->OnComponentBeginOverlap.AddDynamic(this, &AXBProjectile::OnProjectileOverlap);
    }

    // 应用网格缩放
    if (MeshComponent)
    {
        MeshComponent->SetWorldScale3D(MeshScale);
        
        if (!MeshComponent->GetStaticMesh())
        {
            UE_LOG(LogXBCombat, Warning, TEXT("投射物 %s 未配置StaticMesh，可能导致不可见"), *GetName());
        }
    }
}

void AXBProjectile::InitializeProjectile(AActor* InSourceActor, float InDamage, const FVector& ShootDirection, float InSpeed, bool bInUseArc)
{
    // 🔧 修改 - 兼容蓝图调用的初始化入口
    InitializeProjectileWithTarget(InSourceActor, InDamage, ShootDirection, InSpeed, bInUseArc, FVector::ZeroVector);
}

void AXBProjectile::InitializeProjectileWithTarget(AActor* InSourceActor, float InDamage, const FVector& ShootDirection, float InSpeed, bool bInUseArc, const FVector& TargetLocation)
{
    SourceActor = InSourceActor;
    Damage = InDamage;
    
    // 判断发射模式
    const bool bIsArcMode = (LaunchMode == EXBProjectileLaunchMode::Arc) || bInUseArc;
    
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
            ActualTarget = GetActorLocation() + ShootDirection.GetSafeNormal() * ArcDistance;
        }

        if (!ActualTarget.IsZero()) {
            FVector SuggestedVelocity = FVector::ZeroVector;
            const FVector StartLocation = GetActorLocation();
            const float OverrideGravityZ = GetWorld() ? GetWorld()->GetGravityZ() * ArcGravityScale : 0.0f;

            const bool bHasSolution = UGameplayStatics::SuggestProjectileVelocity(
                this,
                SuggestedVelocity,
                StartLocation,
                ActualTarget,
                FinalSpeed,
                false,
                0.0f,
                OverrideGravityZ,
                ESuggestProjVelocityTraceOption::DoNotTrace
            );

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
        GetWorldTimerManager().SetTimer(
            LifeTimerHandle,
            this,
            &AXBProjectile::ResetForPooling,
            LifeSeconds,
            false
        );
    }

    UE_LOG(LogXBCombat, Log, TEXT("投射物初始化: 来源=%s 伤害=%.1f 模式=%s 速度=%.1f"),
        InSourceActor ? *InSourceActor->GetName() : TEXT("无"),
        Damage,
        bIsArcMode ? TEXT("抛物线") : TEXT("直线"),
        FinalSpeed);
}

void AXBProjectile::ActivateFromPool(const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
    SetActorLocation(SpawnLocation);
    SetActorRotation(SpawnRotation);

    if (ProjectileMovementComponent)
    {
        ProjectileMovementComponent->StopMovementImmediately();
    }
}

void AXBProjectile::ResetForPooling()
{
    if (ProjectileMovementComponent)
    {
        ProjectileMovementComponent->StopMovementImmediately();
    }

    GetWorldTimerManager().ClearTimer(LifeTimerHandle);

    // 🔧 修改 - 若启用对象池则回收，否则允许直接销毁
    if (bUsePooling)
    {
        SetActorEnableCollision(false);
        SetActorHiddenInGame(true);
    }
    else
    {
        Destroy();
        return;
    }

    SourceActor = nullptr;

    UE_LOG(LogXBCombat, Verbose, TEXT("投射物 %s 已重置并进入池化休眠"), *GetName());
}

void AXBProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this)
    {
        return;
    }

    if (SourceActor.IsValid() && OtherActor == SourceActor.Get())
    {
        return;
    }

    const bool bDidApplyDamage = ApplyDamageToTarget(OtherActor, SweepResult);

    // 命中敌方且造成伤害时播放效果
    if (bDidApplyDamage)
    {
        FVector HitLocation = GetActorLocation();
        if (!SweepResult.ImpactPoint.IsZero())
        {
            HitLocation = FVector(SweepResult.ImpactPoint);
        }
        
        // 播放命中音效
        if (HitSound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, HitSound, HitLocation);
        }
        
        // 播放命中特效（Niagara）
        if (HitEffect)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                this, HitEffect, HitLocation,
                FRotator::ZeroRotator, FVector(HitEffectScale),
                true, true, ENCPoolMethod::None, true);
        }
    }

    // 仅命中敌方且造成伤害时才允许销毁/回收
    if (bDestroyOnHit && bDidApplyDamage)
    {
        if (bUsePooling)
        {
            if (UWorld* World = GetWorld())
            {
                if (UXBProjectilePoolSubsystem* PoolSubsystem = World->GetSubsystem<UXBProjectilePoolSubsystem>())
                {
                    PoolSubsystem->ReleaseProjectile(this);
                    return;
                }
            }
        }

        Destroy();
    }
}

bool AXBProjectile::ApplyDamageToTarget(AActor* TargetActor, const FHitResult& HitResult)
{
    if (!TargetActor)
    {
        return false;
    }

    AActor* Source = SourceActor.Get();
    if (!Source)
    {
        UE_LOG(LogXBCombat, Warning, TEXT("投射物命中 %s，但没有有效的来源"), *TargetActor->GetName());
        return false;
    }

    EXBFaction SourceFaction = EXBFaction::Neutral;
    if (AXBSoldierCharacter* SourceSoldier = Cast<AXBSoldierCharacter>(Source))
    {
        SourceFaction = SourceSoldier->GetFaction();
    }
    else if (AXBCharacterBase* SourceLeader = Cast<AXBCharacterBase>(Source))
    {
        SourceFaction = SourceLeader->GetFaction();
    }

    EXBFaction TargetFaction = EXBFaction::Neutral;
    if (!GetTargetFaction(TargetActor, TargetFaction))
    {
        return false;
    }

    if (!UXBBlueprintFunctionLibrary::AreFactionsHostile(SourceFaction, TargetFaction))
    {
        // 🔧 修改 - 只对敌人生效，友军直接忽略
        UE_LOG(LogXBCombat, Verbose, TEXT("投射物忽略友军: %s -> %s"), *Source->GetName(), *TargetActor->GetName());
        return false;
    }

    if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(TargetActor))
    {
        // 🔧 修改 - 草丛隐身士兵不可被命中
        if (TargetSoldier->IsHiddenInBush())
        {
            return false;
        }
        float ActualDamage = TargetSoldier->TakeSoldierDamage(Damage, Source);
        UE_LOG(LogXBCombat, Log, TEXT("投射物命中士兵: %s, 伤害: %.1f, 实际: %.1f"),
            *TargetActor->GetName(), Damage, ActualDamage);
        return true;
    }

    AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(TargetActor);
    if (!TargetLeader)
    {
        return false;
    }
    // 🔧 修改 - 草丛隐身主将不可被命中
    if (TargetLeader->IsHiddenInBush())
    {
        return false;
    }

    if (AXBCharacterBase* SourceLeader = Cast<AXBCharacterBase>(Source))
    {
        SourceLeader->OnAttackHit(TargetLeader);
    }

    UAbilitySystemComponent* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Source);
    UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);

    if (!TargetASC)
    {
        UE_LOG(LogXBCombat, Warning, TEXT("投射物命中 %s，但目标没有ASC"), *TargetActor->GetName());
        return false;
    }

    if (DamageEffectClass && SourceASC)
    {
        FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
        ContextHandle.AddSourceObject(Source);
        ContextHandle.AddHitResult(HitResult);

        FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
        if (SpecHandle.IsValid())
        {
            if (DamageTag.IsValid())
            {
                SpecHandle.Data->SetSetByCallerMagnitude(DamageTag, Damage);
            }
            else
            {
                UE_LOG(LogXBCombat, Warning, TEXT("投射物伤害Tag无效(Data.Damage)，目标=%s"), *TargetActor->GetName());
            }

            SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data, TargetASC);

            UE_LOG(LogXBCombat, Log, TEXT("投射物命中将领: %s, 伤害: %.1f (GAS)"),
                *TargetActor->GetName(), Damage);
        }
    }
    else
    {
        TargetASC->SetNumericAttributeBase(UXBAttributeSet::GetIncomingDamageAttribute(), Damage);
        UE_LOG(LogXBCombat, Log, TEXT("投射物命中将领: %s, 伤害: %.1f (直接属性)"),
            *TargetActor->GetName(), Damage);
    }

    return true;
}

bool AXBProjectile::GetTargetFaction(AActor* TargetActor, EXBFaction& OutFaction) const
{
    if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(TargetActor))
    {
        OutFaction = TargetSoldier->GetFaction();
        return true;
    }

    if (AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(TargetActor))
    {
        OutFaction = TargetLeader->GetFaction();
        return true;
    }

    return false;
}

void AXBProjectile::UpdateCollisionType()
{
    const bool bUseCapsule = (CollisionType == EXBProjectileCollisionType::Capsule);
    
    if (CapsuleCollision)
    {
        CapsuleCollision->SetCollisionEnabled(bUseCapsule ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
        CapsuleCollision->SetVisibility(bUseCapsule);
        CapsuleCollision->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
    }
    
    if (BoxCollision)
    {
        BoxCollision->SetCollisionEnabled(bUseCapsule ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
        BoxCollision->SetVisibility(!bUseCapsule);
        BoxCollision->SetBoxExtent(BoxExtent);
    }
    
    if (MeshComponent)
    {
        MeshComponent->SetWorldScale3D(MeshScale);
    }
}

#if WITH_EDITOR
void AXBProjectile::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    
    const FName PropertyName = PropertyChangedEvent.GetPropertyName();
    
    // 碰撞体类型或尺寸变更时更新组件
    if (PropertyName == GET_MEMBER_NAME_CHECKED(AXBProjectile, CollisionType) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(AXBProjectile, CapsuleRadius) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(AXBProjectile, CapsuleHalfHeight) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(AXBProjectile, BoxExtent) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(AXBProjectile, MeshScale))
    {
        UpdateCollisionType();
    }
}
#endif
