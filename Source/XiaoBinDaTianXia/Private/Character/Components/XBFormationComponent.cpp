/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Character/Components/XBFormationComponent.cpp

/**
 * @file XBFormationComponent.cpp
 * @brief 编队组件实现
 * 
 * @note 🔧 修改记录:
 *       1. ✅ 修复 PostEditChangeProperty 无法检测结构体内部修改
 *       2. ✨ 新增 PreEditChange 记录旧配置
 *       3. ✨ 新增详细调试日志
 *       4. 🔧 BeginPlay 强制刷新配置
 */

#include "Character/Components/XBFormationComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "DrawDebugHelpers.h"
#include "Character/XBCharacterBase.h"
#include "Engine/World.h"

UXBFormationComponent::UXBFormationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UXBFormationComponent::BeginPlay()
{
    Super::BeginPlay();

    // 🔧 修改 - 运行时强制应用编辑器配置
    if (ManualSlotCount > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("=== 使用手动槽位数量: %d ==="), ManualSlotCount);
        RegenerateFormation(ManualSlotCount);
    }
    else
    {
        // ✨ 新增 - 强制重建一次，确保配置生效
        int32 CurrentCount = FormationSlots.Num();
        if (CurrentCount > 0)
        {
            RegenerateFormation(CurrentCount);
            UE_LOG(LogTemp, Warning, TEXT("=== 强制重建编队，槽位数: %d ==="), CurrentCount);
        }
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

// ==================== ✅ 修复：增强的编辑器回调 ====================

#if WITH_EDITOR
void UXBFormationComponent::PreEditChange(FProperty* PropertyAboutToChange)
{
    Super::PreEditChange(PropertyAboutToChange);
    
    // 记录旧配置
    OldFormationConfig = FormationConfig;
}

void UXBFormationComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // 🔧 关键修复 - 获取所有相关属性名
    FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? 
        PropertyChangedEvent.Property->GetFName() : NAME_None;
    
    FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ?
        PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;
    
    // ✨ 新增 - 调试日志
    UE_LOG(LogTemp, Log, TEXT("PostEditChangeProperty 触发:"));
    UE_LOG(LogTemp, Log, TEXT("  - PropertyName: %s"), *PropertyName.ToString());
    UE_LOG(LogTemp, Log, TEXT("  - MemberPropertyName: %s"), *MemberPropertyName.ToString());
    
    // 🔧 关键修复 - 检查是否修改了 FormationConfig 相关内容
    bool bFormationConfigChanged = false;
    
    // 情况1：直接修改 FormationConfig 本身
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UXBFormationComponent, FormationConfig))
    {
        bFormationConfigChanged = true;
        UE_LOG(LogTemp, Log, TEXT("  -> FormationConfig 直接修改"));
    }
    // 情况2：修改 FormationConfig 内部字段
    else if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UXBFormationComponent, FormationConfig))
    {
        bFormationConfigChanged = true;
        UE_LOG(LogTemp, Log, TEXT("  -> FormationConfig 内部字段修改: %s"), *PropertyName.ToString());
    }
    // 情况3：修改 ManualSlotCount
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UXBFormationComponent, ManualSlotCount))
    {
        bFormationConfigChanged = true;
        UE_LOG(LogTemp, Log, TEXT("  -> ManualSlotCount 修改"));
    }
    // 情况4：批量修改或未知来源
    else if (PropertyName == NAME_None)
    {
        bFormationConfigChanged = true;
        UE_LOG(LogTemp, Log, TEXT("  -> 批量修改或未知来源"));
    }
    
    if (bFormationConfigChanged)
    {
        UE_LOG(LogTemp, Warning, TEXT(""));
        UE_LOG(LogTemp, Warning, TEXT("============================================="));
        UE_LOG(LogTemp, Warning, TEXT("✅ 编队配置已修改，开始重建..."));
        UE_LOG(LogTemp, Warning, TEXT("============================================="));
        UE_LOG(LogTemp, Log, TEXT("配置详情:"));
        UE_LOG(LogTemp, Log, TEXT("  - HorizontalSpacing: %.1f"), FormationConfig.HorizontalSpacing);
        UE_LOG(LogTemp, Log, TEXT("  - VerticalSpacing: %.1f"), FormationConfig.VerticalSpacing);
        UE_LOG(LogTemp, Log, TEXT("  - MinDistanceToLeader: %.1f"), FormationConfig.MinDistanceToLeader);
        UE_LOG(LogTemp, Log, TEXT("  - MaxColumns: %d"), FormationConfig.MaxColumns);
        UE_LOG(LogTemp, Log, TEXT("  - ManualSlotCount: %d"), ManualSlotCount);
        
        // 确保使用当前的槽位数量
        int32 TargetCount = (ManualSlotCount > 0) ? ManualSlotCount : FormationSlots.Num();
        
        // 至少生成1个以便调试
        if (TargetCount <= 0 && bDrawDebug)
        {
            TargetCount = 1;
            UE_LOG(LogTemp, Warning, TEXT("  - 调试模式：生成1个槽位用于测试"));
        }

        RegenerateFormation(TargetCount);
        
        UE_LOG(LogTemp, Warning, TEXT("✅ 编队重建完成，槽位数: %d"), FormationSlots.Num());
        UE_LOG(LogTemp, Warning, TEXT("============================================="));
        UE_LOG(LogTemp, Warning, TEXT(""));
    }
}
#endif

// ==================== 槽位管理方法 ====================

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

