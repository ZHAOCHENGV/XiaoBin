/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/XBSoldierPerceptionSubsystem.h

/**
 * @file XBSoldierPerceptionSubsystem.h
 * @brief 士兵感知子系统 - 集中管理目标检测与缓存
 * 
 * @note ✨ 新增文件
 *       核心职责：
 *       1. 批量执行目标检测，避免每个士兵独立查询
 *       2. 缓存检测结果，相邻士兵共享
 *       3. 提供统一的感知查询接口
 *       4. 为 R-4 性能优化提供基础设施
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Army/XBSoldierTypes.h"
#include "XBSoldierPerceptionSubsystem.generated.h"

class AXBSoldierCharacter;
class AXBCharacterBase;

/**
 * @brief 感知查询请求
 * @note 用于批量处理时记录每个查询的参数
 */
USTRUCT()
struct FXBPerceptionQuery
{
    GENERATED_BODY()

    /** @brief 查询发起者 */
    UPROPERTY()
    TWeakObjectPtr<AActor> Querier;

    /** @brief 查询位置 */
    FVector Location = FVector::ZeroVector;

    /** @brief 查询半径 */
    float Radius = 0.0f;

    /** @brief 查询者阵营 */
    EXBFaction QuerierFaction = EXBFaction::Neutral;

    /** @brief 查询时间戳 */
    double QueryTime = 0.0;
};

/**
 * @brief 感知查询结果
 * @note 缓存检测结果，支持复用
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBPerceptionResult
{
    GENERATED_BODY()

    /** @brief 是否有效 */
    UPROPERTY(BlueprintReadOnly)
    bool bIsValid = false;

    // 🔧 修改 - NearestEnemy 改为原始指针（蓝图兼容）
    /** @brief 最近的敌人 */
    UPROPERTY(BlueprintReadOnly)
    AActor* NearestEnemy = nullptr;

    /** @brief 到最近敌人的距离 */
    UPROPERTY(BlueprintReadOnly)
    float DistanceToNearest = MAX_FLT;

    // 🔧 修改 - 使用 TArray<AActor*> 替代 TArray<TWeakObjectPtr<AActor>>
    /** @brief 检测到的所有敌人 */
    UPROPERTY(BlueprintReadOnly)
    TArray<AActor*> DetectedEnemies;

    /** @brief 结果生成时间 */
    double ResultTime = 0.0;

    /** @brief 结果有效期（秒） */
    static constexpr double ValidityDuration = 0.15;

    /** @brief 检查结果是否仍然有效 */
    bool IsStillValid(double CurrentTime) const
    {
        return bIsValid && (CurrentTime - ResultTime) < ValidityDuration;
    }

    // ✨ 新增 - 清理无效引用的辅助方法
    void CleanupInvalidRefs()
    {
        if (NearestEnemy && !IsValid(NearestEnemy))
        {
            NearestEnemy = nullptr;
        }
        
        DetectedEnemies.RemoveAll([](AActor* Actor)
        {
            return !Actor || !IsValid(Actor);
        });
    }
};

/**
 * @brief 空间网格单元
 * @note 用于空间划分优化，相邻单元格的查询可以共享结果
 */
USTRUCT()
struct FXBSpatialCell
{
    GENERATED_BODY()

    /** @brief 单元格内的 Actor 列表 */
    TArray<TWeakObjectPtr<AActor>> Actors;

    /** @brief 上次更新时间 */
    double LastUpdateTime = 0.0;

    /** @brief 缓存的检测结果（按阵营索引） */
    TMap<EXBFaction, FXBPerceptionResult> CachedResults;
};
// ✨ 新增 - 查询优先级枚举
UENUM(BlueprintType)
enum class EXBQueryPriority : uint8
{
    Low         UMETA(DisplayName = "低优先级"),      // 非战斗状态
    Normal      UMETA(DisplayName = "普通"),          // 正常查询
    High        UMETA(DisplayName = "高优先级"),      // 战斗中
    Immediate   UMETA(DisplayName = "立即执行")       // 关键决策
};

