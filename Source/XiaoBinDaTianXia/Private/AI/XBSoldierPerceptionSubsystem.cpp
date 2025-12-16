/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/XBSoldierPerceptionSubsystem.cpp

/**
 * @file XBSoldierPerceptionSubsystem.cpp
 * @brief 士兵感知子系统实现
 * 
 * @note ✨ 新增文件
 */

#include "AI/XBSoldierPerceptionSubsystem.h"
#include "Utils/XBLogCategories.h"
#include "Utils/XBBlueprintFunctionLibrary.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Character/XBCharacterBase.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

// ==================== 生命周期 ====================

void UXBSoldierPerceptionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 🔧 修改 - 使用 FTimerManager 替代 FTSTicker
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            TickTimerHandle,
            this,
            &UXBSoldierPerceptionSubsystem::PerformTick,
            0.016f,  // 约 60 FPS
            true     // 循环
        );
    }

    UE_LOG(LogXBAI, Log, TEXT("士兵感知子系统初始化完成，网格大小: %.0f"), CellSize);
}

void UXBSoldierPerceptionSubsystem::Deinitialize()
{
    // 🔧 修改 - 清除定时器
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TickTimerHandle);
    }

    // 清理数据
    RegisteredActors.Empty();
    SpatialGrid.Empty();
    PendingQueries.Empty();
    ActorCellMap.Empty();

    UE_LOG(LogXBAI, Log, TEXT("士兵感知子系统已关闭"));

    Super::Deinitialize();
}

bool UXBSoldierPerceptionSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    // 仅在游戏世界中创建
    if (UWorld* World = Cast<UWorld>(Outer))
    {
        return World->IsGameWorld();
    }
    return false;
}


// ==================== 感知查询接口实现 ====================

/**
 * @brief 查询最近的敌人
 * @note 核心优化逻辑：
 *       1. 首先尝试从缓存获取结果
 *       2. 缓存命中则直接返回
 *       3. 缓存未命中则立即执行查询并缓存结果
 */
bool UXBSoldierPerceptionSubsystem::QueryNearestEnemy(
    AActor* Querier,
    const FVector& Location,
    float Radius,
    EXBFaction QuerierFaction,
    FXBPerceptionResult& OutResult)
{
    if (!Querier)
    {
        return false;
    }

    TotalQueries++;

    // 尝试从缓存获取
    if (TryGetCachedResult(Location, Radius, QuerierFaction, OutResult))
    {
        CacheHits++;
        // 🔧 修改 - 使用 IsValid() 全局函数检查原始指针
        return OutResult.NearestEnemy != nullptr && IsValid(OutResult.NearestEnemy);
    }

    // 缓存未命中，执行查询
    OutResult = ExecuteQuery(Location, Radius, QuerierFaction, Querier);

    // 缓存结果到空间网格
    FIntVector CellIndex = GetCellIndex(Location);
    FXBSpatialCell& Cell = GetOrCreateCell(CellIndex);
    Cell.CachedResults.Add(QuerierFaction, OutResult);

    // 🔧 修改 - 使用 IsValid() 全局函数检查原始指针
    return OutResult.NearestEnemy != nullptr && IsValid(OutResult.NearestEnemy);
}

bool UXBSoldierPerceptionSubsystem::QueryEnemiesInRadius(
    AActor* Querier,
    const FVector& Location,
    float Radius,
    EXBFaction QuerierFaction,
    FXBPerceptionResult& OutResult)
{
    // 与 QueryNearestEnemy 逻辑相同，但返回所有敌人
    return QueryNearestEnemy(Querier, Location, Radius, QuerierFaction, OutResult);
}

bool UXBSoldierPerceptionSubsystem::QueryNearestEnemyImmediate(
    AActor* Querier,
    const FVector& Location,
    float Radius,
    EXBFaction QuerierFaction,
    FXBPerceptionResult& OutResult)
{
    TotalQueries++;
    OutResult = ExecuteQuery(Location, Radius, QuerierFaction, Querier);
    // 🔧 修改 - 使用正确的指针检查方式
    return OutResult.NearestEnemy != nullptr && IsValid(OutResult.NearestEnemy);
}

// ==================== Actor 注册实现 ====================

void UXBSoldierPerceptionSubsystem::RegisterActor(AActor* Actor, EXBFaction Faction)
{
    if (!Actor || !IsValid(Actor))
    {
        return;
    }

    // 添加到阵营列表
    TArray<TWeakObjectPtr<AActor>>& FactionActors = RegisteredActors.FindOrAdd(Faction);
    
    // 检查是否已注册
    bool bAlreadyRegistered = false;
    for (const auto& WeakActor : FactionActors)
    {
        if (WeakActor.Get() == Actor)
        {
            bAlreadyRegistered = true;
            break;
        }
    }

    if (!bAlreadyRegistered)
    {
        FactionActors.Add(Actor);

        // 更新空间网格
        FIntVector CellIndex = GetCellIndex(Actor->GetActorLocation());
        ActorCellMap.Add(Actor, CellIndex);
        GetOrCreateCell(CellIndex).Actors.Add(Actor);

        UE_LOG(LogXBAI, Verbose, TEXT("感知子系统: 注册 Actor %s (阵营: %d)"), 
            *Actor->GetName(), static_cast<int32>(Faction));
    }
}

