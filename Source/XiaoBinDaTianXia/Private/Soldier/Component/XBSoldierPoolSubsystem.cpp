/* --- 需要替换 XBSoldierPoolSubsystem.cpp --- */

/**
 * @file XBSoldierPoolSubsystem.cpp
 * @brief 士兵对象池子系统实现（简化版）
 */

#include "Soldier/Component/XBSoldierPoolSubsystem.h"
#include "Utils/XBLogCategories.h"
#include "Soldier/XBSoldierCharacter.h"

const FVector UXBSoldierPoolSubsystem::RecycleLocation = FVector(0.0f, 0.0f, -10000.0f);

void UXBSoldierPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogXBSoldier, Log, TEXT("士兵对象池子系统初始化（简化版）"));
}

void UXBSoldierPoolSubsystem::Deinitialize()
{
    PrintPoolReport();
    RecycledSoldiers.Empty();
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

void UXBSoldierPoolSubsystem::ReleaseSoldier(AXBSoldierCharacter* Soldier)
{
    if (!Soldier || !IsValid(Soldier))
    {
        return;
    }

    // 检查是否已在池中
    if (RecycledSoldiers.Contains(Soldier))
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s 已在回收池中"), *Soldier->GetName());
        return;
    }

    // 重置士兵状态
    Soldier->ResetForPooling();

    // 移动到回收位置
    Soldier->SetActorLocation(RecycleLocation);

    // 添加到池
    RecycledSoldiers.Add(Soldier);
    Stats.ReleaseCount++;
    Stats.PoolSize = RecycledSoldiers.Num();

    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 已回收到池中，当前池大小: %d"), 
        *Soldier->GetName(), Stats.PoolSize);
}

AXBSoldierCharacter* UXBSoldierPoolSubsystem::AcquireSoldier(const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
    // 清理无效引用
    RecycledSoldiers.RemoveAll([](AXBSoldierCharacter* Soldier)
    {
        return !Soldier || !IsValid(Soldier);
    });

    if (RecycledSoldiers.Num() == 0)
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("对象池为空，无法获取士兵"));
        return nullptr;
    }

    // 从池中取出
    AXBSoldierCharacter* Soldier = RecycledSoldiers.Pop(EAllowShrinking::No);
    Stats.ReuseCount++;
    Stats.PoolSize = RecycledSoldiers.Num();

    // 移动到目标位置
    Soldier->SetActorLocation(SpawnLocation);
    Soldier->SetActorRotation(SpawnRotation);

    // 切换到站立休眠态（可被招募）
    Soldier->EnterDormantState(EXBDormantType::Standing);

    UE_LOG(LogXBSoldier, Log, TEXT("从池中获取士兵 %s，剩余池大小: %d"), 
        *Soldier->GetName(), Stats.PoolSize);

    return Soldier;
}

void UXBSoldierPoolSubsystem::PrintPoolReport() const
{
    UE_LOG(LogXBSoldier, Warning, TEXT(""));
    UE_LOG(LogXBSoldier, Warning, TEXT("╔══════════════════════════════════════════╗"));
    UE_LOG(LogXBSoldier, Warning, TEXT("║        士兵对象池统计报告                ║"));
    UE_LOG(LogXBSoldier, Warning, TEXT("╠══════════════════════════════════════════╣"));
    UE_LOG(LogXBSoldier, Warning, TEXT("║ 当前池大小:     %6d                   ║"), Stats.PoolSize);
    UE_LOG(LogXBSoldier, Warning, TEXT("║ 总回收次数:     %6lld                   ║"), Stats.ReleaseCount);
    UE_LOG(LogXBSoldier, Warning, TEXT("║ 总复用次数:     %6lld                   ║"), Stats.ReuseCount);
    UE_LOG(LogXBSoldier, Warning, TEXT("╚══════════════════════════════════════════╝"));
    UE_LOG(LogXBSoldier, Warning, TEXT(""));
}
