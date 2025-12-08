// Source/XiaoBinDaTianXia/Public/Army/XBArmySubsystem.h

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Army/XBSoldierTypes.h"
#include "XBArmySubsystem.generated.h"

class AXBCharacterBase;
class UXBSoldierRenderer;

// ============================================
// 委托声明 (必须在 UCLASS 之前定义)
// ============================================

/**
 * @brief 士兵状态变更委托
 * @param SoldierId 士兵ID
 * @param OldState 旧状态
 * @param NewState 新状态
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FXBOnSoldierStateChanged,
    int32, SoldierId,
    EXBSoldierState, OldState,
    EXBSoldierState, NewState
);

/**
 * @brief 士兵受伤委托
 * @param SoldierId 士兵ID
 * @param Damage 受到的伤害
 * @param RemainingHealth 剩余血量
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FXBOnSoldierDamaged,
    int32, SoldierId,
    float, Damage,
    float, RemainingHealth
);

/**
 * @brief 士兵死亡委托
 * @param SoldierId 士兵ID
 * @param LeaderId 所属将领ID（如果没有则为-1）
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FXBOnSoldierDied,
    int32, SoldierId,
    int32, LeaderId
);

/**
 * @brief 士兵创建委托
 * @param SoldierId 新创建的士兵ID
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FXBOnSoldierCreated,
    int32, SoldierId
);

// ============================================
// 军队子系统类定义
// ============================================

/**
 * @brief 军队子系统 - 负责管理海量士兵的逻辑、状态和渲染
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBArmySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UXBArmySubsystem();

    // ============ USubsystem 接口 ============
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

    // 自定义 Tick（由 FTSTicker 调用）
    bool TickSubsystem(float DeltaTime);

    // ============ 小兵管理 ============
    
    /** 创建新士兵 */
    UFUNCTION(BlueprintCallable, Category = "Army|Soldier", meta = (DisplayName = "创建士兵"))
    int32 CreateSoldier(EXBSoldierType SoldierType, EXBFaction Faction, const FVector& Position);

    /** 销毁士兵 */
    UFUNCTION(BlueprintCallable, Category = "Army|Soldier", meta = (DisplayName = "销毁士兵"))
    void DestroySoldier(int32 SoldierId);

    /** 获取士兵数据 */
    UFUNCTION(BlueprintCallable, Category = "Army|Soldier", meta = (DisplayName = "获取士兵数据"))
    bool GetSoldierDataById(int32 SoldierId, FXBSoldierData& OutData) const;

    /** 设置士兵位置 */
    UFUNCTION(BlueprintCallable, Category = "Army|Soldier", meta = (DisplayName = "设置士兵位置"))
    void SetSoldierPosition(int32 SoldierId, const FVector& NewPosition);

    /** 设置士兵状态 */
    UFUNCTION(BlueprintCallable, Category = "Army|Soldier", meta = (DisplayName = "设置士兵状态"))
    void SetSoldierState(int32 SoldierId, EXBSoldierState NewState);

    // ============ 主将关联 ============

    /** * 将士兵分配给将领 (Blueprints版)
     * @param SoldierId 士兵ID
     * @param LeaderActor 将领Actor
     * @param SlotIndex 阵型槽位索引
     */
    UFUNCTION(BlueprintCallable, Category = "Army|Leader", meta = (DisplayName = "分配士兵给将领"))
    bool AssignSoldierToLeader(int32 SoldierId, AActor* LeaderActor, int32 SlotIndex);

    /** * 将士兵分配给将领 (C++内部版)
     * @param SoldierId 士兵ID
     * @param LeaderId 将领的唯一ID
     */
    void AssignSoldierToLeader(int32 SoldierId, int32 LeaderId);

    /** 将士兵从将领处移除 */
    UFUNCTION(BlueprintCallable, Category = "Army|Leader", meta = (DisplayName = "移除士兵跟随"))
    void RemoveSoldierFromLeader(int32 SoldierId);

    /** * 获取将领麾下的所有士兵ID (C++内部版)
     * @param LeaderId 将领ID
     */
    UFUNCTION(BlueprintCallable, Category = "Army|Leader", meta = (DisplayName = "获取将领下属士兵(ID)"))
    TArray<int32> GetSoldiersByLeader(int32 LeaderId) const;
    
    /** * 获取将领麾下的所有士兵ID (Actor版)
     * @param LeaderActor 将领Actor
     */
    TArray<int32> GetSoldiersByLeader(const AActor* LeaderActor) const;

    /** 设置士兵的阵型槽位 */
    UFUNCTION(BlueprintCallable, Category = "Army|Formation", meta = (DisplayName = "设置士兵槽位"))
    void SetSoldierFormationSlot(int32 SoldierId, int32 SlotIndex);

    // ============ 战斗与状态 ============

    /** 对士兵造成伤害 */
    UFUNCTION(BlueprintCallable, Category = "Army|Combat", meta = (DisplayName = "伤害士兵"))
    float DamageSoldier(int32 SoldierId, float DamageAmount, AActor* DamageCauser);

    /** 设置士兵的战斗目标 */
    UFUNCTION(BlueprintCallable, Category = "Army|Combat", meta = (DisplayName = "设置士兵战斗目标"))
    void SetSoldierCombatTarget(int32 SoldierId, int32 TargetSoldierId);

    /** 让将领麾下的所有士兵进入战斗状态 */
    UFUNCTION(BlueprintCallable, Category = "Army|Combat", meta = (DisplayName = "主将下令: 全军突击"))
    void EnterCombatForLeader(AActor* LeaderActor);

    /** 设置将领麾下士兵的隐身状态（进草丛） */
    UFUNCTION(BlueprintCallable, Category = "Army|Stealth", meta = (DisplayName = "主将下令: 全军隐蔽"))
    void SetHiddenForLeader(AActor* LeaderActor, bool bHidden);

    // ============ 查询 ============

    UFUNCTION(BlueprintCallable, Category = "Army|Query", meta = (DisplayName = "获取士兵位置"))
    FVector GetSoldierPosition(int32 SoldierId) const;

    UFUNCTION(BlueprintCallable, Category = "Army|Query", meta = (DisplayName = "获取士兵血量"))
    float GetSoldierHealth(int32 SoldierId) const;

    UFUNCTION(BlueprintCallable, Category = "Army|Query", meta = (DisplayName = "获取士兵当前状态"))
    EXBSoldierState GetSoldierState(int32 SoldierId) const;

    UFUNCTION(BlueprintCallable, Category = "Army|Query", meta = (DisplayName = "士兵是否存活"))
    bool IsSoldierAlive(int32 SoldierId) const;

    UFUNCTION(BlueprintCallable, Category = "Army|Query", meta = (DisplayName = "获取阵营所有士兵"))
    TArray<int32> GetSoldiersByFaction(EXBFaction Faction) const;

    UFUNCTION(BlueprintCallable, Category = "Army|Query", meta = (DisplayName = "获取范围内士兵"))
    TArray<int32> GetSoldiersInRadius(const FVector& Center, float Radius) const;

    // ============ 批量操作 ============

    UFUNCTION(BlueprintCallable, Category = "Army|Batch", meta = (DisplayName = "更新将领阵型目标"))
    void UpdateLeaderFormation(int32 LeaderId, const FVector& LeaderPosition, const FRotator& LeaderRotation);

    UFUNCTION(BlueprintCallable, Category = "Army|Batch", meta = (DisplayName = "设置将领士兵冲刺"))
    void SetLeaderSoldiersSprinting(int32 LeaderId, bool bSprinting);

    /** 更新渲染器（内部调用） */
    void UpdateRenderer();

    // ============ 事件委托 ============

    /** 士兵状态变更事件 */
    UPROPERTY(BlueprintAssignable, Category = "Army|Events", meta = (DisplayName = "当士兵状态改变时"))
    FXBOnSoldierStateChanged OnSoldierStateChanged;

    /** 士兵受伤事件 */
    UPROPERTY(BlueprintAssignable, Category = "Army|Events", meta = (DisplayName = "当士兵受伤时"))
    FXBOnSoldierDamaged OnSoldierDamaged;

    /** 士兵死亡事件 */
    UPROPERTY(BlueprintAssignable, Category = "Army|Events", meta = (DisplayName = "当士兵死亡时"))
    FXBOnSoldierDied OnSoldierDied;

    /** 士兵创建事件 */
    UPROPERTY(BlueprintAssignable, Category = "Army|Events", meta = (DisplayName = "当士兵创建时"))
    FXBOnSoldierCreated OnSoldierCreated;

    /** 获取内部数据映射（供渲染器只读访问） */
    const TMap<int32, FXBSoldierData>& GetSoldierMap() const { return SoldierMap; }

protected:
    FXBSoldierData* GetSoldierDataInternal(int32 SoldierId);
    const FXBSoldierData* GetSoldierDataInternal(int32 SoldierId) const;
    
    void UpdateSoldierLogic(float DeltaTime);
    void UpdateSoldierMovement(FXBSoldierData& Soldier, float DeltaTime);
    void ProcessSoldierDeath(int32 SoldierId);

protected:
    /** 所有存活士兵的数据映射表 */
    UPROPERTY()
    TMap<int32, FXBSoldierData> SoldierMap;

    /** 缓存：将领ID -> 士兵ID列表 */
    TMap<int32, TArray<int32>> LeaderToSoldiersMap;
    
    /** 缓存：阵营 -> 士兵ID列表 */
    TMap<EXBFaction, TSet<int32>> FactionSoldiersMap;

    /** ID生成器 */
    int32 NextSoldierId = 1;

    /** 士兵渲染器组件 */
    UPROPERTY()
    TObjectPtr<UXBSoldierRenderer> SoldierRenderer;

    /** Tick 句柄 */
    FTSTicker::FDelegateHandle TickHandle;
};