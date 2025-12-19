/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Soldier/XBSoldierPoolSubsystem.cpp

/**
 * @file XBSoldierPoolSubsystem.cpp
 * @brief 士兵对象池子系统实现
 * 
 * @note ✨ 新增文件
 */

#include "Soldier/XBSoldierPoolSubsystem.h"
#include "Utils/XBLogCategories.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Soldier/Component/XBSoldierFollowComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

// 休眠位置：将休眠士兵放在世界边缘下方
const FVector UXBSoldierPoolSubsystem::DormantLocation = FVector(0.0f, 0.0f, -10000.0f);

void UXBSoldierPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogXBSoldier, Log, TEXT("士兵对象池子系统初始化 - 初始池大小: %d, 最大池大小: %d"),
        PoolConfig.InitialPoolSize, PoolConfig.MaxPoolSize);
}

void UXBSoldierPoolSubsystem::Deinitialize()
{
    // 清理定时器
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(WarmupTimerHandle);
    }

    // 打印最终报告
    PrintPoolReport();

    // 清空所有池
    ClearAllPools(true);

    Super::Deinitialize();
}

bool UXBSoldierPoolSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    if (UWorld* World = Cast<UWorld>(Outer))
    {
        return World->IsGameWorld();
    }
    return false;
}

// ==================== 池管理接口实现 ====================

void UXBSoldierPoolSubsystem::WarmupPool(TSubclassOf<AXBSoldierCharacter> SoldierClass, int32 Count, bool bAsync)
{
    if (!SoldierClass)
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("预热对象池失败: 士兵类为空"));
        return;
    }

    if (Count <= 0)
    {
        return;
    }

    UE_LOG(LogXBSoldier, Log, TEXT("开始预热对象池: %s, 数量: %d, 异步: %s"),
        *SoldierClass->GetName(), Count, bAsync ? TEXT("是") : TEXT("否"));

    if (bAsync)
    {
        // 加入待预热队列
        PendingWarmup.Add(TPair<TSubclassOf<AXBSoldierCharacter>, int32>(SoldierClass, Count));

        // 启动异步预热定时器（如果尚未启动）
        if (!WarmupTimerHandle.IsValid())
        {
            if (UWorld* World = GetWorld())
            {
                World->GetTimerManager().SetTimer(
                    WarmupTimerHandle,
                    this,
                    &UXBSoldierPoolSubsystem::AsyncWarmupTick,
                    0.016f, // 约60FPS
                    true
                );
            }
        }
    }
    else
    {
        // 同步预热：一次性创建所有
        TArray<AXBSoldierCharacter*>& Pool = GetOrCreatePool(SoldierClass);
        FXBPoolStats& Stats = PoolStatistics.FindOrAdd(SoldierClass);

        for (int32 i = 0; i < Count && Stats.TotalCreated < PoolConfig.MaxPoolSize; ++i)
        {
            if (AXBSoldierCharacter* Soldier = CreateDormantSoldier(SoldierClass))
            {
                Pool.Add(Soldier);
                Stats.TotalCreated++;
                Stats.Available++;
            }
        }

        UE_LOG(LogXBSoldier, Log, TEXT("同步预热完成: %s, 当前池大小: %d"),
            *SoldierClass->GetName(), Pool.Num());
    }
}

void UXBSoldierPoolSubsystem::AsyncWarmupTick()
{
    if (PendingWarmup.Num() == 0)
    {
        // 没有待预热任务，停止定时器
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(WarmupTimerHandle);
        }
        return;
    }

    // 获取当前任务
    TPair<TSubclassOf<AXBSoldierCharacter>, int32>& CurrentTask = PendingWarmup[0];
    TSubclassOf<AXBSoldierCharacter> SoldierClass = CurrentTask.Key;
    int32& RemainingCount = CurrentTask.Value;

    TArray<AXBSoldierCharacter*>& Pool = GetOrCreatePool(SoldierClass);
    FXBPoolStats& Stats = PoolStatistics.FindOrAdd(SoldierClass);

    // 本帧生成数量
    int32 SpawnThisFrame = FMath::Min(RemainingCount, PoolConfig.MaxSpawnPerFrame);
    SpawnThisFrame = FMath::Min(SpawnThisFrame, PoolConfig.MaxPoolSize - Stats.TotalCreated);

    for (int32 i = 0; i < SpawnThisFrame; ++i)
    {
        if (AXBSoldierCharacter* Soldier = CreateDormantSoldier(SoldierClass))
        {
            Pool.Add(Soldier);
            Stats.TotalCreated++;
            Stats.Available++;
        }
    }

    RemainingCount -= SpawnThisFrame;

    // 任务完成，移除
    if (RemainingCount <= 0 || Stats.TotalCreated >= PoolConfig.MaxPoolSize)
    {
        UE_LOG(LogXBSoldier, Log, TEXT("异步预热完成: %s, 当前池大小: %d"),
            *SoldierClass->GetName(), Pool.Num());
        PendingWarmup.RemoveAt(0);
    }
}

