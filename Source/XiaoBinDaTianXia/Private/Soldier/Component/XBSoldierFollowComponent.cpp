/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Soldier/Component/XBSoldierFollowComponent.cpp

/**
 * @file XBSoldierFollowComponent.cpp
 * @brief 士兵跟随组件实现
 * 
 * @note 🔧 修改记录:
 *       1. 完善跟随算法
 *       2. 集成编队组件
 *       3. 实现避障逻辑
 */

#include "Soldier/Component/XBSoldierFollowComponent.h"
#include "Character/XBCharacterBase.h"
#include "Character/Components/XBFormationComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Soldier/XBSoldierActor.h"

UXBSoldierFollowComponent::UXBSoldierFollowComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UXBSoldierFollowComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UXBSoldierFollowComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsFollowing)
    {
        UpdateFollowMovement(DeltaTime);
    }
}

// ============ 目标设置实现 ============

void UXBSoldierFollowComponent::SetLeader(AXBCharacterBase* NewLeader)
{
    LeaderRef = NewLeader;
    FollowTargetRef = NewLeader;

    // 缓存编队组件
    if (NewLeader)
    {
        // 尝试获取PlayerCharacter的编队组件
        CachedFormationComponent = NewLeader->FindComponentByClass<UXBFormationComponent>();
    }
    else
    {
        CachedFormationComponent = nullptr;
    }
}

void UXBSoldierFollowComponent::SetFollowTarget(AActor* NewTarget)
{
    FollowTargetRef = NewTarget;
    
    // 如果是角色，尝试获取编队组件
    if (AXBCharacterBase* CharTarget = Cast<AXBCharacterBase>(NewTarget))
    {
        LeaderRef = CharTarget;
        CachedFormationComponent = CharTarget->FindComponentByClass<UXBFormationComponent>();
    }
}

// ============ 编队设置实现 ============

void UXBSoldierFollowComponent::SetFormationOffset(const FVector& Offset)
{
    FormationOffset = Offset;
}

void UXBSoldierFollowComponent::SetFormationSlotIndex(int32 SlotIndex)
{
    FormationSlotIndex = SlotIndex;
}

// ============ 速度设置实现 ============

void UXBSoldierFollowComponent::SetInterpSpeed(float NewSpeed)
{
    InterpSpeed = NewSpeed;
}

void UXBSoldierFollowComponent::SetFollowInterpSpeed(float NewSpeed)
{
    InterpSpeed = NewSpeed;
}

void UXBSoldierFollowComponent::SetFollowSpeed(float NewSpeed)
{
    FollowSpeed = NewSpeed;
    
    // 同时更新角色移动组件的速度
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    // 支持Character和普通Actor
    if (ACharacter* OwnerCharacter = Cast<ACharacter>(Owner))
    {
        if (UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement())
        {
            MoveComp->MaxWalkSpeed = NewSpeed;
        }
    }
}

// ============ 跟随控制实现 ============

void UXBSoldierFollowComponent::StartFollowing()
{
    bIsFollowing = true;
    SetComponentTickEnabled(true);
}

void UXBSoldierFollowComponent::StopFollowing()
{
    bIsFollowing = false;
    SetComponentTickEnabled(false);
}

void UXBSoldierFollowComponent::UpdateFollowing(float DeltaTime)
{
    if (bIsFollowing)
    {
        UpdateFollowMovement(DeltaTime);
    }
}

// ============ 状态查询实现 ============

bool UXBSoldierFollowComponent::IsAtFormationPosition() const
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return false;
    }

    FVector CurrentPosition = Owner->GetActorLocation();
    FVector TargetPosition = CalculateTargetPosition();
    
    return FVector::Dist2D(CurrentPosition, TargetPosition) <= ArrivalThreshold;
}

FVector UXBSoldierFollowComponent::GetTargetPosition() const
{
    return CalculateTargetPosition();
}

float UXBSoldierFollowComponent::GetDistanceToTarget() const
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return 0.0f;
    }

    return FVector::Dist2D(Owner->GetActorLocation(), CalculateTargetPosition());
}

// ============ 内部方法实现 ============

/**
 * @brief 更新跟随移动
 */
