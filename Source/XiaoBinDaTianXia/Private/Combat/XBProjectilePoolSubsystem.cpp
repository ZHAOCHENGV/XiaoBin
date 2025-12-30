/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Combat/XBProjectilePoolSubsystem.cpp

/**
 * @file XBProjectilePoolSubsystem.cpp
 * @brief 投射物对象池子系统实现
 * 
 * @note ✨ 新增文件
 */

#include "Combat/XBProjectilePoolSubsystem.h"
#include "Combat/XBProjectile.h"
#include "Utils/XBLogCategories.h"

const FVector UXBProjectilePoolSubsystem::RecycleLocation = FVector(0.0f, 0.0f, -12000.0f);

void UXBProjectilePoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UXBProjectilePoolSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

bool UXBProjectilePoolSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    return true;
}

void UXBProjectilePoolSubsystem::ReleaseProjectile(AXBProjectile* Projectile)
{
    if (!Projectile)
    {
        return;
    }

    TSubclassOf<AXBProjectile> ProjectileClass = Projectile->GetClass();
    if (!ProjectileClass)
    {
        return;
    }

    // 🔧 修改 - 重置投射物为池化休眠态，避免残留速度与碰撞
    Projectile->ResetForPooling();
    Projectile->SetActorLocation(RecycleLocation);

    // 🔧 修改 - 通过桶结构存储，兼容UHT对TMap值类型的限制
    RecycledProjectiles.FindOrAdd(ProjectileClass).Projectiles.Add(Projectile);

    Stats.ReleaseCount += 1;
    Stats.PoolSize += 1;

    UE_LOG(LogXBCombat, Verbose, TEXT("投射物 %s 已回收到对象池，池大小=%d"),
        *Projectile->GetName(), Stats.PoolSize);
}

AXBProjectile* UXBProjectilePoolSubsystem::AcquireProjectile(TSubclassOf<AXBProjectile> ProjectileClass, const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
    if (!ProjectileClass)
    {
        UE_LOG(LogXBCombat, Warning, TEXT("获取投射物失败：ProjectileClass为空"));
        return nullptr;
    }

    AXBProjectile* Projectile = nullptr;
    if (FXBProjectilePoolBucket* Pool = RecycledProjectiles.Find(ProjectileClass))
    {
        if (Pool->Projectiles.Num() > 0)
        {
            Projectile = Pool->Projectiles.Pop();
        }
    }

    if (Projectile)
    {
        // 🔧 修改 - 复用对象池实例，减少Spawn成本
        Projectile->ActivateFromPool(SpawnLocation, SpawnRotation);
        Stats.ReuseCount += 1;
        Stats.PoolSize = FMath::Max(0, Stats.PoolSize - 1);

        UE_LOG(LogXBCombat, Verbose, TEXT("投射物 %s 复用成功，池大小=%d"),
            *Projectile->GetName(), Stats.PoolSize);
        return Projectile;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogXBCombat, Warning, TEXT("获取投射物失败：World无效"));
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    Projectile = World->SpawnActor<AXBProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
    if (!Projectile)
    {
        UE_LOG(LogXBCombat, Warning, TEXT("获取投射物失败：SpawnActor失败"));
        return nullptr;
    }

    Projectile->ActivateFromPool(SpawnLocation, SpawnRotation);

    UE_LOG(LogXBCombat, Verbose, TEXT("投射物 %s 新建完成"), *Projectile->GetName());

    return Projectile;
}

void UXBProjectilePoolSubsystem::PrewarmProjectiles(TSubclassOf<AXBProjectile> ProjectileClass, int32 PreloadCount)
{
    if (!ProjectileClass || PreloadCount <= 0)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FXBProjectilePoolBucket& Bucket = RecycledProjectiles.FindOrAdd(ProjectileClass);

    // 🔧 修改 - 仅补足到目标数量，避免重复预热
    const int32 MissingCount = PreloadCount - Bucket.Projectiles.Num();
    if (MissingCount <= 0)
    {
        return;
    }

    for (int32 Index = 0; Index < MissingCount; ++Index)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AXBProjectile* Projectile = World->SpawnActor<AXBProjectile>(ProjectileClass, RecycleLocation, FRotator::ZeroRotator, SpawnParams);
        if (!Projectile)
        {
            continue;
        }

        Projectile->ResetForPooling();
        Bucket.Projectiles.Add(Projectile);
        Stats.PoolSize += 1;
    }

    UE_LOG(LogXBCombat, Log, TEXT("投射物对象池预加载完成，类=%s 数量=%d"),
        *ProjectileClass->GetName(), Bucket.Projectiles.Num());
}
