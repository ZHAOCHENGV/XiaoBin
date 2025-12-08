#include "Army/XBArmySubsystem.h"
#include "Army/XBSoldierRenderer.h"
#include "Engine/World.h"

UXBArmySubsystem::UXBArmySubsystem()
{
}

void UXBArmySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    TickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UXBArmySubsystem::TickSubsystem),
        0.0f
    );
    
    // 初始化渲染器
    SoldierRenderer = NewObject<UXBSoldierRenderer>(this);
    SoldierRenderer->Initialize(GetWorld());
}

void UXBArmySubsystem::Deinitialize()
{
    if (TickHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
        TickHandle.Reset();
    }
    
    if (SoldierRenderer)
    {
        SoldierRenderer->Cleanup();
        SoldierRenderer = nullptr;
    }

    SoldierMap.Empty();
    LeaderToSoldiersMap.Empty();
    FactionSoldiersMap.Empty();

    Super::Deinitialize();
}

bool UXBArmySubsystem::TickSubsystem(float DeltaTime)
{UpdateSoldierLogic(DeltaTime);
    return true;
}

void UXBArmySubsystem::UpdateRenderer()
{
    if (SoldierRenderer)
    {
        SoldierRenderer->UpdateInstancesFromData(SoldierMap);
    }
}



int32 UXBArmySubsystem::CreateSoldier(EXBSoldierType SoldierType, EXBFaction Faction, const FVector& Position)
{
    int32 NewId = NextSoldierId++;

    FXBSoldierData NewSoldier;
    NewSoldier.SoldierId = NewId;
    NewSoldier.SoldierType = SoldierType;
    NewSoldier.Faction = Faction;
    NewSoldier.Position = Position;
    NewSoldier.State = EXBSoldierState::Idle;
    NewSoldier.CurrentHealth = 100.0f;
    NewSoldier.MaxHealth = 100.0f;
    NewSoldier.LeaderId = -1;
    NewSoldier.FormationSlotIndex = -1;

    SoldierMap.Add(NewId, NewSoldier);
    FactionSoldiersMap.FindOrAdd(Faction).Add(NewId);
    OnSoldierCreated.Broadcast(NewId);

    return NewId;
}

void UXBArmySubsystem::DestroySoldier(int32 SoldierId)
{
    if (FXBSoldierData* Soldier = GetSoldierDataInternal(SoldierId))
    {
        if (Soldier->HasLeader())
        {
            RemoveSoldierFromLeader(SoldierId);
        }

        if (TSet<int32>* FactionSet = FactionSoldiersMap.Find(Soldier->Faction))
        {
            FactionSet->Remove(SoldierId);
        }

        SoldierMap.Remove(SoldierId);
    }
}

bool UXBArmySubsystem::GetSoldierDataById(int32 SoldierId, FXBSoldierData& OutData) const
{
    if (const FXBSoldierData* Data = GetSoldierDataInternal(SoldierId))
    {
        OutData = *Data;
        return true;
    }
    return false;
}

void UXBArmySubsystem::SetSoldierPosition(int32 SoldierId, const FVector& NewPosition)
{
    if (FXBSoldierData* Soldier = GetSoldierDataInternal(SoldierId))
    {
        Soldier->Position = NewPosition;
    }
}

void UXBArmySubsystem::SetSoldierState(int32 SoldierId, EXBSoldierState NewState)
{
    if (FXBSoldierData* Soldier = GetSoldierDataInternal(SoldierId))
    {
        if (Soldier->State != NewState)
        {
            EXBSoldierState OldState = Soldier->State;
            Soldier->State = NewState;
            OnSoldierStateChanged.Broadcast(SoldierId, OldState, NewState);
        }
    }
}

// ✨ 新增 - 实现 AssignSoldierToLeader (3参数版本)
bool UXBArmySubsystem::AssignSoldierToLeader(int32 SoldierId, AActor* LeaderActor, int32 SlotIndex)
{
    if (!LeaderActor) return false;
    
    // 使用 Actor 的 UniqueID 作为 LeaderId
    int32 LeaderId = LeaderActor->GetUniqueID();
    
    AssignSoldierToLeader(SoldierId, LeaderId); // 调用 2 参数版本
    SetSoldierFormationSlot(SoldierId, SlotIndex);
    
    return true;
}

