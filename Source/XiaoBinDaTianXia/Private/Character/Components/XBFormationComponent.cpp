// Copyright XiaoBing Project. All Rights Reserved.

#include "Character/Components/XBFormationComponent.h"
#include "Army/XBFormationManager.h"
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

void UXBFormationComponent::RegenerateFormation(int32 SoldierCount)
{
    FormationSlots = UXBFormationManager::GenerateFormationSlots(SoldierCount, FormationConfig);
}

FVector UXBFormationComponent::GetSlotWorldPosition(int32 SlotIndex) const
{
    if (!FormationSlots.IsValidIndex(SlotIndex))
    {
        return GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return FVector::ZeroVector;
    }

    const FXBFormationSlot& Slot = FormationSlots[SlotIndex];
    // 🔧 修改 - 调用 Manager 的静态方法，并传入 FVector2D
    return UXBFormationManager::GetWorldSlotPosition(Owner->GetActorLocation(), Owner->GetActorRotation(), Slot.LocalOffset);
}

int32 UXBFormationComponent::GetFirstAvailableSlot() const
{
    for (int32 i = 0; i < FormationSlots.Num(); ++i)
    {
        // 🔧 修改 - 修复变量名错误: bIsOccupied -> bOccupied (与 struct 定义一致)
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
    // 🔧 修改 - bIsOccupied -> bOccupied
    if (Slot.bOccupied)
    {
        return false;
    }

    // 🔧 修改 - bIsOccupied -> bOccupied, OccupyingSoldierId -> OccupantSoldierId
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
    // 🔧 修改 - bIsOccupied -> bOccupied, OccupyingSoldierId -> OccupantSoldierId
    Slot.bOccupied = false;
    Slot.OccupantSoldierId = INDEX_NONE;
    return true;
}

void UXBFormationComponent::SetFormationConfig(const FXBFormationConfig& NewConfig)
{
    FormationConfig = NewConfig;
    RegenerateFormation(FormationSlots.Num());
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
        // 🔧 修改 - bIsOccupied -> bOccupied
        FColor Color = Slot.bOccupied ? FColor::Green : FColor::Yellow;

        DrawDebugSphere(World, WorldPos, 25.0f, 8, Color, false, Duration);
        DrawDebugString(World, WorldPos + FVector(0, 0, 50), FString::Printf(TEXT("%d"), Slot.SlotIndex), nullptr, FColor::White, Duration);
    }
#endif
}