// Copyright XiaoBing Project. All Rights Reserved.

#include "Character/XBCharacterBase.h"
#include "GAS/XBAbilitySystemComponent.h"
#include "GAS/XBAttributeSet.h"
#include "Army/XBArmySubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

AXBCharacterBase::AXBCharacterBase()
{
    PrimaryActorTick.bCanEverTick = true;

    // 创建 ASC
    AbilitySystemComponent = CreateDefaultSubobject<UXBAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

    // 创建属性集
    AttributeSet = CreateDefaultSubobject<UXBAttributeSet>(TEXT("AttributeSet"));

    // 配置移动组件
    if (UCharacterMovementComponent* CMC = GetCharacterMovement())
    {
        CMC->bOrientRotationToMovement = true;
        CMC->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
        CMC->MaxWalkSpeed = 600.0f;
        CMC->BrakingDecelerationWalking = 2000.0f;
    }

    // 配置胶囊碰撞
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionProfileName(TEXT("Pawn"));
    }
}

UAbilitySystemComponent* AXBCharacterBase::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AXBCharacterBase::BeginPlay()
{
    Super::BeginPlay();

    BaseScale = GetActorScale3D().X;
}

void AXBCharacterBase::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    // 服务器端初始化 ASC
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
        InitializeAbilitySystem();
    }
}

void AXBCharacterBase::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();

    // 客户端初始化 ASC
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
    }
}

void AXBCharacterBase::InitializeAbilitySystem()
{
    if (!AbilitySystemComponent)
    {
        return;
    }

    // 旧代码（已弃用）：
    // AbilitySystemComponent->GetSpawnedAttributes_Mutable().AddUnique(AttributeSet);

    // 新代码：使用 AddSpawnedAttribute
    if (AttributeSet)
    {
        AbilitySystemComponent->AddSpawnedAttribute(AttributeSet);
    }

    // 添加初始技能
    AddStartupAbilities();

    // 应用初始效果
    ApplyStartupEffects();
}

void AXBCharacterBase::AddStartupAbilities()
{
    if (!AbilitySystemComponent)
    {
        return;
    }

    for (const TSubclassOf<UGameplayAbility>& AbilityClass : StartupAbilities)
    {
        if (AbilityClass)
        {
            FGameplayAbilitySpec AbilitySpec(AbilityClass, 1, INDEX_NONE, this);
            AbilitySystemComponent->GiveAbility(AbilitySpec);
        }
    }
}

void AXBCharacterBase::ApplyStartupEffects()
{
    if (!AbilitySystemComponent)
    {
        return;
    }

    FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
    EffectContext.AddSourceObject(this);

    for (const TSubclassOf<UGameplayEffect>& EffectClass : StartupEffects)
    {
        if (EffectClass)
        {
            FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
                EffectClass, 1, EffectContext);

            if (SpecHandle.IsValid())
            {
                AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
            }
        }
    }
}

bool AXBCharacterBase::TryActivateAbilityByTag(const FGameplayTag& AbilityTag)
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivateAbilityByTag(AbilityTag);
    }
    return false;
}

bool AXBCharacterBase::IsHostileTo(const AXBCharacterBase* Other) const
{
    if (!Other)
    {
        return false;
    }

    // 中立对所有人都不敌对
    if (Faction == EXBFaction::Neutral || Other->Faction == EXBFaction::Neutral)
    {
        return false;
    }

    // 不同阵营为敌对
    return Faction != Other->Faction;
}

int32 AXBCharacterBase::GetSoldierCount() const
{
    if (UWorld* World = GetWorld())
    {
        if (UXBArmySubsystem* ArmySubsystem = World->GetSubsystem<UXBArmySubsystem>())
        {
            return ArmySubsystem->GetSoldierCountByLeader(this);
        }
    }
    return 0;
}

void AXBCharacterBase::RecruitSoldier(int32 SoldierId)
{
    if (UWorld* World = GetWorld())
    {
        if (UXBArmySubsystem* ArmySubsystem = World->GetSubsystem<UXBArmySubsystem>())
        {
            int32 NewSlotIndex = GetSoldierCount();
            if (ArmySubsystem->AssignSoldierToLeader(SoldierId, this, NewSlotIndex))
            {
                OnSoldierRecruited();
            }
        }
    }
}

