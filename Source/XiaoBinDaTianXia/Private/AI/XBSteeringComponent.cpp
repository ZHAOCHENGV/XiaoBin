/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/XBSteeringComponent.cpp

/**
 * @file XBSteeringComponent.cpp
 * @brief 转向行为组件实现
 */

#include "AI/XBSteeringComponent.h"
#include "AI/XBFlowFieldSubsystem.h"
#include "Character/XBCharacterBase.h"
#include "Soldier/XBSoldierActor.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"

UXBSteeringComponent::UXBSteeringComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UXBSteeringComponent::BeginPlay()
{
    Super::BeginPlay();

    // 缓存流场子系统
    if (UWorld* World = GetWorld())
    {
        FlowFieldSubsystem = World->GetSubsystem<UXBFlowFieldSubsystem>();
    }
}

void UXBSteeringComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bIsEnabled)
    {
        return;
    }

    // 定期更新邻居
    LastNeighborQueryTime += DeltaTime;
    if (LastNeighborQueryTime >= NeighborQueryInterval)
    {
        LastNeighborQueryTime = 0.0f;
        
        if (FlowFieldSubsystem.IsValid())
        {
            const FVector Location = GetOwner()->GetActorLocation();
            const float QueryRadius = FMath::Max3(Config.SeparationRadius, Config.AlignmentRadius, Config.CohesionRadius);
            FlowFieldSubsystem->GetSpatialGrid().QueryNeighbors(Location, QueryRadius, CachedNeighbors, GetOwner());
        }
    }

    // 计算转向力
    SteeringForce = CalculateSteeringForce();

    // 应用转向力 (F = ma, a = F/m)
    const FVector Acceleration = SteeringForce / Config.Mass;
    Velocity += Acceleration * DeltaTime;
    Velocity = Truncate(Velocity, Config.MaxSpeed);

    // 更新位置
    if (!Velocity.IsNearlyZero())
    {
        AActor* Owner = GetOwner();
        FVector NewLocation = Owner->GetActorLocation() + Velocity * DeltaTime;
        Owner->SetActorLocation(NewLocation);

        // 更新朝向
        FRotator NewRotation = Velocity.Rotation();
        NewRotation.Pitch = 0.0f;
        NewRotation.Roll = 0.0f;
        Owner->SetActorRotation(FMath::RInterpTo(Owner->GetActorRotation(), NewRotation, DeltaTime, 5.0f));
    }
}

void UXBSteeringComponent::SetFollowTarget(AXBCharacterBase* NewTarget)
{
    FollowTarget = NewTarget;
}

void UXBSteeringComponent::SetAttackTarget(AActor* NewTarget)
{
    AttackTarget = NewTarget;
}

FVector UXBSteeringComponent::CalculateSteeringForce()
{
    FVector TotalForce = FVector::ZeroVector;

    // 1. 流场跟随 (最高优先级的寻路)
    TotalForce += FlowFieldFollow() * Config.FlowFieldWeight;

    // 2. 障碍物避让
    TotalForce += ObstacleAvoidance() * Config.ObstacleAvoidanceWeight;

    // 3. Boids群集行为
    TotalForce += Separation() * Config.SeparationWeight;
    TotalForce += Alignment() * Config.AlignmentWeight;
    TotalForce += Cohesion() * Config.CohesionWeight;

    // 4. 如果有攻击目标，朝向攻击目标
    if (AttackTarget.IsValid())
    {
        TotalForce += Seek(AttackTarget->GetActorLocation()) * 2.0f;
    }

    // 限制总转向力
    return Truncate(TotalForce, Config.MaxSteeringForce);
}

FVector UXBSteeringComponent::Seek(const FVector& TargetLocation) const
{
    const FVector Location = GetOwner()->GetActorLocation();
    const FVector DesiredVelocity = (TargetLocation - Location).GetSafeNormal() * Config.MaxSpeed;
    return DesiredVelocity - Velocity;
}

FVector UXBSteeringComponent::Flee(const FVector& ThreatLocation) const
{
    const FVector Location = GetOwner()->GetActorLocation();
    const FVector DesiredVelocity = (Location - ThreatLocation).GetSafeNormal() * Config.MaxSpeed;
    return DesiredVelocity - Velocity;
}

FVector UXBSteeringComponent::Arrive(const FVector& TargetLocation) const
{
    const FVector Location = GetOwner()->GetActorLocation();
    const FVector ToTarget = TargetLocation - Location;
    const float Distance = ToTarget.Size();

    if (Distance < 1.0f)
    {
        return -Velocity; // 停止
    }

    float DesiredSpeed = Config.MaxSpeed;
    if (Distance < Config.ArrivalSlowingDistance)
    {
        DesiredSpeed = Config.MaxSpeed * (Distance / Config.ArrivalSlowingDistance);
    }

    const FVector DesiredVelocity = ToTarget.GetSafeNormal() * DesiredSpeed;
    return DesiredVelocity - Velocity;
}

