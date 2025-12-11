/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Character/Components/XBFormationComponent.cpp

/**
 * @file XBFormationComponent.cpp
 * @brief 编队组件实现（支持手动槽位数量控制）
 * 
 * @note 🔧 修改记录:
 *       1. 新增 SetFormationSlotCount 方法
 *       2. 修复 DrawDebugCircle 法向量（确保圆圈朝上）
 *       3. BeginPlay 时根据 ManualSlotCount 自动生成槽位
 */

#include "Character/Components/XBFormationComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

UXBFormationComponent::UXBFormationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UXBFormationComponent::BeginPlay()
{
    Super::BeginPlay();

    // ✨ 新增 - 如果设置了手动槽位数量，自动生成
    if (ManualSlotCount > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("=== 使用手动槽位数量: %d ==="), ManualSlotCount);
        RegenerateFormation(ManualSlotCount);
    }

    UE_LOG(LogTemp, Warning, TEXT("=== 编队组件初始化: %s，槽位数: %d，调试: %s ==="), 
        *GetOwner()->GetName(), 
        FormationSlots.Num(),
        bDrawDebug ? TEXT("启用") : TEXT("禁用"));
}

void UXBFormationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bDrawDebug)
    {
        DrawDebugFormation(0.0f);
    }
}

/**
 * @brief 手动设置槽位数量
 * @param Count 槽位数量
 * @note ✨ 新增方法 - 用于运行时调试
 */
void UXBFormationComponent::SetFormationSlotCount(int32 Count)
{
    if (Count < 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("槽位数量不能为负数: %d"), Count);
        return;
    }

    ManualSlotCount = Count;
    RegenerateFormation(Count);

    UE_LOG(LogTemp, Warning, TEXT("★★★ 手动设置槽位数量: %d ★★★"), Count);
}

void UXBFormationComponent::SetDebugDrawEnabled(bool bEnabled)
{
    bDrawDebug = bEnabled;

    if (bEnabled)
    {
        UE_LOG(LogTemp, Error, TEXT("============================================="));
        UE_LOG(LogTemp, Error, TEXT("编队调试绘制已启用: %s"), *GetOwner()->GetName());
        UE_LOG(LogTemp, Error, TEXT("当前槽位数量: %d"), FormationSlots.Num());
        UE_LOG(LogTemp, Error, TEXT("手动槽位数量: %d"), ManualSlotCount);
        UE_LOG(LogTemp, Error, TEXT("============================================="));

        DrawDebugFormation(10.0f);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("编队调试绘制已禁用: %s"), *GetOwner()->GetName());
    }
}

/**
 * @brief 绘制编队调试信息
 * @param Duration 持续时间
 * @note 🔧 修改 - 修复圆圈法向量，确保朝上显示
 */
