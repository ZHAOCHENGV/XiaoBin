/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/XBSoldierPerceptionSubsystem.cpp

/**
 * @file XBSoldierPerceptionSubsystem.cpp
 * @brief 士兵感知子系统实现 - R-4 性能优化版本
 * 
 * @note 🔧 修改记录:
 *       1. 实现多级缓存系统 (L1/L2/L3)
 *       2. 实现查询合并机制
 *       3. 实现优先级队列处理
 *       4. 实现热点区域优化
 *       5. 添加详细性能统计
 */

#include "AI/XBSoldierPerceptionSubsystem.h"
#include "Utils/XBLogCategories.h"
#include "Utils/XBBlueprintFunctionLibrary.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Character/XBCharacterBase.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformTime.h"

// ==================== 生命周期 ====================

void UXBSoldierPerceptionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            TickTimerHandle,
            this,
            &UXBSoldierPerceptionSubsystem::PerformTick,
            0.016f,
            true
        );
    }

    UE_LOG(LogXBAI, Log, TEXT("士兵感知子系统初始化完成 - 网格大小: %.0f, L1缓存: %.0fms, L2缓存: %.0fms, L3缓存: %.0fms"), 
        CellSize, L1CacheValidityMs, L2CacheValidityMs, L3CacheValidityMs);
}

void UXBSoldierPerceptionSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TickTimerHandle);
    }

    // 打印最终性能报告
    PrintPerformanceReport();

    RegisteredActors.Empty();
    SpatialGrid.Empty();
    PendingQueries.Empty();
    ActorCellMap.Empty();
    HighPriorityQueue.Empty();
    NormalPriorityQueue.Empty();
    LowPriorityQueue.Empty();
    RegionCache.Empty();
    HotspotRegions.Empty();
    HotspotCache.Empty();
    PendingMergedQueries.Empty();

    UE_LOG(LogXBAI, Log, TEXT("士兵感知子系统已关闭"));

    Super::Deinitialize();
}

bool UXBSoldierPerceptionSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    if (UWorld* World = Cast<UWorld>(Outer))
    {
        return World->IsGameWorld();
    }
    return false;
}

void UXBSoldierPerceptionSubsystem::PerformTick()
{
    double CurrentTime = FPlatformTime::Seconds();

    // 处理优先级队列
    ProcessPriorityQueues();

    // 处理批量查询队列（兼容旧接口）
    ProcessQueryQueue();

    // 更新热点区域缓存
    if (CurrentTime - LastHotspotUpdateTime > HotspotUpdateInterval)
    {
        UpdateHotspotCaches();
        LastHotspotUpdateTime = CurrentTime;
    }

    // 定期清理过期缓存
    if (CurrentTime - LastCacheCleanupTime > CacheCleanupInterval)
    {
        CleanupExpiredCache();
        LastCacheCleanupTime = CurrentTime;
    }
}

// ==================== 增强查询接口实现 ====================

/**
 * @brief 带优先级的查询接口
 * @note ✨ 新增 - R-4 核心优化入口
 */
