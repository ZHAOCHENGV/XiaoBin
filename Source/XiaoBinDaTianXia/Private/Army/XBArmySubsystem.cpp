// Copyright XiaoBing Project. All Rights Reserved.

#include "Army/XBArmySubsystem.h"
#include "Army/XBSoldierRenderer.h"
#include "Army/XBSoldierPool.h"
#include "Army/XBFormationManager.h"

void UXBArmySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 创建渲染器和对象池
    SoldierRenderer = NewObject<UXBSoldierRenderer>(this);
    SoldierPool = NewObject<UXBSoldierPool>(this);

    UE_LOG(LogTemp, Log, TEXT("XBArmySubsystem Initialized"));
}

void UXBArmySubsystem::Deinitialize()
{
    SoldierMap.Empty();
    LeaderToSoldiersMap.Empty();

    Super::Deinitialize();
}

void UXBArmySubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    TickAllSoldiers(DeltaTime);

    // 更新渲染
    if (SoldierRenderer)
    {
        SoldierRenderer->UpdateInstances(SoldierMap);
    }
}

TStatId UXBArmySubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UXBArmySubsystem, STATGROUP_Tickables);
}

int32 UXBArmySubsystem::CreateSoldier(const FXBSoldierConfig& Config, EXBFaction Faction, const FVector& SpawnLocation)
{
    const int32 NewId = GenerateSoldierId();

    FXBSoldierAgent NewSoldier;
    NewSoldier.SoldierId = NewId;
    NewSoldier.Faction = Faction;
    NewSoldier.SoldierType = Config.SoldierType;
    NewSoldier.State = EXBSoldierState::Idle;
    NewSoldier.Position = SpawnLocation;
    NewSoldier.CurrentHealth = Config.BaseHealth;
    NewSoldier.MaxHealth = Config.BaseHealth;

    SoldierMap.Add(NewId, NewSoldier);

    // 通知渲染器添加实例
    if (SoldierRenderer)
    {
        SoldierRenderer->AddInstance(NewId, SpawnLocation);
    }

    return NewId;
}

void UXBArmySubsystem::DestroySoldier(int32 SoldierId)
{
    if (FXBSoldierAgent* Soldier = SoldierMap.Find(SoldierId))
    {
        // 从将领移除
        if (Soldier->OwnerLeader.IsValid())
        {
            RemoveSoldierFromLeader(SoldierId);
        }

        // 通知渲染器移除实例
        if (SoldierRenderer)
        {
            SoldierRenderer->RemoveInstance(SoldierId);
        }

        SoldierMap.Remove(SoldierId);
    }
}

bool UXBArmySubsystem::GetSoldierData(int32 SoldierId, FXBSoldierAgent& OutData) const
{
    if (const FXBSoldierAgent* Soldier = SoldierMap.Find(SoldierId))
    {
        OutData = *Soldier;
        return true;
    }
    return false;
}

bool UXBArmySubsystem::ModifySoldierData(int32 SoldierId, const FXBSoldierAgent& NewData)
{
    if (FXBSoldierAgent* Soldier = SoldierMap.Find(SoldierId))
    {
        *Soldier = NewData;
        return true;
    }
    return false;
}

FVector UXBArmySubsystem::GetSoldierPosition(int32 SoldierId) const
{
    if (const FXBSoldierAgent* Soldier = SoldierMap.Find(SoldierId))
    {
        return Soldier->Position;
    }
    return FVector::ZeroVector;
    
}

float UXBArmySubsystem::GetSoldierHealth(int32 SoldierId) const
{
    if (const FXBSoldierAgent* Soldier = SoldierMap.Find(SoldierId))
    {
        return Soldier->CurrentHealth;
    }
    return 0.0f;
}



EXBSoldierState UXBArmySubsystem::GetSoldierState(int32 SoldierId) const
{
    if (const FXBSoldierAgent* Soldier = SoldierMap.Find(SoldierId))
    {
        return Soldier->State;
    }
    return EXBSoldierState::Dead;
}

bool UXBArmySubsystem::IsSoldierAlive(int32 SoldierId) const
{
    if (const FXBSoldierAgent* Soldier = SoldierMap.Find(SoldierId))
    {
        return Soldier->IsAlive();
    }
    return false;
}

