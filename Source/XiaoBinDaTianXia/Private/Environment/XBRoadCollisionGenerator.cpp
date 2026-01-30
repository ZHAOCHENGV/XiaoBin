// Source/XiaoBinDaTianXia/Private/Environment/XBRoadCollisionGenerator.cpp

/**
 * @file XBRoadCollisionGenerator.cpp
 * @brief 道路碰撞生成器实现
 */

#include "Environment/XBRoadCollisionGenerator.h"
#include "Components/SplineComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"

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

    // 创建预览网格实例组件
    PreviewMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PreviewMesh"));
    PreviewMeshComponent->SetupAttachment(Root);
    PreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PreviewMeshComponent->SetCastShadow(false);

    // 默认预览网格使用引擎内置立方体
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        PreviewMesh = CubeMesh.Object;
        PreviewMeshComponent->SetStaticMesh(PreviewMesh);
    }
}

void AXBRoadCollisionGenerator::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // 编辑器中实时生成碰撞
    if (bAutoGenerateCollision)
    {
        GenerateCollision();
    }
    
    // 更新预览
    if (bShowPreview)
    {
        UpdatePreview();
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
    
    // 更新预览
    if (bShowPreview)
    {
        UpdatePreview();
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

    // 沿样条线采样并生成碰撞
    for (float Distance = 0.0f; Distance <= SplineLength; Distance += CollisionSpacing)
    {
        FVector Location, Tangent, Right;
        GetSplinePointInfo(Distance, Location, Tangent, Right);

        // 生成左侧碰撞
        if (bGenerateLeftSide)
        {
            FVector LeftPos = Location - Right * RoadHalfWidth;
            LeftPos.Z += VerticalOffset + CollisionHeight * 0.5f;
            CreateCollisionBox(LeftPos, Tangent, true);
            GeneratedCount++;
        }

        // 生成右侧碰撞
        if (bGenerateRightSide)
        {
            FVector RightPos = Location + Right * RoadHalfWidth;
            RightPos.Z += VerticalOffset + CollisionHeight * 0.5f;
            CreateCollisionBox(RightPos, Tangent, false);
            GeneratedCount++;
        }
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

    UE_LOG(LogTemp, Log, TEXT("道路碰撞生成器: 已清除所有碰撞体"));
}

void AXBRoadCollisionGenerator::UpdatePreview()
{
    if (!PreviewMeshComponent || !SplineComponent)
    {
        return;
    }

    // 清除现有预览实例
    PreviewMeshComponent->ClearInstances();

    if (!bShowPreview)
    {
        return;
    }

    // 设置预览网格
    if (PreviewMesh && PreviewMeshComponent->GetStaticMesh() != PreviewMesh)
    {
        PreviewMeshComponent->SetStaticMesh(PreviewMesh);
    }

    const float SplineLength = SplineComponent->GetSplineLength();
    if (SplineLength <= 0.0f)
    {
        return;
    }

    // 计算预览缩放（基于碰撞体尺寸，假设原始网格为100单位立方体）
    const FVector PreviewScale(
        CollisionLength / 100.0f,
        CollisionThickness / 100.0f,
        CollisionHeight / 100.0f
    );

    // 沿样条线采样并添加预览实例
    for (float Distance = 0.0f; Distance <= SplineLength; Distance += CollisionSpacing)
    {
        FVector Location, Tangent, Right;
        GetSplinePointInfo(Distance, Location, Tangent, Right);

        // 计算旋转（使X轴朝向切线方向）
        FRotator Rotation = Tangent.Rotation();

        // 左侧预览
        if (bGenerateLeftSide)
        {
            FVector LeftPos = Location - Right * RoadHalfWidth;
            LeftPos.Z += VerticalOffset + CollisionHeight * 0.5f;
            
            FTransform InstanceTransform;
            InstanceTransform.SetLocation(LeftPos);
            InstanceTransform.SetRotation(Rotation.Quaternion());
            InstanceTransform.SetScale3D(PreviewScale);
            
            PreviewMeshComponent->AddInstance(InstanceTransform);
        }

        // 右侧预览
        if (bGenerateRightSide)
        {
            FVector RightPos = Location + Right * RoadHalfWidth;
            RightPos.Z += VerticalOffset + CollisionHeight * 0.5f;
            
            FTransform InstanceTransform;
            InstanceTransform.SetLocation(RightPos);
            InstanceTransform.SetRotation(Rotation.Quaternion());
            InstanceTransform.SetScale3D(PreviewScale);
            
            PreviewMeshComponent->AddInstance(InstanceTransform);
        }
    }
}

void AXBRoadCollisionGenerator::CreateCollisionBox(const FVector& Location, const FVector& Tangent, bool bIsLeftSide)
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
    BoxComp->ShapeColor = bIsLeftSide ? FColor::Blue : FColor::Red;

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