void UXBSoldierFollowComponent::UpdateFollowMovement(float DeltaTime)
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    // 计算目标位置
    FVector TargetPosition = CalculateTargetPosition();
    CachedTargetPosition = TargetPosition;
    
    FVector CurrentPosition = Owner->GetActorLocation();
    float DistanceToTarget = FVector::Dist2D(CurrentPosition, TargetPosition);

    // 距离太近不需要移动
    if (DistanceToTarget <= MinDistanceToMove)
    {
        return;
    }

    // 计算移动方向
    FVector Direction = (TargetPosition - CurrentPosition).GetSafeNormal2D();

    // 应用避障
    Direction = ApplyAvoidance(Direction);

    // 根据Owner类型选择移动方式
    if (ACharacter* OwnerCharacter = Cast<ACharacter>(Owner))
    {
        // 使用Character移动组件
        OwnerCharacter->AddMovementInput(Direction, 1.0f);

        // 平滑旋转
        if (!Direction.IsNearlyZero())
        {
            FRotator TargetRotation = Direction.Rotation();
            FRotator CurrentRotation = Owner->GetActorRotation();
            FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, InterpSpeed * 2.0f);
            Owner->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
        }
    }
    else
    {
        // 直接设置位置（非角色Actor）
        FVector NewPosition = FMath::VInterpTo(CurrentPosition, TargetPosition, DeltaTime, InterpSpeed);
        Owner->SetActorLocation(NewPosition);

        // 设置旋转
        if (!Direction.IsNearlyZero())
        {
            FRotator TargetRotation = Direction.Rotation();
            FRotator CurrentRotation = Owner->GetActorRotation();
            FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, InterpSpeed * 2.0f);
            Owner->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
        }
    }
}

/**
 * @brief 计算目标位置
 */
FVector UXBSoldierFollowComponent::CalculateTargetPosition() const
{
    // 优先从编队组件获取
    FVector FormationPos = GetPositionFromFormationComponent();
    if (!FormationPos.IsZero())
    {
        return FormationPos;
    }

    // 回退到手动设置的偏移
    AActor* Target = FollowTargetRef.Get();
    if (!Target)
    {
        Target = LeaderRef.Get();
    }
    
    if (!Target)
    {
        return GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
    }

    FVector TargetLocation = Target->GetActorLocation();
    FRotator TargetRotation = Target->GetActorRotation();

    // 将局部编队偏移转换为世界坐标
    FVector WorldOffset = TargetRotation.RotateVector(FormationOffset);

    return TargetLocation + WorldOffset;
}

/**
 * @brief 从编队组件获取位置
 */
FVector UXBSoldierFollowComponent::GetPositionFromFormationComponent() const
{
    if (!CachedFormationComponent.IsValid())
    {
        return FVector::ZeroVector;
    }

    if (FormationSlotIndex == INDEX_NONE)
    {
        return FVector::ZeroVector;
    }

    return CachedFormationComponent->GetSlotWorldPosition(FormationSlotIndex);
}

/**
 * @brief 应用避障偏移
 * @note 避免多个士兵扎堆
 */
FVector UXBSoldierFollowComponent::ApplyAvoidance(const FVector& DesiredDirection) const
{
    if (AvoidanceStrength <= 0.0f || AvoidanceRadius <= 0.0f)
    {
        return DesiredDirection;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return DesiredDirection;
    }

    FVector AvoidanceForce = FVector::ZeroVector;
    FVector MyLocation = Owner->GetActorLocation();

    // 获取所有附近的士兵
    TArray<AActor*> NearbyActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AXBSoldierActor::StaticClass(), NearbyActors);

    for (AActor* OtherActor : NearbyActors)
    {
        if (OtherActor == Owner)
        {
            continue;
        }

        float Distance = FVector::Dist2D(MyLocation, OtherActor->GetActorLocation());
        if (Distance < AvoidanceRadius && Distance > KINDA_SMALL_NUMBER)
        {
            // 计算远离其他士兵的力
            FVector AwayDirection = (MyLocation - OtherActor->GetActorLocation()).GetSafeNormal2D();
            float Strength = 1.0f - (Distance / AvoidanceRadius);
            AvoidanceForce += AwayDirection * Strength;
        }
    }

    // 混合期望方向和避障力
    if (!AvoidanceForce.IsNearlyZero())
    {
        FVector BlendedDirection = DesiredDirection * (1.0f - AvoidanceStrength) + 
                                   AvoidanceForce.GetSafeNormal() * AvoidanceStrength;
        return BlendedDirection.GetSafeNormal();
    }

    return DesiredDirection;
}