// ✨ 新增 - 增强的感知查询请求
USTRUCT()
struct FXBPerceptionQueryEnhanced
{
    GENERATED_BODY()

    /** @brief 查询发起者 */
    TWeakObjectPtr<AActor> Querier;

    /** @brief 查询位置 */
    FVector Location = FVector::ZeroVector;

    /** @brief 查询半径 */
    float Radius = 0.0f;

    /** @brief 查询者阵营 */
    EXBFaction QuerierFaction = EXBFaction::Neutral;

    /** @brief 查询时间戳 */
    double QueryTime = 0.0;

    /** @brief 查询优先级 */
    EXBQueryPriority Priority = EXBQueryPriority::Normal;

    /** @brief 结果回调（用于异步查询） */
    TFunction<void(const FXBPerceptionResult&)> Callback;

    /** @brief 所在单元格索引 */
    FIntVector CellIndex = FIntVector::ZeroValue;
};

// ✨ 新增 - 区域缓存条目
USTRUCT()
struct FXBRegionCacheEntry
{
    GENERATED_BODY()

    /** @brief 缓存的结果 */
    FXBPerceptionResult Result;

    /** @brief 缓存创建时间 */
    double CreateTime = 0.0;

    /** @brief 缓存命中次数 */
    int32 HitCount = 0;

    /** @brief 覆盖的单元格索引列表 */
    TArray<FIntVector> CoveredCells;

    /** @brief 区域中心位置 */
    FVector CenterLocation = FVector::ZeroVector;

    /** @brief 区域半径 */
    float Radius = 0.0f;
};

// ✨ 新增 - 性能统计数据
USTRUCT(BlueprintType)
struct FXBPerceptionStats
{
    GENERATED_BODY()

    /** @brief 总查询次数 */
    UPROPERTY(BlueprintReadOnly)
    int64 TotalQueries = 0;

    /** @brief L1 缓存命中 */
    UPROPERTY(BlueprintReadOnly)
    int64 L1CacheHits = 0;

    /** @brief L2 缓存命中 */
    UPROPERTY(BlueprintReadOnly)
    int64 L2CacheHits = 0;

    /** @brief L3 缓存命中 */
    UPROPERTY(BlueprintReadOnly)
    int64 L3CacheHits = 0;

    /** @brief 实际执行的查询次数 */
    UPROPERTY(BlueprintReadOnly)
    int64 ActualQueries = 0;

    /** @brief 合并的查询次数 */
    UPROPERTY(BlueprintReadOnly)
    int64 MergedQueries = 0;

    /** @brief 平均查询耗时（微秒） */
    UPROPERTY(BlueprintReadOnly)
    float AvgQueryTimeUs = 0.0f;

    /** @brief 获取总缓存命中率 */
    float GetTotalCacheHitRate() const
    {
        if (TotalQueries == 0) return 0.0f;
        return static_cast<float>(L1CacheHits + L2CacheHits + L3CacheHits) / static_cast<float>(TotalQueries);
    }
};