void UXBSoldierPerceptionSubsystem::UnregisterActor(AActor* Actor)
{
    if (!Actor)
    {
        return;
    }

    // 从所有阵营列表中移除
    for (auto& Pair : RegisteredActors)
    {
        Pair.Value.RemoveAll([Actor](const TWeakObjectPtr<AActor>& WeakActor)
        {
            return !WeakActor.IsValid() || WeakActor.Get() == Actor;
        });
    }

    // 从空间网格中移除
    if (FIntVector* CellIndex = ActorCellMap.Find(Actor))
    {
        if (FXBSpatialCell* Cell = SpatialGrid.Find(*CellIndex))
        {
            Cell->Actors.RemoveAll([Actor](const TWeakObjectPtr<AActor>& WeakActor)
            {
                return !WeakActor.IsValid() || WeakActor.Get() == Actor;
            });
        }
        ActorCellMap.Remove(Actor);
    }

    UE_LOG(LogXBAI, Verbose, TEXT("感知子系统: 注销 Actor %s"), *Actor->GetName());
}

void UXBSoldierPerceptionSubsystem::UpdateActorLocation(AActor* Actor)
{
    if (!Actor || !IsValid(Actor))
    {
        return;
    }

    FIntVector NewCellIndex = GetCellIndex(Actor->GetActorLocation());

    if (FIntVector* OldCellIndex = ActorCellMap.Find(Actor))
    {
        // 如果单元格没变，跳过
        if (*OldCellIndex == NewCellIndex)
        {
            return;
        }

        // 从旧单元格移除
        if (FXBSpatialCell* OldCell = SpatialGrid.Find(*OldCellIndex))
        {
            OldCell->Actors.RemoveAll([Actor](const TWeakObjectPtr<AActor>& WeakActor)
            {
                return !WeakActor.IsValid() || WeakActor.Get() == Actor;
            });
        }
    }

    // 添加到新单元格
    ActorCellMap.Add(Actor, NewCellIndex);
    GetOrCreateCell(NewCellIndex).Actors.Add(Actor);
}

// ==================== 内部方法实现 ====================

FIntVector UXBSoldierPerceptionSubsystem::GetCellIndex(const FVector& Location) const
{
    return FIntVector(
        FMath::FloorToInt(Location.X / CellSize),
        FMath::FloorToInt(Location.Y / CellSize),
        FMath::FloorToInt(Location.Z / CellSize)
    );
}

FXBSpatialCell& UXBSoldierPerceptionSubsystem::GetOrCreateCell(const FIntVector& CellIndex)
{
    return SpatialGrid.FindOrAdd(CellIndex);
}

/**
 * @brief 尝试从缓存获取结果
 * @note 检查相同单元格内是否有有效的缓存结果
 */