bool UXBSoldierPerceptionSubsystem::QueryNearestEnemyWithPriority(
    AActor* Querier,
    const FVector& Location,
    float Radius,
    EXBFaction QuerierFaction,
    EXBQueryPriority Priority,
    FXBPerceptionResult& OutResult)
{
    if (!Querier)
    {
        return false;
    }

    double StartTime = FPlatformTime::Cycles64();
    PerformanceStats.TotalQueries++;

    // ==================== 多级缓存查询 ====================
    int32 CacheLevel = 0;
    if (TryGetFromMultiLevelCache(Location, Radius, QuerierFaction, OutResult, CacheLevel))
    {
        // 记录缓存命中
        switch (CacheLevel)
        {
        case 1: PerformanceStats.L1CacheHits++; break;
        case 2: PerformanceStats.L2CacheHits++; break;
        case 3: PerformanceStats.L3CacheHits++; break;
        }

        double EndTime = FPlatformTime::Cycles64();
        TotalQueryTimeUs += FPlatformTime::ToSeconds64(EndTime - StartTime) * 1000000.0;
        PerformanceStats.AvgQueryTimeUs = static_cast<float>(TotalQueryTimeUs / PerformanceStats.TotalQueries);

        return OutResult.NearestEnemy != nullptr && IsValid(OutResult.NearestEnemy);
    }

    // ==================== 根据优先级处理 ====================
    switch (Priority)
    {
    case EXBQueryPriority::Immediate:
        {
            // 立即执行，不进队列
            TArray<FIntVector> CoveredCells;
            GetCoveredCells(Location, Radius, CoveredCells);
            OutResult = ExecuteOptimizedQuery(Location, Radius, QuerierFaction, Querier, CoveredCells);
            PerformanceStats.ActualQueries++;

            // 更新缓存
            UpdateRegionCache(Location, Radius, OutResult);
        }
        break;

    case EXBQueryPriority::High:
        {
            // 尝试合并，否则立即执行
            FXBPerceptionQueryEnhanced Query;
            Query.Querier = Querier;
            Query.Location = Location;
            Query.Radius = Radius;
            Query.QuerierFaction = QuerierFaction;
            Query.Priority = Priority;
            Query.QueryTime = FPlatformTime::Seconds();
            Query.CellIndex = GetCellIndex(Location);

            if (!TryMergeQuery(Query))
            {
                TArray<FIntVector> CoveredCells;
                GetCoveredCells(Location, Radius, CoveredCells);
                OutResult = ExecuteOptimizedQuery(Location, Radius, QuerierFaction, Querier, CoveredCells);
                PerformanceStats.ActualQueries++;
                UpdateRegionCache(Location, Radius, OutResult);
            }
            else
            {
                PerformanceStats.MergedQueries++;
                // 合并成功，返回上一次的缓存结果（可能略旧）
                TryGetFromMultiLevelCache(Location, Radius, QuerierFaction, OutResult, CacheLevel);
            }
        }
        break;

    case EXBQueryPriority::Normal:
    case EXBQueryPriority::Low:
        {
            // 加入对应队列，延迟处理
            FXBPerceptionQueryEnhanced Query;
            Query.Querier = Querier;
            Query.Location = Location;
            Query.Radius = Radius;
            Query.QuerierFaction = QuerierFaction;
            Query.Priority = Priority;
            Query.QueryTime = FPlatformTime::Seconds();
            Query.CellIndex = GetCellIndex(Location);

            if (Priority == EXBQueryPriority::Normal)
            {
                NormalPriorityQueue.Add(Query);
            }
            else
            {
                LowPriorityQueue.Add(Query);
            }

            // 返回旧缓存结果（如果有）
            TryGetFromMultiLevelCache(Location, Radius, QuerierFaction, OutResult, CacheLevel);
        }
        break;
    }

    double EndTime = FPlatformTime::Cycles64();
    TotalQueryTimeUs += FPlatformTime::ToSeconds64(EndTime - StartTime) * 1000000.0;
    PerformanceStats.AvgQueryTimeUs = static_cast<float>(TotalQueryTimeUs / PerformanceStats.TotalQueries);

    return OutResult.NearestEnemy != nullptr && IsValid(OutResult.NearestEnemy);
}

/**
 * @brief 异步查询接口
 * @note ✨ 新增 - 非阻塞查询
 */
void UXBSoldierPerceptionSubsystem::QueryNearestEnemyAsync(
    AActor* Querier,
    const FVector& Location,
    float Radius,
    EXBFaction QuerierFaction,
    TFunction<void(const FXBPerceptionResult&)> Callback)
{
    if (!Querier || !Callback)
    {
        return;
    }

    FXBPerceptionQueryEnhanced Query;
    Query.Querier = Querier;
    Query.Location = Location;
    Query.Radius = Radius;
    Query.QuerierFaction = QuerierFaction;
    Query.Priority = EXBQueryPriority::Normal;
    Query.QueryTime = FPlatformTime::Seconds();
    Query.CellIndex = GetCellIndex(Location);
    Query.Callback = Callback;

    NormalPriorityQueue.Add(Query);
}

/**
 * @brief 批量查询接口
 * @note ✨ 新增 - 高效批量处理
 */
