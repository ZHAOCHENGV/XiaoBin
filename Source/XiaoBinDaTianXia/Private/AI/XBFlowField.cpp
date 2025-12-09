/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/XBFlowField.cpp

/**
 * @file XBFlowField.cpp
 * @brief 流场实现
 */

#include "AI/XBFlowField.h"
#include "DrawDebugHelpers.h"

FXBFlowField::FXBFlowField()
    : CellSize(100.0f)
    , GridWidth(0)
    , GridHeight(0)
{
}

void FXBFlowField::Initialize(const FVector2D& InWorldOrigin, const FVector2D& InWorldSize, float InCellSize)
{
    WorldOrigin = InWorldOrigin;
    WorldSize = InWorldSize;
    CellSize = FMath::Max(InCellSize, 10.0f);

    GridWidth = FMath::CeilToInt(WorldSize.X / CellSize);
    GridHeight = FMath::CeilToInt(WorldSize.Y / CellSize);

    Cells.SetNum(GridWidth * GridHeight);

    // 初始化所有单元格
    for (FXBFlowFieldCell& Cell : Cells)
    {
        Cell.Cost = 1;
        Cell.IntegrationValue = UINT16_MAX;
        Cell.FlowDirection = FVector2D::ZeroVector;
    }

    UE_LOG(LogTemp, Log, TEXT("流场初始化 - 网格大小: %dx%d, 单元格: %.1f, 原点: (%.1f, %.1f)"),
        GridWidth, GridHeight, CellSize, WorldOrigin.X, WorldOrigin.Y);
}

void FXBFlowField::Clear()
{
    for (FXBFlowFieldCell& Cell : Cells)
    {
        Cell.Cost = 1;
        Cell.IntegrationValue = UINT16_MAX;
        Cell.FlowDirection = FVector2D::ZeroVector;
    }
}

void FXBFlowField::SetTarget(const FVector& TargetWorldLocation)
{
    TargetLocation = FVector2D(TargetWorldLocation.X, TargetWorldLocation.Y);
    TargetGridCoord = WorldToGrid(TargetWorldLocation);

    // 重新计算流场
    GenerateCostField();
    GenerateIntegrationField();
    GenerateFlowField();
}

void FXBFlowField::SetObstacle(const FVector& WorldLocation, float Radius)
{
    const FIntPoint Center = WorldToGrid(WorldLocation);
    const int32 CellRadius = FMath::CeilToInt(Radius / CellSize);

    for (int32 Y = Center.Y - CellRadius; Y <= Center.Y + CellRadius; ++Y)
    {
        for (int32 X = Center.X - CellRadius; X <= Center.X + CellRadius; ++X)
        {
            if (IsValidGridCoord(X, Y))
            {
                const FVector CellWorld = GridToWorld(FIntPoint(X, Y));
                const float Dist = FVector::Dist2D(WorldLocation, CellWorld);
                if (Dist <= Radius)
                {
                    if (FXBFlowFieldCell* Cell = GetCell(X, Y))
                    {
                        Cell->Cost = 255; // 不可通行
                    }
                }
            }
        }
    }
}

void FXBFlowField::ClearObstacle(const FVector& WorldLocation, float Radius)
{
    const FIntPoint Center = WorldToGrid(WorldLocation);
    const int32 CellRadius = FMath::CeilToInt(Radius / CellSize);

    for (int32 Y = Center.Y - CellRadius; Y <= Center.Y + CellRadius; ++Y)
    {
        for (int32 X = Center.X - CellRadius; X <= Center.X + CellRadius; ++X)
        {
            if (IsValidGridCoord(X, Y))
            {
                const FVector CellWorld = GridToWorld(FIntPoint(X, Y));
                const float Dist = FVector::Dist2D(WorldLocation, CellWorld);
                if (Dist <= Radius)
                {
                    if (FXBFlowFieldCell* Cell = GetCell(X, Y))
                    {
                        Cell->Cost = 1; // 恢复可通行
                    }
                }
            }
        }
    }
}

FVector FXBFlowField::GetFlowDirection(const FVector& WorldLocation) const
{
    const FIntPoint GridCoord = WorldToGrid(WorldLocation);

    if (!IsValidGridCoord(GridCoord.X, GridCoord.Y))
    {
        // 超出范围，直接朝向目标
        const FVector2D Dir = TargetLocation - FVector2D(WorldLocation.X, WorldLocation.Y);
        if (Dir.IsNearlyZero())
        {
            return FVector::ZeroVector;
        }
        return FVector(Dir.GetSafeNormal(), 0.0f);
    }

    const FXBFlowFieldCell* Cell = GetCell(GridCoord.X, GridCoord.Y);
    if (!Cell || Cell->IsBlocked())
    {
        return FVector::ZeroVector;
    }

    return FVector(Cell->FlowDirection.X, Cell->FlowDirection.Y, 0.0f);
}

