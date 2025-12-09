/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/XBFlowFieldInitializer.cpp

/**
 * @file XBFlowFieldInitializer.cpp
 * @brief 流场初始化器实现
 * 
 * @details 实现流场的初始化、障碍物扫描、调试绘制等功能
 */

#include "AI/XBFlowFieldInitializer.h"
#include "AI/XBFlowFieldSubsystem.h"
#include "AI/XBFlowField.h"
#include "Character/XBCharacterBase.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "EngineUtils.h"
#include "CollisionQueryParams.h"

/**
 * @brief 构造函数
 * @details 初始化Tick和默认配置
 */
AXBFlowFieldInitializer::AXBFlowFieldInitializer()
{
    // 启用Tick
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    
    // 设置默认障碍物对象类型
    Config.ObstacleConfig.ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
    Config.ObstacleConfig.ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
}

/**
 * @brief BeginPlay
 * @details 流程：
 *          1. 获取流场子系统
 *          2. 自动初始化流场（如果启用）
 *          3. 自动扫描障碍物（如果启用）
 */
void AXBFlowFieldInitializer::BeginPlay()
{
    // 调用父类
    Super::BeginPlay();

    // 获取流场子系统
    FlowFieldSubsystem = GetWorld()->GetSubsystem<UXBFlowFieldSubsystem>();

    // 自动初始化
    if (bAutoInitialize)
    {
        InitializeFlowField();
    }

    // 自动扫描障碍物
    if (bAutoScanObstacles)
    {
        ScanObstacles();
    }
}

/**
 * @brief EndPlay
 * @details 清理缓存数据
 * @param EndPlayReason 结束原因
 */
void AXBFlowFieldInitializer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 清空障碍物缓存
    RuntimeObstacleCache.Empty();
    EditorObstacleCache.Empty();
    
    // 调用父类
    Super::EndPlay(EndPlayReason);
}

/**
 * @brief Tick函数
 * @details 流程：
 *          1. 编辑器模式下绘制预览
 *          2. 运行时定时扫描障碍物
 *          3. 运行时定时更新流场
 *          4. 运行时定时绘制调试信息
 * @param DeltaTime 帧间隔时间
 */
void AXBFlowFieldInitializer::Tick(float DeltaTime)
{
    // 调用父类
    Super::Tick(DeltaTime);

#if WITH_EDITOR
    // 编辑器模式（非游戏运行）
    if (!GetWorld()->IsGameWorld())
    {
        // 绘制编辑器预览
        if (Config.DebugConfig.bShowEditorPreview)
        {
            DrawEditorPreview();
        }
        return;
    }
#endif

    // ========== 运行时逻辑 ==========

    // 障碍物扫描计时
    ObstacleScanTimer += DeltaTime;
    if (ObstacleScanTimer >= Config.ObstacleScanInterval)
    {
        // 重置计时器
        ObstacleScanTimer = 0.0f;
        // 执行障碍物扫描
        PerformObstacleScan();
    }

    // 流场更新计时
    FlowFieldUpdateTimer += DeltaTime;
    if (FlowFieldUpdateTimer >= Config.UpdateInterval)
    {
        // 重置计时器
        FlowFieldUpdateTimer = 0.0f;
        // 刷新流场
        RefreshFlowField();
    }

    // 调试绘制计时
    if (Config.DebugConfig.bEnableDebugDraw)
    {
        DebugDrawTimer += DeltaTime;
        if (DebugDrawTimer >= Config.DebugConfig.DebugDrawInterval)
        {
            // 重置计时器
            DebugDrawTimer = 0.0f;
            // 绘制调试信息
            DrawDebugInfo();
        }
    }
}

#if WITH_EDITOR
/**
 * @brief 属性变更回调
 * @details 编辑器中修改属性时触发障碍物预览扫描
 * @param PropertyChangedEvent 属性变更事件
 */
void AXBFlowFieldInitializer::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    // 调用父类
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // 如果启用编辑器障碍物预览，重新扫描
    if (Config.DebugConfig.bEditorPreviewObstacles)
    {
        ScanObstaclesForEditorPreview();
    }
}
#endif