int32 UXBSoldierPerceptionSubsystem::QueryEnemiesBatch(
    const TArray<FXBPerceptionQueryEnhanced>& Queries,
    TArray<FXBPerceptionResult>& OutResults)
{
    OutResults.Reset();
    OutResults.SetNum(Queries.Num());

    if (Queries.Num() == 0)
    {
        return 0;
    }

    int32 SuccessCount = 0;

    // ==================== 按单元格分组 ====================
    TMap<FIntVector, TArray<int32>> QueriesByCell;
    for (int32 i = 0; i < Queries.Num(); ++i)
    {
        FIntVector CellIndex = GetCellIndex(Queries[i].Location);
        QueriesByCell.FindOrAdd(CellIndex).Add(i);
    }

    // ==================== 逐单元格处理 ====================
    for (const auto& CellPair : QueriesByCell)
    {
        const FIntVector& CellIndex = CellPair.Key;
        const TArray<int32>& QueryIndices = CellPair.Value;

        // 获取该单元格覆盖的所有相关单元格
        TArray<FIntVector> AllCoveredCells;
        
        // 找出该组中最大的半径
        float MaxRadius = 0.0f;
        FVector CenterLocation = FVector::ZeroVector;
        for (int32 Idx : QueryIndices)
        {
            MaxRadius = FMath::Max(MaxRadius, Queries[Idx].Radius);
            CenterLocation += Queries[Idx].Location;
        }
        CenterLocation /= QueryIndices.Num();

        GetCoveredCells(CenterLocation, MaxRadius, AllCoveredCells);

        // 按阵营分组执行查询
        TMap<EXBFaction, FXBPerceptionResult> FactionResults;

        for (int32 Idx : QueryIndices)
        {
            const FXBPerceptionQueryEnhanced& Query = Queries[Idx];

            // 检查是否已有相同阵营的结果
            if (FXBPerceptionResult* ExistingResult = FactionResults.Find(Query.QuerierFaction))
            {
                OutResults[Idx] = *ExistingResult;
                PerformanceStats.MergedQueries++;
            }
            else
            {
                // 执行查询
                FXBPerceptionResult Result = ExecuteOptimizedQuery(
                    Query.Location,
                    Query.Radius,
                    Query.QuerierFaction,
                    Query.Querier.Get(),
                    AllCoveredCells
                );

                FactionResults.Add(Query.QuerierFaction, Result);
                OutResults[Idx] = Result;
                PerformanceStats.ActualQueries++;
            }

            if (OutResults[Idx].NearestEnemy != nullptr)
            {
                SuccessCount++;
            }
        }
    }

    return SuccessCount;
}

// ==================== 多级缓存实现 ====================

/**
 * @brief 尝试从多级缓存获取结果
 * @note ✨ 新增 - R-4 核心优化
 */
bool UXBSoldierPerceptionSubsystem::TryGetFromMultiLevelCache(
    const FVector& Location,
    float Radius,
    EXBFaction QuerierFaction,
    FXBPerceptionResult& OutResult,
    int32& OutCacheLevel)
{
    OutCacheLevel = 0;
    double CurrentTime = FPlatformTime::Seconds();
    FIntVector CellIndex = GetCellIndex(Location);

    // ==================== L1: 单元格缓存 ====================
    if (FXBSpatialCell* Cell = SpatialGrid.Find(CellIndex))
    {
        if (FXBPerceptionResult* CachedResult = Cell->CachedResults.Find(QuerierFaction))
        {
            double CacheAge = (CurrentTime - CachedResult->ResultTime) * 1000.0; // 转换为毫秒
            if (CacheAge < L1CacheValidityMs)
            {
                // 验证结果有效性
                CachedResult->CleanupInvalidRefs();
                if (CachedResult->bIsValid)
                {
                    OutResult = *CachedResult;
                    OutCacheLevel = 1;
                    return true;
                }
            }
        }
    }

    // ==================== L2: 区域缓存 ====================
    if (FXBRegionCacheEntry* RegionEntry = RegionCache.Find(CellIndex))
    {
        double CacheAge = (CurrentTime - RegionEntry->CreateTime) * 1000.0;
        if (CacheAge < L2CacheValidityMs)
        {
            // 检查半径兼容性（区域缓存的半径应该覆盖查询半径）
            if (RegionEntry->Radius >= Radius * 0.8f)
            {
                RegionEntry->Result.CleanupInvalidRefs();
                if (RegionEntry->Result.bIsValid)
                {
                    OutResult = RegionEntry->Result;
                    RegionEntry->HitCount++;
                    OutCacheLevel = 2;
                    return true;
                }
            }
        }
    }

    // ==================== L3: 热点缓存 ====================
    if (IsInHotspotRegion(Location))
    {
        if (FXBPerceptionResult* HotspotResult = HotspotCache.Find(CellIndex))
        {
            double CacheAge = (CurrentTime - HotspotResult->ResultTime) * 1000.0;
            if (CacheAge < L3CacheValidityMs)
            {
                HotspotResult->CleanupInvalidRefs();
                if (HotspotResult->bIsValid)
                {
                    OutResult = *HotspotResult;
                    OutCacheLevel = 3;
                    return true;
                }
            }
        }
    }

    return false;
}

// ==================== 查询合并实现 ====================

/**
 * @brief 尝试合并相似查询
 * @note ✨ 新增 - 减少重复查询
 */
