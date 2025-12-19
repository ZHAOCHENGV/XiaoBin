/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Soldier/XBSoldierPoolSubsystem.h

/**
 * @file XBSoldierPoolSubsystem.h
 * @brief 士兵对象池子系统 - 高性能士兵生成/回收管理
 * 
 * @note ✨ 新增文件
 *       核心职责：
 *       1. 预生成并管理士兵 Actor 对象池
 *       2. 提供快速获取/回收接口
 *       3. 动态扩展池容量
 *       4. 支持多兵种分类池
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Army/XBSoldierTypes.h"
#include "XBSoldierPoolSubsystem.generated.h"

class AXBSoldierCharacter;
class UDataTable;

/**
 * @brief 对象池配置
 */
USTRUCT(BlueprintType)
struct FXBSoldierPoolConfig
{
    GENERATED_BODY()

    /** @brief 初始池大小 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "初始池大小", ClampMin = "10"))
    int32 InitialPoolSize = 50;

    /** @brief 最大池大小 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "最大池大小", ClampMin = "50"))
    int32 MaxPoolSize = 500;

    /** @brief 每帧最大生成数量（用于分帧预热） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "每帧生成上限", ClampMin = "1"))
    int32 MaxSpawnPerFrame = 5;

    /** @brief 池扩展步长 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "扩展步长", ClampMin = "5"))
    int32 ExpandStep = 10;

    /** @brief 低水位阈值（低于此值触发扩展） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "低水位阈值", ClampMin = "5"))
    int32 LowWaterMark = 10;
};

/**
 * @brief 单个池的统计数据
 */
USTRUCT(BlueprintType)
struct FXBPoolStats
{
    GENERATED_BODY()

    /** @brief 总创建数量 */
    UPROPERTY(BlueprintReadOnly)
    int32 TotalCreated = 0;

    /** @brief 当前可用数量 */
    UPROPERTY(BlueprintReadOnly)
    int32 Available = 0;

    /** @brief 当前使用中数量 */
    UPROPERTY(BlueprintReadOnly)
    int32 InUse = 0;

    /** @brief 获取次数 */
    UPROPERTY(BlueprintReadOnly)
    int64 AcquireCount = 0;

    /** @brief 回收次数 */
    UPROPERTY(BlueprintReadOnly)
    int64 ReleaseCount = 0;

    /** @brief 池扩展次数 */
    UPROPERTY(BlueprintReadOnly)
    int32 ExpandCount = 0;
};

/**
 * @brief 士兵对象池子系统
 * 
 * @note 设计理念：
 *       - 预生成：游戏开始时预热对象池
 *       - 复用：士兵死亡后重置并放回池中
 *       - 分类：按士兵类（蓝图类）分池管理
 *       - 异步：支持分帧预热避免卡顿
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBSoldierPoolSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ==================== 生命周期 ====================

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    // ==================== 池管理接口 ====================

    /**
     * @brief 预热对象池
     * @param SoldierClass 士兵蓝图类
     * @param Count 预热数量
     * @param bAsync 是否异步（分帧生成）
     * 
     * @note 建议在关卡加载时调用
     */
    UFUNCTION(BlueprintCallable, Category = "XB|SoldierPool", meta = (DisplayName = "预热对象池"))
    void WarmupPool(TSubclassOf<AXBSoldierCharacter> SoldierClass, int32 Count, bool bAsync = true);

    /**
     * @brief 从池中获取士兵
     * @param SoldierClass 士兵蓝图类
     * @param SpawnLocation 生成位置
     * @param SpawnRotation 生成旋转
     * @return 士兵实例（可能为空如果池已耗尽且达到上限）
     * 
     * @note 获取的士兵处于"休眠"状态，需要调用 ActivateSoldier 激活
     */
    UFUNCTION(BlueprintCallable, Category = "XB|SoldierPool", meta = (DisplayName = "获取士兵"))
    AXBSoldierCharacter* AcquireSoldier(
        TSubclassOf<AXBSoldierCharacter> SoldierClass,
        const FVector& SpawnLocation,
        const FRotator& SpawnRotation
    );

