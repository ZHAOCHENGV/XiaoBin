/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Character/Components/XBFormationComponent.cpp

/**
 * @file XBFormationComponent.cpp
 * @brief 编队组件实现
 * 
 * @note 🔧 修改记录:
 *       1. 新增 CompactSlots() 实现槽位压缩
 *       2. 新增 GetNextSlotIndex() 和 GetOccupiedSlotCount()
 *       3. 优化槽位分配逻辑确保顺序性
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

// ==================== ✨ 新增：槽位管理方法 ====================

/**
 * @brief 获取下一个应分配的槽位索引
 * @param CurrentSoldierCount 当前士兵数量
 * @return 应分配的槽位索引
 * @note 新士兵总是分配到队尾，索引等于当前数量（从0开始）
 */
int32 UXBFormationComponent::GetNextSlotIndex(int32 CurrentSoldierCount) const
{
    // 新士兵的槽位索引 = 当前士兵数量（因为索引从0开始）
    // 例如：当前有0个士兵，新士兵获得槽位0
    //       当前有1个士兵，新士兵获得槽位1
    return CurrentSoldierCount;
}

/**
 * @brief 获取已占用的槽位数量
 * @return 已占用槽位数
 */
int32 UXBFormationComponent::GetOccupiedSlotCount() const
{
    int32 Count = 0;
    for (const FXBFormationSlot& Slot : FormationSlots)
    {
        if (Slot.bOccupied)
        {
            Count++;
        }
    }
    return Count;
}

/**
 * @brief 压缩槽位数组，移除中间的空槽
 * @param Soldiers 当前士兵数组引用
 * @note ✨ 核心逻辑：
 *       1. 遍历士兵数组，按数组顺序重新分配槽位索引
 *       2. 数组中的第i个士兵获得槽位i
 *       3. 通知每个被移动的士兵更新其槽位索引
 *       4. 重新生成编队槽位
 */
void UXBFormationComponent::CompactSlots(const TArray<AXBSoldierCharacter*>& Soldiers)
{
    UE_LOG(LogTemp, Log, TEXT("开始压缩槽位，当前士兵数: %d"), Soldiers.Num());

    // 重新生成正确数量的槽位
    RegenerateFormation(Soldiers.Num());

    // 按数组顺序重新分配槽位
    for (int32 i = 0; i < Soldiers.Num(); ++i)
    {
        AXBSoldierCharacter* Soldier = Soldiers[i];
        if (!Soldier || !IsValid(Soldier))
        {
            continue;
        }

        int32 OldSlotIndex = Soldier->GetFormationSlotIndex();
        int32 NewSlotIndex = i;

        // 如果槽位发生变化，更新士兵并广播事件
        if (OldSlotIndex != NewSlotIndex)
        {
            UE_LOG(LogTemp, Log, TEXT("士兵 %s 槽位变化: %d -> %d"), 
                *Soldier->GetName(), OldSlotIndex, NewSlotIndex);

            // 更新士兵的槽位索引
            Soldier->SetFormationSlotIndex(NewSlotIndex);

            // 广播槽位变化事件
            OnSlotReassigned.Broadcast(OldSlotIndex, NewSlotIndex);
        }

        // 标记槽位为已占用
        if (FormationSlots.IsValidIndex(NewSlotIndex))
        {
            FormationSlots[NewSlotIndex].bOccupied = true;
            FormationSlots[NewSlotIndex].OccupantSoldierId = Soldier->GetUniqueID();
        }
    }

    // 广播编队更新事件
    OnFormationUpdated.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("槽位压缩完成，最终槽位数: %d"), FormationSlots.Num());
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
        FVector(0, 0, 1),
        FVector(1, 0, 0)
    );

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

        FVector LocalOffset3D(Slot.LocalOffset.X, Slot.LocalOffset.Y, 0.0f);
        FVector WorldOffset = LeaderRotation.RotateVector(LocalOffset3D);
        FVector SlotWorldPos = LeaderLocation + WorldOffset;

        FColor SlotColor = Slot.bOccupied ? DebugOccupiedSlotColor : DebugFreeSlotColor;

        DrawDebugCircle(
            World,
            SlotWorldPos,
            DebugSlotRadius,
            24,
            SlotColor,
            false,
            Duration,
            0,
            4.0f,
            FVector(1, 0, 0),
            FVector(0, 1, 0)
        );

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

void UXBFormationComponent::RegenerateFormation(int32 SoldierCount)
{
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
        Slot.bOccupied = false;  // 🔧 修改 - 初始化时不标记占用，由外部调用时设置
        Slot.OccupantSoldierId = INDEX_NONE;
        FormationSlots.Add(Slot);
    }

    OnFormationUpdated.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("编队生成: 槽位数=%d (%dx%d)"), 
        ActualSlotCount, Columns, Rows);
}

FVector UXBFormationComponent::GetSlotWorldPosition(int32 SlotIndex) const
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return FVector::ZeroVector;
    }

    FVector LeaderLocation = Owner->GetActorLocation();
    
    if (!FormationSlots.IsValidIndex(SlotIndex))
    {
        return LeaderLocation;
    }

    const FXBFormationSlot& Slot = FormationSlots[SlotIndex];

    FVector LocalOffset3D(Slot.LocalOffset.X, Slot.LocalOffset.Y, 0.0f);
    FVector WorldOffset = Owner->GetActorRotation().RotateVector(LocalOffset3D);
    
    FVector TargetXY = LeaderLocation + WorldOffset;
    
    UWorld* World = GetWorld();
    if (World)
    {
        FHitResult HitResult;
        
        FVector TraceStart = FVector(TargetXY.X, TargetXY.Y, LeaderLocation.Z + 500.0f);
        FVector TraceEnd = FVector(TargetXY.X, TargetXY.Y, LeaderLocation.Z - 1000.0f);

        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(Owner);

        bool bHit = World->LineTraceSingleByChannel(
            HitResult,
            TraceStart,
            TraceEnd,
            ECC_WorldStatic,
            QueryParams
        );

        if (bHit)
        {
            return HitResult.Location; 
        }
    }

    return FVector(TargetXY.X, TargetXY.Y, LeaderLocation.Z);
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
 * @param Soldier 士兵
 * @return 分配的槽位索引
 * @note 🔧 修改 - 优化分配逻辑，确保顺序分配
 */
int32 UXBFormationComponent::AssignSlotToSoldier(AXBSoldierCharacter* Soldier)
{
    if (!Soldier)
    {
        return INDEX_NONE;
    }

    // 🔧 修改 - 优先使用 GetFirstAvailableSlot 确保从头开始分配
    int32 SlotIndex = GetFirstAvailableSlot();
    
    if (SlotIndex == INDEX_NONE)
    {
        // 没有可用槽位，扩展
        int32 NewSlotCount = FormationSlots.Num() + 1;
        RegenerateFormation(NewSlotCount);
        SlotIndex = FormationSlots.Num() - 1;
    }

    if (OccupySlot(SlotIndex, Soldier->GetUniqueID()))
    {
        Soldier->SetFormationSlotIndex(SlotIndex);
        
        UE_LOG(LogTemp, Log, TEXT("士兵 %s 分配到槽位 %d"), *Soldier->GetName(), SlotIndex);
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

/**
 * @brief 重新分配所有槽位
 * @param Soldiers 士兵数组
 * @note 🔧 修改 - 调用 CompactSlots 确保槽位连续
 */
void UXBFormationComponent::ReassignAllSlots(const TArray<AXBSoldierCharacter*>& Soldiers)
{
    ReleaseAllSlots();
    CompactSlots(Soldiers);

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