int32 UXBFormationComponent::GetNextSlotIndex(int32 CurrentSoldierCount) const
{
    return CurrentSoldierCount;
}

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
        UE_LOG(LogTemp, Log, TEXT("============================================="));
        UE_LOG(LogTemp, Log, TEXT("编队调试绘制已启用: %s"), *GetOwner()->GetName());
        UE_LOG(LogTemp, Log, TEXT("当前槽位数量: %d"), FormationSlots.Num());
        UE_LOG(LogTemp, Log, TEXT("手动槽位数量: %d"), ManualSlotCount);
        UE_LOG(LogTemp, Log, TEXT("配置详情:"));
        UE_LOG(LogTemp, Log, TEXT("  - HorizontalSpacing: %.1f"), FormationConfig.HorizontalSpacing);
        UE_LOG(LogTemp, Log, TEXT("  - VerticalSpacing: %.1f"), FormationConfig.VerticalSpacing);
        UE_LOG(LogTemp, Log, TEXT("  - MinDistanceToLeader: %.1f"), FormationConfig.MinDistanceToLeader);
        UE_LOG(LogTemp, Log, TEXT("============================================="));

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

    // ✨ 使用配置中的间距
    float OffsetX = -(FormationConfig.MinDistanceToLeader + Row * FormationConfig.VerticalSpacing);
    float OffsetY = Column * FormationConfig.HorizontalSpacing - HalfWidth;

    return FVector2D(OffsetX, OffsetY);
}

void UXBFormationComponent::RegenerateFormation(int32 SoldierCount)
{
    // ✨ 诊断日志（调试用，使用 Log 级别避免打包报错）
    UE_LOG(LogTemp, Log, TEXT(""));
    UE_LOG(LogTemp, Log, TEXT("============================================="));
    UE_LOG(LogTemp, Log, TEXT("RegenerateFormation 诊断"));
    UE_LOG(LogTemp, Log, TEXT("============================================="));
    UE_LOG(LogTemp, Log, TEXT("组件实例: %s"), *GetName());
    UE_LOG(LogTemp, Log, TEXT("组件所有者: %s"), GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"));
    UE_LOG(LogTemp, Log, TEXT("组件地址: %p"), this);
    // 检查是否是同一个实例
    if (GetOwner())
    {
        if (AXBCharacterBase* Character = Cast<AXBCharacterBase>(GetOwner()))
        {
            UXBFormationComponent* OwnerComp = Character->GetFormationComponent();
            if (OwnerComp)
            {
                UE_LOG(LogTemp, Log, TEXT("Owner的组件地址: %p"), OwnerComp);
                UE_LOG(LogTemp, Log, TEXT("是否同一实例: %s"), 
                    (OwnerComp == this) ? TEXT("✅ 是") : TEXT("❌ 否"));
            }
        }
    }
    UE_LOG(LogTemp, Log, TEXT("============================================="));
    UE_LOG(LogTemp, Log, TEXT(""));
    
    // ✨ 新增 - 详细调试日志
    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("=== RegenerateFormation 调用 ==="));
    UE_LOG(LogTemp, Log, TEXT("参数:"));
    UE_LOG(LogTemp, Log, TEXT("  - SoldierCount: %d"), SoldierCount);
    UE_LOG(LogTemp, Log, TEXT("  - ManualSlotCount: %d"), ManualSlotCount);
    UE_LOG(LogTemp, Log, TEXT("配置:"));
    UE_LOG(LogTemp, Log, TEXT("  - HorizontalSpacing: %.1f"), FormationConfig.HorizontalSpacing);
    UE_LOG(LogTemp, Log, TEXT("  - VerticalSpacing: %.1f"), FormationConfig.VerticalSpacing);
    UE_LOG(LogTemp, Log, TEXT("  - MinDistanceToLeader: %.1f"), FormationConfig.MinDistanceToLeader);
    UE_LOG(LogTemp, Log, TEXT("  - MaxColumns: %d"), FormationConfig.MaxColumns);
  
    int32 ActualSlotCount = (ManualSlotCount > 0) ? FMath::Max(ManualSlotCount, SoldierCount) : SoldierCount;

    FormationSlots.Empty();

    if (ActualSlotCount <= 0)
    {
        OnFormationUpdated.Broadcast();
        UE_LOG(LogTemp, Warning, TEXT("=== 槽位数为0，跳过生成 ==="));
        UE_LOG(LogTemp, Warning, TEXT(""));
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
        Slot.bOccupied = false;
        Slot.OccupantSoldierId = INDEX_NONE;
        FormationSlots.Add(Slot);
        
        // ✨ 新增 - 详细日志（可选）
        if (i < 3 || i == ActualSlotCount - 1) // 只打印前3个和最后1个
        {
            UE_LOG(LogTemp, Verbose, TEXT("  槽位[%d]: Offset(%.1f, %.1f)"), 
                i, Slot.LocalOffset.X, Slot.LocalOffset.Y);
        }
    }

    OnFormationUpdated.Broadcast();

    UE_LOG(LogTemp, Warning, TEXT("=== 编队生成完成: 槽位数=%d (%dx%d) ==="), 
        ActualSlotCount, Columns, Rows);
    UE_LOG(LogTemp, Warning, TEXT(""));
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

int32 UXBFormationComponent::AssignSlotToSoldier(AXBSoldierCharacter* Soldier)
{
    if (!Soldier)
    {
        return INDEX_NONE;
    }

    int32 SlotIndex = GetFirstAvailableSlot();
    
    if (SlotIndex == INDEX_NONE)
    {
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