bool UXBSoldierPerceptionSubsystem::TryMergeQuery(const FXBPerceptionQueryEnhanced& NewQuery)
{
    FIntVector CellIndex = NewQuery.CellIndex;

    // 检查是否有相近的待处理查询
    if (TArray<FXBPerceptionQueryEnhanced>* PendingList = PendingMergedQueries.Find(CellIndex))
    {
        for (const FXBPerceptionQueryEnhanced& Pending : *PendingList)
        {
            // 检查是否可以合并
            float Distance = FVector::Dist(NewQuery.Location, Pending.Location);
            if (Distance < QueryMergeDistance && 
                NewQuery.QuerierFaction == Pending.QuerierFaction &&
                FMath::Abs(NewQuery.Radius - Pending.Radius) < QueryMergeDistance)
            {
                return true;
            }
        }
    }

    // 添加到待处理列表
    PendingMergedQueries.FindOrAdd(CellIndex).Add(NewQuery);
    return false;
}

// ==================== 优先级队列处理 ====================

/**
 * @brief 处理优先级查询队列
 * @note ✨ 新增 - 分优先级处理
 */
void UXBSoldierPerceptionSubsystem::ProcessPriorityQueues()
{
    // 处理高优先级队列
    int32 HighProcessed = 0;
    while (HighPriorityQueue.Num() > 0 && HighProcessed < MaxHighPriorityQueriesPerFrame)
    {
        FXBPerceptionQueryEnhanced Query = HighPriorityQueue[0];
        HighPriorityQueue.RemoveAt(0);

        if (!Query.Querier.IsValid())
        {
            continue;
        }

        TArray<FIntVector> CoveredCells;
        GetCoveredCells(Query.Location, Query.Radius, CoveredCells);
        
        FXBPerceptionResult Result = ExecuteOptimizedQuery(
            Query.Location,
            Query.Radius,
            Query.QuerierFaction,
            Query.Querier.Get(),
            CoveredCells
        );

        UpdateRegionCache(Query.Location, Query.Radius, Result);
        PerformanceStats.ActualQueries++;

        if (Query.Callback)
        {
            Query.Callback(Result);
        }

        HighProcessed++;
    }

    // 处理普通优先级队列
    int32 NormalProcessed = 0;
    while (NormalPriorityQueue.Num() > 0 && NormalProcessed < MaxNormalQueriesPerFrame)
    {
        FXBPerceptionQueryEnhanced Query = NormalPriorityQueue[0];
        NormalPriorityQueue.RemoveAt(0);

        if (!Query.Querier.IsValid())
        {
            continue;
        }

        // 再次检查缓存（可能已被其他查询填充）
        FXBPerceptionResult Result;
        int32 CacheLevel = 0;
        if (!TryGetFromMultiLevelCache(Query.Location, Query.Radius, Query.QuerierFaction, Result, CacheLevel))
        {
            TArray<FIntVector> CoveredCells;
            GetCoveredCells(Query.Location, Query.Radius, CoveredCells);
            
            Result = ExecuteOptimizedQuery(
                Query.Location,
                Query.Radius,
                Query.QuerierFaction,
                Query.Querier.Get(),
                CoveredCells
            );

            UpdateRegionCache(Query.Location, Query.Radius, Result);
            PerformanceStats.ActualQueries++;
        }

        if (Query.Callback)
        {
            Query.Callback(Result);
        }

        NormalProcessed++;
    }

    // 处理低优先级队列
    int32 LowProcessed = 0;
    while (LowPriorityQueue.Num() > 0 && LowProcessed < MaxLowPriorityQueriesPerFrame)
    {
        FXBPerceptionQueryEnhanced Query = LowPriorityQueue[0];
        LowPriorityQueue.RemoveAt(0);

        if (!Query.Querier.IsValid())
        {
            continue;
        }

        FXBPerceptionResult Result;
        int32 CacheLevel = 0;
        if (!TryGetFromMultiLevelCache(Query.Location, Query.Radius, Query.QuerierFaction, Result, CacheLevel))
        {
            TArray<FIntVector> CoveredCells;
            GetCoveredCells(Query.Location, Query.Radius, CoveredCells);
            
            Result = ExecuteOptimizedQuery(
                Query.Location,
                Query.Radius,
                Query.QuerierFaction,
                Query.Querier.Get(),
                CoveredCells
            );

            UpdateRegionCache(Query.Location, Query.Radius, Result);
            PerformanceStats.ActualQueries++;
        }

        if (Query.Callback)
        {
            Query.Callback(Result);
        }

        LowProcessed++;
    }

    // 清理已处理的合并查询
    PendingMergedQueries.Empty();
}

// ==================== 空间查询优化 ====================

/**
 * @brief 获取查询覆盖的所有单元格
 * @note ✨ 新增 - 空间局部性优化
 */