bool UXBArmySubsystem::AssignSoldierToLeader(int32 SoldierId, AActor* Leader, int32 SlotIndex)
{
    if (!Leader)
    {
        return false;
    }

    FXBSoldierAgent* Soldier = SoldierMap.Find(SoldierId);
    if (!Soldier)
    {
        return false;
    }

    // 如果已有旧将领，先移除
    if (Soldier->OwnerLeader.IsValid() && Soldier->OwnerLeader.Get() != Leader)
    {
        RemoveSoldierFromLeader(SoldierId);
    }

    // 分配给新将领
    Soldier->OwnerLeader = Leader;
    Soldier->FormationSlotIndex = SlotIndex;
    Soldier->State = EXBSoldierState::Following;

    // 更新映射
    TArray<int32>& LeaderSoldiers = LeaderToSoldiersMap.FindOrAdd(Leader);
    LeaderSoldiers.AddUnique(SoldierId);

    return true;
}

bool UXBArmySubsystem::RemoveSoldierFromLeader(int32 SoldierId)
{
    FXBSoldierAgent* Soldier = SoldierMap.Find(SoldierId);
    if (!Soldier || !Soldier->OwnerLeader.IsValid())
    {
        return false;
    }

    AActor* Leader = Soldier->OwnerLeader.Get();
    
    // 从映射移除
    if (TArray<int32>* LeaderSoldiers = LeaderToSoldiersMap.Find(Leader))
    {
        LeaderSoldiers->Remove(SoldierId);
    }

    // 重置士兵状态
    Soldier->OwnerLeader.Reset();
    Soldier->FormationSlotIndex = INDEX_NONE;
    Soldier->State = EXBSoldierState::Idle;

    // 重排剩余士兵
    ReorganizeFormation(Leader);

    return true;
}

TArray<int32> UXBArmySubsystem::GetSoldiersByLeader(const AActor* Leader) const
{
    if (const TArray<int32>* Soldiers = LeaderToSoldiersMap.Find(Leader))
    {
        return *Soldiers;
    }
    return TArray<int32>();
}

int32 UXBArmySubsystem::GetSoldierCountByLeader(const AActor* Leader) const
{
    if (const TArray<int32>* Soldiers = LeaderToSoldiersMap.Find(Leader))
    {
        return Soldiers->Num();
    }
    return 0;
}

void UXBArmySubsystem::EnterCombatForLeader(AActor* Leader)
{
    TArray<int32> Soldiers = GetSoldiersByLeader(Leader);
    for (int32 SoldierId : Soldiers)
    {
        if (FXBSoldierAgent* Soldier = SoldierMap.Find(SoldierId))
        {
            if (Soldier->State == EXBSoldierState::Following)
            {
                Soldier->State = EXBSoldierState::Engaging;
                OnSoldierStateChanged.Broadcast(SoldierId, EXBSoldierState::Engaging);
            }
        }
    }
}

void UXBArmySubsystem::ExitCombatForLeader(AActor* Leader)
{
    TArray<int32> Soldiers = GetSoldiersByLeader(Leader);
    for (int32 SoldierId : Soldiers)
    {
        if (FXBSoldierAgent* Soldier = SoldierMap.Find(SoldierId))
        {
            if (Soldier->IsInCombat())
            {
                Soldier->State = EXBSoldierState::Returning;
                Soldier->TargetEnemyId = INDEX_NONE;
                OnSoldierStateChanged.Broadcast(SoldierId, EXBSoldierState::Returning);
            }
        }
    }
}

float UXBArmySubsystem::ApplyDamageToSoldier(int32 SoldierId, float DamageAmount, AActor* DamageSource)
{
    FXBSoldierAgent* Soldier = SoldierMap.Find(SoldierId);
    if (!Soldier || !Soldier->IsAlive())
    {
        return 0.0f;
    }

    const float ActualDamage = FMath::Min(DamageAmount, Soldier->CurrentHealth);
    Soldier->CurrentHealth -= ActualDamage;

    OnSoldierDamaged.Broadcast(SoldierId, ActualDamage);

    if (Soldier->CurrentHealth <= 0.0f)
    {
        HandleSoldierDeath(SoldierId);
    }

    return ActualDamage;
}

void UXBArmySubsystem::SetHiddenForLeader(AActor* Leader, bool bNewHidden)
{
    TArray<int32> Soldiers = GetSoldiersByLeader(Leader);
    for (int32 SoldierId : Soldiers)
    {
        if (FXBSoldierAgent* Soldier = SoldierMap.Find(SoldierId))
        {
            Soldier->bIsHidden = bNewHidden;
            Soldier->bCollisionEnabled = !bNewHidden;
        }
    }

    // 通知渲染器更新可见性
    if (SoldierRenderer)
    {
        SoldierRenderer->SetVisibilityForLeader(Leader, !bNewHidden);
    }
}