FVector UXBSteeringComponent::Separation()
{
    // 🔧 修改 - 重命名为 SeparationForce
    FVector SeparationForce = FVector::ZeroVector;
    int32 Count = 0;

    const FVector Location = GetOwner()->GetActorLocation();

    for (AActor* Neighbor : CachedNeighbors)
    {
        if (!Neighbor || Neighbor == GetOwner())
        {
            continue;
        }

        const FVector ToNeighbor = Location - Neighbor->GetActorLocation();
        const float Distance = ToNeighbor.Size();

        if (Distance > 0.0f && Distance < Config.SeparationRadius)
        {
            // 距离越近，排斥力越大
            FVector RepulsionForce = ToNeighbor.GetSafeNormal() / Distance;
            SeparationForce += RepulsionForce;
            ++Count;
        }
    }

    if (Count > 0)
    {
        SeparationForce /= Count;
        if (!SeparationForce.IsNearlyZero())
        {
            SeparationForce = SeparationForce.GetSafeNormal() * Config.MaxSpeed - Velocity;
        }
    }

    return SeparationForce;
}

FVector UXBSteeringComponent::Alignment()
{
    FVector AverageVelocity = FVector::ZeroVector;
    int32 Count = 0;

    const FVector Location = GetOwner()->GetActorLocation();

    for (AActor* Neighbor : CachedNeighbors)
    {
        if (!Neighbor || Neighbor == GetOwner())
        {
            continue;
        }

        const float Distance = FVector::Dist(Location, Neighbor->GetActorLocation());
        if (Distance < Config.AlignmentRadius)
        {
            // 获取邻居的速度
            if (UXBSteeringComponent* NeighborSteering = Neighbor->FindComponentByClass<UXBSteeringComponent>())
            {
                AverageVelocity += NeighborSteering->GetVelocity();
                ++Count;
            }
        }
    }

    if (Count > 0)
    {
        AverageVelocity /= Count;
        if (!AverageVelocity.IsNearlyZero())
        {
            return AverageVelocity.GetSafeNormal() * Config.MaxSpeed - Velocity;
        }
    }

    return FVector::ZeroVector;
}

FVector UXBSteeringComponent::Cohesion()
{
    FVector CenterOfMass = FVector::ZeroVector;
    int32 Count = 0;

    const FVector Location = GetOwner()->GetActorLocation();

    for (AActor* Neighbor : CachedNeighbors)
    {
        if (!Neighbor || Neighbor == GetOwner())
        {
            continue;
        }

        const float Distance = FVector::Dist(Location, Neighbor->GetActorLocation());
        if (Distance < Config.CohesionRadius)
        {
            CenterOfMass += Neighbor->GetActorLocation();
            ++Count;
        }
    }

    if (Count > 0)
    {
        CenterOfMass /= Count;
        return Seek(CenterOfMass);
    }

    return FVector::ZeroVector;
}

FVector UXBSteeringComponent::FlowFieldFollow()
{
    if (!FlowFieldSubsystem.IsValid() || !FollowTarget.IsValid())
    {
        return FVector::ZeroVector;
    }

    const FVector Location = GetOwner()->GetActorLocation();
    const FVector FlowDirection = FlowFieldSubsystem->GetFlowDirectionToLeader(FollowTarget.Get(), Location);

    if (FlowDirection.IsNearlyZero())
    {
        return FVector::ZeroVector;
    }

    const FVector DesiredVelocity = FlowDirection * Config.MaxSpeed;
    return DesiredVelocity - Velocity;
}

FVector UXBSteeringComponent::ObstacleAvoidance()
{
    // 🔧 修改 - 重命名为 AvoidanceForce
    FVector AvoidanceForce = FVector::ZeroVector;

    AActor* Owner = GetOwner();
    const FVector Location = Owner->GetActorLocation();
    const FVector Forward = Velocity.IsNearlyZero() ? Owner->GetActorForwardVector() : Velocity.GetSafeNormal();

    // 射线检测前方障碍物
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(Owner);

    const FVector TraceEnd = Location + Forward * Config.ObstacleDetectionDistance;

    if (GetWorld()->LineTraceSingleByChannel(HitResult, Location, TraceEnd, ECC_WorldStatic, QueryParams))
    {
        // 发现障碍物，计算避让方向
        const FVector HitNormal = HitResult.ImpactNormal;
        const float DistanceToObstacle = HitResult.Distance;

        // 避让力与距离成反比
        const float AvoidanceStrength = 1.0f - (DistanceToObstacle / Config.ObstacleDetectionDistance);
        
        // 选择左转还是右转
        const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
        const float DotRight = FVector::DotProduct(HitNormal, Right);
        
        FVector AvoidanceDir = (DotRight > 0) ? Right : -Right;
        AvoidanceForce = AvoidanceDir * Config.MaxSpeed * AvoidanceStrength;
    }

    return AvoidanceForce;
}

FVector UXBSteeringComponent::Truncate(const FVector& V, float MaxLength) const
{
    if (V.SizeSquared() > MaxLength * MaxLength)
    {
        return V.GetSafeNormal() * MaxLength;
    }
    return V;
}