/**
 * @brief 初始化流场
 * @details 流程：
 *          1. 获取或验证流场子系统
 *          2. 初始化流场子系统参数
 *          3. 查找并注册主将
 */
void AXBFlowFieldInitializer::InitializeFlowField()
{
    // 获取流场子系统
    if (!FlowFieldSubsystem)
    {
        FlowFieldSubsystem = GetWorld()->GetSubsystem<UXBFlowFieldSubsystem>();
    }

    // 验证子系统
    if (!FlowFieldSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("[流场初始化器] 无法获取 FlowFieldSubsystem"));
        return;
    }

    // 🔧 修改 - 初始化流场子系统，传入配置参数
    // 计算流场原点（以Actor位置为中心）
    FVector2D FlowFieldOrigin = FVector2D(GetActorLocation().X, GetActorLocation().Y) - Config.FlowFieldSize * 0.5f;
    FlowFieldSubsystem->InitializeFlowField(FlowFieldOrigin, Config.FlowFieldSize, Config.CellSize);

    // 查找主将
    CurrentLeader = FindLeader();

    // 注册主将
    if (CurrentLeader.IsValid())
    {
        FlowFieldSubsystem->RegisterLeader(CurrentLeader.Get());
        UE_LOG(LogTemp, Log, TEXT("[流场初始化器] 已注册主将: %s"), *CurrentLeader->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[流场初始化器] 未找到主将"));
    }

    // 标记已初始化
    bIsInitialized = true;
    UE_LOG(LogTemp, Log, TEXT("[流场初始化器] 流场初始化完成 - 大小: (%.1f, %.1f), 格子: %.1f"), 
        Config.FlowFieldSize.X, Config.FlowFieldSize.Y, Config.CellSize);
}

/**
 * @brief 扫描障碍物（公开接口）
 * @details 调用内部扫描函数
 */
void AXBFlowFieldInitializer::ScanObstacles()
{
    PerformObstacleScan();
}

/**
 * @brief 执行障碍物扫描
 * @details 流程：
 *          1. 清空障碍物缓存
 *          2. 遍历流场每个格子
 *          3. 获取地形高度
 *          4. 检测障碍物
 *          5. 扩展障碍物标记
 *          6. 更新流场障碍物数据
 */
void AXBFlowFieldInitializer::PerformObstacleScan()
{
    // 清空运行时障碍物缓存
    RuntimeObstacleCache.Empty();

    // 获取流场参数
    const FVector Origin = GetActorLocation();
    const float CellSize = Config.CellSize;
    const int32 GridWidth = FMath::CeilToInt(Config.FlowFieldSize.X / CellSize);
    const int32 GridHeight = FMath::CeilToInt(Config.FlowFieldSize.Y / CellSize);
    const FVector2D HalfSize = Config.FlowFieldSize * 0.5f;

    // 收集需要忽略的Actor
    TArray<AActor*> IgnoredActors;
    // 忽略自身
    IgnoredActors.Add(this);
    
    // 添加忽略的Actor类实例
    for (const TSubclassOf<AActor>& IgnoredClass : Config.ObstacleConfig.IgnoredActorClasses)
    {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), IgnoredClass, FoundActors);
        IgnoredActors.Append(FoundActors);
    }

    // 添加带有忽略标签的Actor
    for (const FName& IgnoreTag : Config.ObstacleConfig.IgnoredActorTags)
    {
        TArray<AActor*> TaggedActors;
        UGameplayStatics::GetAllActorsWithTag(GetWorld(), IgnoreTag, TaggedActors);
        IgnoredActors.Append(TaggedActors);
    }

    // 遍历每个格子进行检测
    for (int32 Y = 0; Y < GridHeight; ++Y)
    {
        for (int32 X = 0; X < GridWidth; ++X)
        {
            // 计算格子中心世界坐标
            FVector CellWorldPos = Origin + FVector(
                (X * CellSize) - HalfSize.X + (CellSize * 0.5f),
                (Y * CellSize) - HalfSize.Y + (CellSize * 0.5f),
                0.0f
            );

            // 获取地形高度
            CellWorldPos.Z = GetTerrainHeight(CellWorldPos);

            // 检测障碍物
            if (CheckObstacleAt(CellWorldPos))
            {
                // 添加到障碍物缓存
                RuntimeObstacleCache.Add(FIntPoint(X, Y));
            }
        }
    }

    // 扩展障碍物标记
    ExpandObstacleMarking();

    // 更新流场的障碍物数据
    if (FlowFieldSubsystem)
    {
        // 先清除所有障碍物
        FlowFieldSubsystem->ClearBlockedCells();
        
        // 将障碍物信息传递给流场
        for (const FIntPoint& ObstacleCell : RuntimeObstacleCache)
        {
            FlowFieldSubsystem->MarkCellAsBlocked(ObstacleCell.X, ObstacleCell.Y);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[流场初始化器] 障碍物扫描完成，检测到 %d 个障碍物格子"), RuntimeObstacleCache.Num());
}

/**
 * @brief 获取地形高度
 * @details 使用配置的地形碰撞通道进行射线检测
 * @param WorldPosition 世界坐标（XY）
 * @return 地形Z坐标，未检测到返回原始Z
 */
float AXBFlowFieldInitializer::GetTerrainHeight(const FVector& WorldPosition) const
{
    // 射线起点（向上偏移）
    FVector TraceStart = WorldPosition + FVector(0, 0, 500.0f);
    // 射线终点（向下）
    FVector TraceEnd = WorldPosition - FVector(0, 0, 500.0f);
    
    // 执行射线检测
    FHitResult HitResult;
    // 🔧 修改 - 使用配置的地形碰撞通道
    if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, Config.ObstacleConfig.TerrainChannel))
    {
        return HitResult.Location.Z;
    }
    
    // 未检测到地形，返回原始Z
    return WorldPosition.Z;
}