/**
 * @brief 士兵感知子系统
 * 
 * @note 设计理念：
 *       - 集中化：所有感知查询通过此子系统执行
 *       - 批量化：每帧批量处理查询请求
 *       - 缓存化：相近位置/时间的查询共享结果
 *       - 空间化：使用网格划分加速查询
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBSoldierPerceptionSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ==================== 生命周期 ====================

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;



    // ==================== 感知查询接口 ====================

    /**
     * @brief 查询最近的敌人（主要接口）
     * @param Querier 查询发起者
     * @param Location 查询位置
     * @param Radius 检测半径
     * @param QuerierFaction 查询者阵营
     * @param OutResult 输出结果
     * @return 是否找到敌人
     * 
     * @note 核心优化：
     *       1. 首先检查缓存是否命中
     *       2. 缓存未命中时加入批量查询队列
     *       3. 返回上一帧的缓存结果（如果有效）
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Perception", meta = (DisplayName = "查询最近敌人"))
    bool QueryNearestEnemy(
        AActor* Querier,
        const FVector& Location,
        float Radius,
        EXBFaction QuerierFaction,
        FXBPerceptionResult& OutResult
    );

    /**
     * @brief 查询范围内所有敌人
     * @param Querier 查询发起者
     * @param Location 查询位置
     * @param Radius 检测半径
     * @param QuerierFaction 查询者阵营
     * @param OutResult 输出结果
     * @return 是否找到敌人
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Perception", meta = (DisplayName = "查询范围内敌人"))
    bool QueryEnemiesInRadius(
        AActor* Querier,
        const FVector& Location,
        float Radius,
        EXBFaction QuerierFaction,
        FXBPerceptionResult& OutResult
    );

    /**
     * @brief 立即执行查询（绕过缓存，用于关键决策）
     * @note 性能敏感，仅在必要时使用
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Perception", meta = (DisplayName = "立即查询敌人"))
    bool QueryNearestEnemyImmediate(
        AActor* Querier,
        const FVector& Location,
        float Radius,
        EXBFaction QuerierFaction,
        FXBPerceptionResult& OutResult
    );

    /**
     * @brief  根据指定敌方阵营快速查询最近敌人
     * @param  Querier 查询发起者
     * @param  Location 查询位置
     * @param  Radius 查询半径
     * @param  TargetFaction 目标敌方阵营
     * @param  OutResult 输出结果
     * @param  bPreferSoldiers 是否优先士兵
     * @return 是否找到目标
     * @note   详细流程分析: 获取查询者阵营 -> 快速遍历阵营注册列表 -> 过滤死亡/无效 -> 优先士兵择近
     *         性能/架构注意事项: 仅在战斗态回退时调用，避免频繁遍历所有阵营 Actor
     */
    bool QueryNearestEnemyByFaction(
        AActor* Querier,
        const FVector& Location,
        float Radius,
        EXBFaction TargetFaction,
        FXBPerceptionResult& OutResult,
        bool bPreferSoldiers = true
    );

    // ==================== Actor 注册 ====================

    /**
     * @brief 注册可检测的 Actor
     * @param Actor 要注册的 Actor
     * @param Faction 阵营
     * @note 士兵/将领生成时调用
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Perception", meta = (DisplayName = "注册Actor"))
    void RegisterActor(AActor* Actor, EXBFaction Faction);

    /**
     * @brief 注销 Actor
     * @param Actor 要注销的 Actor
     * @note Actor 销毁时调用
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Perception", meta = (DisplayName = "注销Actor"))
    void UnregisterActor(AActor* Actor);

    /**
     * @brief 更新 Actor 位置
     * @param Actor Actor
     * @note 移动后调用，更新空间网格
     */
    void UpdateActorLocation(AActor* Actor);

    // ==================== 调试接口 ====================

    /**
     * @brief 获取缓存命中率
     */
    UFUNCTION(BlueprintPure, Category = "XB|Perception|Debug", meta = (DisplayName = "获取缓存命中率"))
    float GetCacheHitRate() const;

    /**
     * @brief 获取当前注册的 Actor 数量
     */
    UFUNCTION(BlueprintPure, Category = "XB|Perception|Debug", meta = (DisplayName = "获取注册Actor数量"))
    int32 GetRegisteredActorCount() const;

    /**
     * @brief 绘制调试信息
     */
    void DrawDebugInfo(float Duration = 0.0f);