void UXBArmySubsystem::ConvertSoldierType(AActor* Leader, EXBSoldierType NewType)
{
    TArray<int32> Soldiers = GetSoldiersByLeader(Leader);
    for (int32 SoldierId : Soldiers)
    {
        if (FXBSoldierAgent* Soldier = SoldierMap.Find(SoldierId))
        {
            Soldier->SoldierType = NewType;
        }
    }

    // 通知渲染器更新网格
    if (SoldierRenderer)
    {
        SoldierRenderer->UpdateMeshForLeader(Leader, NewType);
    }
}

int32 UXBArmySubsystem::FindNearestEnemy(const FVector& Location, EXBFaction MyFaction, float MaxDistance) const
{
    int32 NearestId = INDEX_NONE;
    float NearestDistSq = MaxDistance * MaxDistance;

    for (const auto& Pair : SoldierMap)
    {
        const FXBSoldierAgent& Soldier = Pair.Value;
        
        // 跳过同阵营、死亡、隐身的士兵
        if (Soldier.Faction == MyFaction || !Soldier.IsAlive() || Soldier.bIsHidden)
        {
            continue;
        }

        const float DistSq = FVector::DistSquared(Location, Soldier.Position);
        if (DistSq < NearestDistSq)
        {
            NearestDistSq = DistSq;
            NearestId = Soldier.SoldierId;
        }
    }

    return NearestId;
}

TArray<int32> UXBArmySubsystem::FindEnemiesInRadius(const FVector& Location, EXBFaction MyFaction, float Radius) const
{
    TArray<int32> Result;
    const float RadiusSq = Radius * Radius;

    for (const auto& Pair : SoldierMap)
    {
        const FXBSoldierAgent& Soldier = Pair.Value;
        
        if (Soldier.Faction == MyFaction || !Soldier.IsAlive() || Soldier.bIsHidden)
        {
            continue;
        }

        if (FVector::DistSquared(Location, Soldier.Position) <= RadiusSq)
        {
            Result.Add(Soldier.SoldierId);
        }
    }

    return Result;
}

void UXBArmySubsystem::TickAllSoldiers(float DeltaTime)
{
    for (auto& Pair : SoldierMap)
    {
        FXBSoldierAgent& Soldier = Pair.Value;
        if (Soldier.IsAlive())
        {
            TickSoldier(Soldier, DeltaTime);
        }
    }
}

void UXBArmySubsystem::TickSoldier(FXBSoldierAgent& Soldier, float DeltaTime)
{
    // 更新攻击冷却
    if (Soldier.AttackCooldown > 0.0f)
    {
        Soldier.AttackCooldown -= DeltaTime;
    }

    // 根据状态更新
    switch (Soldier.State)
    {
    case EXBSoldierState::Following:
        UpdateFollowingState(Soldier, DeltaTime);
        break;
    case EXBSoldierState::Engaging:
    case EXBSoldierState::Seeking:
        UpdateEngagingState(Soldier, DeltaTime);
        break;
    case EXBSoldierState::Returning:
        UpdateReturningState(Soldier, DeltaTime);
        break;
    default:
        break;
    }

    // 更新动画时间
    Soldier.AnimationTime += DeltaTime;
}

void UXBArmySubsystem::UpdateFollowingState(FXBSoldierAgent& Soldier, float DeltaTime)
{
    if (!Soldier.OwnerLeader.IsValid())
    {
        return;
    }

    // 获取目标位置（从编队管理器计算）
    const FVector LeaderPos = Soldier.OwnerLeader->GetActorLocation();
    const FRotator LeaderRot = Soldier.OwnerLeader->GetActorRotation();

    // 计算槽位世界位置
    const FVector SlotOffset = UXBFormationManager::CalculateSlotOffset(
        Soldier.FormationSlotIndex,
        GetSoldierCountByLeader(Soldier.OwnerLeader.Get()));
    
    Soldier.TargetPosition = LeaderPos + LeaderRot.RotateVector(SlotOffset);

    // 平滑移动
    const float InterpSpeed = 5.0f;
    Soldier.Position = FMath::VInterpTo(Soldier.Position, Soldier.TargetPosition, DeltaTime, InterpSpeed);

    // 更新朝向
    const FVector MoveDir = (Soldier.TargetPosition - Soldier.Position).GetSafeNormal();
    if (!MoveDir.IsNearlyZero())
    {
        Soldier.Rotation = FMath::RInterpTo(Soldier.Rotation, MoveDir.Rotation(), DeltaTime, InterpSpeed);
    }
}

