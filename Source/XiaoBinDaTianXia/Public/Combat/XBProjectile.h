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

class UCapsuleComponent;
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

    /**
     * @brief  初始化投射物
     * @param  InSourceActor 来源Actor
     * @param  InDamage 伤害数值
     * @param  ShootDirection 发射方向
     * @param  InSpeed 发射速度
     * @param  bInUseArc 是否使用抛射
     * @note   详细流程分析: 写入来源/伤害/速度 -> 设置投射物速度 -> 根据抛射参数修正Z轴速度
     *         性能/架构注意事项: 该接口仅负责运动与基础伤害，不负责对象池状态切换
     */
    UFUNCTION(BlueprintCallable, Category = "投射物", meta = (DisplayName = "初始化投射物"))
    void InitializeProjectile(AActor* InSourceActor, float InDamage, const FVector& ShootDirection, float InSpeed, bool bInUseArc);

    /**
     * @brief  从对象池激活投射物
     * @param  SpawnLocation 生成位置
     * @param  SpawnRotation 生成旋转
     * @note   详细流程分析: 恢复显示与碰撞 -> 清理运动状态 -> 更新Transform
     *         性能/架构注意事项: 与对象池配合使用，避免频繁Spawn/Destroy
     */
    UFUNCTION(BlueprintCallable, Category = "投射物", meta = (DisplayName = "从对象池激活"))
    void ActivateFromPool(const FVector& SpawnLocation, const FRotator& SpawnRotation);

    /**
     * @brief  重置投射物以便回收
     * @note   详细流程分析: 停止运动 -> 关闭碰撞 -> 隐藏Actor -> 清理来源引用
     *         性能/架构注意事项: 由对象池统一回收，避免残留引用
     */
    UFUNCTION(BlueprintCallable, Category = "投射物", meta = (DisplayName = "重置到对象池"))
    void ResetForPooling();

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "投射物", meta = (DisplayName = "启用对象池"))
    bool bUsePooling = true;

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

    UPROPERTY(VisibleAnywhere, Category = "组件", meta = (DisplayName = "碰撞胶囊"))
    TObjectPtr<UCapsuleComponent> CollisionComponent;

    UPROPERTY(VisibleAnywhere, Category = "组件", meta = (DisplayName = "网格组件"))
    TObjectPtr<UStaticMeshComponent> MeshComponent;

    UPROPERTY(VisibleAnywhere, Category = "组件", meta = (DisplayName = "投射物运动"))
    TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

    TWeakObjectPtr<AActor> SourceActor;

    FGameplayTag DamageTag;
};
