/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Character/Components/XBFormationComponent.cpp

/**
 * @file XBFormationComponent.cpp
 * @brief 编队组件实现
 * 
 * @note 🔧 修改记录:
 *       1. 实现设计文档的编队规则
 *       2. 完善士兵分配和补位逻辑
 */

#include "Character/Components/XBFormationComponent.h"
#include "Soldier/XBSoldierActor.h"
#include "DrawDebugHelpers.h"

UXBFormationComponent::UXBFormationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UXBFormationComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UXBFormationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if WITH_EDITOR
    if (bDrawDebugInEditor)
    {
        DrawDebugFormation(0.0f);
    }
#endif
}

/**
 * @brief 根据设计文档规则计算编队维度
 * @note 设计文档规则:
 *       横向<4：纵向1
 *       横向4：纵向2
 *       横向6：纵向3
 *       横向8：纵向4
 *       以此类推...
 *       规律: 横向 = 纵向 × 2
 */
void UXBFormationComponent::CalculateFormationDimensions(int32 SoldierCount, int32& OutColumns, int32& OutRows) const
{
    if (SoldierCount <= 0)
    {
        OutColumns = 0;
        OutRows = 0;
        return;
    }

    // 特殊情况：少于4人时横向排列
    if (SoldierCount < 4)
    {
        OutColumns = SoldierCount;
        OutRows = 1;
        return;
    }

    // 🔧 修改 - 实现设计文档规则: 横向 = 纵向 × 2
    // 从最小的配置开始尝试
    // 4x2=8, 6x3=18, 8x4=32...
    // 找到能容纳所有士兵的最小配置
    
    OutRows = 2;  // 从2行开始
    OutColumns = 4;  // 对应4列
    
    while (OutColumns * OutRows < SoldierCount)
    {
        OutRows++;
        OutColumns = OutRows * 2;
        
        // 防止无限循环
        if (OutRows > 100)
        {
            break;
        }
    }

    // 限制最大列数
    OutColumns = FMath::Min(OutColumns, FormationConfig.MaxColumns);
    
    // 重新计算行数
    if (OutColumns > 0)
    {
        OutRows = FMath::CeilToInt(static_cast<float>(SoldierCount) / OutColumns);
    }

    UE_LOG(LogTemp, Log, TEXT("编队计算: 士兵数=%d, 列=%d, 行=%d"), SoldierCount, OutColumns, OutRows);
}

/**
 * @brief 计算槽位本地偏移
 */
FVector2D UXBFormationComponent::CalculateSlotLocalOffset(int32 SlotIndex, int32 TotalSoldiers, int32 Columns, int32 Rows) const
{
    if (Columns <= 0 || SlotIndex < 0)
    {
        return FVector2D::ZeroVector;
    }

    // 计算行列位置
    int32 Row = SlotIndex / Columns;
    int32 Column = SlotIndex % Columns;

    // 计算当前行实际的士兵数（最后一行可能不满）
    int32 SoldiersInThisRow = (Row == Rows - 1) ? 
        (TotalSoldiers - Row * Columns) : Columns;

    // 计算水平中心偏移（使队列居中）
    float HalfWidth = (SoldiersInThisRow - 1) * FormationConfig.HorizontalSpacing * 0.5f;

    // X轴：向后排列（负值表示在将领后方）
    float OffsetX = -(FormationConfig.MinDistanceToLeader + Row * FormationConfig.VerticalSpacing);
    
    // Y轴：左右排列（从中心展开）
    float OffsetY = Column * FormationConfig.HorizontalSpacing - HalfWidth;

    return FVector2D(OffsetX, OffsetY);
}

/**
 * @brief 重新生成编队
 */
void UXBFormationComponent::RegenerateFormation(int32 SoldierCount)
{
    FormationSlots.Empty();

    if (SoldierCount <= 0)
    {
        OnFormationUpdated.Broadcast();
        return;
    }

    int32 Columns, Rows;
    CalculateFormationDimensions(SoldierCount, Columns, Rows);

    FormationSlots.Reserve(SoldierCount);

    for (int32 i = 0; i < SoldierCount; ++i)
    {
        FXBFormationSlot Slot;
        Slot.SlotIndex = i;
        Slot.LocalOffset = CalculateSlotLocalOffset(i, SoldierCount, Columns, Rows);
        Slot.bOccupied = false;
        Slot.OccupantSoldierId = INDEX_NONE;
        FormationSlots.Add(Slot);
    }

    OnFormationUpdated.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("编队生成完成: %d个槽位 (%dx%d)"), SoldierCount, Columns, Rows);
}

/**
 * @brief 获取槽位世界坐标
 */
FVector UXBFormationComponent::GetSlotWorldPosition(int32 SlotIndex) const
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return FVector::ZeroVector;
    }

    if (!FormationSlots.IsValidIndex(SlotIndex))
    {
        return Owner->GetActorLocation();
    }

    const FXBFormationSlot& Slot = FormationSlots[SlotIndex];
    
    // 将本地偏移转换为世界坐标
    FVector LocalOffset3D(Slot.LocalOffset.X, Slot.LocalOffset.Y, 0.0f);
    FVector WorldOffset = Owner->GetActorRotation().RotateVector(LocalOffset3D);
    
    return Owner->GetActorLocation() + WorldOffset;
}