FVector FXBFlowField::GetFlowDirectionSmooth(const FVector& WorldLocation) const
{
    const FIntPoint GridCoord = WorldToGrid(WorldLocation);

    if (!IsValidGridCoord(GridCoord.X, GridCoord.Y))
    {
        return GetFlowDirection(WorldLocation);
    }

    // 双线性插值
    const FVector CellWorld = GridToWorld(GridCoord);
    const float Tx = (WorldLocation.X - CellWorld.X) / CellSize;
    const float Ty = (WorldLocation.Y - CellWorld.Y) / CellSize;

    FVector2D D00 = FVector2D::ZeroVector;
    FVector2D D10 = FVector2D::ZeroVector;
    FVector2D D01 = FVector2D::ZeroVector;
    FVector2D D11 = FVector2D::ZeroVector;

    if (const FXBFlowFieldCell* C00 = GetCell(GridCoord.X, GridCoord.Y))
        D00 = C00->FlowDirection;
    if (const FXBFlowFieldCell* C10 = GetCell(GridCoord.X + 1, GridCoord.Y))
        D10 = C10->FlowDirection;
    if (const FXBFlowFieldCell* C01 = GetCell(GridCoord.X, GridCoord.Y + 1))
        D01 = C01->FlowDirection;
    if (const FXBFlowFieldCell* C11 = GetCell(GridCoord.X + 1, GridCoord.Y + 1))
        D11 = C11->FlowDirection;

    // 双线性插值
    const FVector2D D0 = FMath::Lerp(D00, D10, Tx);
    const FVector2D D1 = FMath::Lerp(D01, D11, Tx);
    const FVector2D Result = FMath::Lerp(D0, D1, Ty);

    if (Result.IsNearlyZero())
    {
        return FVector::ZeroVector;
    }

    return FVector(Result.GetSafeNormal(), 0.0f);
}

bool FXBFlowField::IsWalkable(const FVector& WorldLocation) const
{
    const FIntPoint GridCoord = WorldToGrid(WorldLocation);
    if (!IsValidGridCoord(GridCoord.X, GridCoord.Y))
    {
        return false;
    }

    const FXBFlowFieldCell* Cell = GetCell(GridCoord.X, GridCoord.Y);
    return Cell && !Cell->IsBlocked();
}

void FXBFlowField::SetCellBlocked(int32 X, int32 Y, bool bBlocked)
{
    if (FXBFlowFieldCell* Cell = GetCell(X, Y))
    {
        Cell->Cost = bBlocked ? 255 : 1;
        
        // 如果设置了障碍物，清除该格子的流向
        if (bBlocked)
        {
            Cell->FlowDirection = FVector2D::ZeroVector;
            Cell->IntegrationValue = UINT16_MAX;
        }
    }
}

void FXBFlowField::ClearAllBlockedCells()
{
    for (FXBFlowFieldCell& Cell : Cells)
    {
        if (Cell.IsBlocked())
        {
            Cell.Cost = 1;
        }
    }
}

bool FXBFlowField::IsCellBlocked(int32 X, int32 Y) const
{
    if (const FXBFlowFieldCell* Cell = GetCell(X, Y))
    {
        return Cell->IsBlocked();
    }
    return true; // 无效格子视为阻塞
}

void FXBFlowField::GenerateCostField()
{
    // 代价场已在SetObstacle中设置
    // 这里可以添加地形代价等
}

void FXBFlowField::GenerateIntegrationField()
{
    // 重置积分值
    for (FXBFlowFieldCell& Cell : Cells)
    {
        Cell.IntegrationValue = UINT16_MAX;
    }

    // 检查目标是否有效
    if (!IsValidGridCoord(TargetGridCoord.X, TargetGridCoord.Y))
    {
        UE_LOG(LogTemp, Warning, TEXT("流场目标坐标无效: (%d, %d)"), TargetGridCoord.X, TargetGridCoord.Y);
        return;
    }

    // BFS从目标开始扩散
    TQueue<FIntPoint> OpenList;

    if (FXBFlowFieldCell* TargetCell = GetCell(TargetGridCoord.X, TargetGridCoord.Y))
    {
        TargetCell->IntegrationValue = 0;
        OpenList.Enqueue(TargetGridCoord);
    }

    // 8方向邻居偏移
    static const FIntPoint Neighbors[] = {
        FIntPoint(-1, 0), FIntPoint(1, 0), FIntPoint(0, -1), FIntPoint(0, 1),
        FIntPoint(-1, -1), FIntPoint(1, -1), FIntPoint(-1, 1), FIntPoint(1, 1)
    };
    static const uint16 NeighborCosts[] = {
        10, 10, 10, 10, // 正交方向代价
        14, 14, 14, 14  // 对角方向代价 (约 10 * sqrt(2))
    };

    FIntPoint Current;
    while (OpenList.Dequeue(Current))
    {
        const FXBFlowFieldCell* CurrentCell = GetCell(Current.X, Current.Y);
        if (!CurrentCell)
        {
            continue;
        }

        for (int32 i = 0; i < 8; ++i)
        {
            const FIntPoint NeighborCoord = Current + Neighbors[i];

            if (!IsValidGridCoord(NeighborCoord.X, NeighborCoord.Y))
            {
                continue;
            }

            FXBFlowFieldCell* NeighborCell = GetCell(NeighborCoord.X, NeighborCoord.Y);
            if (!NeighborCell || NeighborCell->IsBlocked())
            {
                continue;
            }

            const uint16 NewValue = CurrentCell->IntegrationValue + NeighborCosts[i] * NeighborCell->Cost;
            if (NewValue < NeighborCell->IntegrationValue)
            {
                NeighborCell->IntegrationValue = NewValue;
                OpenList.Enqueue(NeighborCoord);
            }
        }
    }
}