/**
 * @brief 检查指定位置是否有障碍物
 * @details 流程：
 *          1. 设置检测位置（加高度偏移）
 *          2. 根据配置使用盒体或胶囊体检测
 *          3. 过滤检测结果（忽略标签、最小尺寸）
 * @param WorldPosition 世界坐标
 * @return 是否有障碍物
 */
bool AXBFlowFieldInitializer::CheckObstacleAt(const FVector& WorldPosition) const
{
    // 获取障碍物配置引用
    const FXBObstacleDetectionConfig& ObsConfig = Config.ObstacleConfig;
    
    // 🔧 修改 - 检查是否配置了对象类型
    if (ObsConfig.ObjectTypes.Num() == 0)
    {
        // 未配置对象类型，跳过检测
        return false;
    }
    
    // 计算检测位置（加上高度偏移）
    FVector CheckPosition = WorldPosition + FVector(0, 0, ObsConfig.DetectionHeightOffset);
    // 计算检测半径
    float DetectionRadius = Config.CellSize * ObsConfig.DetectionRadiusScale;

    // 设置碰撞查询参数
    TArray<FOverlapResult> OverlapResults;
    FCollisionQueryParams QueryParams;
    QueryParams.bTraceComplex = false;
    QueryParams.bReturnPhysicalMaterial = false;
    // 忽略自身
    QueryParams.AddIgnoredActor(const_cast<AXBFlowFieldInitializer*>(this));

    // 添加忽略的Actor类实例
    for (const TSubclassOf<AActor>& IgnoredClass : ObsConfig.IgnoredActorClasses)
    {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), IgnoredClass, FoundActors);
        QueryParams.AddIgnoredActors(FoundActors);
    }

    // 🔧 修改 - 使用配置的 ObjectTypes 构建查询参数
    FCollisionObjectQueryParams ObjectQueryParams;
    for (const TEnumAsByte<EObjectTypeQuery>& ObjectType : ObsConfig.ObjectTypes)
    {
        ObjectQueryParams.AddObjectTypesToQuery(UEngineTypes::ConvertToCollisionChannel(ObjectType));
    }

    // 执行重叠检测
    bool bHasObstacle = false;
    // 检测中心位置（加上高度的一半）
    FVector OverlapCenter = CheckPosition + FVector(0, 0, ObsConfig.DetectionHeight * 0.5f);

    if (ObsConfig.bUseBoxDetection)
    {
        // 使用盒体检测 - 更精确地覆盖格子区域
        FVector BoxExtent(DetectionRadius, DetectionRadius, ObsConfig.DetectionHeight * 0.5f);
        bHasObstacle = GetWorld()->OverlapMultiByObjectType(
            OverlapResults,
            OverlapCenter,
            FQuat::Identity,
            ObjectQueryParams,
            FCollisionShape::MakeBox(BoxExtent),
            QueryParams
        );
    }
    else
    {
        // 使用胶囊体检测
        bHasObstacle = GetWorld()->OverlapMultiByObjectType(
            OverlapResults,
            OverlapCenter,
            FQuat::Identity,
            ObjectQueryParams,
            FCollisionShape::MakeCapsule(DetectionRadius, ObsConfig.DetectionHeight * 0.5f),
            QueryParams
        );
    }

    // 过滤检测结果
    if (bHasObstacle)
    {
        for (const FOverlapResult& Result : OverlapResults)
        {
            // 检查Actor有效性
            if (!Result.GetActor())
            {
                continue;
            }

            // 检查忽略标签
            bool bShouldIgnore = false;
            for (const FName& IgnoreTag : ObsConfig.IgnoredActorTags)
            {
                if (Result.GetActor()->ActorHasTag(IgnoreTag))
                {
                    bShouldIgnore = true;
                    break;
                }
            }

            if (bShouldIgnore)
            {
                continue;
            }

            // 检查最小尺寸
            if (Result.GetComponent())
            {
                FVector BoundsExtent = Result.GetComponent()->Bounds.BoxExtent;
                float MaxExtent = FMath::Max3(BoundsExtent.X, BoundsExtent.Y, BoundsExtent.Z);
                if (MaxExtent < ObsConfig.MinObstacleSize)
                {
                    continue;
                }
            }

            // 找到有效障碍物
            return true;
        }
    }

    // 未找到有效障碍物
    return false;
}