protected:
    // ==================== 内部方法 ====================

    /**
     * @brief 计算空间网格索引
     * @param Location 世界位置
     * @return 网格索引
     */
    FIntVector GetCellIndex(const FVector& Location) const;

    /**
     * @brief 获取或创建空间单元格
     * @param CellIndex 单元格索引
     * @return 单元格引用
     */
    FXBSpatialCell& GetOrCreateCell(const FIntVector& CellIndex);

    /**
     * @brief 尝试从缓存获取结果
     * @param Location 查询位置
     * @param Radius 检测半径
     * @param QuerierFaction 查询者阵营
     * @param OutResult 输出结果
     * @return 是否命中缓存
     */
    bool TryGetCachedResult(
        const FVector& Location,
        float Radius,
        EXBFaction QuerierFaction,
        FXBPerceptionResult& OutResult
    );

    /**
     * @brief 执行实际的感知查询
     * @param Location 查询位置
     * @param Radius 检测半径
     * @param QuerierFaction 查询者阵营
     * @param Querier 查询者（用于排除自己）
     * @return 查询结果
     */
    FXBPerceptionResult ExecuteQuery(
        const FVector& Location,
        float Radius,
        EXBFaction QuerierFaction,
        AActor* Querier
    );

    /**
     * @brief 处理批量查询队列
     */
    void ProcessQueryQueue();

    /**
     * @brief 清理过期缓存
     */
    void CleanupExpiredCache();

    /**
     * @brief 更新空间网格
     */
    void UpdateSpatialGrid();

protected:
    // ==================== 配置 ====================

    /** @brief 空间网格单元格大小（厘米） */
    UPROPERTY(EditDefaultsOnly, Category = "配置", meta = (DisplayName = "网格单元格大小"))
    float CellSize = 500.0f;

    /** @brief 缓存清理间隔（秒） */
    UPROPERTY(EditDefaultsOnly, Category = "配置", meta = (DisplayName = "缓存清理间隔"))
    float CacheCleanupInterval = 1.0f;

    /** @brief 每帧最大查询处理数量 */
    UPROPERTY(EditDefaultsOnly, Category = "配置", meta = (DisplayName = "每帧最大查询数"))
    int32 MaxQueriesPerFrame = 50;

    /** @brief 缓存有效期（秒） */
    UPROPERTY(EditDefaultsOnly, Category = "配置", meta = (DisplayName = "缓存有效期"))
    float CacheValidityDuration = 0.15f;

    // ==================== 运行时数据 ====================

    /** @brief 已注册的 Actor（按阵营分组） */
    TMap<EXBFaction, TArray<TWeakObjectPtr<AActor>>> RegisteredActors;

    /** @brief 空间网格 */
    TMap<FIntVector, FXBSpatialCell> SpatialGrid;

    /** @brief 待处理的查询队列 */
    TArray<FXBPerceptionQuery> PendingQueries;

    /** @brief Actor 到网格索引的映射 */
    TMap<TWeakObjectPtr<AActor>, FIntVector> ActorCellMap;

    /** @brief 上次缓存清理时间 */
    double LastCacheCleanupTime = 0.0;

    // ==================== 统计数据 ====================

    /** @brief 总查询次数 */
    int64 TotalQueries = 0;

    /** @brief 缓存命中次数 */
    int64 CacheHits = 0;

    // 🔧 修改 - 使用 FTimerHandle 替代 FTSTicker
    /** @brief 定时器句柄 */
    FTimerHandle TickTimerHandle;
    
    // ✨ 新增 - 内部 Tick 方法
    void PerformTick();


