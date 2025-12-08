// Copyright XiaoBing Project. All Rights Reserved.

/**
 * @file XBSoldierActor.h
 * @brief 士兵Actor类
 * 
 * 功能说明：
 * - 可拾取的士兵实体
 * - 跟随将领移动
 * - 支持三种兵种类型
 * - 包含战斗逻辑
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/XBDataTypes.h"
#include "XBSoldierActor.generated.h"

// 前向声明
class USkeletalMeshComponent;
class UCapsuleComponent;
class UXBSoldierFollowComponent;

/**
 * @brief 士兵Actor类
 */
UCLASS()
class XIAOBINDATIANXIA_API AXBSoldierActor : public AActor
{
    GENERATED_BODY()

public:
    AXBSoldierActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // ==================== 初始化 ====================

    /**
     * @brief 初始化士兵
     * @param InConfig 士兵配置
     * @param InFaction 所属阵营
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "初始化士兵"))
    void InitializeSoldier(const FXBSoldierConfig& InConfig, EXBFaction InFaction);

    // ==================== 跟随系统 ====================

    /**
     * @brief 设置跟随的将领
     * @param NewLeader 将领Actor
     * @param SlotIndex 编队槽位索引
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置跟随将领"))
    void SetFollowTarget(AActor* NewLeader, int32 SlotIndex);

    /**
     * @brief 获取跟随的将领
     * @return 将领Actor指针
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取跟随将领"))
    AActor* GetFollowTarget() const { return FollowTarget.Get(); }

    /**
     * @brief 获取编队槽位索引
     * @return 槽位索引
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取槽位索引"))
    int32 GetFormationSlotIndex() const { return FormationSlotIndex; }

    /**
     * @brief 设置编队槽位索引
     * @param NewIndex 新的槽位索引
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置槽位索引"))
    void SetFormationSlotIndex(int32 NewIndex) { FormationSlotIndex = NewIndex; }

    // ==================== 状态管理 ====================

    /**
     * @brief 获取士兵类型
     * @return 士兵类型
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取士兵类型"))
    EXBSoldierType GetSoldierType() const { return SoldierType; }

    /**
     * @brief 获取士兵状态
     * @return 士兵状态
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取士兵状态"))
    EXBSoldierState GetSoldierState() const { return CurrentState; }

    /**
     * @brief 设置士兵状态
     * @param NewState 新状态
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置士兵状态"))
    void SetSoldierState(EXBSoldierState NewState);

    /**
     * @brief 获取阵营
     * @return 阵营
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "获取阵营"))
    EXBFaction GetFaction() const { return Faction; }

    // ==================== 战斗系统 ====================

    /**
     * @brief 进入战斗状态
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "进入战斗"))
    void EnterCombat();

    /**
     * @brief 退出战斗状态（返回跟随）
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "退出战斗"))
    void ExitCombat();

    /**
     * @brief 受到伤害
     * @param DamageAmount 伤害量
     * @param DamageSource 伤害来源
     * @return 实际伤害量
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "受到伤害"))
    float TakeSoldierDamage(float DamageAmount, AActor* DamageSource);

    // ==================== 逃跑系统 ====================

    /**
     * @brief 设置逃跑加速状态
     * @param bEscaping 是否正在逃跑
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置逃跑状态"))
    void SetEscaping(bool bEscaping);

protected:
    // ==================== 组件 ====================

    /** 根碰撞体 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "碰撞体"))
    TObjectPtr<UCapsuleComponent> CapsuleComponent;

    /** 骨骼网格体 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "网格体"))
    TObjectPtr<USkeletalMeshComponent> MeshComponent;

    /** 跟随组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "跟随组件"))
    TObjectPtr<UXBSoldierFollowComponent> FollowComponent;

    // ==================== 配置数据 ====================

    /** 士兵类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "士兵类型"))
    EXBSoldierType SoldierType = EXBSoldierType::Infantry;

    /** 阵营 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "阵营"))
    EXBFaction Faction = EXBFaction::Neutral;

    /** 士兵配置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "士兵配置"))
    FXBSoldierConfig SoldierConfig;

    // ==================== 状态数据 ====================

    /** 当前状态 */
    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "当前状态"))
    EXBSoldierState CurrentState = EXBSoldierState::Idle;

    /** 跟随目标 */
    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "跟随目标"))
    TWeakObjectPtr<AActor> FollowTarget;

    /** 编队槽位索引 */
    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "槽位索引"))
    int32 FormationSlotIndex = INDEX_NONE;

    /** 当前血量 */
    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "当前血量"))
    float CurrentHealth = 100.0f;

    /** 是否正在逃跑 */
    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "正在逃跑"))
    bool bIsEscaping = false;

    /** 攻击冷却计时器 */
    float AttackCooldownTimer = 0.0f;

    // ==================== 内部方法 ====================

    /** 更新跟随逻辑 */
    void UpdateFollowing(float DeltaTime);

    /** 更新战斗逻辑 */
    void UpdateCombat(float DeltaTime);

    /** 处理死亡 */
    void HandleDeath();
};