void UXBArmySubsystem::UpdateEngagingState(FXBSoldierAgent& Soldier, float DeltaTime)
{
    // 检查当前目标是否有效
    if (Soldier.TargetEnemyId != INDEX_NONE)
    {
        FXBSoldierAgent TargetData;
        if (!GetSoldierData(Soldier.TargetEnemyId, TargetData) || !TargetData.IsAlive())
        {
            Soldier.TargetEnemyId = INDEX_NONE;
            Soldier.State = EXBSoldierState::Seeking;
        }
    }

    // 寻找新目标
    if (Soldier.TargetEnemyId == INDEX_NONE)
    {
        Soldier.TargetEnemyId = FindNearestEnemy(Soldier.Position, Soldier.Faction, 1500.0f);
        
        if (Soldier.TargetEnemyId == INDEX_NONE)
        {
            // 没有敌人，返回队列
            Soldier.State = EXBSoldierState::Returning;
            return;
        }
    }

    // 获取目标位置
    FXBSoldierAgent TargetData;
    if (GetSoldierData(Soldier.TargetEnemyId, TargetData))
    {
        Soldier.TargetPosition = TargetData.Position;
        
        const float DistToTarget = FVector::Dist(Soldier.Position, Soldier.TargetPosition);
        const float AttackRange = 150.0f; // TODO: 从配置读取

        if (DistToTarget <= AttackRange)
        {
            // 在攻击范围内
            // TODO: 执行攻击
        }
        else
        {
            // 移动向目标
            const FVector MoveDir = (Soldier.TargetPosition - Soldier.Position).GetSafeNormal();
            const float MoveSpeed = 400.0f; // TODO: 从配置读取
            
            Soldier.Position += MoveDir * MoveSpeed * DeltaTime;
            Soldier.Rotation = MoveDir.Rotation();
        }
    }
}

void UXBArmySubsystem::UpdateReturningState(FXBSoldierAgent& Soldier, float DeltaTime)
{
    if (!Soldier.OwnerLeader.IsValid())
    {
        Soldier.State = EXBSoldierState::Idle;
        return;
    }

    // 计算目标槽位位置
    const FVector LeaderPos = Soldier.OwnerLeader->GetActorLocation();
    const FRotator LeaderRot = Soldier.OwnerLeader->GetActorRotation();

    const FVector SlotOffset = UXBFormationManager::CalculateSlotOffset(
        Soldier.FormationSlotIndex,
        GetSoldierCountByLeader(Soldier.OwnerLeader.Get()));
    
    Soldier.TargetPosition = LeaderPos + LeaderRot.RotateVector(SlotOffset);

    // 快速返回
    const float ReturnSpeed = 600.0f;
    const float DistToTarget = FVector::Dist(Soldier.Position, Soldier.TargetPosition);

    if (DistToTarget < 50.0f)
    {
        // 到达目标，切换为跟随
        Soldier.State = EXBSoldierState::Following;
        OnSoldierStateChanged.Broadcast(Soldier.SoldierId, EXBSoldierState::Following);
    }
    else
    {
        const FVector MoveDir = (Soldier.TargetPosition - Soldier.Position).GetSafeNormal();
        Soldier.Position += MoveDir * ReturnSpeed * DeltaTime;
        Soldier.Rotation = MoveDir.Rotation();
    }
}

void UXBArmySubsystem::HandleSoldierDeath(int32 SoldierId)
{
    FXBSoldierAgent* Soldier = SoldierMap.Find(SoldierId);
    if (!Soldier)
    {
        return;
    }

    Soldier->State = EXBSoldierState::Dead;
    
    // 广播死亡事件
    OnSoldierDied.Broadcast(SoldierId);

    // 如果有将领，通知并重排
    if (Soldier->OwnerLeader.IsValid())
    {
        AActor* Leader = Soldier->OwnerLeader.Get();
        RemoveSoldierFromLeader(SoldierId);
    }

    // 延迟销毁（给死亡动画时间）
    // TODO: 使用 Timer 延迟调用 DestroySoldier
}

void UXBArmySubsystem::ReorganizeFormation(AActor* Leader)
{
    if (!Leader)
    {
        return;
    }

    TArray<int32>* LeaderSoldiers = LeaderToSoldiersMap.Find(Leader);
    if (!LeaderSoldiers)
    {
        return;
    }

    // 重新分配槽位索引
    for (int32 i = 0; i < LeaderSoldiers->Num(); ++i)
    {
        if (FXBSoldierAgent* Soldier = SoldierMap.Find((*LeaderSoldiers)[i]))
        {
            Soldier->FormationSlotIndex = i;
        }
    }
}

int32 UXBArmySubsystem::GenerateSoldierId()
{
    return NextSoldierId++;
}