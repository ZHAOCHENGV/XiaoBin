/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/XBFlowFieldSubsystem.cpp

/**
 * @file XBFlowFieldSubsystem.cpp
 * @brief 流场子系统实现
 */

#include "AI/XBFlowFieldSubsystem.h"
#include "Character/XBCharacterBase.h"
#include "Soldier/XBSoldierActor.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

void UXBFlowFieldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 注册Tick
    if (UWorld* World = GetWorld())
    {
        TickDelegateHandle = FWorldDelegates::OnWorldTickStart.AddUObject(this, &UXBFlowFieldSubsystem::OnWorldTick);
    }

    UE_LOG(LogTemp, Log, TEXT("流场子系统初始化"));
}

void UXBFlowFieldSubsystem::Deinitialize()
{
    FWorldDelegates::OnWorldTickStart.Remove(TickDelegateHandle);

    LeaderFlowFields.Empty();
    RegisteredSoldiers.Empty();
    RegisteredLeaders.Empty();
    SpatialGrid.Clear();

    UE_LOG(LogTemp, Log, TEXT("流场子系统销毁"));

    Super::Deinitialize();
}

void UXBFlowFieldSubsystem::InitializeFlowField(const FVector2D& WorldOrigin, const FVector2D& WorldSize, float CellSize)
{
    CachedWorldOrigin = WorldOrigin;
    CachedWorldSize = WorldSize;
    CachedCellSize = CellSize;

    SpatialGrid.Initialize(CellSize);

    bInitialized = true;

    UE_LOG(LogTemp, Log, TEXT("流场系统初始化 - 原点: (%.1f, %.1f), 大小: (%.1f, %.1f), 单元格: %.1f"),
        WorldOrigin.X, WorldOrigin.Y, WorldSize.X, WorldSize.Y, CellSize);
}

void UXBFlowFieldSubsystem::RegisterLeader(AXBCharacterBase* Leader)
{
    if (!Leader || !bInitialized)
    {
        return;
    }

    if (LeaderFlowFields.Contains(Leader))
    {
        return;
    }

    // 创建流场
    FXBFlowField& FlowField = LeaderFlowFields.Add(Leader);
    FlowField.Initialize(CachedWorldOrigin, CachedWorldSize, CachedCellSize);
    FlowField.SetTarget(Leader->GetActorLocation());

    RegisteredLeaders.Add(Leader);
    SpatialGrid.AddActor(Leader);

    UE_LOG(LogTemp, Log, TEXT("注册主将: %s"), *Leader->GetName());
}

void UXBFlowFieldSubsystem::UnregisterLeader(AXBCharacterBase* Leader)
{
    if (!Leader)
    {
        return;
    }

    LeaderFlowFields.Remove(Leader);
    RegisteredLeaders.RemoveAll([Leader](const TWeakObjectPtr<AXBCharacterBase>& Ptr) {
        return !Ptr.IsValid() || Ptr.Get() == Leader;
    });
    SpatialGrid.RemoveActor(Leader);

    UE_LOG(LogTemp, Log, TEXT("注销主将: %s"), *Leader->GetName());
}

void UXBFlowFieldSubsystem::RegisterSoldier(AXBSoldierActor* Soldier)
{
    if (!Soldier)
    {
        return;
    }

    RegisteredSoldiers.Add(Soldier);
    SpatialGrid.AddActor(Soldier);
}

void UXBFlowFieldSubsystem::UnregisterSoldier(AXBSoldierActor* Soldier)
{
    if (!Soldier)
    {
        return;
    }

    RegisteredSoldiers.RemoveAll([Soldier](const TWeakObjectPtr<AXBSoldierActor>& Ptr) {
        return !Ptr.IsValid() || Ptr.Get() == Soldier;
    });
    SpatialGrid.RemoveActor(Soldier);
}

FVector UXBFlowFieldSubsystem::GetFlowDirectionToLeader(AXBCharacterBase* Leader, const FVector& WorldLocation) const
{
    if (!Leader)
    {
        return FVector::ZeroVector;
    }

    if (const FXBFlowField* FlowField = LeaderFlowFields.Find(Leader))
    {
        return FlowField->GetFlowDirectionSmooth(WorldLocation);
    }

    // 如果没有流场，直接朝向目标
    const FVector Direction = Leader->GetActorLocation() - WorldLocation;
    return Direction.GetSafeNormal2D();
}

void UXBFlowFieldSubsystem::GetNearbySoldiers(const FVector& Location, float Radius, TArray<AXBSoldierActor*>& OutSoldiers, AActor* ExcludeActor)
{
    SpatialGrid.QueryNeighborsOfType<AXBSoldierActor>(Location, Radius, OutSoldiers, ExcludeActor);
}