void FXBFlowField::GenerateFlowField()
{
    // 8方向邻居
    static const FIntPoint Neighbors[] = {
        FIntPoint(-1, 0), FIntPoint(1, 0), FIntPoint(0, -1), FIntPoint(0, 1),
        FIntPoint(-1, -1), FIntPoint(1, -1), FIntPoint(-1, 1), FIntPoint(1, 1)
    };
    static const FVector2D NeighborDirs[] = {
        FVector2D(-1, 0), FVector2D(1, 0), FVector2D(0, -1), FVector2D(0, 1),
        FVector2D(-1, -1).GetSafeNormal(), FVector2D(1, -1).GetSafeNormal(),
        FVector2D(-1, 1).GetSafeNormal(), FVector2D(1, 1).GetSafeNormal()
    };

    for (int32 Y = 0; Y < GridHeight; ++Y)
    {
        for (int32 X = 0; X < GridWidth; ++X)
        {
            FXBFlowFieldCell* Cell = GetCell(X, Y);
            if (!Cell || Cell->IsBlocked())
            {
                continue;
            }

            // 找到积分值最小的邻居
            uint16 MinValue = Cell->IntegrationValue;
            FVector2D BestDir = FVector2D::ZeroVector;

            for (int32 i = 0; i < 8; ++i)
            {
                const FIntPoint NeighborCoord = FIntPoint(X, Y) + Neighbors[i];
                if (!IsValidGridCoord(NeighborCoord.X, NeighborCoord.Y))
                {
                    continue;
                }

                const FXBFlowFieldCell* NeighborCell = GetCell(NeighborCoord.X, NeighborCoord.Y);
                if (!NeighborCell || NeighborCell->IsBlocked())
                {
                    continue;
                }

                if (NeighborCell->IntegrationValue < MinValue)
                {
                    MinValue = NeighborCell->IntegrationValue;
                    BestDir = NeighborDirs[i];
                }
            }

            Cell->FlowDirection = BestDir;
        }
    }
}

FIntPoint FXBFlowField::WorldToGrid(const FVector& WorldLocation) const
{
    return FIntPoint(
        FMath::FloorToInt((WorldLocation.X - WorldOrigin.X) / CellSize),
        FMath::FloorToInt((WorldLocation.Y - WorldOrigin.Y) / CellSize)
    );
}

FVector FXBFlowField::GridToWorld(const FIntPoint& GridCoord) const
{
    return FVector(
        WorldOrigin.X + (GridCoord.X + 0.5f) * CellSize,
        WorldOrigin.Y + (GridCoord.Y + 0.5f) * CellSize,
        0.0f
    );
}

int32 FXBFlowField::GetGridIndex(int32 X, int32 Y) const
{
    return Y * GridWidth + X;
}

bool FXBFlowField::IsValidGridCoord(int32 X, int32 Y) const
{
    return X >= 0 && X < GridWidth && Y >= 0 && Y < GridHeight;
}

FXBFlowFieldCell* FXBFlowField::GetCell(int32 X, int32 Y)
{
    if (!IsValidGridCoord(X, Y))
    {
        return nullptr;
    }
    return &Cells[GetGridIndex(X, Y)];
}

const FXBFlowFieldCell* FXBFlowField::GetCell(int32 X, int32 Y) const
{
    if (!IsValidGridCoord(X, Y))
    {
        return nullptr;
    }
    return &Cells[GetGridIndex(X, Y)];
}

void FXBFlowField::DebugDraw(UWorld* World, float Duration) const
{
    if (!World)
    {
        return;
    }

    for (int32 Y = 0; Y < GridHeight; ++Y)
    {
        for (int32 X = 0; X < GridWidth; ++X)
        {
            const FXBFlowFieldCell* Cell = GetCell(X, Y);
            if (!Cell)
            {
                continue;
            }

            const FVector CellCenter = GridToWorld(FIntPoint(X, Y));

            if (Cell->IsBlocked())
            {
                // 绘制障碍物
                DrawDebugBox(World, CellCenter, FVector(CellSize * 0.4f), FColor::Red, false, Duration);
            }
            else if (!Cell->FlowDirection.IsNearlyZero())
            {
                // 绘制流向箭头
                const FVector ArrowEnd = CellCenter + FVector(Cell->FlowDirection.X, Cell->FlowDirection.Y, 0.0f) * CellSize * 0.4f;
                DrawDebugDirectionalArrow(World, CellCenter, ArrowEnd, 5.0f, FColor::Green, false, Duration);
            }
        }
    }

    // 绘制目标
    const FVector TargetWorld = GridToWorld(TargetGridCoord);
    DrawDebugSphere(World, TargetWorld, 30.0f, 8, FColor::Yellow, false, Duration);
}
