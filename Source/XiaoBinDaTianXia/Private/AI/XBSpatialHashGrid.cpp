/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/XBSpatialHashGrid.cpp

/**
 * @file XBSpatialHashGrid.cpp
 * @brief 空间哈希网格实现
 */

#include "AI/XBSpatialHashGrid.h"
#include "GameFramework/Actor.h"

FXBSpatialHashGrid::FXBSpatialHashGrid()
    : CellSize(100.0f)
{
}

void FXBSpatialHashGrid::Initialize(float InCellSize)
{
    CellSize = FMath::Max(InCellSize, 10.0f);
    Clear();
    UE_LOG(LogTemp, Log, TEXT("空间哈希网格初始化 - 单元格大小: %.1f"), CellSize);
}

void FXBSpatialHashGrid::Clear()
{
    Grid.Empty();
    ActorCellMap.Empty();
    RegisteredActors.Empty();
}

int32 FXBSpatialHashGrid::GetCellKey(const FVector& Location) const
{
    return CoordToKey(GetCellCoord(Location));
}

FIntVector FXBSpatialHashGrid::GetCellCoord(const FVector& Location) const
{
    return FIntVector(
        FMath::FloorToInt(Location.X / CellSize),
        FMath::FloorToInt(Location.Y / CellSize),
        FMath::FloorToInt(Location.Z / CellSize)
    );
}

int32 FXBSpatialHashGrid::CoordToKey(const FIntVector& Coord) const
{
    // 使用大质数避免哈希碰撞
    const int32 Prime1 = 73856093;
    const int32 Prime2 = 19349663;
    const int32 Prime3 = 83492791;
    return (Coord.X * Prime1) ^ (Coord.Y * Prime2) ^ (Coord.Z * Prime3);
}

void FXBSpatialHashGrid::AddActor(AActor* Actor)
{
    if (!Actor || RegisteredActors.Contains(Actor))
    {
        return;
    }

    RegisteredActors.Add(Actor);

    const int32 CellKey = GetCellKey(Actor->GetActorLocation());
    Grid.FindOrAdd(CellKey).Add(Actor);
    ActorCellMap.Add(Actor, CellKey);
}

void FXBSpatialHashGrid::RemoveActor(AActor* Actor)
{
    if (!Actor || !RegisteredActors.Contains(Actor))
    {
        return;
    }

    if (const int32* OldCellKey = ActorCellMap.Find(Actor))
    {
        if (TArray<AActor*>* Cell = Grid.Find(*OldCellKey))
        {
            Cell->Remove(Actor);
            if (Cell->Num() == 0)
            {
                Grid.Remove(*OldCellKey);
            }
        }
    }

    ActorCellMap.Remove(Actor);
    RegisteredActors.Remove(Actor);
}

void FXBSpatialHashGrid::UpdateActor(AActor* Actor)
{
    if (!Actor || !RegisteredActors.Contains(Actor))
    {
        return;
    }

    const int32 NewCellKey = GetCellKey(Actor->GetActorLocation());

    if (const int32* OldCellKey = ActorCellMap.Find(Actor))
    {
        if (*OldCellKey == NewCellKey)
        {
            return; // 没有移动到新的Cell
        }

        // 从旧Cell移除
        if (TArray<AActor*>* OldCell = Grid.Find(*OldCellKey))
        {
            OldCell->Remove(Actor);
            if (OldCell->Num() == 0)
            {
                Grid.Remove(*OldCellKey);
            }
        }
    }

    // 添加到新Cell
    Grid.FindOrAdd(NewCellKey).Add(Actor);
    ActorCellMap.Add(Actor, NewCellKey);
}

void FXBSpatialHashGrid::UpdateAll()
{
    for (AActor* Actor : RegisteredActors)
    {
        if (Actor && IsValid(Actor))
        {
            UpdateActor(Actor);
        }
    }
}

void FXBSpatialHashGrid::QueryNeighbors(const FVector& Location, float Radius, TArray<AActor*>& OutNeighbors, AActor* ExcludeActor) const
{
    OutNeighbors.Reset();

    const float RadiusSq = Radius * Radius;
    const int32 CellRadius = FMath::CeilToInt(Radius / CellSize);
    const FIntVector CenterCoord = GetCellCoord(Location);

    // 遍历周围的Cell
    for (int32 X = -CellRadius; X <= CellRadius; ++X)
    {
        for (int32 Y = -CellRadius; Y <= CellRadius; ++Y)
        {
            for (int32 Z = -CellRadius; Z <= CellRadius; ++Z)
            {
                const FIntVector Coord = CenterCoord + FIntVector(X, Y, Z);
                const int32 CellKey = CoordToKey(Coord);

                if (const TArray<AActor*>* Cell = Grid.Find(CellKey))
                {
                    for (AActor* Actor : *Cell)
                    {
                        if (Actor && Actor != ExcludeActor && IsValid(Actor))
                        {
                            const float DistSq = FVector::DistSquared(Location, Actor->GetActorLocation());
                            if (DistSq <= RadiusSq)
                            {
                                OutNeighbors.Add(Actor);
                            }
                        }
                    }
                }
            }
        }
    }
}

AActor* FXBSpatialHashGrid::GetNearestNeighbor(const FVector& Location, float Radius, AActor* ExcludeActor) const
{
    TArray<AActor*> Neighbors;
    QueryNeighbors(Location, Radius, Neighbors, ExcludeActor);

    AActor* Nearest = nullptr;
    float MinDistSq = FLT_MAX;

    for (AActor* Actor : Neighbors)
    {
        const float DistSq = FVector::DistSquared(Location, Actor->GetActorLocation());
        if (DistSq < MinDistSq)
        {
            MinDistSq = DistSq;
            Nearest = Actor;
        }
    }

    return Nearest;
}
