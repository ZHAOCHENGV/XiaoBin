/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Soldier/Component/XBSoldierFollowComponent.h

/**
 * @file XBSoldierFollowComponent.h
 * @brief 士兵跟随组件 - 处理士兵跟随将领的移动逻辑
 * 
 * @note 🔧 修改记录:
 *       1. 完善跟随算法
 *       2. 新增编队位置计算
 *       3. 支持冲刺加速
 *       4. 优化避障逻辑
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "XBSoldierFollowComponent.generated.h"

class AXBCharacterBase;
class UXBFormationComponent;

/**
 * @brief 士兵跟随组件
 * 
 * 负责计算士兵应该移动到的目标位置（编队位置）
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName = "XB Soldier Follow"))
class XIAOBINDATIANXIA_API UXBSoldierFollowComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UXBSoldierFollowComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, 
        FActorComponentTickFunction* ThisTickFunction) override;

    // ============ 目标设置 ============

    /**
     * @brief 设置将领引用
     * @param NewLeader 将领角色
     */
    UFUNCTION(BlueprintCallable, Category = "Follow")
    void SetLeader(AXBCharacterBase* NewLeader);

    /**
     * @brief 设置跟随目标（通用）
     * @param NewTarget 目标Actor
     */
    UFUNCTION(BlueprintCallable, Category = "Follow")
    void SetFollowTarget(AActor* NewTarget);

    /**
     * @brief 获取跟随目标
     */
    UFUNCTION(BlueprintCallable, Category = "Follow")
    AActor* GetFollowTarget() const { return FollowTargetRef.Get(); }

    // ============ 编队设置 ============

    /**
     * @brief 设置编队偏移
     * @param Offset 相对于将领的偏移
     */
    UFUNCTION(BlueprintCallable, Category = "Follow")
    void SetFormationOffset(const FVector& Offset);

    /**
     * @brief 设置编队槽位索引
     * @param SlotIndex 槽位索引
     */
    UFUNCTION(BlueprintCallable, Category = "Follow")
    void SetFormationSlotIndex(int32 SlotIndex);

    /**
     * @brief 获取编队槽位索引
     */
    UFUNCTION(BlueprintCallable, Category = "Follow")
    int32 GetFormationSlotIndex() const { return FormationSlotIndex; }

    // ============ 速度设置 ============

    /**
     * @brief 设置插值速度
     */
    UFUNCTION(BlueprintCallable, Category = "Follow")
    void SetInterpSpeed(float NewSpeed);

    /**
     * @brief 设置跟随插值速度（别名）
     */
    UFUNCTION(BlueprintCallable, Category = "Follow")
    void SetFollowInterpSpeed(float NewSpeed);

    /**
     * @brief 设置跟随移动速度
     */
    UFUNCTION(BlueprintCallable, Category = "Follow")
    void SetFollowSpeed(float NewSpeed);

    /**
     * @brief 获取当前跟随速度
     */
    UFUNCTION(BlueprintCallable, Category = "Follow")
    float GetFollowSpeed() const { return FollowSpeed; }

    // ============ 跟随控制 ============

    /**
     * @brief 开始跟随
     */
    UFUNCTION(BlueprintCallable, Category = "Follow")
    void StartFollowing();

    /**
     * @brief 停止跟随
     */
    UFUNCTION(BlueprintCallable, Category = "Follow")
    void StopFollowing();

    /**
     * @brief 更新跟随逻辑（外部调用）
     * @param DeltaTime 帧时间
     */
    UFUNCTION(BlueprintCallable, Category = "Follow")
    void UpdateFollowing(float DeltaTime);

    // ============ 状态查询 ============

    /**
     * @brief 检查是否到达编队位置
     */
    UFUNCTION(BlueprintCallable, Category = "Follow")
    bool IsAtFormationPosition() const;

    /**
     * @brief 检查是否正在跟随
     */
    UFUNCTION(BlueprintCallable, Category = "Follow")
    bool IsFollowing() const { return bIsFollowing; }

    /**
     * @brief 获取目标位置（编队位置）
     */
    UFUNCTION(BlueprintCallable, Category = "Follow")
    FVector GetTargetPosition() const;

    /**
     * @brief 获取到目标位置的距离
     */
    UFUNCTION(BlueprintCallable, Category = "Follow")
    float GetDistanceToTarget() const;

protected:
    // ============ 内部方法 ============

    /**
     * @brief 更新跟随移动
     */
    void UpdateFollowMovement(float DeltaTime);

    /**
     * @brief 计算目标位置
     */
    FVector CalculateTargetPosition() const;

    /**
     * @brief 从编队组件获取位置
     */
    FVector GetPositionFromFormationComponent() const;

    /**
     * @brief 应用避障偏移
     * @param DesiredDirection 期望方向
     * @return 避障后的方向
     */
    FVector ApplyAvoidance(const FVector& DesiredDirection) const;

protected:
    // ============ 引用 ============

    /** @brief 将领引用 */
    UPROPERTY()
    TWeakObjectPtr<AXBCharacterBase> LeaderRef;

    /** @brief 跟随目标引用 */
    UPROPERTY()
    TWeakObjectPtr<AActor> FollowTargetRef;

    /** @brief 缓存的编队组件 */
    UPROPERTY()
    TWeakObjectPtr<UXBFormationComponent> CachedFormationComponent;

    // ============ 配置 ============

    /** @brief 编队偏移（手动设置） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow", meta = (DisplayName = "编队偏移"))
    FVector FormationOffset = FVector::ZeroVector;

    /** @brief 编队槽位索引 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow", meta = (DisplayName = "槽位索引"))
    int32 FormationSlotIndex = INDEX_NONE;

    /** @brief 插值速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow", meta = (DisplayName = "插值速度"))
    float InterpSpeed = 5.0f;

    /** @brief 跟随移动速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow", meta = (DisplayName = "移动速度"))
    float FollowSpeed = 400.0f;

    /** @brief 开始移动的最小距离 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow", meta = (DisplayName = "最小移动距离"))
    float MinDistanceToMove = 10.0f;

    /** @brief 到达判定阈值 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow", meta = (DisplayName = "到达阈值"))
    float ArrivalThreshold = 50.0f;

    /** @brief 避障检测半径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow", meta = (DisplayName = "避障半径"))
    float AvoidanceRadius = 100.0f;

    /** @brief 避障强度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow", meta = (DisplayName = "避障强度"))
    float AvoidanceStrength = 0.5f;

    // ============ 状态 ============

    /** @brief 是否正在跟随 */
    UPROPERTY(BlueprintReadOnly, Category = "Follow")
    bool bIsFollowing = false;

    /** @brief 缓存的目标位置 */
    UPROPERTY(BlueprintReadOnly, Category = "Follow")
    FVector CachedTargetPosition = FVector::ZeroVector;
};