void UXBSoldierPerceptionSubsystem::GetCoveredCells(const FVector& Location, float Radius, TArray<FIntVector>& OutCells)
{
    OutCells.Reset();

    // 计算边界单元格
    FIntVector MinCell = GetCellIndex(Location - FVector(Radius, Radius, Radius));
    FIntVector MaxCell = GetCellIndex(Location + FVector(Radius, Radius, Radius));

    // 遍历范围内的所有单元格
    for (int32 X = MinCell.X; X <= MaxCell.X; ++X)
    {
        for (int32 Y = MinCell.Y; Y <= MaxCell.Y; ++Y)
        {
            for (int32 Z = MinCell.Z; Z <= MaxCell.Z; ++Z)
            {
                OutCells.Add(FIntVector(X, Y, Z));
            }
        }
    }
}

/**
 * @brief 优化的区域敌人搜索
 * @note ✨ 新增 - 只遍历相关单元格
 */
FXBPerceptionResult UXBSoldierPerceptionSubsystem::ExecuteOptimizedQuery(
    const FVector& Location,
    float Radius,
    EXBFaction QuerierFaction,
    AActor* Querier,
    const TArray<FIntVector>& CoveredCells)
{
    FXBPerceptionResult Result;
    Result.ResultTime = FPlatformTime::Seconds();
    Result.bIsValid = true;

    float RadiusSq = Radius * Radius;
    float NearestDistanceSq = MAX_FLT;

    // ✨ 核心优化 - 只遍历覆盖的单元格
    for (const FIntVector& CellIndex : CoveredCells)
    {
        FXBSpatialCell* Cell = SpatialGrid.Find(CellIndex);
        if (!Cell)
        {
            continue;
        }

        for (const auto& WeakActor : Cell->Actors)
        {
            AActor* Actor = WeakActor.Get();
            
            if (!Actor || !IsValid(Actor) || Actor == Querier)
            {
                continue;
            }

            // 获取 Actor 阵营
            EXBFaction ActorFaction = EXBFaction::Neutral;
            if (AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(Actor))
            {
                if (Soldier->GetSoldierState() == EXBSoldierState::Dead)
                {
                    continue;
                }
                ActorFaction = Soldier->GetFaction();
            }
            else if (AXBCharacterBase* Character = Cast<AXBCharacterBase>(Actor))
            {
                if (Character->IsDead())
                {
                    continue;
                }
                ActorFaction = Character->GetFaction();
            }

            // 检查是否敌对
            if (!UXBBlueprintFunctionLibrary::AreFactionsHostile(QuerierFaction, ActorFaction))
            {
                continue;
            }

            // 距离检查
            float DistanceSq = FVector::DistSquared(Location, Actor->GetActorLocation());
            if (DistanceSq <= RadiusSq)
            {
                Result.DetectedEnemies.Add(Actor);

                if (DistanceSq < NearestDistanceSq)
                {
                    NearestDistanceSq = DistanceSq;
                    Result.NearestEnemy = Actor;
                    Result.DistanceToNearest = FMath::Sqrt(DistanceSq);
                }
            }
        }
    }

    return Result;
}

// ==================== 区域缓存管理 ====================

/**
 * @brief 更新区域缓存
 * @note ✨ 新增 - L2 缓存更新
 */
void UXBSoldierPerceptionSubsystem::UpdateRegionCache(const FVector& Location, float Radius, const FXBPerceptionResult& Result)
{
    FIntVector CellIndex = GetCellIndex(Location);

    // 更新 L1 缓存
    FXBSpatialCell& Cell = GetOrCreateCell(CellIndex);
    
    // 尝试从结果中推断阵营（取第一个敌人的相反阵营）
    EXBFaction ResultFaction = EXBFaction::Neutral;
    if (Result.DetectedEnemies.Num() > 0)
    {
        AActor* FirstEnemy = Result.DetectedEnemies[0];
        if (AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(FirstEnemy))
        {
            // 敌人是 Enemy，查询者应该是 Player/Ally
            if (Soldier->GetFaction() == EXBFaction::Enemy)
            {
                ResultFaction = EXBFaction::Player;
            }
            else
            {
                ResultFaction = EXBFaction::Enemy;
            }
        }
    }
    
    Cell.CachedResults.Add(ResultFaction, Result);

    // 更新 L2 区域缓存
    FXBRegionCacheEntry& RegionEntry = RegionCache.FindOrAdd(CellIndex);
    RegionEntry.Result = Result;
    RegionEntry.CreateTime = FPlatformTime::Seconds();
    RegionEntry.CenterLocation = Location;
    RegionEntry.Radius = Radius;

    // 如果在热点区域，更新 L3 缓存
    if (IsInHotspotRegion(Location))
    {
        HotspotCache.Add(CellIndex, Result);
    }
}

