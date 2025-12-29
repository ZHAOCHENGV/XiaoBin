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
#include "Combat/XBProjectilePoolSubsystem.h"
#include "GAS/XBAttributeSet.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Character/XBCharacterBase.h"
#include "Utils/XBBlueprintFunctionLibrary.h"
#include "Utils/XBLogCategories.h"
#include "XBCollisionChannels.h"

AXBProjectile::AXBProjectile()
{
    PrimaryActorTick.bCanEverTick = false;

    CollisionComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionComponent"));
    CollisionComponent->InitCapsuleSize(12.0f, 24.0f);
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionComponent->SetCollisionResponseToChannel(XBCollision::Soldier, ECR_Overlap);
    CollisionComponent->SetCollisionResponseToChannel(XBCollision::Leader, ECR_Overlap);
    CollisionComponent->SetGenerateOverlapEvents(true);
    RootComponent = CollisionComponent;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(CollisionComponent);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

    if (CollisionComponent)
    {
        CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AXBProjectile::OnProjectileOverlap);
    }
}

void AXBProjectile::InitializeProjectile(AActor* InSourceActor, float InDamage, const FVector& ShootDirection, float InSpeed, bool bInUseArc)
{
    SourceActor = InSourceActor;
    Damage = InDamage;
    bUseArc = bInUseArc;

    const float FinalSpeed = InSpeed > 0.0f ? InSpeed : LinearSpeed;
    ProjectileMovementComponent->InitialSpeed = FinalSpeed;
    ProjectileMovementComponent->MaxSpeed = FinalSpeed;

    FVector Velocity = ShootDirection.GetSafeNormal() * FinalSpeed;
    if (bUseArc)
    {
        ProjectileMovementComponent->ProjectileGravityScale = ArcGravityScale;
        Velocity.Z += ArcLaunchSpeed;
    }
    else
    {
        ProjectileMovementComponent->ProjectileGravityScale = 0.0f;
    }

    ProjectileMovementComponent->Velocity = Velocity;

    UE_LOG(LogXBCombat, Log, TEXT("投射物初始化: 来源=%s 伤害=%.1f 模式=%s 速度=%.1f"),
        InSourceActor ? *InSourceActor->GetName() : TEXT("无"),
        Damage,
        bUseArc ? TEXT("抛射") : TEXT("直线"),
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

    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);

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

    ApplyDamageToTarget(OtherActor, SweepResult);

    if (bDestroyOnHit)
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

void AXBProjectile::ApplyDamageToTarget(AActor* TargetActor, const FHitResult& HitResult)
{
    if (!TargetActor)
    {
        return;
    }

    AActor* Source = SourceActor.Get();
    if (!Source)
    {
        UE_LOG(LogXBCombat, Warning, TEXT("投射物命中 %s，但没有有效的来源"), *TargetActor->GetName());
        return;
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
        return;
    }

    if (!UXBBlueprintFunctionLibrary::AreFactionsHostile(SourceFaction, TargetFaction))
    {
        UE_LOG(LogXBCombat, Verbose, TEXT("投射物忽略友军: %s -> %s"), *Source->GetName(), *TargetActor->GetName());
        return;
    }

    if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(TargetActor))
    {
        float ActualDamage = TargetSoldier->TakeSoldierDamage(Damage, Source);
        UE_LOG(LogXBCombat, Log, TEXT("投射物命中士兵: %s, 伤害: %.1f, 实际: %.1f"),
            *TargetActor->GetName(), Damage, ActualDamage);
        return;
    }

    AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(TargetActor);
    if (!TargetLeader)
    {
        return;
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
        return;
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
