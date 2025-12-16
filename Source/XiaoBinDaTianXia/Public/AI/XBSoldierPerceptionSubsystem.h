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
};
