/**
 * @file XBSoldierPoolSubsystem.h
 * @brief 士兵对象池子系统 - 简化版（用于回收场景中的士兵）
 * 
 * @note 🔧 修改记录:
 *       1. 简化架构 - 不再预生成，只负责回收和复用
 *       2. 支持场景中预放置的士兵
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Army/XBSoldierTypes.h"
#include "XBSoldierPoolSubsystem.generated.h"

class AXBSoldierCharacter;

/**
 * @brief 池统计数据
 */
USTRUCT(BlueprintType)
struct FXBPoolStats
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
 * @brief 士兵对象池子系统（简化版）
 * 
 * @note 设计理念：
 *       - 士兵在场景中预放置（休眠态）
 *       - 死亡后回收到池中（变为 Hidden 休眠态）
 *       - 需要额外士兵时从池中获取（掉落等场景）
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBSoldierPoolSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    // ==================== 核心接口 ====================

    /**
     * @brief 回收士兵到池中
     * @param Soldier 要回收的士兵
     * @note 士兵会被重置为 Hidden 休眠态
     */
    UFUNCTION(BlueprintCallable, Category = "XB|SoldierPool", meta = (DisplayName = "回收士兵"))
    void ReleaseSoldier(AXBSoldierCharacter* Soldier);

    /**
     * @brief 从池中获取士兵（用于动态生成场景，如掉落）
     * @param SpawnLocation 生成位置
     * @param SpawnRotation 生成旋转
     * @return 士兵实例，如果池为空则返回 nullptr
     */
    UFUNCTION(BlueprintCallable, Category = "XB|SoldierPool", meta = (DisplayName = "获取士兵"))
    AXBSoldierCharacter* AcquireSoldier(const FVector& SpawnLocation, const FRotator& SpawnRotation);

    /**
     * @brief 检查池中是否有可用士兵
     */
    UFUNCTION(BlueprintPure, Category = "XB|SoldierPool", meta = (DisplayName = "有可用士兵"))
    bool HasAvailableSoldier() const { return RecycledSoldiers.Num() > 0; }

    /**
     * @brief 获取池统计数据
     */
    UFUNCTION(BlueprintPure, Category = "XB|SoldierPool", meta = (DisplayName = "获取统计"))
    const FXBPoolStats& GetPoolStats() const { return Stats; }

    /**
     * @brief 打印统计报告
     */
    UFUNCTION(BlueprintCallable, Category = "XB|SoldierPool", meta = (DisplayName = "打印报告"))
    void PrintPoolReport() const;

protected:
    /** @brief 回收的士兵列表 */
    UPROPERTY()
    TArray<AXBSoldierCharacter*> RecycledSoldiers;

    /** @brief 统计数据 */
    FXBPoolStats Stats;

    /** @brief 回收位置（隐藏的士兵放置位置） */
    static const FVector RecycleLocation;
};