    /**
     * @brief 激活士兵（从休眠状态唤醒）
     * @param Soldier 士兵实例
     * @param DataTable 数据表
     * @param RowName 行名
     * @param Faction 阵营
     * 
     * @note 这将初始化士兵数据并使其可见
     */
    UFUNCTION(BlueprintCallable, Category = "XB|SoldierPool", meta = (DisplayName = "激活士兵"))
    void ActivateSoldier(
        AXBSoldierCharacter* Soldier,
        UDataTable* DataTable,
        FName RowName,
        EXBFaction Faction
    );

    /**
     * @brief 回收士兵到池中
     * @param Soldier 士兵实例
     * 
     * @note 士兵将被重置并进入休眠状态
     */
    UFUNCTION(BlueprintCallable, Category = "XB|SoldierPool", meta = (DisplayName = "回收士兵"))
    void ReleaseSoldier(AXBSoldierCharacter* Soldier);

    /**
     * @brief 获取池统计数据
     * @param SoldierClass 士兵蓝图类
     * @return 统计数据
     */
    UFUNCTION(BlueprintPure, Category = "XB|SoldierPool", meta = (DisplayName = "获取池统计"))
    FXBPoolStats GetPoolStats(TSubclassOf<AXBSoldierCharacter> SoldierClass) const;

    /**
     * @brief 打印所有池的统计报告
     */
    UFUNCTION(BlueprintCallable, Category = "XB|SoldierPool", meta = (DisplayName = "打印统计报告"))
    void PrintPoolReport() const;

    /**
     * @brief 清空所有池
     * @param bDestroyActors 是否销毁 Actor（通常在关卡卸载时使用）
     */
    UFUNCTION(BlueprintCallable, Category = "XB|SoldierPool", meta = (DisplayName = "清空所有池"))
    void ClearAllPools(bool bDestroyActors = true);

protected:
    // ==================== 内部方法 ====================

    /**
     * @brief 创建单个休眠状态的士兵
     * @param SoldierClass 士兵蓝图类
     * @return 士兵实例
     */
    AXBSoldierCharacter* CreateDormantSoldier(TSubclassOf<AXBSoldierCharacter> SoldierClass);

    /**
     * @brief 重置士兵到休眠状态
     * @param Soldier 士兵实例
     */
    void ResetSoldierToDormant(AXBSoldierCharacter* Soldier);

    /**
     * @brief 检查并扩展池
     * @param SoldierClass 士兵蓝图类
     */
    void CheckAndExpandPool(TSubclassOf<AXBSoldierCharacter> SoldierClass);

    /**
     * @brief 获取或创建类型池
     * @param SoldierClass 士兵蓝图类
     * @return 池引用
     */
    TArray<AXBSoldierCharacter*>& GetOrCreatePool(TSubclassOf<AXBSoldierCharacter> SoldierClass);

    /**
     * @brief 异步预热 Tick
     */
    void AsyncWarmupTick();

protected:
    // ==================== 配置 ====================

    /** @brief 池配置 */
    UPROPERTY(EditDefaultsOnly, Category = "配置", meta = (DisplayName = "池配置"))
    FXBSoldierPoolConfig PoolConfig;

    // ==================== 运行时数据 ====================

    /** @brief 可用士兵池（按类分类） */
    TMap<TSubclassOf<AXBSoldierCharacter>, TArray<AXBSoldierCharacter*>> AvailablePools;

    /** @brief 使用中的士兵（按类分类） */
    TMap<TSubclassOf<AXBSoldierCharacter>, TSet<AXBSoldierCharacter*>> InUseSoldiers;

    /** @brief 池统计数据 */
    TMap<TSubclassOf<AXBSoldierCharacter>, FXBPoolStats> PoolStatistics;

    /** @brief 待预热队列 */
    TArray<TPair<TSubclassOf<AXBSoldierCharacter>, int32>> PendingWarmup;

    /** @brief 预热定时器句柄 */
    FTimerHandle WarmupTimerHandle;

    /** @brief 休眠位置（用于隐藏休眠士兵） */
    static const FVector DormantLocation;
};