void UXBFlowFieldSubsystem::GetNearbyEnemies(const FVector& Location, float Radius, EXBFaction MyFaction, TArray<AActor*>& OutEnemies)
{
    OutEnemies.Reset();

    TArray<AActor*> AllActors;
    SpatialGrid.QueryNeighbors(Location, Radius, AllActors, nullptr);

    for (AActor* Actor : AllActors)
    {
        if (AXBCharacterBase* Character = Cast<AXBCharacterBase>(Actor))
        {
            // 检查是否为敌对阵营
            if (Character->GetFaction() != MyFaction && 
                Character->GetFaction() != EXBFaction::Neutral)
            {
                // 简单的敌对判断
                bool bIsEnemy = false;
                if (MyFaction == EXBFaction::Player || MyFaction == EXBFaction::Ally)
                {
                    bIsEnemy = (Character->GetFaction() == EXBFaction::Enemy);
                }
                else if (MyFaction == EXBFaction::Enemy)
                {
                    bIsEnemy = (Character->GetFaction() == EXBFaction::Player || Character->GetFaction() == EXBFaction::Ally);
                }

                if (bIsEnemy)
                {
                    OutEnemies.Add(Actor);
                }
            }
        }
        else if (AXBSoldierActor* Soldier = Cast<AXBSoldierActor>(Actor))
        {
            // 小兵的敌对判断
            // TODO: 根据小兵的阵营判断
        }
    }
}

AActor* UXBFlowFieldSubsystem::GetNearestEnemy(const FVector& Location, float Radius, EXBFaction MyFaction)
{
    TArray<AActor*> Enemies;
    GetNearbyEnemies(Location, Radius, MyFaction, Enemies);

    AActor* Nearest = nullptr;
    float MinDistSq = FLT_MAX;

    for (AActor* Enemy : Enemies)
    {
        const float DistSq = FVector::DistSquared(Location, Enemy->GetActorLocation());
        if (DistSq < MinDistSq)
        {
            MinDistSq = DistSq;
            Nearest = Enemy;
        }
    }

    return Nearest;
}

void UXBFlowFieldSubsystem::AddObstacle(const FVector& Location, float Radius)
{
    for (auto& Pair : LeaderFlowFields)
    {
        Pair.Value.SetObstacle(Location, Radius);
    }
}

void UXBFlowFieldSubsystem::RemoveObstacle(const FVector& Location, float Radius)
{
    for (auto& Pair : LeaderFlowFields)
    {
        Pair.Value.ClearObstacle(Location, Radius);
    }
}

void UXBFlowFieldSubsystem::UpdateFlowFields()
{
    // 更新空间网格
    SpatialGrid.UpdateAll();

    // 更新每个主将的流场
    for (auto& Pair : LeaderFlowFields)
    {
        if (Pair.Key && IsValid(Pair.Key))
        {
            Pair.Value.SetTarget(Pair.Key->GetActorLocation());
        }
    }
}

void UXBFlowFieldSubsystem::OnWorldTick(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
    if (!bInitialized || World != GetWorld())
    {
        return;
    }

    LastUpdateTime += DeltaSeconds;
    if (LastUpdateTime >= FlowFieldUpdateInterval)
    {
        LastUpdateTime = 0.0f;
        UpdateFlowFields();
    }

    // 更新空间网格（每帧）
    SpatialGrid.UpdateAll();

    if (bDebugDraw)
    {
        DebugDraw();
    }
}

void UXBFlowFieldSubsystem::DebugDraw()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (const auto& Pair : LeaderFlowFields)
    {
        if (Pair.Key && IsValid(Pair.Key))
        {
            Pair.Value.DebugDraw(World, 0.0f);
        }
    }
}

void UXBFlowFieldSubsystem::MarkCellAsBlocked(int32 X, int32 Y)
{
    BlockedCells.Add(FIntPoint(X, Y));
    
    // 更新所有流场
    for (auto& Pair : LeaderFlowFields)
    {
        Pair.Value.SetCellBlocked(X, Y, true);
    }
}

void UXBFlowFieldSubsystem::ClearBlockedCells()
{
    BlockedCells.Empty();
    
    for (auto& Pair : LeaderFlowFields)
    {
        Pair.Value.ClearAllBlockedCells();
    }
}

FVector UXBFlowFieldSubsystem::GetFlowDirection(const FVector& WorldPosition) const
{
    // 找到对应的流场并返回方向
    for (const auto& Pair : LeaderFlowFields)
    {
        if (Pair.Key && IsValid(Pair.Key))
        {
            return Pair.Value.GetFlowDirectionSmooth(WorldPosition);
        }
    }
    return FVector::ZeroVector;
}