/**
 * @brief 扩展障碍物标记
 * @details 将障碍物标记扩展到周围格子，用于更好地包裹障碍物
 */
void AXBFlowFieldInitializer::ExpandObstacleMarking()
{
    // 检查扩展格数配置
    if (Config.ObstacleConfig.ExpansionCells <= 0)
    {
        return;
    }

    // 创建扩展后的障碍物集合
    TSet<FIntPoint> ExpandedObstacles;
    const int32 Expansion = Config.ObstacleConfig.ExpansionCells;
    const int32 GridWidth = FMath::CeilToInt(Config.FlowFieldSize.X / Config.CellSize);
    const int32 GridHeight = FMath::CeilToInt(Config.FlowFieldSize.Y / Config.CellSize);

    // 遍历当前障碍物
    for (const FIntPoint& ObstacleCell : RuntimeObstacleCache)
    {
        // 扩展到周围格子
        for (int32 DY = -Expansion; DY <= Expansion; ++DY)
        {
            for (int32 DX = -Expansion; DX <= Expansion; ++DX)
            {
                // 计算新格子坐标
                int32 NewX = ObstacleCell.X + DX;
                int32 NewY = ObstacleCell.Y + DY;

                // 检查是否在有效范围内
                if (NewX >= 0 && NewX < GridWidth && NewY >= 0 && NewY < GridHeight)
                {
                    ExpandedObstacles.Add(FIntPoint(NewX, NewY));
                }
            }
        }
    }

    // 替换为扩展后的障碍物集合
    RuntimeObstacleCache = MoveTemp(ExpandedObstacles);
}

/**
 * @brief 刷新流场
 * @details 更新流场子系统
 */
void AXBFlowFieldInitializer::RefreshFlowField()
{
    // 验证子系统和主将
    if (!FlowFieldSubsystem || !CurrentLeader.IsValid())
    {
        return;
    }

    // 更新流场
    FlowFieldSubsystem->UpdateFlowFields();
}

/**
 * @brief 切换调试绘制
 * @param bEnable 是否启用
 */
void AXBFlowFieldInitializer::ToggleDebugDraw(bool bEnable)
{
    Config.DebugConfig.bEnableDebugDraw = bEnable;
}

