/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Combat/XBProjectile.h

/**
 * @file XBProjectile.h
 * @brief 远程投射物基类 - 支持直线与抛射模式
 *
 * @note ✨ 新增文件
 */

#pragma once

#include "Army/XBSoldierTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "XBProjectile.generated.h"

class UCapsuleComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UGameplayEffect;
class USoundBase;
class UNiagaraSystem;
class UBoxComponent;
struct FHitResult;
class UNiagaraComponent;
class UParticleSystem;

/**
 * @brief 投射物碰撞体类型
 */
UENUM(BlueprintType)
enum class EXBProjectileCollisionType : uint8 {
  /** 胶囊体碰撞 */
  Capsule UMETA(DisplayName = "胶囊体"),
  /** 方体碰撞 */
  Box UMETA(DisplayName = "方体")
};

/**
 * @brief 投射物发射模式
 */
UENUM(BlueprintType)
enum class EXBProjectileLaunchMode : uint8 {
  /** 直线飞行，不受重力影响 */
  Linear UMETA(DisplayName = "直线"),
  /** 抛物线飞行，受重力影响 */
  Arc UMETA(DisplayName = "抛物线")
};

/**
 * @brief 投射物伤害类型
 */
UENUM(BlueprintType)
enum class EXBProjectileDamageType : uint8 {
  /** 仅飞行伤害（碰撞目标时造成伤害） */
  FlightOnly UMETA(DisplayName = "仅飞行伤害"),
  /** 仅爆炸伤害（命中时范围爆炸） */
  ExplosionOnly UMETA(DisplayName = "仅爆炸伤害"),
  /** 飞行+爆炸伤害（两者都能造成伤害） */
  Both UMETA(DisplayName = "飞行+爆炸伤害")
};

/**
 * @brief 击中特效类型（Niagara 或 Cascade）
 */
UENUM(BlueprintType)
enum class EXBHitEffectType : uint8 {
  /** Niagara 粒子系统 */
  Niagara UMETA(DisplayName = "Niagara"),
  /** Cascade 粒子系统 */
  Cascade UMETA(DisplayName = "Cascade")
};

UCLASS()
class XIAOBINDATIANXIA_API AXBProjectile : public AActor {
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
   * @note   详细流程分析: 直线模式使用方向速度，抛射模式回退上抛速度
   *         性能/架构注意事项: 对蓝图暴露，避免UHT参数兼容问题
   */
  UFUNCTION(BlueprintCallable, Category = "投射物",
            meta = (DisplayName = "初始化投射物"))
  void InitializeProjectile(AActor *InSourceActor, float InDamage,
                            const FVector &ShootDirection, float InSpeed,
                            bool bInUseArc);

  /**
   * @brief  初始化投射物（带目标位置）
   * @param  InSourceActor 来源Actor
   * @param  InDamage 伤害数值
   * @param  ShootDirection 发射方向
   * @param  InSpeed 发射速度
   * @param  bInUseArc 是否使用抛射
   * @param  TargetLocation 目标位置（抛射使用）
   * @note   详细流程分析: 抛射模式优先使用目标位置计算抛物线速度
   *         性能/架构注意事项: 仅C++使用，避免UHT参数差异
   */
  void InitializeProjectileWithTarget(AActor *InSourceActor, float InDamage,
                                      const FVector &ShootDirection,
                                      float InSpeed, bool bInUseArc,
                                      const FVector &TargetLocation);

  /**
   * @brief  从对象池激活投射物
   * @param  SpawnLocation 生成位置
   * @param  SpawnRotation 生成旋转
   * @note   详细流程分析: 恢复显示与碰撞 -> 清理运动状态 -> 更新Transform
   *         性能/架构注意事项: 与对象池配合使用，避免频繁Spawn/Destroy
   */
  UFUNCTION(BlueprintCallable, Category = "发射物配置",
            meta = (DisplayName = "从对象池激活"))
  void ActivateFromPool(const FVector &SpawnLocation,
                        const FRotator &SpawnRotation);