AXBSoldierCharacter* UXBSoldierPoolSubsystem::AcquireSoldier(
    TSubclassOf<AXBSoldierCharacter> SoldierClass,
    const FVector& SpawnLocation,
    const FRotator& SpawnRotation)
{
    if (!SoldierClass)
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("获取士兵失败: 士兵类为空"));
        return nullptr;
    }

    TArray<AXBSoldierCharacter*>& Pool = GetOrCreatePool(SoldierClass);
    FXBPoolStats& Stats = PoolStatistics.FindOrAdd(SoldierClass);
    TSet<AXBSoldierCharacter*>& InUse = InUseSoldiers.FindOrAdd(SoldierClass);

    AXBSoldierCharacter* Soldier = nullptr;

    // 尝试从池中获取
    while (Pool.Num() > 0)
    {
        AXBSoldierCharacter* Candidate = Pool.Pop(false);
        
        // 验证 Actor 有效性
        if (Candidate && IsValid(Candidate) && !Candidate->IsPendingKillPending())
        {
            Soldier = Candidate;
            break;
        }
        else
        {
            // 无效的 Actor，更新统计
            Stats.Available--;
            Stats.TotalCreated--;
        }
    }

    // 池为空，尝试创建新的
    if (!Soldier)
    {
        if (Stats.TotalCreated < PoolConfig.MaxPoolSize)
        {
            Soldier = CreateDormantSoldier(SoldierClass);
            if (Soldier)
            {
                Stats.TotalCreated++;
                UE_LOG(LogXBSoldier, Verbose, TEXT("池已空，创建新士兵: %s"), *Soldier->GetName());
            }
        }
        else
        {
            UE_LOG(LogXBSoldier, Warning, TEXT("对象池已达上限 (%d)，无法获取更多士兵: %s"),
                PoolConfig.MaxPoolSize, *SoldierClass->GetName());
            return nullptr;
        }
    }
    else
    {
        Stats.Available--;
    }

    if (Soldier)
    {
        // 移动到目标位置
        Soldier->SetActorLocation(SpawnLocation);
        Soldier->SetActorRotation(SpawnRotation);

        // 标记为使用中
        InUse.Add(Soldier);
        Stats.InUse++;
        Stats.AcquireCount++;

        // 检查是否需要扩展池
        CheckAndExpandPool(SoldierClass);
    }

    return Soldier;
}

void UXBSoldierPoolSubsystem::ActivateSoldier(
    AXBSoldierCharacter* Soldier,
    UDataTable* DataTable,
    FName RowName,
    EXBFaction Faction)
{
    if (!Soldier || !IsValid(Soldier))
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("激活士兵失败: 士兵无效"));
        return;
    }

    // ✨ 关键：重新初始化数据
    Soldier->InitializeFromDataTable(DataTable, RowName, Faction);

    // 恢复可见性和碰撞
    Soldier->SetActorHiddenInGame(false);
    Soldier->SetActorEnableCollision(true);
    Soldier->SetActorTickEnabled(true);

    // 恢复移动组件
    if (UCharacterMovementComponent* MoveComp = Soldier->GetCharacterMovement())
    {
        MoveComp->SetComponentTickEnabled(true);
        MoveComp->SetMovementMode(MOVE_Walking);
    }

    // 恢复胶囊体碰撞
    if (UCapsuleComponent* Capsule = Soldier->GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }

    // 重置状态
    Soldier->SetSoldierState(EXBSoldierState::Idle);

    UE_LOG(LogXBSoldier, Log, TEXT("士兵激活成功: %s, 数据行: %s, 阵营: %d"),
        *Soldier->GetName(), *RowName.ToString(), static_cast<int32>(Faction));
}

