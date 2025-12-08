// Copyright XiaoBing Project. All Rights Reserved.

#include "Army/XBFormationManager.h"

// 🔧 修改 - 匹配 Header 中的新签名（增加了 MaxColumns）
void UXBFormationManager::CalculateFormationDimensions(int32 TotalSoldiers, int32 MaxColumns, int32& OutColumns, int32& OutRows)
{
    if (TotalSoldiers <= 0)
    {
        OutRows = 0;
        OutColumns = 0;
        return;
    }

    // 🔧 修改 - 使用传入的 MaxColumns 参数，修复 "未声明标识符" 错误
    OutColumns = FMath::Min(TotalSoldiers, MaxColumns);
    OutRows = FMath::CeilToInt(static_cast<float>(TotalSoldiers) / OutColumns);
}

FVector UXBFormationManager::CalculateSlotOffset(int32 SlotIndex, int32 TotalSoldiers, const FXBFormationConfig& Config)
{
    int32 Rows, Columns;
    // 🔧 修改 - 传入 Config.MaxColumns
    CalculateFormationDimensions(TotalSoldiers, Config.MaxColumns, Rows, Columns);

    if (Columns <= 0)
    {
        return FVector::ZeroVector;
    }

    int32 Row = SlotIndex / Columns;
    int32 Column = SlotIndex % Columns;

    int32 SoldiersInThisRow = (Row == Rows - 1) ? (TotalSoldiers - Row * Columns) : Columns;
    
    // 🔧 修改 - 修复 "必须初始化 const 对象" 错误，直接初始化
    // 🔧 修改 - 修复 Config 成员变量名错误 (使用新增的 HorizontalSpacing 等)
    const float HalfWidth = (SoldiersInThisRow - 1) * Config.HorizontalSpacing * 0.5f;

    // 🔧 修改 - 修复 "必须初始化 const 对象" 错误
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
        // 🔧 修改 - FVector 转换为 FVector2D (Slot使用FVector2D更省内存)
        FVector TempOffset = CalculateSlotOffset(i, TotalSoldiers, Config);
        Slot.LocalOffset = FVector2D(TempOffset.X, TempOffset.Y);
        Slot.bOccupied = false;
        Slot.OccupantSoldierId = INDEX_NONE;
        Slots.Add(Slot);
    }

    return Slots;
}

// ✨ 新增 - 实现 GetWorldSlotPosition
FVector UXBFormationManager::GetWorldSlotPosition(const FVector& LeaderPosition, const FRotator& LeaderRotation, const FVector2D& LocalOffset)
{
    // 🔧 修改 - 修复 FVector2D 到 FVector 的转换错误
    FVector Offset3D(LocalOffset.X, LocalOffset.Y, 0.0f);
    FVector WorldOffset = LeaderRotation.RotateVector(Offset3D);
    return LeaderPosition + WorldOffset;
}

bool UXBFormationManager::IsValidSlotIndex(int32 SlotIndex, int32 TotalSoldiers)
{
    return SlotIndex >= 0 && SlotIndex < TotalSoldiers;
}

// 🔧 修改 - 增加 MaxColumns 参数
void UXBFormationManager::SlotIndexToRowColumn(int32 SlotIndex, int32 TotalSoldiers, int32 MaxColumns, int32& OutRow, int32& OutColumn)
{
    int32 Columns, Rows;
    CalculateFormationDimensions(TotalSoldiers, MaxColumns, Columns, Rows);

    if (Columns <= 0)
    {
        OutRow = 0;
        OutColumn = 0;
        return;
    }

    OutRow = SlotIndex / Columns;
    OutColumn = SlotIndex % Columns;
}