  /**
   * @brief  重置投射物以便回收
   * @note   详细流程分析: 停止运动 -> 关闭碰撞 -> 隐藏Actor -> 清理来源引用
   *         性能/架构注意事项: 由对象池统一回收，避免残留引用
   */
  UFUNCTION(BlueprintCallable, Category = "发射物配置",
            meta = (DisplayName = "重置到对象池"))
  void ResetForPooling();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置",
            meta = (DisplayName = "基础伤害", ClampMin = "0.0"))
  float Damage = 10.0f;

  /** 发射模式 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置",
            meta = (DisplayName = "发射模式"))
  EXBProjectileLaunchMode LaunchMode = EXBProjectileLaunchMode::Linear;

  /** 直线速度（仅直线模式生效） */
  UPROPERTY(
      EditAnywhere, BlueprintReadWrite, Category = "发射物配置",
      meta = (DisplayName = "直线速度", ClampMin = "0.0",
              EditCondition = "LaunchMode == EXBProjectileLaunchMode::Linear"))
  float LinearSpeed = 1200.0f;

  /** 抛物线初速度（仅抛物线模式生效） */
  UPROPERTY(
      EditAnywhere, BlueprintReadWrite, Category = "发射物配置",
      meta = (DisplayName = "抛物线初速度", ClampMin = "0.0",
              EditCondition = "LaunchMode == EXBProjectileLaunchMode::Arc"))
  float ArcSpeed = 800.0f;

  /** 抛物线飞行距离（水平距离，仅抛物线模式生效） */
  UPROPERTY(
      EditAnywhere, BlueprintReadWrite, Category = "发射物配置",
      meta = (DisplayName = "抛物线飞行距离", ClampMin = "0.0",
              EditCondition = "LaunchMode == EXBProjectileLaunchMode::Arc"))
  float ArcDistance = 500.0f;

  /** 抛物线重力缩放（仅抛物线模式生效） */
  UPROPERTY(
      EditAnywhere, BlueprintReadWrite, Category = "发射物配置",
      meta = (DisplayName = "抛物线重力缩放", ClampMin = "0.0",
              EditCondition = "LaunchMode == EXBProjectileLaunchMode::Arc"))
  float ArcGravityScale = 1.0f;

  /** 最大存活时间 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置",
            meta = (DisplayName = "最大存活时间", ClampMin = "0.0"))
  float LifeSeconds = 3.0f;

  /** 启用对象池 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置",
            meta = (DisplayName = "启用对象池"))
  bool bUsePooling = true;

  /** 命中后销毁 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置",
            meta = (DisplayName = "命中后销毁"))
  bool bDestroyOnHit = true;

  /** 拖尾渐隐延迟时间（销毁前等待拖尾消散的时间） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置",
            meta = (DisplayName = "拖尾渐隐延迟", ClampMin = "0.0", ClampMax = "2.0"))
  float TrailFadeDelay = 0.3f;

  /** 启用网格渐隐（通过材质参数控制透明度） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置|网格渐隐",
            meta = (DisplayName = "启用网格渐隐"))
  bool bEnableMeshFade = false;

  /** 网格渐隐材质参数名称（Scalar参数） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置|网格渐隐",
            meta = (DisplayName = "渐隐参数名称",
                    EditCondition = "bEnableMeshFade", EditConditionHides))
  FName MeshFadeParameterName = FName("Opacity");

  /** 网格渐隐时间（秒） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置|网格渐隐",
            meta = (DisplayName = "渐隐时间", ClampMin = "0.05", ClampMax = "2.0",
                    EditCondition = "bEnableMeshFade", EditConditionHides))
  float MeshFadeDuration = 0.2f;

  /** 伤害效果（GAS） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置|伤害",
            meta = (DisplayName = "伤害效果"))
  TSubclassOf<UGameplayEffect> DamageEffectClass;

  /** 伤害类型 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置|伤害",
            meta = (DisplayName = "伤害类型"))
  EXBProjectileDamageType DamageType = EXBProjectileDamageType::FlightOnly;

  // ========== 爆炸效果配置（ExplosionOnly 或 Both 时显示） ==========

  /** 爆炸半径 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置|爆炸效果",
            meta = (DisplayName = "爆炸半径", ClampMin = "10.0",
                    EditCondition =
                        "DamageType == EXBProjectileDamageType::ExplosionOnly || DamageType == EXBProjectileDamageType::Both",
                    EditConditionHides))
  float ExplosionRadius = 200.0f;

  /** 爆炸伤害（0表示使用基础伤害） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置|爆炸效果",
            meta = (DisplayName = "爆炸伤害", ClampMin = "0.0",
                    EditCondition =
                        "DamageType == EXBProjectileDamageType::ExplosionOnly || DamageType == EXBProjectileDamageType::Both",
                    EditConditionHides))
  float ExplosionDamage = 0.0f;

  /** 爆炸特效（Niagara） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置|爆炸效果",
            meta = (DisplayName = "爆炸特效",
                    EditCondition =
                        "DamageType == EXBProjectileDamageType::ExplosionOnly || DamageType == EXBProjectileDamageType::Both",
                    EditConditionHides))
  TObjectPtr<UNiagaraSystem> ExplosionEffect;

  /** 爆炸特效缩放 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置|爆炸效果",
            meta = (DisplayName = "爆炸特效缩放", ClampMin = "0.1",
                    EditCondition =
                        "DamageType == EXBProjectileDamageType::ExplosionOnly || DamageType == EXBProjectileDamageType::Both",
                    EditConditionHides))
  float ExplosionEffectScale = 1.0f;

  /** 爆炸音效（旧，将被废弃） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置|爆炸效果",
            meta = (DisplayName = "爆炸音效（旧）",
                    EditCondition =
                        "DamageType == EXBProjectileDamageType::ExplosionOnly || DamageType == EXBProjectileDamageType::Both",
                    EditConditionHides))
  TObjectPtr<USoundBase> ExplosionSound;

  /** 爆炸音效Tag（优先使用） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置|爆炸效果",
            meta = (DisplayName = "爆炸音效Tag",
                    EditCondition =
                        "DamageType == EXBProjectileDamageType::ExplosionOnly || DamageType == EXBProjectileDamageType::Both",
                    EditConditionHides))
  FGameplayTag ExplosionSoundTag;

  /** 启用爆炸半径调试可视化（运行时绘制爆炸范围球体） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置|爆炸效果",
            meta = (DisplayName = "调试爆炸半径",
                    EditCondition =
                        "DamageType == EXBProjectileDamageType::ExplosionOnly || DamageType == EXBProjectileDamageType::Both",
                    EditConditionHides))
  bool bDebugExplosionRadius = false;

  /** 爆炸检测对象类型（默认检测士兵和主将通道） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置|爆炸效果",
            meta = (DisplayName = "爆炸检测对象类型",
                    EditCondition =
                        "DamageType == EXBProjectileDamageType::ExplosionOnly || DamageType == EXBProjectileDamageType::Both",
                    EditConditionHides))
  TArray<TEnumAsByte<EObjectTypeQuery>> ExplosionObjectTypes;

  // ========== 碰撞体配置 ==========

  /** 碰撞体类型 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置|碰撞体",
            meta = (DisplayName = "碰撞体类型"))
  EXBProjectileCollisionType CollisionType =
      EXBProjectileCollisionType::Capsule;

  /** 胶囊体半径（仅胶囊体生效） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置|碰撞体",
            meta = (DisplayName = "胶囊体半径", ClampMin = "1.0",
                    EditCondition =
                        "CollisionType == EXBProjectileCollisionType::Capsule",
                    EditConditionHides))
  float CapsuleRadius = 12.0f;

  /** 胶囊体半高（仅胶囊体生效） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置|碰撞体",
            meta = (DisplayName = "胶囊体半高", ClampMin = "1.0",
                    EditCondition =
                        "CollisionType == EXBProjectileCollisionType::Capsule",
                    EditConditionHides))
  float CapsuleHalfHeight = 24.0f;

  /** 方体尺寸（仅方体生效） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置|碰撞体",
            meta = (DisplayName = "方体尺寸",
                    EditCondition =
                        "CollisionType == EXBProjectileCollisionType::Box",
                    EditConditionHides))
  FVector BoxExtent = FVector(24.0f, 24.0f, 24.0f);

  /** 网格缩放 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置|碰撞体",
            meta = (DisplayName = "网格缩放", ClampMin = "0.01"))
  FVector MeshScale = FVector(1.0f, 1.0f, 1.0f);

#if WITH_EDITOR
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
#endif

  // ========== 生成音效配置 ==========

  /** 投射物生成音效Tag */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "发射物配置|生成音效",
            meta = (DisplayName = "生成音效Tag"))
  FGameplayTag SpawnSoundTag;

  // ========== 飞行命中效果配置（FlightOnly 或 Both 时显示） ==========

  /** 飞行命中音效（旧，将被废弃） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "发射物配置|飞行命中效果",
            meta = (DisplayName = "飞行命中音效（旧）",
                    EditCondition =
                        "DamageType == EXBProjectileDamageType::FlightOnly || DamageType == EXBProjectileDamageType::Both",
                    EditConditionHides))
  TObjectPtr<USoundBase> HitSound;

  /** 飞行命中音效Tag（优先使用） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "发射物配置|飞行命中效果",
            meta = (DisplayName = "飞行命中音效Tag",
                    EditCondition =
                        "DamageType == EXBProjectileDamageType::FlightOnly || DamageType == EXBProjectileDamageType::Both",
                    EditConditionHides))
  FGameplayTag HitSoundTag;

  /** 击中特效类型（选择 Niagara 或 Cascade） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "发射物配置|飞行命中效果",
            meta = (DisplayName = "击中特效类型"))
  EXBHitEffectType HitEffectType = EXBHitEffectType::Niagara;

  /** 飞行命中特效（Niagara） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "发射物配置|飞行命中效果",
            meta = (DisplayName = "击中特效(Niagara)",
                    EditCondition = "HitEffectType == EXBHitEffectType::Niagara",
                    EditConditionHides))
  TObjectPtr<UNiagaraSystem> HitEffect;

  /** 飞行命中特效（Cascade） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "发射物配置|飞行命中效果",
            meta = (DisplayName = "击中特效(Cascade)",
                    EditCondition = "HitEffectType == EXBHitEffectType::Cascade",
                    EditConditionHides))
  TObjectPtr<UParticleSystem> HitEffectCascade;

  /** 飞行命中特效缩放 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "发射物配置|飞行命中效果",
            meta = (DisplayName = "击中特效缩放", ClampMin = "0.1"))
  float HitEffectScale = 1.0f;

protected:
  virtual void BeginPlay() override;

private:
  UFUNCTION()
  void OnProjectileOverlap(UPrimitiveComponent *OverlappedComponent,
                           AActor *OtherActor, UPrimitiveComponent *OtherComp,
                           int32 OtherBodyIndex, bool bFromSweep,
                           const FHitResult &SweepResult);

  bool ApplyDamageToTarget(AActor *TargetActor, const FHitResult &HitResult);
  bool GetTargetFaction(AActor *TargetActor, EXBFaction &OutFaction) const;

  /** 更新碰撞体类型 */
  void UpdateCollisionType();

  /** 执行爆炸伤害检测 */
  void PerformExplosionDamage(const FVector &ExplosionLocation);

  UPROPERTY(VisibleAnywhere, Category = "组件",
            meta = (DisplayName = "胶囊碰撞体"))
  TObjectPtr<UCapsuleComponent> CapsuleCollision;

  UPROPERTY(VisibleAnywhere, Category = "组件",
            meta = (DisplayName = "方体碰撞体"))
  TObjectPtr<UBoxComponent> BoxCollision;

  UPROPERTY(VisibleAnywhere, Category = "组件",
            meta = (DisplayName = "网格组件"))
  TObjectPtr<UStaticMeshComponent> MeshComponent;

  UPROPERTY(VisibleAnywhere, Category = "组件",
            meta = (DisplayName = "投射物运动"))
  TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

  /** 拖尾 Niagara 组件（直接在组件上配置 Niagara 系统） */
  UPROPERTY(VisibleAnywhere, Category = "组件",
            meta = (DisplayName = "拖尾特效组件"))
  TObjectPtr<UNiagaraComponent> TrailNiagaraComponent;

  TWeakObjectPtr<AActor> SourceActor;

  FGameplayTag DamageTag;

  FTimerHandle LifeTimerHandle;

  /** 拖尾渐隐后延迟销毁计时器 */
  FTimerHandle TrailFadeTimerHandle;

  /** 停用拖尾特效 */
  void DeactivateTrailEffect();

  /**
   * @brief  准备销毁/回收（隐藏网格、停用拖尾、延迟销毁）
   * @param  bHitEnemy 是否命中敌人（true=直接隐藏网格，false=渐隐效果）
   * @note   详细流程分析: 隐藏网格 -> 禁用碰撞 -> 停用拖尾 -> 延迟销毁/回收
   *         让拖尾有时间渐隐后再销毁Actor
   */
  void PrepareForDestroy(bool bHitEnemy = false);

  /** 延迟销毁完成后执行实际销毁/回收 */
  void ExecuteDestroyOrPool();

  /** 开始网格渐隐动画 */
  void StartMeshFade();

  /** 更新网格渐隐进度 */
  void UpdateMeshFade();

  /** 网格动态材质实例（用于渐隐） */
  UPROPERTY(Transient)
  TObjectPtr<UMaterialInstanceDynamic> MeshFadeMaterial;

  /** 网格渐隐计时器 */
  FTimerHandle MeshFadeTimerHandle;

  /** 网格渐隐当前进度（0~1） */
  float MeshFadeProgress = 0.0f;

  /** 处理场景碰撞 */
  UFUNCTION()
  void OnProjectileHit(UPrimitiveComponent *HitComponent, AActor *OtherActor,
                       UPrimitiveComponent *OtherComp, FVector NormalImpulse,
                       const FHitResult &Hit);
};
