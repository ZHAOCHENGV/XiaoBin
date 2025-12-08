#include "Soldier/Component/XBSoldierFollowComponent.h"
#include "Character/XBCharacterBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UXBSoldierFollowComponent::UXBSoldierFollowComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UXBSoldierFollowComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UXBSoldierFollowComponent::SetLeader(AXBCharacterBase* NewLeader)
{
    LeaderRef = NewLeader;
    FollowTargetRef = NewLeader;
}

void UXBSoldierFollowComponent::SetFollowTarget(AActor* NewTarget)
{
    FollowTargetRef = NewTarget;
}

void UXBSoldierFollowComponent::SetFormationOffset(const FVector& Offset)
{
    FormationOffset = Offset;
}

void UXBSoldierFollowComponent::SetFormationSlotIndex(int32 SlotIndex)
{
    FormationSlotIndex = SlotIndex;
}

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
    if (AActor* Owner = GetOwner())
    {
        if (ACharacter* OwnerCharacter = Cast<ACharacter>(Owner))
        {
            if (UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement())
            {
                MoveComp->MaxWalkSpeed = NewSpeed;
            }
        }
    }
}

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

void UXBSoldierFollowComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsFollowing)
    {
        UpdateFollowMovement(DeltaTime);
    }
}

void UXBSoldierFollowComponent::UpdateFollowMovement(float DeltaTime)
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    FVector TargetPosition = CalculateTargetPosition();
    CachedTargetPosition = TargetPosition;
    
    FVector CurrentPosition = Owner->GetActorLocation();
    float DistanceToTarget = FVector::Dist2D(CurrentPosition, TargetPosition);

    if (DistanceToTarget > MinDistanceToMove)
    {
        FVector Direction = (TargetPosition - CurrentPosition).GetSafeNormal2D();

        if (ACharacter* OwnerCharacter = Cast<ACharacter>(Owner))
        {
            // 使用移动组件
            OwnerCharacter->AddMovementInput(Direction, 1.0f);

            // 平滑旋转
            FRotator TargetRotation = Direction.Rotation();
            FRotator CurrentRotation = Owner->GetActorRotation();
            FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, InterpSpeed * 2.0f);
            Owner->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
        }
        else
        {
            // 直接设置位置（非角色）
            FVector NewPosition = FMath::VInterpTo(CurrentPosition, TargetPosition, DeltaTime, InterpSpeed);
            Owner->SetActorLocation(NewPosition);
        }
    }
}

FVector UXBSoldierFollowComponent::CalculateTargetPosition() const
{
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

    // 将局部阵型偏移转换为世界坐标
    FVector WorldOffset = TargetRotation.RotateVector(FormationOffset);

    return TargetLocation + WorldOffset;
}
