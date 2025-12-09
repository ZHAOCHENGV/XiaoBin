/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/XBSteeringComponent.h

/**
 * @file XBSteeringComponent.h
 * @brief 转向行为组件 - 实现Boids群集算法
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "XBSteeringComponent.generated.h"

class UXBFlowFieldSubsystem;
class AXBCharacterBase;

/**
 * @brief 转向行为配置
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBSteeringConfig
{
    GENERATED_BODY()

    /** @brief 最大速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础", meta = (DisplayName = "最大速度", ClampMin = "0.0"))
    float MaxSpeed = 400.0f;

    /** @brief 最大转向力 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础", meta = (DisplayName = "最大转向力", ClampMin = "0.0"))
    float MaxSteeringForce = 200.0f;

    /** @brief 质量 (影响加速度) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础", meta = (DisplayName = "质量", ClampMin = "0.1"))
    float Mass = 1.0f;

    /** @brief 到达减速距离 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "行为", meta = (DisplayName = "到达减速距离", ClampMin = "0.0"))
    float ArrivalSlowingDistance = 200.0f;

    /** @brief 分离感知半径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids", meta = (DisplayName = "分离半径", ClampMin = "0.0"))
    float SeparationRadius = 100.0f;

    /** @brief 对齐感知半径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids", meta = (DisplayName = "对齐半径", ClampMin = "0.0"))
    float AlignmentRadius = 150.0f;

    /** @brief 聚合感知半径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids", meta = (DisplayName = "聚合半径", ClampMin = "0.0"))
    float CohesionRadius = 200.0f;

    /** @brief 分离权重 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "权重", meta = (DisplayName = "分离权重", ClampMin = "0.0"))
    float SeparationWeight = 1.5f;

    /** @brief 对齐权重 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "权重", meta = (DisplayName = "对齐权重", ClampMin = "0.0"))
    float AlignmentWeight = 1.0f;

    /** @brief 聚合权重 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "权重", meta = (DisplayName = "聚合权重", ClampMin = "0.0"))
    float CohesionWeight = 1.0f;

    /** @brief 流场跟随权重 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "权重", meta = (DisplayName = "流场权重", ClampMin = "0.0"))
    float FlowFieldWeight = 2.0f;

    /** @brief 障碍物避让权重 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "权重", meta = (DisplayName = "避障权重", ClampMin = "0.0"))
    float ObstacleAvoidanceWeight = 3.0f;

    /** @brief 障碍物检测距离 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "避障", meta = (DisplayName = "避障检测距离", ClampMin = "0.0"))
    float ObstacleDetectionDistance = 150.0f;
};

/**
 * @brief 转向行为组件
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent, DisplayName = "XB转向行为组件"))
class XIAOBINDATIANXIA_API UXBSteeringComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UXBSteeringComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /**
     * @brief 设置跟随目标
     * @param NewTarget 新的跟随目标
     */
    UFUNCTION(BlueprintCallable, Category = "转向")
    void SetFollowTarget(AXBCharacterBase* NewTarget);

    /**
     * @brief 设置攻击目标
     * @param NewTarget 新的攻击目标
     */
    UFUNCTION(BlueprintCallable, Category = "转向")
    void SetAttackTarget(AActor* NewTarget);

    /**
     * @brief 获取当前速度
     */
    UFUNCTION(BlueprintPure, Category = "转向")
    FVector GetVelocity() const { return Velocity; }

    /**
     * @brief 获取当前转向力
     */
    UFUNCTION(BlueprintPure, Category = "转向")
    FVector GetSteeringForce() const { return SteeringForce; }

    /**
     * @brief 启用/禁用转向
     */
    UFUNCTION(BlueprintCallable, Category = "转向")
    void SetEnabled(bool bEnabled) { bIsEnabled = bEnabled; }

public:
    /** @brief 转向配置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "转向配置"))
    FXBSteeringConfig Config;

protected:
    /**
     * @brief 计算所有转向力
     * @return 合成的转向力
     */
    FVector CalculateSteeringForce();

    /**
     * @brief Seek行为 - 朝向目标移动
     */
    FVector Seek(const FVector& TargetLocation) const;

    /**
     * @brief Flee行为 - 远离目标
     */
    FVector Flee(const FVector& ThreatLocation) const;

    /**
     * @brief Arrive行为 - 到达时减速
     */
    FVector Arrive(const FVector& TargetLocation) const;

    /**
     * @brief Separation行为 - 与邻居保持距离
     */
    FVector Separation();

    /**
     * @brief Alignment行为 - 与邻居保持相同方向
     */
    FVector Alignment();

    /**
     * @brief Cohesion行为 - 向群体中心移动
     */
    FVector Cohesion();

    /**
     * @brief FlowFieldFollow行为 - 跟随流场
     */
    FVector FlowFieldFollow();

    /**
     * @brief ObstacleAvoidance行为 - 避开障碍物
     */
    FVector ObstacleAvoidance();

    /**
     * @brief 限制向量长度
     */
    FVector Truncate(const FVector& V, float MaxLength) const;

private:
    /** @brief 当前速度 */
    FVector Velocity;

    /** @brief 当前转向力 */
    FVector SteeringForce;

    /** @brief 跟随目标 */
    UPROPERTY()
    TWeakObjectPtr<AXBCharacterBase> FollowTarget;

    /** @brief 攻击目标 */
    UPROPERTY()
    TWeakObjectPtr<AActor> AttackTarget;

    /** @brief 流场子系统缓存 */
    UPROPERTY()
    TWeakObjectPtr<UXBFlowFieldSubsystem> FlowFieldSubsystem;

    /** @brief 邻居缓存 */
    TArray<AActor*> CachedNeighbors;

    /** @brief 是否启用 */
    bool bIsEnabled = true;

    /** @brief 邻居查询间隔 */
    float NeighborQueryInterval = 0.1f;

    /** @brief 上次邻居查询时间 */
    float LastNeighborQueryTime = 0.0f;
};