void UXBArmySubsystem::AssignSoldierToLeader(int32 SoldierId, int32 LeaderId)
{
    if (FXBSoldierData* Soldier = GetSoldierDataInternal(SoldierId))
    {
        if (Soldier->HasLeader() && Soldier->LeaderId != LeaderId)
        {
            RemoveSoldierFromLeader(SoldierId);
        }

        Soldier->LeaderId = LeaderId;
        LeaderToSoldiersMap.FindOrAdd(LeaderId).AddUnique(SoldierId);
        SetSoldierState(SoldierId, EXBSoldierState::Following);
    }
}

// ✨ 新增 - 实现 GetSoldiersByLeader (Actor版本)
TArray<int32> UXBArmySubsystem::GetSoldiersByLeader(const AActor* LeaderActor) const
{
    if (!LeaderActor) return TArray<int32>();
    return GetSoldiersByLeader(LeaderActor->GetUniqueID());
}
void UXBArmySubsystem::RemoveSoldierFromLeader(int32 SoldierId)
{
    if (FXBSoldierData* Soldier = GetSoldierDataInternal(SoldierId))
    {
        int32 OldLeaderId = Soldier->LeaderId;
        Soldier->LeaderId = -1;
        Soldier->FormationSlotIndex = -1;

        if (TArray<int32>* LeaderSoldiers = LeaderToSoldiersMap.Find(OldLeaderId))
        {
            LeaderSoldiers->Remove(SoldierId);
        }

        SetSoldierState(SoldierId, EXBSoldierState::Idle);
    }
}

TArray<int32> UXBArmySubsystem::GetSoldiersByLeader(int32 LeaderId) const
{
    if (const TArray<int32>* Soldiers = LeaderToSoldiersMap.Find(LeaderId))
    {
        return *Soldiers;
    }
    return TArray<int32>();
}


void UXBArmySubsystem::SetSoldierFormationSlot(int32 SoldierId, int32 SlotIndex)
{
    if (FXBSoldierData* Soldier = GetSoldierDataInternal(SoldierId))
    {
        Soldier->FormationSlotIndex = SlotIndex;
    }
}

void UXBArmySubsystem::EnterCombatForLeader(AActor* LeaderActor)
{
    if (!LeaderActor) return;
    
    TArray<int32> Soldiers = GetSoldiersByLeader(LeaderActor);
    for (int32 SoldierId : Soldiers)
    {
        SetSoldierState(SoldierId, EXBSoldierState::Combat);
    }
}

void UXBArmySubsystem::SetHiddenForLeader(AActor* LeaderActor, bool bHidden)
{
    if (!LeaderActor) return;

    if (SoldierRenderer)
    {
        SoldierRenderer->SetVisibilityForLeader(LeaderActor, !bHidden);
    }
}

float UXBArmySubsystem::DamageSoldier(int32 SoldierId, float DamageAmount, AActor* DamageCauser)
{
    if (FXBSoldierData* Soldier = GetSoldierDataInternal(SoldierId))
    {
        if (!Soldier->IsAlive())
        {
            return 0.0f;
        }

        float ActualDamage = FMath::Min(DamageAmount, Soldier->CurrentHealth);
        Soldier->CurrentHealth -= ActualDamage;

        OnSoldierDamaged.Broadcast(SoldierId, ActualDamage, Soldier->CurrentHealth);

        if (Soldier->CurrentHealth <= 0.0f)
        {
            ProcessSoldierDeath(SoldierId);
        }

        return ActualDamage;
    }
    return 0.0f;
}

void UXBArmySubsystem::SetSoldierCombatTarget(int32 SoldierId, int32 TargetSoldierId)
{
    if (FXBSoldierData* Soldier = GetSoldierDataInternal(SoldierId))
    {
        SetSoldierState(SoldierId, EXBSoldierState::Combat);
    }
}

FVector UXBArmySubsystem::GetSoldierPosition(int32 SoldierId) const
{
    if (const FXBSoldierData* Soldier = GetSoldierDataInternal(SoldierId))
    {
        return Soldier->Position;
    }
    return FVector::ZeroVector;
}

float UXBArmySubsystem::GetSoldierHealth(int32 SoldierId) const
{
    if (const FXBSoldierData* Soldier = GetSoldierDataInternal(SoldierId))
    {
        return Soldier->CurrentHealth;
    }
    return 0.0f;
}

