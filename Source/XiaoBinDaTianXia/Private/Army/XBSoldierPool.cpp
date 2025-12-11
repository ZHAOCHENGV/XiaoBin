// Copyright XiaoBing Project. All Rights Reserved.

#include "Army/XBSoldierPool.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"

// ==================== AXBSoldierCollisionProxy ====================

AXBSoldierCollisionProxy::AXBSoldierCollisionProxy()
{
    PrimaryActorTick.bCanEverTick = false;

    // 创建碰撞胶囊
    CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
    CapsuleComponent->InitCapsuleSize(34.0f, 88.0f);
    CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
    CapsuleComponent->SetGenerateOverlapEvents(true);

    RootComponent = CapsuleComponent;

    // 默认不激活
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
}

void AXBSoldierCollisionProxy::InitializeProxy(int32 InSoldierId)
{
    SoldierId = InSoldierId;
}

void AXBSoldierCollisionProxy::ActivateProxy(const FVector& Location)
{
    bIsActive = true;
    SetActorLocation(Location);
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);

    // 绑定碰撞事件
    if (CapsuleComponent && !CapsuleComponent->OnComponentBeginOverlap.IsBound())
    {
        CapsuleComponent->OnComponentBeginOverlap.AddDynamic(this, &AXBSoldierCollisionProxy::OnCapsuleBeginOverlap);
    }
}

void AXBSoldierCollisionProxy::DeactivateProxy()
{
    bIsActive = false;
    SoldierId = INDEX_NONE;
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorLocation(FVector(0, 0, -10000)); // 移到地下
}

void AXBSoldierCollisionProxy::UpdateLocation(const FVector& NewLocation)
{
    if (bIsActive)
    {
        SetActorLocation(NewLocation);
    }
}

void AXBSoldierCollisionProxy::OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!bIsActive || SoldierId == INDEX_NONE)
    {
        return;
    }

    // 检查是否是另一个代理
    if (AXBSoldierCollisionProxy* OtherProxy = Cast<AXBSoldierCollisionProxy>(OtherActor))
    {
        // TODO: 通知 ArmySubsystem 处理士兵碰撞/攻击
    }
}

// ==================== UXBSoldierPool ====================

UXBSoldierPool::UXBSoldierPool()
{
    ProxyClass = AXBSoldierCollisionProxy::StaticClass();
}

void UXBSoldierPool::Initialize(UWorld* InWorld, int32 InitialPoolSize)
{
    if (!InWorld)
    {
        UE_LOG(LogTemp, Error, TEXT("UXBSoldierPool::Initialize - Invalid World!"));
        return;
    }

    OwningWorld = InWorld;

    // 预创建代理
    ExpandPool(InitialPoolSize);

    UE_LOG(LogTemp, Log, TEXT("UXBSoldierPool Initialized with %d proxies"), InitialPoolSize);
}

void UXBSoldierPool::Cleanup()
{
    // 销毁所有活跃代理
    for (auto& Pair : ActiveProxies)
    {
        if (Pair.Value)
        {
            Pair.Value->Destroy();
        }
    }
    ActiveProxies.Empty();

    // 销毁池中的代理
    for (AXBSoldierCollisionProxy* Proxy : PooledProxies)
    {
        if (Proxy)
        {
            Proxy->Destroy();
        }
    }
    PooledProxies.Empty();

    OwningWorld.Reset();

    UE_LOG(LogTemp, Log, TEXT("UXBSoldierPool Cleaned up"));
}

AXBSoldierCollisionProxy* UXBSoldierPool::AcquireProxy(int32 SoldierId, const FVector& Location)
{
    // 检查是否已有该士兵的代理
    if (TObjectPtr<AXBSoldierCollisionProxy>* ExistingProxy = ActiveProxies.Find(SoldierId))
    {
        (*ExistingProxy)->UpdateLocation(Location);
        return *ExistingProxy;
    }

    // 从池中获取或创建新代理
    AXBSoldierCollisionProxy* Proxy = nullptr;

    if (PooledProxies.Num() > 0)
    {
        Proxy = PooledProxies.Pop();
    }
    else
    {
        Proxy = CreateNewProxy();
    }

    if (Proxy)
    {
        Proxy->InitializeProxy(SoldierId);
        Proxy->ActivateProxy(Location);
        ActiveProxies.Add(SoldierId, Proxy);
    }

    return Proxy;
}

void UXBSoldierPool::ReleaseProxy(AXBSoldierCollisionProxy* Proxy)
{
    if (!Proxy)
    {
        return;
    }

    int32 SoldierId = Proxy->GetSoldierId();
    
    // 从活跃映射移除
    ActiveProxies.Remove(SoldierId);

    // 停用并归还池
    Proxy->DeactivateProxy();
    PooledProxies.Add(Proxy);
}

void UXBSoldierPool::ReleaseProxyBySoldierId(int32 SoldierId)
{
    if (TObjectPtr<AXBSoldierCollisionProxy>* ProxyPtr = ActiveProxies.Find(SoldierId))
    {
        ReleaseProxy(*ProxyPtr);
    }
}

AXBSoldierCollisionProxy* UXBSoldierPool::CreateNewProxy()
{
    if (!OwningWorld.IsValid() || !ProxyClass)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AXBSoldierCollisionProxy* NewProxy = OwningWorld->SpawnActor<AXBSoldierCollisionProxy>(
        ProxyClass, FVector(0, 0, -10000), FRotator::ZeroRotator, SpawnParams);

    if (NewProxy)
    {
        NewProxy->DeactivateProxy();
    }

    return NewProxy;
}

void UXBSoldierPool::ExpandPool(int32 Count)
{
    PooledProxies.Reserve(PooledProxies.Num() + Count);

    for (int32 i = 0; i < Count; ++i)
    {
        if (AXBSoldierCollisionProxy* NewProxy = CreateNewProxy())
        {
            PooledProxies.Add(NewProxy);
        }
    }
}