/**
 * @brief 查找主将
 * @details 根据配置的查找模式查找主将
 * @return 找到的主将，未找到返回nullptr
 */
AXBCharacterBase* AXBFlowFieldInitializer::FindLeader()
{
    switch (LeaderFindMode)
    {
    case EXBLeaderFindMode::ByTag:
        {
            // 通过标签查找
            for (TActorIterator<AXBCharacterBase> It(GetWorld()); It; ++It)
            {
                AXBCharacterBase* Character = *It;
                // 检查是否有能力系统组件
                if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
                {
                    // 检查是否匹配标签
                    if (ASC->HasMatchingGameplayTag(LeaderTag))
                    {
                        return Character;
                    }
                }
            }
        }
        break;

    case EXBLeaderFindMode::ByClass:
        {
            // 通过类查找
            AActor* FoundActor = UGameplayStatics::GetActorOfClass(GetWorld(), LeaderClass);
            return Cast<AXBCharacterBase>(FoundActor);
        }

    case EXBLeaderFindMode::Manual:
        // 手动指定
        return ManualLeader.Get();
    }

    return nullptr;
}

/**
 * @brief 设置主将
 * @param NewLeader 新的主将
 */
void AXBFlowFieldInitializer::SetLeader(AXBCharacterBase* NewLeader)
{
    // 更新主将引用
    CurrentLeader = NewLeader;
    
    // 注册到流场子系统
    if (FlowFieldSubsystem && NewLeader)
    {
        FlowFieldSubsystem->RegisterLeader(NewLeader);
    }
}

/**
 * @brief 绘制调试信息（运行时）
 * @details 流程：
 *          1. 绘制边界框
 *          2. 遍历格子绘制障碍物和流向箭头
 *          3. 绘制目标位置（主将）
 */