EXBSoldierState UXBArmySubsystem::GetSoldierState(int32 SoldierId) const
{
    if (const FXBSoldierData* Soldier = GetSoldierDataInternal(SoldierId))
    {
        return Soldier->State;
    }
    return EXBSoldierState::Idle;
}

bool UXBArmySubsystem::IsSoldierAlive(int32 SoldierId) const
{
    if (const FXBSoldierData* Soldier = GetSoldierDataInternal(SoldierId))
    {
        return Soldier->IsAlive();
    }
    return false;
}

TArray<int32> UXBArmySubsystem::GetSoldiersByFaction(EXBFaction Faction) const
{
    TArray<int32> Result;
    if (const TSet<int32>* FactionSet = FactionSoldiersMap.Find(Faction))
    {
        Result = FactionSet->Array();
    }
    return Result;
}

TArray<int32> UXBArmySubsystem::GetSoldiersInRadius(const FVector& Center, float Radius) const
{
    TArray<int32> Result;
    float RadiusSq = Radius * Radius;

    for (const auto& Pair : SoldierMap)
    {
        if (FVector::DistSquared(Pair.Value.Position, Center) <= RadiusSq)
        {
            Result.Add(Pair.Key);
        }
    }
    return Result;
}

void UXBArmySubsystem::UpdateLeaderFormation(int32 LeaderId, const FVector& LeaderPosition, const FRotator& LeaderRotation)
{
    const TArray<int32> Soldiers = GetSoldiersByLeader(LeaderId);
    
    for (int32 SoldierId : Soldiers)
    {
        if (FXBSoldierData* Soldier = GetSoldierDataInternal(SoldierId))
        {
            FVector SlotOffset = FVector(-100.0f * (Soldier->FormationSlotIndex / 5 + 1),
                                         (Soldier->FormationSlotIndex % 5 - 2) * 80.0f,
                                         0.0f);
            FVector WorldOffset = LeaderRotation.RotateVector(SlotOffset);
            Soldier->TargetPosition = LeaderPosition + WorldOffset;
        }
    }
}

void UXBArmySubsystem::SetLeaderSoldiersSprinting(int32 LeaderId, bool bSprinting)
{
    const TArray<int32> Soldiers = GetSoldiersByLeader(LeaderId);
    
    for (int32 SoldierId : Soldiers)
    {
        if (FXBSoldierData* Soldier = GetSoldierDataInternal(SoldierId))
        {
            Soldier->bIsSprinting = bSprinting;
        }
    }
}

FXBSoldierData* UXBArmySubsystem::GetSoldierDataInternal(int32 SoldierId)
{
    return SoldierMap.Find(SoldierId);
}

const FXBSoldierData* UXBArmySubsystem::GetSoldierDataInternal(int32 SoldierId) const
{
    return SoldierMap.Find(SoldierId);
}

void UXBArmySubsystem::UpdateSoldierLogic(float DeltaTime)
{
    for (auto& Pair : SoldierMap)
    {
        FXBSoldierData& Soldier = Pair.Value;
        
        if (!Soldier.IsAlive())
        {
            continue;
        }

        switch (Soldier.State)
        {
        case EXBSoldierState::Following:
            UpdateSoldierMovement(Soldier, DeltaTime);
            break;
            
        case EXBSoldierState::Combat:
            break;
            
        default:
            break;
        }
    }

    UpdateRenderer();
}

void UXBArmySubsystem::UpdateSoldierMovement(FXBSoldierData& Soldier, float DeltaTime)
{
    FVector Direction = Soldier.TargetPosition - Soldier.Position;
    float Distance = Direction.Size2D();

    if (Distance > 10.0f)
    {
        Direction.Normalize();
        float MoveSpeed = Soldier.bIsSprinting ? 600.0f : 400.0f;
        FVector NewPosition = Soldier.Position + Direction * MoveSpeed * DeltaTime;
        Soldier.Position = NewPosition;
        Soldier.Rotation = Direction.Rotation();
    }
}

void UXBArmySubsystem::ProcessSoldierDeath(int32 SoldierId)
{
    if (FXBSoldierData* Soldier = GetSoldierDataInternal(SoldierId))
    {
        int32 LeaderId = Soldier->LeaderId;
        Soldier->State = EXBSoldierState::Dead;
        OnSoldierDied.Broadcast(SoldierId, LeaderId);

        if (LeaderId >= 0)
        {
            RemoveSoldierFromLeader(SoldierId);
        }
    }
}
