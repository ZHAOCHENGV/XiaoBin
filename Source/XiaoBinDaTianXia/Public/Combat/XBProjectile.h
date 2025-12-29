/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Combat/XBProjectile.h

/**
 * @file XBProjectile.h
 * @brief 远程投射物基类 - 支持直线与抛射模式
 * 
 * @note ✨ 新增文件
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Army/XBSoldierTypes.h"
#include "XBProjectile.generated.h"

class USphereComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UGameplayEffect;
struct FHitResult;

UCLASS()
class XIAOBINDATIANXIA_API AXBProjectile : public AActor
{
    GENERATED_BODY()

public:
    AXBProjectile();

    UFUNCTION(BlueprintCallable, Category = "投射物", meta = (DisplayName = "初始化投射物"))
    void InitializeProjectile(AActor* InSourceActor, float InDamage, const FVector& ShootDirection, float InSpeed, bool bInUseArc);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "投射物", meta = (DisplayName = "基础伤害", ClampMin = "0.0"))
    float Damage = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "投射物", meta = (DisplayName = "直线速度", ClampMin = "0.0"))
    float LinearSpeed = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "投射物", meta = (DisplayName = "抛射模式"))
    bool bUseArc = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "投射物", meta = (DisplayName = "抛射上抛速度"))
    float ArcLaunchSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "投射物", meta = (DisplayName = "抛射重力缩放"))
    float ArcGravityScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "投射物", meta = (DisplayName = "命中后销毁"))
    bool bDestroyOnHit = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "伤害", meta = (DisplayName = "伤害效果"))
    TSubclassOf<UGameplayEffect> DamageEffectClass;

protected:
    virtual void BeginPlay() override;

private:
    UFUNCTION()
    void OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    void ApplyDamageToTarget(AActor* TargetActor, const FHitResult& HitResult);
    bool GetTargetFaction(AActor* TargetActor, EXBFaction& OutFaction) const;

    UPROPERTY(VisibleAnywhere, Category = "组件")
    TObjectPtr<USphereComponent> CollisionComponent;

    UPROPERTY(VisibleAnywhere, Category = "组件")
    TObjectPtr<UStaticMeshComponent> MeshComponent;

    UPROPERTY(VisibleAnywhere, Category = "组件")
    TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

    TWeakObjectPtr<AActor> SourceActor;

    FGameplayTag DamageTag;
};
