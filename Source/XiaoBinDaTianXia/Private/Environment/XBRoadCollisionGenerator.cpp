// Source/XiaoBinDaTianXia/Private/Environment/XBRoadCollisionGenerator.cpp

/**
 * @file XBRoadCollisionGenerator.cpp
 * @brief 道路碰撞生成器实现
 */

#include "Environment/XBRoadCollisionGenerator.h"
#include "Components/SplineComponent.h"
#include "Components/BoxComponent.h"

AXBRoadCollisionGenerator::AXBRoadCollisionGenerator()
{
    PrimaryActorTick.bCanEverTick = false;

    // 创建根组件
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    // 创建样条线组件
    SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
    SplineComponent->SetupAttachment(Root);
    SplineComponent->SetClosedLoop(false);
}

void AXBRoadCollisionGenerator::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // 编辑器中实时生成碰撞
    if (bAutoGenerateCollision)
    {
        GenerateCollision();
    }
}

#if WITH_EDITOR
void AXBRoadCollisionGenerator::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // 编辑属性时实时生成碰撞
    if (bAutoGenerateCollision)
    {
        GenerateCollision();
    }
}
#endif

void AXBRoadCollisionGenerator::GenerateCollision()
{
    // 先清除现有碰撞
    ClearCollision();

    if (!SplineComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("道路碰撞生成器: 样条线组件无效"));
        return;
    }

    const float SplineLength = SplineComponent->GetSplineLength();
    if (SplineLength <= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("道路碰撞生成器: 样条线长度为0"));
        return;
    }

    int32 GeneratedCount = 0;

    // 沿样条线采样并生成碰撞（直接在样条线上）
    for (float Distance = 0.0f; Distance <= SplineLength; Distance += CollisionSpacing)
    {
        FVector Location, Tangent, Right;
        GetSplinePointInfo(Distance, Location, Tangent, Right);

        // 直接在样条线位置生成碰撞
        FVector CollisionPos = Location;
        CollisionPos.Z += VerticalOffset + CollisionHeight * 0.5f;
        CreateCollisionBox(CollisionPos, Tangent);
        GeneratedCount++;
    }

    UE_LOG(LogTemp, Log, TEXT("道路碰撞生成器: 生成了 %d 个碰撞体"), GeneratedCount);
}

void AXBRoadCollisionGenerator::ClearCollision()
{
    for (UBoxComponent* Box : GeneratedCollisions)
    {
        if (IsValid(Box))
        {
            Box->DestroyComponent();
        }
    }
    GeneratedCollisions.Empty();
}

void AXBRoadCollisionGenerator::CreateCollisionBox(const FVector& Location, const FVector& Tangent)
{
    // 创建碰撞盒组件
    UBoxComponent* BoxComp = NewObject<UBoxComponent>(this);
    if (!BoxComp)
    {
        return;
    }

    BoxComp->SetupAttachment(RootComponent);
    BoxComp->RegisterComponent();

    // 设置碰撞盒尺寸（BoxExtent 是半尺寸）
    BoxComp->SetBoxExtent(FVector(
        CollisionLength * 0.5f,
        CollisionThickness * 0.5f,
        CollisionHeight * 0.5f
    ));

    // 设置位置
    BoxComp->SetWorldLocation(Location);

    // 设置旋转（使X轴朝向切线方向）
    FRotator Rotation = Tangent.Rotation();
    BoxComp->SetWorldRotation(Rotation);

    // 配置碰撞
    BoxComp->SetCollisionProfileName(CollisionProfileName);
    BoxComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    
    // 设置可见性（仅用于调试，运行时隐藏）
    BoxComp->SetHiddenInGame(true);
    BoxComp->ShapeColor = FColor::Green;

#if WITH_EDITOR
    // 编辑器中显示碰撞形状
    BoxComp->SetVisibility(true);
#endif

    // 添加到列表
    GeneratedCollisions.Add(BoxComp);
}

void AXBRoadCollisionGenerator::GetSplinePointInfo(float Distance, FVector& OutLocation, FVector& OutTangent, FVector& OutRight) const
{
    if (!SplineComponent)
    {
        OutLocation = FVector::ZeroVector;
        OutTangent = FVector::ForwardVector;
        OutRight = FVector::RightVector;
        return;
    }

    // 获取位置和切线
    OutLocation = SplineComponent->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
    OutTangent = SplineComponent->GetTangentAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World).GetSafeNormal();

    // 如果切线接近垂直，使用前向量作为参考
    FVector UpVector = FVector::UpVector;
    if (FMath::Abs(FVector::DotProduct(OutTangent, UpVector)) > 0.99f)
    {
        UpVector = FVector::ForwardVector;
    }

    // 计算右向量（垂直于切线和上向量）
    OutRight = FVector::CrossProduct(OutTangent, UpVector).GetSafeNormal();

    // 确保右向量有效
    if (OutRight.IsNearlyZero())
    {
        OutRight = FVector::RightVector;
    }
}