void UXBSoldierPoolSubsystem::ReleaseSoldier(AXBSoldierCharacter* Soldier)
{
    if (!Soldier)
    {
        return;
    }

    // 查找士兵所属的池
    TSubclassOf<AXBSoldierCharacter> SoldierClass = Soldier->GetClass();
    
    TSet<AXBSoldierCharacter*>* InUse = InUseSoldiers.Find(SoldierClass);
    if (!InUse || !InUse->Contains(Soldier))
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("回收士兵失败: 士兵不在使用中列表: %s"), *Soldier->GetName());
        return;
    }

    // 从使用中移除
    InUse->Remove(Soldier);
    
    FXBPoolStats& Stats = PoolStatistics.FindOrAdd(SoldierClass);
    Stats.InUse--;
    Stats.ReleaseCount++;

    // 检查 Actor 有效性
    if (!IsValid(Soldier) || Soldier->IsPendingKillPending())
    {
        Stats.TotalCreated--;
        UE_LOG(LogXBSoldier, Verbose, TEXT("士兵已失效，不放回池中: %s"), *Soldier->GetName());
        return;
    }

    // 重置到休眠状态
    ResetSoldierToDormant(Soldier);

    // 放回池中
    TArray<AXBSoldierCharacter*>& Pool = GetOrCreatePool(SoldierClass);
    Pool.Add(Soldier);
    Stats.Available++;

    UE_LOG(LogXBSoldier, Verbose, TEXT("士兵回收成功: %s, 池可用: %d"), *Soldier->GetName(), Pool.Num());
}

// ==================== 内部方法实现 ====================

AXBSoldierCharacter* UXBSoldierPoolSubsystem::CreateDormantSoldier(TSubclassOf<AXBSoldierCharacter> SoldierClass)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AXBSoldierCharacter* Soldier = World->SpawnActor<AXBSoldierCharacter>(
        SoldierClass,
        DormantLocation,
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (Soldier)
    {
        // 立即设置为休眠状态
        ResetSoldierToDormant(Soldier);

        UE_LOG(LogXBSoldier, Verbose, TEXT("创建休眠士兵: %s"), *Soldier->GetName());
    }

    return Soldier;
}