void AXBCharacterBase::RecallAllSoldiers()
{
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->ExitCombat();
    }

    if (UWorld* World = GetWorld())
    {
        if (UXBArmySubsystem* ArmySubsystem = World->GetSubsystem<UXBArmySubsystem>())
        {
            ArmySubsystem->ExitCombatForLeader(const_cast<AXBCharacterBase*>(this));
        }
    }
}

void AXBCharacterBase::EnterCombat()
{
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->EnterCombat();
    }

    if (UWorld* World = GetWorld())
    {
        if (UXBArmySubsystem* ArmySubsystem = World->GetSubsystem<UXBArmySubsystem>())
        {
            ArmySubsystem->EnterCombatForLeader(const_cast<AXBCharacterBase*>(this));
        }
    }
}

void AXBCharacterBase::ExitCombat()
{
    RecallAllSoldiers();
}

bool AXBCharacterBase::IsInCombat() const
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->IsInCombat();
    }
    return false;
}

void AXBCharacterBase::UpdateScaleFromSoldierCount()
{
    const int32 SoldierCount = GetSoldierCount();
    
    // 线性叠加：Scale = BaseScale + (SoldierCount * ScalePerSoldier)
    const float NewScale = FMath::Min(
        BaseScale + (SoldierCount * GrowthConfig.ScalePerSoldier),
        GrowthConfig.MaxScale);

    SetActorScale3D(FVector(NewScale));

    // 更新属性集中的 Scale
    if (AttributeSet)
    {
        AttributeSet->SetScale(NewScale);
    }
}

void AXBCharacterBase::OnSoldierRecruited()
{
    if (!AttributeSet)
    {
        return;
    }

    const float CurrentHealth = AttributeSet->GetHealth();
    const float CurrentMaxHealth = AttributeSet->GetMaxHealth();
    const float HealAmount = GrowthConfig.HealthPerSoldier;

    // 根据策划文档的逻辑：
    // 如果 CurrentHealth + HealAmount > CurrentMaxHealth，则提升 MaxHealth
    // 否则只回复血量
    const float NewHealth = CurrentHealth + HealAmount;
    
    if (NewHealth > CurrentMaxHealth)
    {
        // 提升最大血量
        AttributeSet->SetMaxHealth(NewHealth);
    }
    
    // 回复血量（不超过最大血量）
    AttributeSet->SetHealth(FMath::Min(NewHealth, AttributeSet->GetMaxHealth()));

    // 更新缩放
    UpdateScaleFromSoldierCount();
}

void AXBCharacterBase::OnSoldierDied()
{
    // 只缩小，不扣血
    UpdateScaleFromSoldierCount();
}

void AXBCharacterBase::SetCharacterHidden(bool bNewHidden)
{
    bIsHiddenInGrass = bNewHidden;

    // 通知士兵也隐身
    if (UWorld* World = GetWorld())
    {
        if (UXBArmySubsystem* ArmySubsystem = World->GetSubsystem<UXBArmySubsystem>())
        {
            ArmySubsystem->SetHiddenForLeader(const_cast<AXBCharacterBase*>(this), bNewHidden);
        }
    }

    // 关闭与敌人的碰撞
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        if (bNewHidden)
        {
            Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
        }
        else
        {
            Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
        }
    }
}



void AXBCharacterBase::OnHealthChanged(float OldValue, float NewValue)
{
    if (NewValue <= 0.0f && OldValue > 0.0f)
    {
        OnDeath();
    }
}

void AXBCharacterBase::OnScaleChanged(float OldValue, float NewValue)
{
    // 缩放变化时更新碰撞体大小
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        const float ScaleRatio = NewValue / BaseScale;
        Capsule->SetCapsuleSize(
            Capsule->GetUnscaledCapsuleRadius() * ScaleRatio,
            Capsule->GetUnscaledCapsuleHalfHeight() * ScaleRatio);
    }
}

void AXBCharacterBase::OnDeath_Implementation()
{
    // 销毁所有士兵
    if (UWorld* World = GetWorld())
    {
        if (UXBArmySubsystem* ArmySubsystem = World->GetSubsystem<UXBArmySubsystem>())
        {
            TArray<int32> Soldiers = ArmySubsystem->GetSoldiersByLeader(this);
            for (int32 SoldierId : Soldiers)
            {
                ArmySubsystem->DestroySoldier(SoldierId);
            }
        }
    }

    // 禁用碰撞
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // 禁用移动
    if (UCharacterMovementComponent* CMC = GetCharacterMovement())
    {
        CMC->DisableMovement();
    }

    // TODO: 播放死亡动画，掉落士兵
}