// XBArmySubsystem.h

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Army/XBSoldierTypes.h"
#include "XBArmySubsystem.generated.h"

class UXBSoldierRenderer;
class UXBSoldierPool;

UCLASS()
class XIAOBINDATIANXIA_API UXBArmySubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    // ============ USubsystem 接口 ============
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override { return true; }
    virtual bool IsTickableInEditor() const override { return false; }

    // ============ 士兵管理 ============

    /** 创建新士兵并返回ID */
    UFUNCTION(BlueprintCallable, Category = "XB|Army")
    int32 CreateSoldier(const FXBSoldierConfig& Config, EXBFaction Faction, const FVector& SpawnLocation);

    /** 销毁士兵 */
    UFUNCTION(BlueprintCallable, Category = "XB|Army")
    void DestroySoldier(int32 SoldierId);

    /** 
     * 获取士兵数据（只读）
     * 注意：TMap 和复杂引用类型不支持蓝图，所以这个函数不暴露给蓝图
     */
    bool GetSoldierData(int32 SoldierId, FXBSoldierAgent& OutData) const;  // 移除 UFUNCTION

    /** 
     * 修改士兵数据
     * 不暴露给蓝图
     */
    bool ModifySoldierData(int32 SoldierId, const FXBSoldierAgent& NewData);  // 移除 UFUNCTION

    /** 蓝图友好版本：获取士兵位置 */
    UFUNCTION(BlueprintCallable, Category = "XB|Army")
    FVector GetSoldierPosition(int32 SoldierId) const;

    /** 蓝图友好版本：获取士兵血量 */
    UFUNCTION(BlueprintCallable, Category = "XB|Army")
    float GetSoldierHealth(int32 SoldierId) const;

    /** 蓝图友好版本：获取士兵状态 */
    UFUNCTION(BlueprintCallable, Category = "XB|Army")
    EXBSoldierState GetSoldierState(int32 SoldierId) const;

    /** 蓝图友好版本：检查士兵是否存活 */
    UFUNCTION(BlueprintCallable, Category = "XB|Army")
    bool IsSoldierAlive(int32 SoldierId) const;

    /** 将士兵分配给将领 */
    UFUNCTION(BlueprintCallable, Category = "XB|Army")
    bool AssignSoldierToLeader(int32 SoldierId, AActor* Leader, int32 SlotIndex);

    /** 从将领处移除士兵 */
    UFUNCTION(BlueprintCallable, Category = "XB|Army")
    bool RemoveSoldierFromLeader(int32 SoldierId);

    /** 获取将领的所有士兵ID */
    UFUNCTION(BlueprintCallable, Category = "XB|Army")
    TArray<int32> GetSoldiersByLeader(const AActor* Leader) const;

    /** 获取将领的士兵数量 */
    UFUNCTION(BlueprintCallable, Category = "XB|Army")
    int32 GetSoldierCountByLeader(const AActor* Leader) const;

    // ============ 战斗控制 ============

    UFUNCTION(BlueprintCallable, Category = "XB|Army")
    void EnterCombatForLeader(AActor* Leader);

    UFUNCTION(BlueprintCallable, Category = "XB|Army")
    void ExitCombatForLeader(AActor* Leader);

    UFUNCTION(BlueprintCallable, Category = "XB|Army")
    float ApplyDamageToSoldier(int32 SoldierId, float DamageAmount, AActor* DamageSource);

    // ============ 隐身控制 ============

    UFUNCTION(BlueprintCallable, Category = "XB|Army")
    void SetHiddenForLeader(AActor* Leader, bool bNewHidden);  // 改名避免遮蔽

    // ============ 兵种转换 ============

    UFUNCTION(BlueprintCallable, Category = "XB|Army")
    void ConvertSoldierType(AActor* Leader, EXBSoldierType NewType);

    // ============ 查询 ============

    UFUNCTION(BlueprintCallable, Category = "XB|Army")
    int32 FindNearestEnemy(const FVector& Location, EXBFaction MyFaction, float MaxDistance) const;

    UFUNCTION(BlueprintCallable, Category = "XB|Army")
    TArray<int32> FindEnemiesInRadius(const FVector& Location, EXBFaction MyFaction, float Radius) const;

    // ============ 委托 ============

    UPROPERTY(BlueprintAssignable, Category = "XB|Army")
    FXBOnSoldierStateChanged OnSoldierStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "XB|Army")
    FXBOnSoldierDamaged OnSoldierDamaged;

    UPROPERTY(BlueprintAssignable, Category = "XB|Army")
    FXBOnSoldierDied OnSoldierDied;

protected:
    /** 所有士兵数据（核心存储） */
    UPROPERTY()
    TMap<int32, FXBSoldierAgent> SoldierMap;

    /** 将领到士兵的映射 */
    TMap<TWeakObjectPtr<AActor>, TArray<int32>> LeaderToSoldiersMap;

    /** 渲染器 */
    UPROPERTY()
    TObjectPtr<UXBSoldierRenderer> SoldierRenderer;

    /** 对象池 */
    UPROPERTY()
    TObjectPtr<UXBSoldierPool> SoldierPool;

    /** ID 生成器 */
    int32 NextSoldierId = 1;

private:
    void TickAllSoldiers(float DeltaTime);
    void TickSoldier(FXBSoldierAgent& Soldier, float DeltaTime);
    void UpdateFollowingState(FXBSoldierAgent& Soldier, float DeltaTime);
    void UpdateEngagingState(FXBSoldierAgent& Soldier, float DeltaTime);
    void UpdateReturningState(FXBSoldierAgent& Soldier, float DeltaTime);
    void HandleSoldierDeath(int32 SoldierId);
    void ReorganizeFormation(AActor* Leader);
    int32 GenerateSoldierId();
};