void UXBSoldierPoolSubsystem::ResetSoldierToDormant(AXBSoldierCharacter* Soldier)
{
    if (!Soldier || !IsValid(Soldier))
    {
        return;
    }

    // 移动到休眠位置
    Soldier->SetActorLocation(DormantLocation);

    // 隐藏和禁用
    Soldier->SetActorHiddenInGame(true);
    Soldier->SetActorEnableCollision(false);
    Soldier->SetActorTickEnabled(false);

    // 禁用移动组件
    if (UCharacterMovementComponent* MoveComp = Soldier->GetCharacterMovement())
    {
        MoveComp->SetComponentTickEnabled(false);
        MoveComp->StopMovementImmediately();
        MoveComp->DisableMovement();
    }

    // 禁用胶囊体碰撞
    if (UCapsuleComponent* Capsule = Soldier->GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // 重置跟随组件
    if (UXBSoldierFollowComponent* FollowComp = Soldier->FollowComponent)
    {
        FollowComp->SetFollowTarget(nullptr);
        FollowComp->SetFollowMode(EXBFollowMode::Free);
    }

    // 🔧 关键：重置关键状态变量（不调用 InitializeFromDataTable，因为那会加载资源）
    // 这些变量会在 ActivateSoldier 中被正确设置
    
    // 使用反射或友元访问私有变量（这里通过公开方法）
    Soldier->SetSoldierState(EXBSoldierState::Idle);
    
    // 清除攻击目标
    Soldier->CurrentAttackTarget = nullptr;

    // 解除 AI 控制器（回收时不需要 AI）
    if (AController* Controller = Soldier->GetController())
    {
        Controller->UnPossess();
    }
}

void UXBSoldierPoolSubsystem::CheckAndExpandPool(TSubclassOf<AXBSoldierCharacter> SoldierClass)
{
    TArray<AXBSoldierCharacter*>& Pool = GetOrCreatePool(SoldierClass);
    FXBPoolStats& Stats = PoolStatistics.FindOrAdd(SoldierClass);

    // 如果可用数量低于低水位，且未达到最大值，则扩展
    if (Pool.Num() < PoolConfig.LowWaterMark && Stats.TotalCreated < PoolConfig.MaxPoolSize)
    {
        int32 ExpandCount = FMath::Min(PoolConfig.ExpandStep, PoolConfig.MaxPoolSize - Stats.TotalCreated);
        
        UE_LOG(LogXBSoldier, Log, TEXT("池低水位告警，异步扩展: %s, 扩展数量: %d"),
            *SoldierClass->GetName(), ExpandCount);

        // 使用异步预热扩展
        WarmupPool(SoldierClass, ExpandCount, true);
        Stats.ExpandCount++;
    }
}

TArray<AXBSoldierCharacter*>& UXBSoldierPoolSubsystem::GetOrCreatePool(TSubclassOf<AXBSoldierCharacter> SoldierClass)
{
    return AvailablePools.FindOrAdd(SoldierClass);
}

// ==================== 统计和调试 ====================

FXBPoolStats UXBSoldierPoolSubsystem::GetPoolStats(TSubclassOf<AXBSoldierCharacter> SoldierClass) const
{
    if (const FXBPoolStats* Stats = PoolStatistics.Find(SoldierClass))
    {
        return *Stats;
    }
    return FXBPoolStats();
}

void UXBSoldierPoolSubsystem::PrintPoolReport() const
{
    UE_LOG(LogXBSoldier, Warning, TEXT(""));
    UE_LOG(LogXBSoldier, Warning, TEXT("╔══════════════════════════════════════════════════════════════╗"));
    UE_LOG(LogXBSoldier, Warning, TEXT("║              士兵对象池统计报告                              ║"));
    UE_LOG(LogXBSoldier, Warning, TEXT("╠══════════════════════════════════════════════════════════════╣"));

    for (const auto& Pair : PoolStatistics)
    {
        const FXBPoolStats& Stats = Pair.Value;
        FString ClassName = Pair.Key ? Pair.Key->GetName() : TEXT("Unknown");

        UE_LOG(LogXBSoldier, Warning, TEXT("║ 类型: %-50s ║"), *ClassName);
        UE_LOG(LogXBSoldier, Warning, TEXT("║   总创建: %6d | 可用: %6d | 使用中: %6d          ║"), 
            Stats.TotalCreated, Stats.Available, Stats.InUse);
        UE_LOG(LogXBSoldier, Warning, TEXT("║   获取次数: %8lld | 回收次数: %8lld | 扩展: %3d      ║"), 
            Stats.AcquireCount, Stats.ReleaseCount, Stats.ExpandCount);
        UE_LOG(LogXBSoldier, Warning, TEXT("╠══════════════════════════════════════════════════════════════╣"));
    }

    UE_LOG(LogXBSoldier, Warning, TEXT("╚══════════════════════════════════════════════════════════════╝"));
    UE_LOG(LogXBSoldier, Warning, TEXT(""));
}

void UXBSoldierPoolSubsystem::ClearAllPools(bool bDestroyActors)
{
    if (bDestroyActors)
    {
        // 销毁所有池中的 Actor
        for (auto& Pair : AvailablePools)
        {
            for (AXBSoldierCharacter* Soldier : Pair.Value)
            {
                if (Soldier && IsValid(Soldier))
                {
                    Soldier->Destroy();
                }
            }
        }

        for (auto& Pair : InUseSoldiers)
        {
            for (AXBSoldierCharacter* Soldier : Pair.Value)
            {
                if (Soldier && IsValid(Soldier))
                {
                    Soldier->Destroy();
                }
            }
        }
    }

    AvailablePools.Empty();
    InUseSoldiers.Empty();
    PoolStatistics.Empty();
    PendingWarmup.Empty();

    UE_LOG(LogXBSoldier, Log, TEXT("所有对象池已清空"));
}