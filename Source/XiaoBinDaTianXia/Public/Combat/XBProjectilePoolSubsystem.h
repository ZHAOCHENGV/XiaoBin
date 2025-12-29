/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Combat/XBProjectilePoolSubsystem.h

/**
 * @file XBProjectilePoolSubsystem.h
 * @brief 投射物对象池子系统 - 频繁生成/销毁的投射物复用
 * 
 * @note ✨ 新增文件
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "XBProjectilePoolSubsystem.generated.h"

class AXBProjectile;

/**
 * @brief 投射物对象池统计数据
 */
USTRUCT(BlueprintType)
struct FXBProjectilePoolStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "统计", meta = (DisplayName = "回收次数"))
    int64 ReleaseCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "统计", meta = (DisplayName = "复用次数"))
    int64 ReuseCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "统计", meta = (DisplayName = "当前池大小"))
    int32 PoolSize = 0;
};

/**
 * @brief 投射物池桶
 * @note 用于规避 UHT 对 TMap<..., TArray<...>> 的限制
 */
USTRUCT(BlueprintType)
struct FXBProjectilePoolBucket
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "对象池", meta = (DisplayName = "投射物列表"))
    TArray<AXBProjectile*> Projectiles;
};

/**
 * @brief 投射物对象池子系统
 * 
 * @note 设计理念:
 *       - 适配频繁发射/回收的投射物
 *       - 按Class分桶缓存，避免频繁Spawn/Destroy造成的GC抖动
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBProjectilePoolSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    /**
     * @brief 回收投射物到池中
     * @param Projectile 要回收的投射物
     * @note 投射物会被重置为Hidden休眠态
     */
    UFUNCTION(BlueprintCallable, Category = "XB|ProjectilePool", meta = (DisplayName = "回收投射物"))
    void ReleaseProjectile(AXBProjectile* Projectile);

    /**
     * @brief 从池中获取投射物
     * @param ProjectileClass 投射物类
     * @param SpawnLocation 生成位置
     * @param SpawnRotation 生成旋转
     * @return 投射物实例
     */
    UFUNCTION(BlueprintCallable, Category = "XB|ProjectilePool", meta = (DisplayName = "获取投射物"))
    AXBProjectile* AcquireProjectile(TSubclassOf<AXBProjectile> ProjectileClass, const FVector& SpawnLocation, const FRotator& SpawnRotation);

    /**
     * @brief 获取池统计数据
     */
    UFUNCTION(BlueprintPure, Category = "XB|ProjectilePool", meta = (DisplayName = "获取统计"))
    const FXBProjectilePoolStats& GetPoolStats() const { return Stats; }

protected:
    /** @brief 回收的投射物列表（按类分桶） */
    UPROPERTY(meta = (DisplayName = "回收列表"))
    TMap<TSubclassOf<AXBProjectile>, FXBProjectilePoolBucket> RecycledProjectiles;

    /** @brief 统计数据 */
    UPROPERTY(meta = (DisplayName = "统计数据"))
    FXBProjectilePoolStats Stats;

    /** @brief 回收位置（隐藏的投射物放置位置） */
    static const FVector RecycleLocation;
};
