#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Army/XBSoldierTypes.h"
#include "XBArmySubsystem.generated.h"

class AXBCharacterBase;
class UXBSoldierRenderer;

// ============================================
// 委托声明
// ============================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FXBOnSoldierStateChanged,
    int32, SoldierId,
    EXBSoldierState, OldState,
    EXBSoldierState, NewState
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FXBOnSoldierDamaged,
    int32, SoldierId,
    float, Damage,
    float, RemainingHealth
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FXBOnSoldierDied,
    int32, SoldierId,
    int32, LeaderId
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FXBOnSoldierCreated,
    int32, SoldierId
);

// ============================================
// 军队子系统
// ============================================

UCLASS()
class XIAOBINDATIANXIA_API UXBArmySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UXBArmySubsystem();

    // USubsystem 接口
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

    // 自定义 Tick（非继承）- 由 Ticker 调用
    bool TickSubsystem(float DeltaTime);

    // ============================================
    // 小兵管理
    // ============================================
    
    UFUNCTION(BlueprintCallable, Category = "Army|Soldier")
    int32 CreateSoldier(EXBSoldierType SoldierType, EXBFaction Faction, const FVector& Position);

    UFUNCTION(BlueprintCallable, Category = "Army|Soldier")
    void DestroySoldier(int32 SoldierId);

    UFUNCTION(BlueprintCallable, Category = "Army|Soldier")
    bool GetSoldierDataById(int32 SoldierId, FXBSoldierData& OutData) const;

    UFUNCTION(BlueprintCallable, Category = "Army|Soldier")
    void SetSoldierPosition(int32 SoldierId, const FVector& NewPosition);

    UFUNCTION(BlueprintCallable, Category = "Army|Soldier")
    void SetSoldierState(int32 SoldierId, EXBSoldierState NewState);

    // ============================================
    // 主将关联
    // ============================================

    UFUNCTION(BlueprintCallable, Category = "Army|Leader")
    void AssignSoldierToLeader(int32 SoldierId, int32 LeaderId);

    UFUNCTION(BlueprintCallable, Category = "Army|Leader")
    void RemoveSoldierFromLeader(int32 SoldierId);

    UFUNCTION(BlueprintCallable, Category = "Army|Leader")
    TArray<int32> GetSoldiersByLeader(int32 LeaderId) const;

    UFUNCTION(BlueprintCallable, Category = "Army|Formation")
    void SetSoldierFormationSlot(int32 SoldierId, int32 SlotIndex);

    // ============================================
    // 战斗
    // ============================================

    UFUNCTION(BlueprintCallable, Category = "Army|Combat")
    float DamageSoldier(int32 SoldierId, float DamageAmount, AActor* DamageCauser);

    UFUNCTION(BlueprintCallable, Category = "Army|Combat")
    void SetSoldierCombatTarget(int32 SoldierId, int32 TargetSoldierId);

    // ============================================
    // 查询方法
    // ============================================

    UFUNCTION(BlueprintCallable, Category = "Army|Query")
    FVector GetSoldierPosition(int32 SoldierId) const;

    UFUNCTION(BlueprintCallable, Category = "Army|Query")
    float GetSoldierHealth(int32 SoldierId) const;

    UFUNCTION(BlueprintCallable, Category = "Army|Query")
    EXBSoldierState GetSoldierState(int32 SoldierId) const;

    UFUNCTION(BlueprintCallable, Category = "Army|Query")
    bool IsSoldierAlive(int32 SoldierId) const;

    UFUNCTION(BlueprintCallable, Category = "Army|Query")
    TArray<int32> GetSoldiersByFaction(EXBFaction Faction) const;

    UFUNCTION(BlueprintCallable, Category = "Army|Query")
    TArray<int32> GetSoldiersInRadius(const FVector& Center, float Radius) const;

    // ============================================
    // 批量操作
    // ============================================

    UFUNCTION(BlueprintCallable, Category = "Army|Batch")
    void UpdateLeaderFormation(int32 LeaderId, const FVector& LeaderPosition, const FRotator& LeaderRotation);

    UFUNCTION(BlueprintCallable, Category = "Army|Batch")
    void SetLeaderSoldiersSprinting(int32 LeaderId, bool bSprinting);

    // ============================================
    // 渲染器更新（内部使用）
    // ============================================
    
    void UpdateRenderer();

    // ============================================
    // 事件委托
    // ============================================

    UPROPERTY(BlueprintAssignable, Category = "Army|Events")
    FXBOnSoldierStateChanged OnSoldierStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Army|Events")
    FXBOnSoldierDamaged OnSoldierDamaged;

    UPROPERTY(BlueprintAssignable, Category = "Army|Events")
    FXBOnSoldierDied OnSoldierDied;

    UPROPERTY(BlueprintAssignable, Category = "Army|Events")
    FXBOnSoldierCreated OnSoldierCreated;

    // 获取小兵数据映射（供渲染器使用）
    const TMap<int32, FXBSoldierData>& GetSoldierMap() const { return SoldierMap; }

protected:
    FXBSoldierData* GetSoldierDataInternal(int32 SoldierId);
    const FXBSoldierData* GetSoldierDataInternal(int32 SoldierId) const;
    
    void UpdateSoldierLogic(float DeltaTime);
    void UpdateSoldierMovement(FXBSoldierData& Soldier, float DeltaTime);
    void ProcessSoldierDeath(int32 SoldierId);

protected:
    UPROPERTY()
    TMap<int32, FXBSoldierData> SoldierMap;

    TMap<int32, TArray<int32>> LeaderToSoldiersMap;
    TMap<EXBFaction, TSet<int32>> FactionSoldiersMap;

    int32 NextSoldierId = 1;

    UPROPERTY()
    TObjectPtr<UXBSoldierRenderer> SoldierRenderer;

    FTSTicker::FDelegateHandle TickHandle;
};