public:
        // ==================== ✨ 新增：增强查询接口 ====================

    /**
     * @brief 带优先级的查询接口
     * @param Querier 查询发起者
     * @param Location 查询位置
     * @param Radius 检测半径
     * @param QuerierFaction 查询者阵营
     * @param Priority 查询优先级
     * @param OutResult 输出结果
     * @return 是否找到敌人
     * 
     * @note 优化策略：
     *       - High/Immediate: 立即执行，跳过合并
     *       - Normal: 可能合并，使用缓存
     *       - Low: 延迟执行，优先使用缓存
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Perception", meta = (DisplayName = "带优先级查询敌人"))
    bool QueryNearestEnemyWithPriority(
        AActor* Querier,
        const FVector& Location,
        float Radius,
        EXBFaction QuerierFaction,
        EXBQueryPriority Priority,
        FXBPerceptionResult& OutResult
    );

    /**
     * @brief 异步查询接口（非阻塞）
     * @param Querier 查询发起者
     * @param Location 查询位置
     * @param Radius 检测半径
     * @param QuerierFaction 查询者阵营
     * @param Callback 结果回调
     * 
     * @note 查询将在下一帧处理，结果通过回调返回
     */
    void QueryNearestEnemyAsync(
        AActor* Querier,
        const FVector& Location,
        float Radius,
        EXBFaction QuerierFaction,
        TFunction<void(const FXBPerceptionResult&)> Callback
    );

    /**
     * @brief 批量查询接口
     * @param Queries 查询请求列表
     * @param OutResults 输出结果列表
     * @return 成功查询的数量
     * 
     * @note 优化策略：
     *       1. 按单元格分组
     *       2. 合并相近查询
     *       3. 复用中间结果
     */
    UFUNCTION(Category = "XB|Perception", meta = (DisplayName = "批量查询敌人"))
    int32 QueryEnemiesBatch(
        const TArray<FXBPerceptionQueryEnhanced>& Queries,
        TArray<FXBPerceptionResult>& OutResults
    );

    // ==================== ✨ 新增：性能监控接口 ====================

    /**
     * @brief 获取性能统计数据
     */
    UFUNCTION(BlueprintPure, Category = "XB|Perception|Stats", meta = (DisplayName = "获取性能统计"))
    const FXBPerceptionStats& GetPerformanceStats() const { return PerformanceStats; }

    /**
     * @brief 重置性能统计
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Perception|Stats", meta = (DisplayName = "重置性能统计"))
    void ResetPerformanceStats();

    /**
     * @brief 打印性能报告
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Perception|Stats", meta = (DisplayName = "打印性能报告"))
    void PrintPerformanceReport();

    // ==================== ✨ 新增：预热和优化接口 ====================

    /**
     * @brief 预热指定区域的缓存
     * @param Center 区域中心
     * @param Radius 区域半径
     * 
     * @note 在战斗开始前调用，预先填充缓存
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Perception", meta = (DisplayName = "预热区域缓存"))
    void WarmupRegionCache(const FVector& Center, float Radius);

    /**
     * @brief 标记热点区域（战斗区域）
     * @param Center 区域中心
     * @param Radius 区域半径
     * 
     * @note 热点区域的缓存有效期更长，更新更频繁
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Perception", meta = (DisplayName = "标记热点区域"))
    void MarkHotspotRegion(const FVector& Center, float Radius);

    /**
     * @brief 清除热点区域标记
     * @param Center 区域中心
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Perception", meta = (DisplayName = "清除热点区域"))
    void ClearHotspotRegion(const FVector& Center);

protected:
    // ==================== ✨ 新增：内部优化方法 ====================

    /**
     * @brief 尝试从多级缓存获取结果
     * @param Location 查询位置
     * @param Radius 检测半径
     * @param QuerierFaction 查询者阵营
     * @param OutResult 输出结果
     * @param OutCacheLevel 命中的缓存级别（1/2/3）
     * @return 是否命中缓存
     */
    bool TryGetFromMultiLevelCache(
        const FVector& Location,
        float Radius,
        EXBFaction QuerierFaction,
        FXBPerceptionResult& OutResult,
        int32& OutCacheLevel
    );

    /**
     * @brief 尝试合并相似查询
     * @param NewQuery 新查询
     * @return 是否成功合并（合并后无需单独执行）
     */
    bool TryMergeQuery(const FXBPerceptionQueryEnhanced& NewQuery);

    /**
     * @brief 处理优先级查询队列
     */
    void ProcessPriorityQueues();

    /**
     * @brief 获取查询覆盖的所有单元格
     * @param Location 查询位置
     * @param Radius 查询半径
     * @param OutCells 输出单元格列表
     */
    void GetCoveredCells(const FVector& Location, float Radius, TArray<FIntVector>& OutCells);

    /**
     * @brief 优化的区域敌人搜索
     * @param Location 查询位置
     * @param Radius 查询半径
     * @param QuerierFaction 查询者阵营
     * @param Querier 查询者
     * @param CoveredCells 覆盖的单元格
     * @return 查询结果
     * 
     * @note 只遍历相关单元格，而非全局
     */
    FXBPerceptionResult ExecuteOptimizedQuery(
        const FVector& Location,
        float Radius,
        EXBFaction QuerierFaction,
        AActor* Querier,
        const TArray<FIntVector>& CoveredCells
    );

    /**
     * @brief 更新区域缓存
     * @param Location 区域中心
     * @param Radius 区域半径
     * @param Result 查询结果
     */
    void UpdateRegionCache(const FVector& Location, float Radius, const FXBPerceptionResult& Result);

    /**
     * @brief 检查是否在热点区域内
     * @param Location 位置
     * @return 是否在热点区域
     */
    bool IsInHotspotRegion(const FVector& Location) const;

    /**
     * @brief 更新热点区域缓存
     */
    void UpdateHotspotCaches();