int32 UXBFormationComponent::GetFirstAvailableSlot() const
{
    for (int32 i = 0; i < FormationSlots.Num(); ++i)
    {
        if (!FormationSlots[i].bOccupied)
        {
            return i;
        }
    }
    return INDEX_NONE;
}

bool UXBFormationComponent::OccupySlot(int32 SlotIndex, int32 SoldierId)
{
    if (!FormationSlots.IsValidIndex(SlotIndex))
    {
        return false;
    }

    FXBFormationSlot& Slot = FormationSlots[SlotIndex];
    if (Slot.bOccupied)
    {
        return false;
    }

    Slot.bOccupied = true;
    Slot.OccupantSoldierId = SoldierId;
    return true;
}

bool UXBFormationComponent::ReleaseSlot(int32 SlotIndex)
{
    if (!FormationSlots.IsValidIndex(SlotIndex))
    {
        return false;
    }

    FXBFormationSlot& Slot = FormationSlots[SlotIndex];
    Slot.bOccupied = false;
    Slot.OccupantSoldierId = INDEX_NONE;
    return true;
}

void UXBFormationComponent::ReleaseAllSlots()
{
    for (FXBFormationSlot& Slot : FormationSlots)
    {
        Slot.bOccupied = false;
        Slot.OccupantSoldierId = INDEX_NONE;
    }
}

/**
 * @brief 为士兵分配槽位
 */
int32 UXBFormationComponent::AssignSlotToSoldier(AXBSoldierActor* Soldier)
{
    if (!Soldier)
    {
        return INDEX_NONE;
    }

    // 查找空闲槽位
    int32 SlotIndex = GetFirstAvailableSlot();
    if (SlotIndex == INDEX_NONE)
    {
        // 没有空闲槽位，需要扩展编队
        RegenerateFormation(FormationSlots.Num() + 1);
        SlotIndex = FormationSlots.Num() - 1;
    }

    // 占用槽位
    if (OccupySlot(SlotIndex, Soldier->GetUniqueID()))
    {
        Soldier->SetFormationSlotIndex(SlotIndex);
        return SlotIndex;
    }

    return INDEX_NONE;
}

/**
 * @brief 移除士兵的槽位分配
 */
void UXBFormationComponent::RemoveSoldierFromSlot(AXBSoldierActor* Soldier)
{
    if (!Soldier)
    {
        return;
    }

    int32 SlotIndex = Soldier->GetFormationSlotIndex();
    if (SlotIndex != INDEX_NONE)
    {
        ReleaseSlot(SlotIndex);
    }
}

/**
 * @brief 重新分配所有士兵的槽位（补位逻辑）
 * @note 当士兵死亡后，后面的士兵向前补位
 */
void UXBFormationComponent::ReassignAllSlots(const TArray<AXBSoldierActor*>& Soldiers)
{
    // 释放所有槽位
    ReleaseAllSlots();

    // 重新生成编队（可能需要缩小）
    RegenerateFormation(Soldiers.Num());

    // 按顺序分配
    for (int32 i = 0; i < Soldiers.Num(); ++i)
    {
        if (Soldiers[i])
        {
            OccupySlot(i, Soldiers[i]->GetUniqueID());
            Soldiers[i]->SetFormationSlotIndex(i);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("编队重新分配: %d个士兵"), Soldiers.Num());
}

void UXBFormationComponent::SetFormationConfig(const FXBFormationConfig& NewConfig)
{
    FormationConfig = NewConfig;
    
    // 重新生成编队以应用新配置
    if (FormationSlots.Num() > 0)
    {
        RegenerateFormation(FormationSlots.Num());
    }
}

void UXBFormationComponent::DrawDebugFormation(float Duration)
{
#if ENABLE_DRAW_DEBUG
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    
    if (!World || !Owner)
    {
        return;
    }

    for (const FXBFormationSlot& Slot : FormationSlots)
    {
        FVector WorldPos = GetSlotWorldPosition(Slot.SlotIndex);
        FColor Color = Slot.bOccupied ? FColor::Green : FColor::Yellow;

        // 绘制球体表示槽位
        DrawDebugSphere(World, WorldPos, 25.0f, 8, Color, false, Duration);
        
        // 绘制槽位索引
        DrawDebugString(World, WorldPos + FVector(0, 0, 50), 
            FString::Printf(TEXT("%d"), Slot.SlotIndex), nullptr, FColor::White, Duration);

        // 绘制从将领到槽位的连线
        DrawDebugLine(World, Owner->GetActorLocation(), WorldPos, FColor::Cyan, false, Duration);
    }

    // 绘制将领位置
    DrawDebugSphere(World, Owner->GetActorLocation(), 50.0f, 12, FColor::Red, false, Duration);
#endif
}
