/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Character/Components/XBFormationComponent.h

/**
 * @file XBFormationComponent.h
 * @brief 编队组件 - 管理士兵队列位置（整合调试功能）
 * 
 * @note 🔧 修改记录:
 *       1. 新增手动槽位数量控制（便于调试）
 *       2. 修复圆圈朝向问题
 *       3. 支持运行时动态调整槽位数量
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Army/XBSoldierTypes.h"
#include "XBFormationComponent.generated.h"

class AXBSoldierCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFormationUpdated);

/**
 * @brief 编队组件（整合调试功能）
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName = "XB Formation Component"))
class XIAOBINDATIANXIA_API UXBFormationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UXBFormationComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ============ 编队管理 ============

    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    const TArray<FXBFormationSlot>& GetFormationSlots() const { return FormationSlots; }

    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    void RegenerateFormation(int32 SoldierCount);

    // ✨ 新增 - 手动设置槽位数量（用于调试）
    /**
     * @brief 手动设置编队槽位数量
     * @param Count 槽位数量
     * @note 用于调试，运行时可动态调整
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation", meta = (DisplayName = "设置槽位数量"))
    void SetFormationSlotCount(int32 Count);

    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    FVector GetSlotWorldPosition(int32 SlotIndex) const;

    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    int32 GetFirstAvailableSlot() const;

    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    bool OccupySlot(int32 SlotIndex, int32 SoldierId);

    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    bool ReleaseSlot(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    void ReleaseAllSlots();

    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    int32 AssignSlotToSoldier(AXBSoldierCharacter* Soldier);

    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    void RemoveSoldierFromSlot(AXBSoldierCharacter* Soldier);

    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    void ReassignAllSlots(const TArray<AXBSoldierCharacter*>& Soldiers);

    // ============ 配置接口 ============

    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    const FXBFormationConfig& GetFormationConfig() const { return FormationConfig; }

    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    void SetFormationConfig(const FXBFormationConfig& NewConfig);

    // ============ 调试功能 ============

    UFUNCTION(BlueprintCallable, Category = "XB|Formation|Debug", meta = (DisplayName = "切换调试显示"))
    void SetDebugDrawEnabled(bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "XB|Formation|Debug", meta = (DisplayName = "是否启用调试"))
    bool IsDebugDrawEnabled() const { return bDrawDebug; }

    UFUNCTION(BlueprintCallable, Category = "XB|Formation|Debug", meta = (DisplayName = "绘制调试信息"))
    void DrawDebugFormation(float Duration = -1.0f);

    // ============ 委托 ============

    UPROPERTY(BlueprintAssignable, Category = "XB|Formation")
    FOnFormationUpdated OnFormationUpdated;

protected:
    // ============ 编队配置 ============

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Formation", meta = (DisplayName = "编队配置"))
    FXBFormationConfig FormationConfig;

    UPROPERTY(BlueprintReadOnly, Category = "XB|Formation", meta = (DisplayName = "编队槽位列表"))
    TArray<FXBFormationSlot> FormationSlots;

    // ✨ 新增 - 手动槽位数量控制
    /** @brief 手动指定的槽位数量（0表示自动根据士兵数量计算） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Formation", meta = (DisplayName = "槽位数量（0=自动）", ClampMin = "0", ClampMax = "999"))
    int32 ManualSlotCount = 0;

    // ============ 调试配置 ============

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Formation|Debug", meta = (DisplayName = "启用调试绘制"))
    bool bDrawDebug = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Formation|Debug", meta = (DisplayName = "槽位半径", ClampMin = "10.0"))
    float DebugSlotRadius = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Formation|Debug", meta = (DisplayName = "占用槽位颜色"))
    FColor DebugOccupiedSlotColor = FColor::Green;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Formation|Debug", meta = (DisplayName = "空闲槽位颜色"))
    FColor DebugFreeSlotColor = FColor::Yellow;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Formation|Debug", meta = (DisplayName = "连线颜色"))
    FColor DebugLineColor = FColor::Cyan;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Formation|Debug", meta = (DisplayName = "将领标记颜色"))
    FColor DebugLeaderColor = FColor::Red;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Formation|Debug", meta = (DisplayName = "将领标记半径", ClampMin = "10.0"))
    float DebugLeaderRadius = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Formation|Debug", meta = (DisplayName = "序号颜色"))
    FColor DebugTextColor = FColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Formation|Debug", meta = (DisplayName = "序号高度偏移", ClampMin = "0.0"))
    float DebugTextHeightOffset = 50.0f;

private:
    void CalculateFormationDimensions(int32 SoldierCount, int32& OutColumns, int32& OutRows) const;
    FVector2D CalculateSlotLocalOffset(int32 SlotIndex, int32 TotalSoldiers, int32 Columns, int32 Rows) const;
    void DrawDebugSlot(const FXBFormationSlot& Slot, const FVector& LeaderLocation, const FRotator& LeaderRotation, float Duration);
};
