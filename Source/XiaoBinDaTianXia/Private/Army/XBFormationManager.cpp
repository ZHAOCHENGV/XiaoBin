// Copyright XiaoBing Project. All Rights Reserved.

#include "Army/XBFormationManager.h"

void UXBFormationManager::CalculateFormationDimensions(int32 TotalSoldiers, int32& OutColumns, int32& OutRows)
{
    if (TotalSoldiers <= 0)
    {
        OutRows = 0;
        OutColumns = 0;
        return;
    }

    OutColumns = FMath::Min(TotalSoldiers, MaxColumns);
    OutRows = FMath::CeilToInt(static_cast<float>(TotalSoldiers) / OutColumns);
}

FVector UXBFormationManager::CalculateSlotOffset(int32 SlotIndex, int32 TotalSoldiers, const FXBFormationConfig& Config)
{
    int32 Rows, Columns;
    CalculateFormationDimensions(TotalSoldiers, Config.MaxColumns, Rows, Columns);

    if (Columns <= 0)
    {
        return FVector::ZeroVector;
    }

    int32 Row = SlotIndex / Columns;
    int32 Column = SlotIndex % Columns;

    // 计算当前行的实际列数
    int32 SoldiersInThisRow = (Row == Rows - 1) ? (TotalSoldiers - Row * Columns) : Columns;
    
    // 计算半宽（用于居中）
    const float HalfWidth = (SoldiersInThisRow - 1) * Config.HorizontalSpacing * 0.5f;

    // X 为后方偏移（负值），Y 为左右偏移
    const float OffsetX = -(Config.MinDistanceToLeader + Row * Config.VerticalSpacing);
    const float OffsetY = Column * Config.HorizontalSpacing - HalfWidth;

    return FVector(OffsetX, OffsetY, 0.0f);
}

TArray<FXBFormationSlot> UXBFormationManager::GenerateFormationSlots(int32 TotalSoldiers, const FXBFormationConfig& Config)
{
    TArray<FXBFormationSlot> Slots;
    Slots.Reserve(TotalSoldiers);

    for (int32 i = 0; i < TotalSoldiers; ++i)
    {
        FXBFormationSlot Slot;
        Slot.SlotIndex = i;
        Slot.LocalOffset = CalculateSlotOffset(i, TotalSoldiers, Config);
        Slot.bOccupied = false;
        Slot.OccupantSoldierId = INDEX_NONE;
        Slots.Add(Slot);
    }

    return Slots;
}

FVector UXBFormationManager::GetWorldSlotPosition(const FVector& LeaderPosition, const FRotator& LeaderRotation, const FVector& LocalOffset)
{
    FVector WorldOffset = LeaderRotation.RotateVector(LocalOffset);
    return LeaderPosition + WorldOffset;
}

bool UXBFormationManager::IsValidSlotIndex(int32 SlotIndex, int32 TotalSoldiers)
{
    return SlotIndex >= 0 && SlotIndex < TotalSoldiers;
}

void UXBFormationManager::SlotIndexToRowColumn(int32 SlotIndex, int32 TotalSoldiers, int32& OutRow, int32& OutColumn)
{
    int32 Columns, Rows;
    CalculateFormationDimensions(TotalSoldiers, Columns, Rows);

    if (Columns <= 0)
    {
        OutRow = 0;
        OutColumn = 0;
        return;
    }

    OutRow = SlotIndex / Columns;
    OutColumn = SlotIndex % Columns;
}