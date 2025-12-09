/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/XBFlowFieldSubsystem.h

/**
 * @file XBFlowFieldSubsystem.h
 * @brief 流场子系统 - 管理所有流场和空间划分
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AI/XBFlowField.h"
#include "AI/XBSpatialHashGrid.h"
#include "XBFlowFieldSubsystem.generated.h"

class AXBCharacterBase;
class AXBSoldierActor;

/**
 * @brief 流场子系统
 * @note 世界子系统，每个World一个实例
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBFlowFieldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // 子系统生命周期
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

    /**
   * @brief 设置流场更新间隔
   * @param Interval 更新间隔（秒）
   */
    UFUNCTION(BlueprintCallable, Category = "流场")
    void SetUpdateInterval(float Interval) { FlowFieldUpdateInterval = FMath::Max(Interval, 0.016f); }

    /**
     * @brief 获取流场更新间隔
     */
    UFUNCTION(BlueprintPure, Category = "流场")
    float GetUpdateInterval() const { return FlowFieldUpdateInterval; }

    /**
     * @brief 获取主将的流场（只读）
     * @param Leader 主将
     * @return 流场指针，无效返回nullptr
     */
    const FXBFlowField* GetFlowFieldForLeader(AXBCharacterBase* Leader) const
    {
        return LeaderFlowFields.Find(Leader);
    }

    /**
     * @brief 获取所有流场的迭代器
     */
    const TMap<AXBCharacterBase*, FXBFlowField>& GetAllFlowFields() const { return LeaderFlowFields; };

    /**
     * @brief 设置调试绘制状态
     * @param bEnabled 是否启用
     */
    UFUNCTION(BlueprintCallable, Category = "调试")
    void SetDebugDrawEnabled(bool bEnabled) { bDebugDraw = bEnabled; }

    /**
     * @brief 获取调试绘制状态
     */
    UFUNCTION(BlueprintPure, Category = "调试")
    bool IsDebugDrawEnabled() const { return bDebugDraw; }

    /**
     * @brief 初始化流场系统
     * @param WorldOrigin 世界原点
     * @param WorldSize 世界大小
     * @param CellSize 单元格大小
     */
    UFUNCTION(BlueprintCallable, Category = "流场")
    void InitializeFlowField(const FVector2D& WorldOrigin, const FVector2D& WorldSize, float CellSize = 100.0f);

    /**
     * @brief 注册主将 (作为流场目标)
     * @param Leader 主将
     */
    UFUNCTION(BlueprintCallable, Category = "流场")
    void RegisterLeader(AXBCharacterBase* Leader);

    /**
     * @brief 注销主将
     * @param Leader 主将
     */
    UFUNCTION(BlueprintCallable, Category = "流场")
    void UnregisterLeader(AXBCharacterBase* Leader);

    /**
     * @brief 注册小兵
     * @param Soldier 小兵
     */
    UFUNCTION(BlueprintCallable, Category = "流场")
    void RegisterSoldier(AXBSoldierActor* Soldier);

    /**
     * @brief 注销小兵
     * @param Soldier 小兵
     */
    UFUNCTION(BlueprintCallable, Category = "流场")
    void UnregisterSoldier(AXBSoldierActor* Soldier);

    /**
     * @brief 获取指向主将的流向
     * @param Leader 目标主将
     * @param WorldLocation 查询位置
     * @return 流向
     */
    UFUNCTION(BlueprintPure, Category = "流场")
    FVector GetFlowDirectionToLeader(AXBCharacterBase* Leader, const FVector& WorldLocation) const;

    /**
     * @brief 获取附近的小兵
     * @param Location 位置
     * @param Radius 半径
     * @param OutSoldiers 输出小兵数组
     * @param ExcludeActor 排除的Actor
     */
    UFUNCTION(BlueprintCallable, Category = "空间查询")
    void GetNearbySoldiers(const FVector& Location, float Radius, TArray<AXBSoldierActor*>& OutSoldiers, AActor* ExcludeActor = nullptr);

    /**
     * @brief 获取附近的敌人
     * @param Location 位置
     * @param Radius 半径
     * @param MyFaction 我方阵营
     * @param OutEnemies 输出敌人数组
     */
    UFUNCTION(BlueprintCallable, Category = "空间查询")
    void GetNearbyEnemies(const FVector& Location, float Radius, EXBFaction MyFaction, TArray<AActor*>& OutEnemies);

    /**
     * @brief 获取最近的敌人
     * @param Location 位置
     * @param Radius 半径
     * @param MyFaction 我方阵营
     * @return 最近的敌人
     */
    UFUNCTION(BlueprintPure, Category = "空间查询")
    AActor* GetNearestEnemy(const FVector& Location, float Radius, EXBFaction MyFaction);

    /**
     * @brief 设置障碍物
     * @param Location 位置
     * @param Radius 半径
     */
    UFUNCTION(BlueprintCallable, Category = "流场")
    void AddObstacle(const FVector& Location, float Radius);

    /**
     * @brief 移除障碍物
     * @param Location 位置
     * @param Radius 半径
     */
    UFUNCTION(BlueprintCallable, Category = "流场")
    void RemoveObstacle(const FVector& Location, float Radius);

    // 标记格子为障碍物
    UFUNCTION(BlueprintCallable, Category = "Flow Field")
    void MarkCellAsBlocked(int32 X, int32 Y);

    // 清除障碍物标记
    UFUNCTION(BlueprintCallable, Category = "Flow Field")
    void ClearBlockedCells();

    // 获取流向
    UFUNCTION(BlueprintCallable, Category = "Flow Field")
    FVector GetFlowDirection(const FVector& WorldPosition) const;

    /**
     * @brief 更新流场 (每帧调用)
     */
    UFUNCTION(BlueprintCallable, Category = "流场")
    void UpdateFlowFields();

    /**
     * @brief 绘制调试信息
     */
    UFUNCTION(BlueprintCallable, Category = "调试")
    void DebugDraw();

    /**
     * @brief 获取空间网格引用
     */
    FXBSpatialHashGrid& GetSpatialGrid() { return SpatialGrid; }

protected:
    /** @brief Tick函数 */
    void OnWorldTick(UWorld* World, ELevelTick TickType, float DeltaSeconds);
    // 障碍物格子集合
    TSet<FIntPoint> BlockedCells;
    

private:
    /** @brief 主将到流场的映射 */
    UPROPERTY()
    TMap<AXBCharacterBase*, FXBFlowField> LeaderFlowFields;

    /** @brief 空间哈希网格 */
    FXBSpatialHashGrid SpatialGrid;

    /** @brief 所有注册的小兵 */
    UPROPERTY()
    TArray<TWeakObjectPtr<AXBSoldierActor>> RegisteredSoldiers;

    /** @brief 所有注册的主将 */
    UPROPERTY()
    TArray<TWeakObjectPtr<AXBCharacterBase>> RegisteredLeaders;

    /** @brief 是否已初始化 */
    bool bInitialized = false;

    /** @brief 流场更新间隔 */
    float FlowFieldUpdateInterval = 0.1f;

    /** @brief 上次更新时间 */
    float LastUpdateTime = 0.0f;

    /** @brief 世界配置 */
    FVector2D CachedWorldOrigin;
    FVector2D CachedWorldSize;
    float CachedCellSize;

    /** @brief 是否启用调试绘制 */
    UPROPERTY()
    bool bDebugDraw = false;

    /** @brief Tick委托句柄 */
    FDelegateHandle TickDelegateHandle;
};