void UXBFormationComponent::DrawDebugFormation(float Duration)
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();

    if (!World || !Owner)
    {
        return;
    }

    if (FormationSlots.Num() == 0)
    {
        UE_LOG(LogTemp, Verbose, TEXT("槽位数量为 0，跳过绘制"));
        return;
    }

    FVector LeaderLocation = Owner->GetActorLocation();
    FRotator LeaderRotation = Owner->GetActorRotation();

    // ==================== 绘制将领标记 ====================

    // 🔧 修改 - 将领脚底位置的圆圈（红色，法向量向上）
    DrawDebugCircle(
        World,
        LeaderLocation,
        DebugLeaderRadius,
        32,
        DebugLeaderColor,
        false,
        Duration,
        0,
        5.0f,
        FVector(0, 0, 1), // ✅ 法向量：Z轴向上
        FVector(1, 0, 0)  // ✅ 参考向量：X轴向前
    );

    // 将领头顶球体
    DrawDebugSphere(
        World,
        LeaderLocation + FVector(0, 0, 100.0f),
        30.0f,
        16,
        DebugLeaderColor,
        false,
        Duration,
        0,
        3.0f
    );

    // 将领文字
    DrawDebugString(
        World,
        LeaderLocation + FVector(0, 0, 150.0f),
        FString::Printf(TEXT("将领\n槽位数: %d"), FormationSlots.Num()),
        nullptr,
        FColor::White,
        Duration,
        true,
        2.0f
    );

    // ==================== 绘制所有槽位 ====================

    for (int32 i = 0; i < FormationSlots.Num(); ++i)
    {
        const FXBFormationSlot& Slot = FormationSlots[i];

        // 计算槽位世界坐标
        FVector LocalOffset3D(Slot.LocalOffset.X, Slot.LocalOffset.Y, 0.0f);
        FVector WorldOffset = LeaderRotation.RotateVector(LocalOffset3D);
        FVector SlotWorldPos = LeaderLocation + WorldOffset;

        // 确定颜色
        FColor SlotColor = Slot.bOccupied ? DebugOccupiedSlotColor : DebugFreeSlotColor;

        // ✅ 核心修复 - 绘制平面圆圈（法向量向上）
        DrawDebugCircle(
            World,
            SlotWorldPos,
            DebugSlotRadius,
            24, // 段数（更圆滑）
            SlotColor,
            false,
            Duration,
            0,
            4.0f, // 线宽
            FVector(1, 0, 0), // ✅ 关键：法向量Z轴向上，确保圆圈平行于地面
            FVector(0, 1, 0)  // ✅ 参考向量X轴向前
        );

        // 绘制十字标记（定位槽位中心）
        float CrossSize = DebugSlotRadius * 0.5f;
        DrawDebugLine(
            World,
            SlotWorldPos + FVector(-CrossSize, 0, 1),
            SlotWorldPos + FVector(CrossSize, 0, 1),
            SlotColor,
            false,
            Duration,
            0,
            3.0f
        );

        DrawDebugLine(
            World,
            SlotWorldPos + FVector(0, -CrossSize, 1),
            SlotWorldPos + FVector(0, CrossSize, 1),
            SlotColor,
            false,
            Duration,
            0,
            3.0f
        );

        // 绘制序号文字
        FString SlotText = FString::Printf(TEXT("%d"), Slot.SlotIndex);
        DrawDebugString(
            World,
            SlotWorldPos + FVector(0, 0, DebugTextHeightOffset),
            SlotText,
            nullptr,
            DebugTextColor,
            Duration,
            true,
            1.8f
        );

        // 绘制从将领到槽位的连线
        DrawDebugLine(
            World,
            LeaderLocation + FVector(0, 0, 10),
            SlotWorldPos + FVector(0, 0, 10),
            DebugLineColor,
            false,
            Duration,
            0,
            2.0f
        );

        // 如果被占用，绘制占用者信息
        if (Slot.bOccupied && Slot.OccupantSoldierId != INDEX_NONE)
        {
            FString OccupantText = FString::Printf(TEXT("ID:%d"), Slot.OccupantSoldierId);
            DrawDebugString(
                World,
                SlotWorldPos + FVector(0, 0, DebugTextHeightOffset + 30.0f),
                OccupantText,
                nullptr,
                FColor::Cyan,
                Duration,
                true,
                1.3f
            );
        }
    }
}

void UXBFormationComponent::DrawDebugSlot(const FXBFormationSlot& Slot, const FVector& LeaderLocation, 
    const FRotator& LeaderRotation, float Duration)
{
    // 已整合到 DrawDebugFormation
}

// ==================== 以下方法保持不变 ====================

void UXBFormationComponent::CalculateFormationDimensions(int32 SoldierCount, int32& OutColumns, int32& OutRows) const
{
    if (SoldierCount <= 0)
    {
        OutColumns = 0;
        OutRows = 0;
        return;
    }

    if (SoldierCount < 4)
    {
        OutColumns = SoldierCount;
        OutRows = 1;
        return;
    }

    OutRows = 2;
    OutColumns = 4;

    while (OutColumns * OutRows < SoldierCount)
    {
        OutRows++;
        OutColumns = OutRows * 2;

        if (OutRows > 100)
        {
            break;
        }
    }

    OutColumns = FMath::Min(OutColumns, FormationConfig.MaxColumns);

    if (OutColumns > 0)
    {
        OutRows = FMath::CeilToInt(static_cast<float>(SoldierCount) / OutColumns);
    }
}