protected:
    // ==================== ✨ 新增：配置参数 ====================

    /** @brief L1 缓存有效期（单元格缓存，毫秒） */
    UPROPERTY(EditDefaultsOnly, Category = "配置|缓存", meta = (DisplayName = "L1缓存有效期"))
    float L1CacheValidityMs = 150.0f;

    /** @brief L2 缓存有效期（区域缓存，毫秒） */
    UPROPERTY(EditDefaultsOnly, Category = "配置|缓存", meta = (DisplayName = "L2缓存有效期"))
    float L2CacheValidityMs = 300.0f;

    /** @brief L3 缓存有效期（热点缓存，毫秒） */
    UPROPERTY(EditDefaultsOnly, Category = "配置|缓存", meta = (DisplayName = "L3缓存有效期"))
    float L3CacheValidityMs = 500.0f;

    /** @brief 查询合并距离阈值 */
    UPROPERTY(EditDefaultsOnly, Category = "配置|优化", meta = (DisplayName = "查询合并距离"))
    float QueryMergeDistance = 100.0f;

    /** @brief 每帧处理的高优先级查询数量上限 */
    UPROPERTY(EditDefaultsOnly, Category = "配置|优化", meta = (DisplayName = "每帧高优先级查询上限"))
    int32 MaxHighPriorityQueriesPerFrame = 20;

    /** @brief 每帧处理的普通优先级查询数量上限 */
    UPROPERTY(EditDefaultsOnly, Category = "配置|优化", meta = (DisplayName = "每帧普通查询上限"))
    int32 MaxNormalQueriesPerFrame = 30;

    /** @brief 每帧处理的低优先级查询数量上限 */
    UPROPERTY(EditDefaultsOnly, Category = "配置|优化", meta = (DisplayName = "每帧低优先级查询上限"))
    int32 MaxLowPriorityQueriesPerFrame = 10;

    /** @brief 热点区域更新间隔（秒） */
    UPROPERTY(EditDefaultsOnly, Category = "配置|优化", meta = (DisplayName = "热点更新间隔"))
    float HotspotUpdateInterval = 0.1f;

    // ==================== ✨ 新增：运行时数据 ====================

    /** @brief 高优先级查询队列 */
    TArray<FXBPerceptionQueryEnhanced> HighPriorityQueue;

    /** @brief 普通优先级查询队列 */
    TArray<FXBPerceptionQueryEnhanced> NormalPriorityQueue;

    /** @brief 低优先级查询队列 */
    TArray<FXBPerceptionQueryEnhanced> LowPriorityQueue;

    /** @brief L2 区域缓存 */
    TMap<FIntVector, FXBRegionCacheEntry> RegionCache;

    /** @brief L3 热点区域列表 */
    TArray<TPair<FVector, float>> HotspotRegions;

    /** @brief 热点区域缓存 */
    TMap<FIntVector, FXBPerceptionResult> HotspotCache;

    /** @brief 上次热点更新时间 */
    double LastHotspotUpdateTime = 0.0;

    /** @brief 性能统计数据 */
    FXBPerceptionStats PerformanceStats;

    /** @brief 查询耗时累计（用于计算平均值） */
    double TotalQueryTimeUs = 0.0;

    /** @brief 合并的查询映射（查询位置 -> 等待结果的查询列表） */
    TMap<FIntVector, TArray<FXBPerceptionQueryEnhanced>> PendingMergedQueries;
};
