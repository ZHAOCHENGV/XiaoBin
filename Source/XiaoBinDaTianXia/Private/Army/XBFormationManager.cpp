// Copyright XiaoBing Project. All Rights Reserved.

#include "Army/XBFormationManager.h"

void UXBFormationManager::CalculateFormationDimensions(int32 TotalSoldiers, int32& OutColumns, int32& OutRows)
{
    if (TotalSoldiers <= 0)
    {
        OutColumns = 0;
        OutRows = 0;
        return;
    }

    // 根据策划文档的规则：
    // 横向＜4，纵向1
    // 横向4，纵向2
    // 横向6，纵向3
    // 横向8，纵向4
    // 规律：Columns = 2 * Rows

    // 推导公式：Rows = ceil(sqrt(TotalSoldiers / 2))
    OutRows = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(TotalSoldiers) / 2.0f));
    OutRows = FMath::Max(OutRows, 1);
    
    OutColumns = OutRows * 2;

    // 特殊情况：士兵数小于等于3时，单行排列
    if (TotalSoldiers <= 3)
    {
        OutColumns = TotalSoldiers;
        OutRows = 1;
    }
}

FVector UXBFormationManager::CalculateSlotOffset(int32 SlotIndex, int32 TotalSoldiers, const FXBFormationConfig& Config)
{
    if (SlotIndex < 0 || TotalSoldiers <= 0)
    {
        return FVector::ZeroVector;
    }

    int32 Columns, Rows;
    CalculateFormationDimensions(TotalSoldiers, Columns, Rows);

    int32 Row, Column;
    SlotIndexToRowColumn(SlotIndex, TotalSoldiers, Row, Column);

    // 计算偏移（将领在前，士兵在后）
    // X轴：向后（负方向）
    // Y轴：左右（正负方向）

    // 计算居中偏移
    const float HalfWidth = (Columns - 1) * Config.HorizontalSpacing * 0.5f;

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
        Slot.bIsOccupied = false;
        Slot.OccupyingSoldierId = INDEX_NONE;
        Slots.Add(Slot);
    }

    return Slots;
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