void AXBFlowFieldInitializer::DrawDebugInfo()
{
    // 检查是否启用调试绘制
    if (!Config.DebugConfig.bEnableDebugDraw)
    {
        return;
    }

    // 获取世界
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // 获取流场参数
    const FVector Origin = GetActorLocation();
    const float CellSize = Config.CellSize;
    const int32 GridWidth = FMath::CeilToInt(Config.FlowFieldSize.X / CellSize);
    const int32 GridHeight = FMath::CeilToInt(Config.FlowFieldSize.Y / CellSize);
    const FVector2D HalfSize = Config.FlowFieldSize * 0.5f;
    // 绘制持续时间（略大于间隔，避免闪烁）
    const float DrawDuration = Config.DebugConfig.DebugDrawInterval * 1.5f;
    // 获取调试配置引用
    const FXBFlowFieldDebugConfig& DebugCfg = Config.DebugConfig;

    // 绘制边界框
    if (DebugCfg.bDrawBounds)
    {
        DrawDebugBox(World, Origin + FVector(0, 0, 100), FVector(HalfSize.X, HalfSize.Y, 100), 
            DebugCfg.BoundsColor, false, DrawDuration, 0, DebugCfg.BoundsLineThickness);
    }

    // 🔧 修改 - 获取流场数据用于绘制流向箭头
    const FXBFlowField* FlowFieldData = nullptr;
    if (FlowFieldSubsystem && CurrentLeader.IsValid())
    {
        FlowFieldData = FlowFieldSubsystem->GetFlowFieldForLeader(CurrentLeader.Get());
    }

    // 遍历格子
    for (int32 Y = 0; Y < GridHeight; ++Y)
    {
        for (int32 X = 0; X < GridWidth; ++X)
        {
            // 计算格子中心
            FVector CellCenter = Origin + FVector(
                (X * CellSize) - HalfSize.X + (CellSize * 0.5f),
                (Y * CellSize) - HalfSize.Y + (CellSize * 0.5f),
                1.0f
            );

            // 获取地形高度
            CellCenter.Z = GetTerrainHeight(CellCenter) + 5.0f;

            // 检查是否为障碍物
            FIntPoint CellCoord(X, Y);
            bool bIsObstacle = RuntimeObstacleCache.Contains(CellCoord);

            // 绘制障碍物 - 红色填充区域
            if (bIsObstacle && DebugCfg.bDrawObstacles)
            {
                // 绘制填充的红色方块
                FVector BoxExtent(CellSize * 0.48f, CellSize * 0.48f, 50.0f);
                DrawDebugBox(World, CellCenter + FVector(0, 0, 50), BoxExtent, 
                    FColor(DebugCfg.ObstacleColor.R, DebugCfg.ObstacleColor.G, DebugCfg.ObstacleColor.B, DebugCfg.ObstacleFillAlpha), 
                    false, DrawDuration, 0, DebugCfg.ObstacleLineThickness);
                
                // 绘制X标记
                FVector TopLeft = CellCenter + FVector(-CellSize * 0.4f, -CellSize * 0.4f, 100);
                FVector TopRight = CellCenter + FVector(CellSize * 0.4f, -CellSize * 0.4f, 100);
                FVector BottomLeft = CellCenter + FVector(-CellSize * 0.4f, CellSize * 0.4f, 100);
                FVector BottomRight = CellCenter + FVector(CellSize * 0.4f, CellSize * 0.4f, 100);
                DrawDebugLine(World, TopLeft, BottomRight, FColor::Red, false, DrawDuration, 0, DebugCfg.ObstacleLineThickness);
                DrawDebugLine(World, TopRight, BottomLeft, FColor::Red, false, DrawDuration, 0, DebugCfg.ObstacleLineThickness);
            }

            // 绘制格子边框（非障碍物格子）
            if (DebugCfg.bDrawCells && !bIsObstacle)
            {
                DrawDebugBox(World, CellCenter, FVector(CellSize * 0.49f, CellSize * 0.49f, 1), 
                    FColor::White, false, DrawDuration, 0, DebugCfg.CellLineThickness);
            }

            // 🔧 修改 - 绘制流向箭头（仅非障碍物格子）
            if (DebugCfg.bDrawFlowDirections && !bIsObstacle)
            {
                // 从流场获取流向
                FVector FlowDirection = FVector::ZeroVector;
                
                if (FlowFieldData && FlowFieldData->IsValid())
                {
                    // 使用流场的平滑方向
                    FlowDirection = FlowFieldData->GetFlowDirectionSmooth(CellCenter);
                }
                else if (CurrentLeader.IsValid())
                {
                    // 🔧 修改 - 如果没有流场数据，直接朝向主将
                    FVector ToLeader = CurrentLeader->GetActorLocation() - CellCenter;
                    ToLeader.Z = 0;
                    FlowDirection = ToLeader.GetSafeNormal();
                }
                
                // 绘制箭头（流向有效时）
                if (!FlowDirection.IsNearlyZero())
                {
                    FVector ArrowStart = CellCenter + FVector(0, 0, 10);
                    FVector ArrowEnd = ArrowStart + FlowDirection * CellSize * DebugCfg.ArrowLengthScale;
                    
                    DrawDebugDirectionalArrow(World, ArrowStart, ArrowEnd, DebugCfg.ArrowHeadSize, 
                        DebugCfg.FlowArrowColor, false, DrawDuration, 0, DebugCfg.ArrowLineThickness);
                }
            }
        }
    }

    // 绘制目标位置（主将）
    if (CurrentLeader.IsValid())
    {
        FVector LeaderPos = CurrentLeader->GetActorLocation();
        DrawDebugSphere(World, LeaderPos + FVector(0, 0, 100), 50.0f, 16, FColor::Yellow, false, DrawDuration, 0, 3.0f);
    }
}

/**
 * @brief 扫描障碍物（编辑器预览用）
 * @details 与运行时扫描类似，但结果存储在编辑器缓存中
 */