FVector2D UXBFormationComponent::CalculateSlotLocalOffset(int32 SlotIndex, int32 TotalSoldiers, int32 Columns, int32 Rows) const
{
    if (Columns <= 0 || SlotIndex < 0)
    {
        return FVector2D::ZeroVector;
    }

    int32 Row = SlotIndex / Columns;
    int32 Column = SlotIndex % Columns;

    int32 SoldiersInThisRow = (Row == Rows - 1) ? 
        (TotalSoldiers - Row * Columns) : Columns;

    float HalfWidth = (SoldiersInThisRow - 1) * FormationConfig.HorizontalSpacing * 0.5f;

    float OffsetX = -(FormationConfig.MinDistanceToLeader + Row * FormationConfig.VerticalSpacing);
    float OffsetY = Column * FormationConfig.HorizontalSpacing - HalfWidth;

    return FVector2D(OffsetX, OffsetY);
}

/**
 * @brief 重新生成编队
 * @param SoldierCount 士兵数量
 * @note 🔧 修改 - 支持自动扩展槽位数量
 */
void UXBFormationComponent::RegenerateFormation(int32 SoldierCount)
{
    // ✨ 新增 - 如果手动槽位数量为0，使用传入的士兵数量
    // 如果手动槽位数量大于士兵数量，使用手动槽位数量（允许预留空槽）
    int32 ActualSlotCount = (ManualSlotCount > 0) ? FMath::Max(ManualSlotCount, SoldierCount) : SoldierCount;

    FormationSlots.Empty();

    if (ActualSlotCount <= 0)
    {
        OnFormationUpdated.Broadcast();
        return;
    }

    int32 Columns, Rows;
    CalculateFormationDimensions(ActualSlotCount, Columns, Rows);

    FormationSlots.Reserve(ActualSlotCount);

    for (int32 i = 0; i < ActualSlotCount; ++i)
    {
        FXBFormationSlot Slot;
        Slot.SlotIndex = i;
        Slot.LocalOffset = CalculateSlotLocalOffset(i, ActualSlotCount, Columns, Rows);
        Slot.bOccupied = (i < SoldierCount); // ✨ 新增 - 前面的槽位标记为已占用
        Slot.OccupantSoldierId = INDEX_NONE;
        FormationSlots.Add(Slot);
    }

    OnFormationUpdated.Broadcast();

    UE_LOG(LogTemp, Warning, TEXT("★★★ 编队生成: 槽位数=%d, 士兵数=%d (%dx%d) ★★★"), 
        ActualSlotCount, SoldierCount, Columns, Rows);
}

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

    FVector LocalOffset3D(Slot.LocalOffset.X, Slot.LocalOffset.Y, 0.0f);
    FVector WorldOffset = Owner->GetActorRotation().RotateVector(LocalOffset3D);

    FVector LeaderLocation = Owner->GetActorLocation();
    FVector SlotWorldPosition = LeaderLocation + WorldOffset;
    SlotWorldPosition.Z = LeaderLocation.Z;

    return SlotWorldPosition;
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

int32 UXBFormationComponent::AssignSlotToSoldier(AXBSoldierCharacter* Soldier)
{
    if (!Soldier)
    {
        return INDEX_NONE;
    }

    int32 SlotIndex = GetFirstAvailableSlot();
    if (SlotIndex == INDEX_NONE)
    {
        // ✨ 新增 - 自动扩展槽位
        int32 NewSlotCount = FormationSlots.Num() + 1;
        RegenerateFormation(NewSlotCount);
        SlotIndex = FormationSlots.Num() - 1;
    }

    if (OccupySlot(SlotIndex, Soldier->GetUniqueID()))
    {
        Soldier->SetFormationSlotIndex(SlotIndex);
        return SlotIndex;
    }

    return INDEX_NONE;
}

void UXBFormationComponent::RemoveSoldierFromSlot(AXBSoldierCharacter* Soldier)
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

void UXBFormationComponent::ReassignAllSlots(const TArray<AXBSoldierCharacter*>& Soldiers)
{
    ReleaseAllSlots();
    RegenerateFormation(Soldiers.Num());

    for (int32 i = 0; i < Soldiers.Num(); ++i)
    {
        if (Soldiers[i])
        {
            OccupySlot(i, Soldiers[i]->GetUniqueID());
            Soldiers[i]->SetFormationSlotIndex(i);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("★★★ 编队重新分配: %d个士兵 ★★★"), Soldiers.Num());
}

void UXBFormationComponent::SetFormationConfig(const FXBFormationConfig& NewConfig)
{
    FormationConfig = NewConfig;

    if (FormationSlots.Num() > 0)
    {
        RegenerateFormation(FormationSlots.Num());
    }
}
