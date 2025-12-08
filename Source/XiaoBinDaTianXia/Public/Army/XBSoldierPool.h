// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Army/XBSoldierTypes.h"
#include "XBSoldierPool.generated.h"

/**
 * 士兵碰撞代理 Actor（用于战斗时的碰撞检测）
 */
UCLASS()
class XIAOBINDATIANXIA_API AXBSoldierCollisionProxy : public AActor
{
    GENERATED_BODY()

public:
    AXBSoldierCollisionProxy();

    /** 初始化代理 */
    void InitializeProxy(int32 InSoldierId);

    /** 激活代理 */
    void ActivateProxy(const FVector& Location);

    /** 停用代理 */
    void DeactivateProxy();

    /** 更新位置 */
    void UpdateLocation(const FVector& NewLocation);

    /** 获取关联的士兵ID */
    int32 GetSoldierId() const { return SoldierId; }

    /** 是否激活 */
    bool IsProxyActive() const { return bIsActive; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "XB|Collision")
    TObjectPtr<class UCapsuleComponent> CapsuleComponent;

    UPROPERTY()
    int32 SoldierId = INDEX_NONE;

    UPROPERTY()
    bool bIsActive = false;

    UFUNCTION()
    void OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};

/**
 * 士兵对象池
 * 管理碰撞代理的复用
 */
UCLASS(BlueprintType)
class XIAOBINDATIANXIA_API UXBSoldierPool : public UObject
{
    GENERATED_BODY()

public:
    UXBSoldierPool();

    /** 初始化对象池 */
    UFUNCTION(BlueprintCallable, Category = "XB|Pool")
    void Initialize(UWorld* InWorld, int32 InitialPoolSize = 100);

    /** 清理对象池 */
    UFUNCTION(BlueprintCallable, Category = "XB|Pool")
    void Cleanup();

    /** 获取碰撞代理 */
    UFUNCTION(BlueprintCallable, Category = "XB|Pool")
    AXBSoldierCollisionProxy* AcquireProxy(int32 SoldierId, const FVector& Location);

    /** 归还碰撞代理 */
    UFUNCTION(BlueprintCallable, Category = "XB|Pool")
    void ReleaseProxy(AXBSoldierCollisionProxy* Proxy);

    /** 归还指定士兵的代理 */
    UFUNCTION(BlueprintCallable, Category = "XB|Pool")
    void ReleaseProxyBySoldierId(int32 SoldierId);

    /** 获取当前活跃代理数量 */
    UFUNCTION(BlueprintCallable, Category = "XB|Pool")
    int32 GetActiveProxyCount() const { return ActiveProxies.Num(); }

    /** 获取池中可用代理数量 */
    UFUNCTION(BlueprintCallable, Category = "XB|Pool")
    int32 GetAvailableProxyCount() const { return PooledProxies.Num(); }

protected:
    /** 池中的可用代理 */
    UPROPERTY()
    TArray<TObjectPtr<AXBSoldierCollisionProxy>> PooledProxies;

    /** 活跃的代理 */
    UPROPERTY()
    TMap<int32, TObjectPtr<AXBSoldierCollisionProxy>> ActiveProxies;

    /** 所属世界 */
    UPROPERTY()
    TWeakObjectPtr<UWorld> OwningWorld;

    /** 代理模板类 */
    UPROPERTY()
    TSubclassOf<AXBSoldierCollisionProxy> ProxyClass;

private:
    /** 创建新代理 */
    AXBSoldierCollisionProxy* CreateNewProxy();

    /** 扩展池 */
    void ExpandPool(int32 Count);
};