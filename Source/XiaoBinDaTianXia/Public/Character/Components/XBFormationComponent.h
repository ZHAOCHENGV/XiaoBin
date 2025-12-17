/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Character/Components/XBFormationComponent.h

/**
 * @file XBFormationComponent.h
 * @brief 编队组件 - 管理士兵队列位置
 * 
 * @note 🔧 修改记录:
 *       1. 新增槽位递补逻辑
 *       2. 新增 CompactSlots() 方法用于压缩槽位
 *       3. 优化槽位分配算法
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Army/XBSoldierTypes.h"
#include "XBFormationComponent.generated.h"

class AXBSoldierCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFormationUpdated);
// ✨ 新增 - 槽位变化委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSlotReassigned, int32, OldSlotIndex, int32, NewSlotIndex);

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

    // ✨ 新增 - 压缩槽位（移除空洞，确保连续）
    /**
     * @brief 压缩槽位数组，移除中间的空槽
     * @param Soldiers 当前士兵数组引用
     * @note 当士兵死亡后调用，确保槽位连续无空洞
     *       会触发 OnSlotReassigned 委托通知每个被移动的士兵
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation", meta = (DisplayName = "压缩槽位"))
    void CompactSlots(const TArray<AXBSoldierCharacter*>& Soldiers);

    // ✨ 新增 - 获取下一个可用槽位索引（总是返回当前士兵数量，即队尾）
    /**
     * @brief 获取下一个应分配的槽位索引
     * @param CurrentSoldierCount 当前士兵数量
     * @return 应分配的槽位索引（等于当前数量，从0开始）
     */
    UFUNCTION(BlueprintPure, Category = "XB|Formation", meta = (DisplayName = "获取下一槽位索引"))
    int32 GetNextSlotIndex(int32 CurrentSoldierCount) const;

    // ✨ 新增 - 获取已占用槽位数量
    UFUNCTION(BlueprintPure, Category = "XB|Formation", meta = (DisplayName = "获取已占用槽位数"))
    int32 GetOccupiedSlotCount() const;

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

    // ✨ 新增 - 槽位重新分配委托
    UPROPERTY(BlueprintAssignable, Category = "XB|Formation", meta = (DisplayName = "槽位重分配事件"))
    FOnSlotReassigned OnSlotReassigned;

protected:
    // ============ 编队配置 ============

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Formation", meta = (DisplayName = "编队配置"))
    FXBFormationConfig FormationConfig;

    UPROPERTY(BlueprintReadOnly, Category = "XB|Formation", meta = (DisplayName = "编队槽位列表"))
    TArray<FXBFormationSlot> FormationSlots;

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