bool UXBSoldierPerceptionSubsystem::TryGetCachedResult(
    const FVector& Location,
    float Radius,
    EXBFaction QuerierFaction,
    FXBPerceptionResult& OutResult)
{
    FIntVector CellIndex = GetCellIndex(Location);
    
    if (FXBSpatialCell* Cell = SpatialGrid.Find(CellIndex))
    {
        if (FXBPerceptionResult* CachedResult = Cell->CachedResults.Find(QuerierFaction))
        {
            double CurrentTime = FPlatformTime::Seconds();
            if (CachedResult->IsStillValid(CurrentTime))
            {
                OutResult = *CachedResult;
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief 执行实际的感知查询
 * @note 使用已注册的 Actor 列表进行遍历，避免物理查询
 */
FXBPerceptionResult UXBSoldierPerceptionSubsystem::ExecuteQuery(
    const FVector& Location,
    float Radius,
    EXBFaction QuerierFaction,
    AActor* Querier)
{
    FXBPerceptionResult Result;
    Result.ResultTime = FPlatformTime::Seconds();
    Result.bIsValid = true;

    float RadiusSq = Radius * Radius;
    float NearestDistanceSq = MAX_FLT;

    for (const auto& FactionPair : RegisteredActors)
    {
        EXBFaction ActorFaction = FactionPair.Key;

        if (!UXBBlueprintFunctionLibrary::AreFactionsHostile(QuerierFaction, ActorFaction))
        {
            continue;
        }

        for (const auto& WeakActor : FactionPair.Value)
        {
            AActor* Actor = WeakActor.Get();
            
            if (!Actor || !IsValid(Actor) || Actor == Querier)
            {
                continue;
            }

            if (AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(Actor))
            {
                if (Soldier->GetSoldierState() == EXBSoldierState::Dead)
                {
                    continue;
                }
            }
            else if (AXBCharacterBase* Character = Cast<AXBCharacterBase>(Actor))
            {
                if (Character->IsDead())
                {
                    continue;
                }
            }

            float DistanceSq = FVector::DistSquared(Location, Actor->GetActorLocation());
            if (DistanceSq <= RadiusSq)
            {
                // 🔧 修改 - 直接添加原始指针
                Result.DetectedEnemies.Add(Actor);

                if (DistanceSq < NearestDistanceSq)
                {
                    NearestDistanceSq = DistanceSq;
                    // 🔧 修改 - 直接赋值原始指针
                    Result.NearestEnemy = Actor;
                    Result.DistanceToNearest = FMath::Sqrt(DistanceSq);
                }
            }
        }
    }

    return Result;
}

void UXBSoldierPerceptionSubsystem::ProcessQueryQueue()
{
    if (PendingQueries.Num() == 0)
    {
        return;
    }

    int32 ProcessedCount = 0;
    double CurrentTime = FPlatformTime::Seconds();

    while (PendingQueries.Num() > 0 && ProcessedCount < MaxQueriesPerFrame)
    {
        FXBPerceptionQuery Query = PendingQueries[0];
        PendingQueries.RemoveAt(0);

        // 检查查询是否过期
        if (CurrentTime - Query.QueryTime > CacheValidityDuration)
        {
            continue;
        }

        // 执行查询
        FXBPerceptionResult Result = ExecuteQuery(
            Query.Location,
            Query.Radius,
            Query.QuerierFaction,
            Query.Querier.Get()
        );

        // 缓存结果
        FIntVector CellIndex = GetCellIndex(Query.Location);
        FXBSpatialCell& Cell = GetOrCreateCell(CellIndex);
        Cell.CachedResults.Add(Query.QuerierFaction, Result);

        ProcessedCount++;
    }
}

void UXBSoldierPerceptionSubsystem::CleanupExpiredCache()
{
    double CurrentTime = FPlatformTime::Seconds();

    // 清理过期的缓存结果
    for (auto& CellPair : SpatialGrid)
    {
        FXBSpatialCell& Cell = CellPair.Value;

        // 移除过期的缓存结果
        for (auto It = Cell.CachedResults.CreateIterator(); It; ++It)
        {
            if (!It->Value.IsStillValid(CurrentTime))
            {
                It.RemoveCurrent();
            }
        }

        // 清理无效的 Actor 引用
        Cell.Actors.RemoveAll([](const TWeakObjectPtr<AActor>& WeakActor)
        {
            return !WeakActor.IsValid();
        });
    }

    // 清理所有阵营列表中的无效引用
    for (auto& FactionPair : RegisteredActors)
    {
        FactionPair.Value.RemoveAll([](const TWeakObjectPtr<AActor>& WeakActor)
        {
            return !WeakActor.IsValid();
        });
    }

    UE_LOG(LogXBAI, Verbose, TEXT("感知子系统: 清理过期缓存完成"));
}

void UXBSoldierPerceptionSubsystem::PerformTick()
{
    // 处理批量查询
    ProcessQueryQueue();

    // 定期清理过期缓存
    double CurrentTime = FPlatformTime::Seconds();
    if (CurrentTime - LastCacheCleanupTime > CacheCleanupInterval)
    {
        CleanupExpiredCache();
        LastCacheCleanupTime = CurrentTime;
    }
}

// ==================== 调试接口实现 ====================

float UXBSoldierPerceptionSubsystem::GetCacheHitRate() const
{
    if (TotalQueries == 0)
    {
        return 0.0f;
    }
    return static_cast<float>(CacheHits) / static_cast<float>(TotalQueries);
}

int32 UXBSoldierPerceptionSubsystem::GetRegisteredActorCount() const
{
    int32 Count = 0;
    for (const auto& FactionPair : RegisteredActors)
    {
        Count += FactionPair.Value.Num();
    }
    return Count;
}

void UXBSoldierPerceptionSubsystem::DrawDebugInfo(float Duration)
{
#if ENABLE_DRAW_DEBUG
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // 绘制空间网格
    for (const auto& CellPair : SpatialGrid)
    {
        FVector CellCenter = FVector(
            CellPair.Key.X * CellSize + CellSize * 0.5f,
            CellPair.Key.Y * CellSize + CellSize * 0.5f,
            CellPair.Key.Z * CellSize + CellSize * 0.5f
        );

        FColor CellColor = CellPair.Value.Actors.Num() > 0 ? FColor::Green : FColor::Red;

        DrawDebugBox(
            World,
            CellCenter,
            FVector(CellSize * 0.5f),
            CellColor,
            false,
            Duration,
            0,
            2.0f
        );

        // 显示单元格内 Actor 数量
        DrawDebugString(
            World,
            CellCenter,
            FString::Printf(TEXT("%d"), CellPair.Value.Actors.Num()),
            nullptr,
            FColor::White,
            Duration
        );
    }
#endif
}
