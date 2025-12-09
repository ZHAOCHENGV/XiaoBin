/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/XBFlowField.h

/**
 * @file XBFlowField.h
 * @brief 流场数据结构 - 存储寻路方向信息
 */

#pragma once

#include "CoreMinimal.h"
#include "XBFlowField.generated.h"

/**
 * @brief 流场单元格数据
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBFlowFieldCell
{
    GENERATED_BODY()

    /** @brief 移动代价 (255 = 不可通行) */
    UPROPERTY()
    uint8 Cost = 1;

    /** @brief 积分值 (到目标的累计代价) */
    UPROPERTY()
    uint16 IntegrationValue = UINT16_MAX;

    /** @brief 流向 (指向目标的方向) */
    UPROPERTY()
    FVector2D FlowDirection = FVector2D::ZeroVector;

    /** @brief 是否为障碍物 */
    bool IsBlocked() const { return Cost == 255; }
};

/**
 * @brief 流场类
 * @note 管理单个目标的流场数据
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBFlowField
{
    GENERATED_BODY()

public:
    FXBFlowField();

    /**
     * @brief 初始化流场
     * @param InWorldOrigin 世界原点
     * @param InWorldSize 世界大小
     * @param InCellSize 单元格大小
     */
    void Initialize(const FVector2D& InWorldOrigin, const FVector2D& InWorldSize, float InCellSize);

    /**
     * @brief 清空流场
     */
    void Clear();

    /**
     * @brief 设置目标位置并重新计算流场
     * @param TargetLocation 目标世界位置
     */
    void SetTarget(const FVector& TargetLocation);

    /**
     * @brief 设置障碍物
     * @param WorldLocation 障碍物世界位置
     * @param Radius 障碍物半径
     */
    void SetObstacle(const FVector& WorldLocation, float Radius);

    /**
     * @brief 清除障碍物
     * @param WorldLocation 障碍物世界位置
     * @param Radius 障碍物半径
     */
    void ClearObstacle(const FVector& WorldLocation, float Radius);

    /**
     * @brief 获取指定位置的流向
     * @param WorldLocation 世界位置
     * @return 流向 (归一化的2D方向)
     */
    FVector GetFlowDirection(const FVector& WorldLocation) const;

    /**
     * @brief 获取指定位置的流向 (带插值)
     * @param WorldLocation 世界位置
     * @return 平滑的流向
     */
    FVector GetFlowDirectionSmooth(const FVector& WorldLocation) const;

    /**
     * @brief 检查位置是否可通行
     * @param WorldLocation 世界位置
     * @return 是否可通行
     */
    bool IsWalkable(const FVector& WorldLocation) const;

    /**
     * @brief 获取调试绘制数据
     */
    void DebugDraw(UWorld* World, float Duration = 0.0f) const;

    /**
     * @brief 是否有效
     */
    bool IsValid() const { return GridWidth > 0 && GridHeight > 0; }

    /**
     * @brief 获取目标位置
     */
    FVector GetTargetLocation() const { return FVector(TargetLocation.X, TargetLocation.Y, 0.0f); }

    /**
    * @brief 获取网格宽度
    */
    int32 GetGridWidth() const { return GridWidth; }

    /**
     * @brief 获取网格高度
     */
    int32 GetGridHeight() const { return GridHeight; }

    /**
     * @brief 获取单元格大小
     */
    float GetCellSize() const { return CellSize; }

    /**
     * @brief 获取世界原点
     */
    FVector2D GetWorldOrigin() const { return WorldOrigin; }

    /**
     * @brief 获取指定网格坐标的单元格（只读）
     * @param X X坐标
     * @param Y Y坐标
     * @return 单元格指针，无效返回nullptr
     */
    const FXBFlowFieldCell* GetCellAt(int32 X, int32 Y) const { return GetCell(X, Y); }

    /**
     * @brief 网格坐标转世界坐标（公开版本）
     */
    FVector GridToWorldPublic(int32 X, int32 Y) const { return GridToWorld(FIntPoint(X, Y)); }

    /**
  * @brief 设置指定格子为阻塞状态
  * @param X X坐标
  * @param Y Y坐标
  * @param bBlocked 是否阻塞
  */
    void SetCellBlocked(int32 X, int32 Y, bool bBlocked);

    /**
     * @brief 清除所有阻塞格子
     */
    void ClearAllBlockedCells();

    /**
     * @brief 检查指定格子是否被阻塞
     * @param X X坐标
     * @param Y Y坐标
     * @return 是否被阻塞
     */
    bool IsCellBlocked(int32 X, int32 Y) const;
    
    
private:
    /**
     * @brief 生成代价场
     */
    void GenerateCostField();

    /**
     * @brief 生成积分场
     */
    void GenerateIntegrationField();

    /**
     * @brief 生成流场
     */
    void GenerateFlowField();

    /**
     * @brief 世界坐标转网格坐标
     */
    FIntPoint WorldToGrid(const FVector& WorldLocation) const;

    /**
     * @brief 网格坐标转世界坐标
     */
    FVector GridToWorld(const FIntPoint& GridCoord) const;

    /**
     * @brief 获取网格索引
     */
    int32 GetGridIndex(int32 X, int32 Y) const;

    /**
     * @brief 检查网格坐标是否有效
     */
    bool IsValidGridCoord(int32 X, int32 Y) const;

    /**
     * @brief 获取单元格
     */
    FXBFlowFieldCell* GetCell(int32 X, int32 Y);
    const FXBFlowFieldCell* GetCell(int32 X, int32 Y) const;

private:
    /** @brief 世界原点 (左下角) */
    FVector2D WorldOrigin;

    /** @brief 世界大小 */
    FVector2D WorldSize;

    /** @brief 单元格大小 */
    float CellSize;

    /** @brief 网格宽度 (单元格数) */
    int32 GridWidth;

    /** @brief 网格高度 (单元格数) */
    int32 GridHeight;

    /** @brief 目标位置 */
    FVector2D TargetLocation;

    /** @brief 目标网格坐标 */
    FIntPoint TargetGridCoord;

    /** @brief 网格数据 */
    TArray<FXBFlowFieldCell> Cells;
};
