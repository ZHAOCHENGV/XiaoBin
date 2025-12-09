/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/XBSpatialHashGrid.h

/**
 * @file XBSpatialHashGrid.h
 * @brief 空间哈希网格 - 加速邻居查询，O(1)复杂度
 */

#pragma once

#include "CoreMinimal.h"
#include "XBSpatialHashGrid.generated.h"

class AActor;

/**
 * @brief 空间哈希网格
 * @note 用于加速大量Actor的邻居查询
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBSpatialHashGrid
{
    GENERATED_BODY()

public:
    FXBSpatialHashGrid();

    /**
     * @brief 初始化网格
     * @param InCellSize 单元格大小
     */
    void Initialize(float InCellSize = 100.0f);

    /**
     * @brief 清空所有数据
     */
    void Clear();

    /**
     * @brief 添加Actor到网格
     * @param Actor 要添加的Actor
     */
    void AddActor(AActor* Actor);

    /**
     * @brief 从网格移除Actor
     * @param Actor 要移除的Actor
     */
    void RemoveActor(AActor* Actor);

    /**
     * @brief 更新Actor位置
     * @param Actor 要更新的Actor
     */
    void UpdateActor(AActor* Actor);

    /**
     * @brief 批量更新所有Actor
     */
    void UpdateAll();

    /**
     * @brief 查询范围内的邻居
     * @param Location 查询中心
     * @param Radius 查询半径
     * @param OutNeighbors 输出邻居数组
     * @param ExcludeActor 排除的Actor
     */
    void QueryNeighbors(const FVector& Location, float Radius, TArray<AActor*>& OutNeighbors, AActor* ExcludeActor = nullptr) const;

    /**
     * @brief 查询范围内指定类型的邻居
     * @param Location 查询中心
     * @param Radius 查询半径
     * @param ActorClass 目标类型
     * @param OutNeighbors 输出邻居数组
     * @param ExcludeActor 排除的Actor
     */
    template<typename T>
    void QueryNeighborsOfType(const FVector& Location, float Radius, TArray<T*>& OutNeighbors, AActor* ExcludeActor = nullptr) const;

    /**
     * @brief 获取最近的邻居
     * @param Location 查询位置
     * @param Radius 查询半径
     * @param ExcludeActor 排除的Actor
     * @return 最近的Actor，无则返回nullptr
     */
    AActor* GetNearestNeighbor(const FVector& Location, float Radius, AActor* ExcludeActor = nullptr) const;

private:
    /**
     * @brief 计算位置对应的网格键
     * @param Location 世界位置
     * @return 网格键
     */
    int32 GetCellKey(const FVector& Location) const;

    /**
     * @brief 计算位置对应的网格坐标
     * @param Location 世界位置
     * @return 网格坐标
     */
    FIntVector GetCellCoord(const FVector& Location) const;

    /**
     * @brief 从坐标计算键
     */
    int32 CoordToKey(const FIntVector& Coord) const;

private:
    /** @brief 单元格大小 */
    float CellSize;

    /** @brief 网格数据 <CellKey, Actors> */
    TMap<int32, TArray<AActor*>> Grid;

    /** @brief Actor到Cell的映射 */
    TMap<AActor*, int32> ActorCellMap;

    /** @brief 所有注册的Actor */
    TSet<AActor*> RegisteredActors;
};

// 模板实现
template<typename T>
void FXBSpatialHashGrid::QueryNeighborsOfType(const FVector& Location, float Radius, TArray<T*>& OutNeighbors, AActor* ExcludeActor) const
{
    TArray<AActor*> AllNeighbors;
    QueryNeighbors(Location, Radius, AllNeighbors, ExcludeActor);

    for (AActor* Actor : AllNeighbors)
    {
        if (T* TypedActor = Cast<T>(Actor))
        {
            OutNeighbors.Add(TypedActor);
        }
    }
}
