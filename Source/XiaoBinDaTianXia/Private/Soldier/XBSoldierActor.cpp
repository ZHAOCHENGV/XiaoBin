
/**
 * @file XBSoldierActor.cpp
 * @brief 士兵Actor实现
 */

#include "Soldier/XBSoldierActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Soldier/Component/XBSoldierFollowComponent.h"
#include "Character/XBCharacterBase.h"
#include "Kismet/KismetMathLibrary.h"

AXBSoldierActor::AXBSoldierActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // 创建碰撞体
    CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
    CapsuleComponent->InitCapsuleSize(34.0f, 88.0f);
    CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
    RootComponent = CapsuleComponent;

    // 创建网格体
    MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(RootComponent);
    MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));

    // 创建跟随组件
    FollowComponent = CreateDefaultSubobject<UXBSoldierFollowComponent>(TEXT("FollowComponent"));
}

void AXBSoldierActor::BeginPlay()
{
    Super::BeginPlay();

    // 初始化血量
    CurrentHealth = SoldierConfig.BaseHealth;
}

void AXBSoldierActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 更新攻击冷却
    if (AttackCooldownTimer > 0.0f)
    {
        AttackCooldownTimer -= DeltaTime;
    }

    // 根据状态更新
    switch (CurrentState)
    {
    case EXBSoldierState::Following:
        UpdateFollowing(DeltaTime);
        break;
    case EXBSoldierState::Combat:
        UpdateCombat(DeltaTime);
        break;
    case EXBSoldierState::Returning:
        UpdateFollowing(DeltaTime);
        // 检查是否已返回到编队位置
        if (FollowComponent && FollowComponent->IsAtFormationPosition())
        {
            SetSoldierState(EXBSoldierState::Following);
        }
        break;
    default:
        break;
    }
}

void AXBSoldierActor::InitializeSoldier(const FXBSoldierConfig& InConfig, EXBFaction InFaction)
{
    SoldierConfig = InConfig;
    SoldierType = InConfig.SoldierType;
    Faction = InFaction;
    CurrentHealth = InConfig.BaseHealth;

    // 设置网格
    if (InConfig.SoldierMesh)
    {
        MeshComponent->SetSkeletalMesh(InConfig.SoldierMesh);
    }

    // 配置跟随组件
    if (FollowComponent)
    {
        FollowComponent->SetFollowSpeed(InConfig.MoveSpeed);
        FollowComponent->SetFollowInterpSpeed(InConfig.FollowInterpSpeed);
    }

    UE_LOG(LogTemp, Log, TEXT("Soldier initialized: Type=%d, Health=%.1f"), 
        static_cast<int32>(SoldierType), CurrentHealth);
}

void AXBSoldierActor::SetFollowTarget(AActor* NewLeader, int32 SlotIndex)
{
    FollowTarget = NewLeader;
    FormationSlotIndex = SlotIndex;

    if (FollowComponent)
    {
        FollowComponent->SetFollowTarget(NewLeader);
        FollowComponent->SetFormationSlotIndex(SlotIndex);
    }

    // 设置为跟随状态
    if (NewLeader)
    {
        SetSoldierState(EXBSoldierState::Following);
    }
    else
    {
        SetSoldierState(EXBSoldierState::Idle);
    }
}

void AXBSoldierActor::SetSoldierState(EXBSoldierState NewState)
{
    if (CurrentState == NewState)
    {
        return;
    }

    EXBSoldierState OldState = CurrentState;
    CurrentState = NewState;

    UE_LOG(LogTemp, Log, TEXT("Soldier state changed: %d -> %d"), 
        static_cast<int32>(OldState), static_cast<int32>(NewState));
}

void AXBSoldierActor::EnterCombat()
{
    SetSoldierState(EXBSoldierState::Combat);
}

void AXBSoldierActor::ExitCombat()
{
    SetSoldierState(EXBSoldierState::Returning);
}

float AXBSoldierActor::TakeSoldierDamage(float DamageAmount, AActor* DamageSource)
{
    if (CurrentState == EXBSoldierState::Dead)
    {
        return 0.0f;
    }

    float ActualDamage = FMath::Min(DamageAmount, CurrentHealth);
    CurrentHealth -= ActualDamage;

    UE_LOG(LogTemp, Log, TEXT("Soldier took %.1f damage, remaining health: %.1f"), 
        ActualDamage, CurrentHealth);

    if (CurrentHealth <= 0.0f)
    {
        HandleDeath();
    }

    return ActualDamage;
}

void AXBSoldierActor::SetEscaping(bool bEscaping)
{
    bIsEscaping = bEscaping;

    if (FollowComponent)
    {
        // 逃跑时加速
        float SpeedMultiplier = bEscaping ? 2.0f : 1.0f;
        FollowComponent->SetFollowSpeed(SoldierConfig.MoveSpeed * SpeedMultiplier);
    }
}

void AXBSoldierActor::UpdateFollowing(float DeltaTime)
{
    // 跟随逻辑由 FollowComponent 处理
    if (FollowComponent)
    {
        FollowComponent->UpdateFollowing(DeltaTime);
    }
}

void AXBSoldierActor::UpdateCombat(float DeltaTime)
{
    // TODO: 实现战斗逻辑
    // - 寻找最近的敌人
    // - 移动到攻击范围
    // - 攻击敌人
    
    // 简化实现：暂时只做跟随
    if (FollowComponent)
    {
        FollowComponent->UpdateFollowing(DeltaTime);
    }
}

void AXBSoldierActor::HandleDeath()
{
    SetSoldierState(EXBSoldierState::Dead);

    // 通知将领士兵死亡
    if (FollowTarget.IsValid())
    {
        if (AXBCharacterBase* LeaderCharacter = Cast<AXBCharacterBase>(FollowTarget.Get()))
        {
            LeaderCharacter->OnSoldierDied();
        }
    }

    // 禁用碰撞
    CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // TODO: 播放死亡动画，然后销毁
    SetLifeSpan(2.0f);
}