// ==================== 热点区域管理 ====================

bool UXBSoldierPerceptionSubsystem::IsInHotspotRegion(const FVector& Location) const
{
    for (const auto& Hotspot : HotspotRegions)
    {
        float Distance = FVector::Dist(Location, Hotspot.Key);
        if (Distance <= Hotspot.Value)
        {
            return true;
        }
    }
    return false;
}

void UXBSoldierPerceptionSubsystem::UpdateHotspotCaches()
{
    if (HotspotRegions.Num() == 0)
    {
        return;
    }

    // 对每个热点区域执行预查询
    for (const auto& Hotspot : HotspotRegions)
    {
        TArray<FIntVector> CoveredCells;
        GetCoveredCells(Hotspot.Key, Hotspot.Value, CoveredCells);

        // 为每个阵营预查询
        TArray<EXBFaction> Factions = { EXBFaction::Player, EXBFaction::Enemy };
        for (EXBFaction Faction : Factions)
        {
            FXBPerceptionResult Result = ExecuteOptimizedQuery(
                Hotspot.Key,
                Hotspot.Value,
                Faction,
                nullptr,
                CoveredCells
            );

            // 更新热点缓存
            for (const FIntVector& CellIndex : CoveredCells)
            {
                HotspotCache.Add(CellIndex, Result);
            }
        }
    }
}

void UXBSoldierPerceptionSubsystem::WarmupRegionCache(const FVector& Center, float Radius)
{
    UE_LOG(LogXBAI, Log, TEXT("预热区域缓存: 中心 %s, 半径 %.0f"), *Center.ToString(), Radius);

    TArray<FIntVector> CoveredCells;
    GetCoveredCells(Center, Radius, CoveredCells);

    // 为每个阵营预查询
    TArray<EXBFaction> Factions = { EXBFaction::Player, EXBFaction::Enemy };
    for (EXBFaction Faction : Factions)
    {
        FXBPerceptionResult Result = ExecuteOptimizedQuery(
            Center,
            Radius,
            Faction,
            nullptr,
            CoveredCells
        );

        UpdateRegionCache(Center, Radius, Result);
    }
}

void UXBSoldierPerceptionSubsystem::MarkHotspotRegion(const FVector& Center, float Radius)
{
    // 检查是否已存在
    for (auto& Hotspot : HotspotRegions)
    {
        if (FVector::Dist(Hotspot.Key, Center) < CellSize)
        {
            // 更新现有热点
            Hotspot.Value = FMath::Max(Hotspot.Value, Radius);
            return;
        }
    }

    HotspotRegions.Add(TPair<FVector, float>(Center, Radius));
    UE_LOG(LogXBAI, Log, TEXT("标记热点区域: 中心 %s, 半径 %.0f"), *Center.ToString(), Radius);
}

void UXBSoldierPerceptionSubsystem::ClearHotspotRegion(const FVector& Center)
{
    HotspotRegions.RemoveAll([&Center, this](const TPair<FVector, float>& Hotspot)
    {
        return FVector::Dist(Hotspot.Key, Center) < CellSize;
    });
}

// ==================== 性能监控 ====================

void UXBSoldierPerceptionSubsystem::ResetPerformanceStats()
{
    PerformanceStats = FXBPerceptionStats();
    TotalQueryTimeUs = 0.0;
    UE_LOG(LogXBAI, Log, TEXT("性能统计已重置"));
}