void AXBFlowFieldInitializer::ScanObstaclesForEditorPreview()
{
    // 清空编辑器障碍物缓存
    EditorObstacleCache.Empty();

    // 获取流场参数
    const FVector Origin = GetActorLocation();
    const float CellSize = Config.CellSize;
    const int32 GridWidth = FMath::CeilToInt(Config.FlowFieldSize.X / CellSize);
    const int32 GridHeight = FMath::CeilToInt(Config.FlowFieldSize.Y / CellSize);
    const FVector2D HalfSize = Config.FlowFieldSize * 0.5f;

    // 遍历每个格子
    for (int32 Y = 0; Y < GridHeight; ++Y)
    {
        for (int32 X = 0; X < GridWidth; ++X)
        {
            // 计算格子中心世界坐标
            FVector CellWorldPos = Origin + FVector(
                (X * CellSize) - HalfSize.X + (CellSize * 0.5f),
                (Y * CellSize) - HalfSize.Y + (CellSize * 0.5f),
                0.0f
            );

            // 获取地形高度
            CellWorldPos.Z = GetTerrainHeight(CellWorldPos);

            // 检测障碍物
            if (CheckObstacleAt(CellWorldPos))
            {
                EditorObstacleCache.Add(FIntPoint(X, Y));
            }
        }
    }

    // 扩展障碍物（如果配置了）
    if (Config.ObstacleConfig.ExpansionCells > 0)
    {
        TSet<FIntPoint> ExpandedObstacles;
        const int32 Expansion = Config.ObstacleConfig.ExpansionCells;

        for (const FIntPoint& ObstacleCell : EditorObstacleCache)
        {
            for (int32 DY = -Expansion; DY <= Expansion; ++DY)
            {
                for (int32 DX = -Expansion; DX <= Expansion; ++DX)
                {
                    int32 NewX = ObstacleCell.X + DX;
                    int32 NewY = ObstacleCell.Y + DY;

                    const int32 GridWidthLocal = FMath::CeilToInt(Config.FlowFieldSize.X / Config.CellSize);
                    const int32 GridHeightLocal = FMath::CeilToInt(Config.FlowFieldSize.Y / Config.CellSize);

                    if (NewX >= 0 && NewX < GridWidthLocal && NewY >= 0 && NewY < GridHeightLocal)
                    {
                        ExpandedObstacles.Add(FIntPoint(NewX, NewY));
                    }
                }
            }
        }
        EditorObstacleCache = MoveTemp(ExpandedObstacles);
    }

    UE_LOG(LogTemp, Log, TEXT("[流场初始化器] 编辑器预览扫描完成，检测到 %d 个障碍物格子"), EditorObstacleCache.Num());
}

#if WITH_EDITOR
/**
 * @brief 绘制编辑器预览
 * @details 在编辑器中（非运行时）绘制流场边界和障碍物
 */
void AXBFlowFieldInitializer::DrawEditorPreview()
{
    // 获取世界
    UWorld* World = GetWorld();
    if (!World || !Config.DebugConfig.bShowEditorPreview)
    {
        return;
    }

    // 获取流场参数
    const FVector Origin = GetActorLocation();
    const float CellSize = Config.CellSize;
    const FVector2D HalfSize = Config.FlowFieldSize * 0.5f;
    const FXBFlowFieldDebugConfig& DebugCfg = Config.DebugConfig;

    // 绘制边界（持续绘制，Duration = -1）
    if (DebugCfg.bDrawBounds)
    {
        DrawDebugBox(World, Origin + FVector(0, 0, 100), FVector(HalfSize.X, HalfSize.Y, 100), 
            DebugCfg.BoundsColor, false, -1, 0, DebugCfg.BoundsLineThickness);
    }

    // 绘制障碍物（编辑器缓存）
    if (DebugCfg.bEditorPreviewObstacles)
    {
        for (const FIntPoint& ObstacleCell : EditorObstacleCache)
        {
            // 计算格子中心
            FVector CellCenter = Origin + FVector(
                (ObstacleCell.X * CellSize) - HalfSize.X + (CellSize * 0.5f),
                (ObstacleCell.Y * CellSize) - HalfSize.Y + (CellSize * 0.5f),
                50.0f
            );

            // 绘制红色填充方块
            FVector BoxExtent(CellSize * 0.48f, CellSize * 0.48f, 50.0f);
            DrawDebugBox(World, CellCenter, BoxExtent, 
                FColor(DebugCfg.ObstacleColor.R, DebugCfg.ObstacleColor.G, DebugCfg.ObstacleColor.B, DebugCfg.ObstacleFillAlpha), 
                false, -1, 0, DebugCfg.ObstacleLineThickness);
        }
    }
}
#endif