void UXBSoldierPerceptionSubsystem::PrintPerformanceReport()
{
    float TotalHitRate = PerformanceStats.GetTotalCacheHitRate() * 100.0f;
    float L1HitRate = PerformanceStats.TotalQueries > 0 ? 
        (static_cast<float>(PerformanceStats.L1CacheHits) / PerformanceStats.TotalQueries * 100.0f) : 0.0f;
    float L2HitRate = PerformanceStats.TotalQueries > 0 ? 
        (static_cast<float>(PerformanceStats.L2CacheHits) / PerformanceStats.TotalQueries * 100.0f) : 0.0f;
    float L3HitRate = PerformanceStats.TotalQueries > 0 ? 
        (static_cast<float>(PerformanceStats.L3CacheHits) / PerformanceStats.TotalQueries * 100.0f) : 0.0f;

    UE_LOG(LogXBAI, Warning, TEXT(""));
    UE_LOG(LogXBAI, Warning, TEXT("╔══════════════════════════════════════════════════════════════╗"));
    UE_LOG(LogXBAI, Warning, TEXT("║              士兵感知子系统性能报告                          ║"));
    UE_LOG(LogXBAI, Warning, TEXT("╠══════════════════════════════════════════════════════════════╣"));
    UE_LOG(LogXBAI, Warning, TEXT("║ 总查询次数:        %10lld                              ║"), PerformanceStats.TotalQueries);
    UE_LOG(LogXBAI, Warning, TEXT("║ 实际执行查询:      %10lld                              ║"), PerformanceStats.ActualQueries);
    UE_LOG(LogXBAI, Warning, TEXT("║ 合并查询次数:      %10lld                              ║"), PerformanceStats.MergedQueries);
    UE_LOG(LogXBAI, Warning, TEXT("╠══════════════════════════════════════════════════════════════╣"));
    UE_LOG(LogXBAI, Warning, TEXT("║ L1 缓存命中:       %10lld (%.1f%%)                       ║"), PerformanceStats.L1CacheHits, L1HitRate);
    UE_LOG(LogXBAI, Warning, TEXT("║ L2 缓存命中:       %10lld (%.1f%%)                       ║"), PerformanceStats.L2CacheHits, L2HitRate);
    UE_LOG(LogXBAI, Warning, TEXT("║ L3 缓存命中:       %10lld (%.1f%%)                       ║"), PerformanceStats.L3CacheHits, L3HitRate);
    UE_LOG(LogXBAI, Warning, TEXT("║ 总缓存命中率:      %.1f%%                                   ║"), TotalHitRate);
    UE_LOG(LogXBAI, Warning, TEXT("╠══════════════════════════════════════════════════════════════╣"));
    UE_LOG(LogXBAI, Warning, TEXT("║ 平均查询耗时:      %.2f 微秒                               ║"), PerformanceStats.AvgQueryTimeUs);
    UE_LOG(LogXBAI, Warning, TEXT("║ 注册 Actor 数量:   %10d                              ║"), GetRegisteredActorCount());
    UE_LOG(LogXBAI, Warning, TEXT("║ 空间网格单元格数:  %10d                              ║"), SpatialGrid.Num());
    UE_LOG(LogXBAI, Warning, TEXT("║ 热点区域数量:      %10d                              ║"), HotspotRegions.Num());
    UE_LOG(LogXBAI, Warning, TEXT("╚══════════════════════════════════════════════════════════════╝"));
    UE_LOG(LogXBAI, Warning, TEXT(""));
}

// ==================== 现有方法更新 ====================

/**
 * @brief 查询最近的敌人（兼容旧接口）
 * @note 🔧 修改 - 使用优化后的查询路径
 */
bool UXBSoldierPerceptionSubsystem::QueryNearestEnemy(
    AActor* Querier,
    const FVector& Location,
    float Radius,
    EXBFaction QuerierFaction,
    FXBPerceptionResult& OutResult)
{
    // 默认使用普通优先级
    return QueryNearestEnemyWithPriority(
        Querier,
        Location,
        Radius,
        QuerierFaction,
        EXBQueryPriority::Normal,
        OutResult
    );
}

bool UXBSoldierPerceptionSubsystem::QueryEnemiesInRadius(AActor* Querier, const FVector& Location, float Radius,
    EXBFaction QuerierFaction, FXBPerceptionResult& OutResult)
{
    // 与 QueryNearestEnemy 逻辑相同
    return QueryNearestEnemy(Querier, Location, Radius, QuerierFaction, OutResult);
}

/**
 * @brief 执行实际的感知查询
 * @note 🔧 修改 - 使用优化后的执行路径
 */
FXBPerceptionResult UXBSoldierPerceptionSubsystem::ExecuteQuery(
    const FVector& Location,
    float Radius,
    EXBFaction QuerierFaction,
    AActor* Querier)
{
    TArray<FIntVector> CoveredCells;
    GetCoveredCells(Location, Radius, CoveredCells);
    return ExecuteOptimizedQuery(Location, Radius, QuerierFaction, Querier, CoveredCells);
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

// ==================== Actor 注册（增强版） ====================

bool UXBSoldierPerceptionSubsystem::QueryNearestEnemyImmediate(AActor* Querier, const FVector& Location, float Radius,
    EXBFaction QuerierFaction, FXBPerceptionResult& OutResult)
{
    TotalQueries++;
    OutResult = ExecuteQuery(Location, Radius, QuerierFaction, Querier);
    // 🔧 修改 - 使用正确的指针检查方式
    return OutResult.NearestEnemy != nullptr && IsValid(OutResult.NearestEnemy);
}

void UXBSoldierPerceptionSubsystem::RegisterActor(AActor* Actor, EXBFaction Faction)
{
    if (!Actor || !IsValid(Actor))
    {
        return;
    }

    TArray<TWeakObjectPtr<AActor>>& FactionActors = RegisteredActors.FindOrAdd(Faction);
    
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

        // ✨ 新增 - 同时添加到空间网格
        FIntVector CellIndex = GetCellIndex(Actor->GetActorLocation());
        ActorCellMap.Add(Actor, CellIndex);
        GetOrCreateCell(CellIndex).Actors.Add(Actor);

        UE_LOG(LogXBAI, Verbose, TEXT("感知子系统: 注册 Actor %s (阵营: %d, 单元格: %s)"), 
            *Actor->GetName(), static_cast<int32>(Faction), *CellIndex.ToString());
    }
}

void UXBSoldierPerceptionSubsystem::UnregisterActor(AActor* Actor)
{
    if (!Actor)
    {
        return;
    }

    for (auto& Pair : RegisteredActors)
    {
        Pair.Value.RemoveAll([Actor](const TWeakObjectPtr<AActor>& WeakActor)
        {
            return !WeakActor.IsValid() || WeakActor.Get() == Actor;
        });
    }

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
        if (*OldCellIndex == NewCellIndex)
        {
            return;
        }

        if (FXBSpatialCell* OldCell = SpatialGrid.Find(*OldCellIndex))
        {
            OldCell->Actors.RemoveAll([Actor](const TWeakObjectPtr<AActor>& WeakActor)
            {
                return !WeakActor.IsValid() || WeakActor.Get() == Actor;
            });
        }
    }

    ActorCellMap.Add(Actor, NewCellIndex);
    GetOrCreateCell(NewCellIndex).Actors.Add(Actor);
}

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

void UXBSoldierPerceptionSubsystem::CleanupExpiredCache()
{
    double CurrentTime = FPlatformTime::Seconds();

    // 清理 L1 缓存
    for (auto& CellPair : SpatialGrid)
    {
        FXBSpatialCell& Cell = CellPair.Value;

        for (auto It = Cell.CachedResults.CreateIterator(); It; ++It)
        {
            double CacheAge = (CurrentTime - It->Value.ResultTime) * 1000.0;
            if (CacheAge > L1CacheValidityMs)
            {
                It.RemoveCurrent();
            }
        }

        Cell.Actors.RemoveAll([](const TWeakObjectPtr<AActor>& WeakActor)
        {
            return !WeakActor.IsValid();
        });
    }

    // 清理 L2 缓存
    for (auto It = RegionCache.CreateIterator(); It; ++It)
    {
        double CacheAge = (CurrentTime - It->Value.CreateTime) * 1000.0;
        if (CacheAge > L2CacheValidityMs)
        {
            It.RemoveCurrent();
        }
    }

    // 清理 L3 热点缓存
    for (auto It = HotspotCache.CreateIterator(); It; ++It)
    {
        double CacheAge = (CurrentTime - It->Value.ResultTime) * 1000.0;
        if (CacheAge > L3CacheValidityMs)
        {
            It.RemoveCurrent();
        }
    }

    // 清理阵营列表
    for (auto& FactionPair : RegisteredActors)
    {
        FactionPair.Value.RemoveAll([](const TWeakObjectPtr<AActor>& WeakActor)
        {
            return !WeakActor.IsValid();
        });
    }

    UE_LOG(LogXBAI, Verbose, TEXT("感知子系统: 清理过期缓存完成"));
}

float UXBSoldierPerceptionSubsystem::GetCacheHitRate() const
{
    return PerformanceStats.GetTotalCacheHitRate();
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

    for (const auto& CellPair : SpatialGrid)
    {
        FVector CellCenter = FVector(
            CellPair.Key.X * CellSize + CellSize * 0.5f,
            CellPair.Key.Y * CellSize + CellSize * 0.5f,
            CellPair.Key.Z * CellSize + CellSize * 0.5f
        );

        int32 ActorCount = CellPair.Value.Actors.Num();
        FColor CellColor = ActorCount > 0 ? FColor::Green : FColor::Red;

        // 热点区域用特殊颜色
        if (IsInHotspotRegion(CellCenter))
        {
            CellColor = FColor::Orange;
        }

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

        DrawDebugString(
            World,
            CellCenter,
            FString::Printf(TEXT("%d"), ActorCount),
            nullptr,
            FColor::White,
            Duration
        );
    }

    // 绘制热点区域
    for (const auto& Hotspot : HotspotRegions)
    {
        DrawDebugSphere(
            World,
            Hotspot.Key,
            Hotspot.Value,
            32,
            FColor::Yellow,
            false,
            Duration,
            0,
            3.0f
        );
    }
#endif
}
