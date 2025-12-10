/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Character/Components/XBFormationComponent.h

/**
 * @file XBFormationComponent.h
 * @brief 编队组件 - 管理士兵队列位置
 * 
 * @note 🔧 修改记录:
 *       1. 完善编队算法（横向=纵向×2规则）
 *       2. 新增士兵分配/释放接口
 *       3. 新增补位逻辑支持
 *       4. 优化世界坐标计算
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Army/XBSoldierTypes.h"
#include "XBFormationComponent.generated.h"

class AXBSoldierActor;

// ✨ 新增 - 编队更新委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFormationUpdated);

/**
 * @brief 编队组件 (Formation Component)
 * 
 * 挂载于将领（Leader）身上，负责计算和管理跟随士兵的目标位置。
 * 
 * @details 核心逻辑：
 * 1. 根据当前士兵总数动态计算阵型结构（基于设计文档规则）
 *    - 横向<4：纵向1
 *    - 横向4：纵向2
 *    - 横向6：纵向3
 *    - 横向8：纵向4
 *    以此类推...
 * 2. 维护一组 Slot（槽位），每个 Slot 对应一个相对于将领的本地坐标
 * 3. 提供槽位分配和释放接口，供士兵调用
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

    /**
     * @brief 获取当前所有的编队槽位数据
     * @return 槽位数组引用
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    const TArray<FXBFormationSlot>& GetFormationSlots() const { return FormationSlots; }

    /**
     * @brief 根据士兵数量重新生成编队槽位
     * @param SoldierCount 当前士兵总数
     * @note 实现设计文档的编队规则
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    void RegenerateFormation(int32 SoldierCount);

    /**
     * @brief 获取指定槽位的世界空间位置
     * @param SlotIndex 槽位索引
     * @return 世界坐标
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    FVector GetSlotWorldPosition(int32 SlotIndex) const;

    /**
     * @brief 获取第一个空闲（未被占用）的槽位索引
     * @return 索引值，若无空闲则返回 INDEX_NONE (-1)
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    int32 GetFirstAvailableSlot() const;

    /**
     * @brief 尝试占用指定槽位
     * @param SlotIndex 目标槽位索引
     * @param SoldierId 占用者的士兵ID
     * @return 是否占用成功
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    bool OccupySlot(int32 SlotIndex, int32 SoldierId);

    /**
     * @brief 释放指定槽位
     * @param SlotIndex 要释放的槽位索引
     * @return 是否释放成功
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    bool ReleaseSlot(int32 SlotIndex);

    /**
     * @brief 释放所有槽位
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    void ReleaseAllSlots();

    // ✨ 新增 - 士兵分配接口

    /**
     * @brief 为士兵分配槽位
     * @param Soldier 士兵Actor
     * @return 分配的槽位索引，失败返回 INDEX_NONE
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    int32 AssignSlotToSoldier(AXBSoldierActor* Soldier);

    /**
     * @brief 移除士兵的槽位分配
     * @param Soldier 士兵Actor
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    void RemoveSoldierFromSlot(AXBSoldierActor* Soldier);

    /**
     * @brief 重新分配所有士兵的槽位（补位逻辑）
     * @param Soldiers 士兵数组
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    void ReassignAllSlots(const TArray<AXBSoldierActor*>& Soldiers);

    // ============ 配置接口 ============

    /**
     * @brief 获取当前的编队配置参数
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    const FXBFormationConfig& GetFormationConfig() const { return FormationConfig; }

    /**
     * @brief 更新编队配置参数
     * @param NewConfig 新配置
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    void SetFormationConfig(const FXBFormationConfig& NewConfig);

    // ============ 调试 ============

    /**
     * @brief 绘制编队调试信息
     * @param Duration 调试线持续时间
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation|Debug")
    void DrawDebugFormation(float Duration = 0.0f);

    // ============ 委托 ============

    /** @brief 编队更新事件 */
    UPROPERTY(BlueprintAssignable, Category = "XB|Formation")
    FOnFormationUpdated OnFormationUpdated;

protected:
    /** @brief 编队配置参数 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Formation", meta = (DisplayName = "编队配置"))
    FXBFormationConfig FormationConfig;

    /** @brief 运行时存储的编队槽位数组 */
    UPROPERTY(BlueprintReadOnly, Category = "XB|Formation", meta = (DisplayName = "编队槽位列表"))
    TArray<FXBFormationSlot> FormationSlots;

    /** @brief 是否在编辑器运行时自动绘制调试图形 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Formation|Debug", meta = (DisplayName = "编辑器内绘制调试"))
    bool bDrawDebugInEditor = false;

private:
    // ✨ 新增 - 内部辅助方法

    /**
     * @brief 根据设计文档规则计算编队维度
     * @param SoldierCount 士兵数量
     * @param OutColumns 输出列数（横向）
     * @param OutRows 输出行数（纵向）
     * @note 规则: 横向<4时纵向1, 横向4纵向2, 横向6纵向3, 横向8纵向4...
     */
    void CalculateFormationDimensions(int32 SoldierCount, int32& OutColumns, int32& OutRows) const;

    /**
     * @brief 计算指定槽位的本地偏移
     * @param SlotIndex 槽位索引
     * @param TotalSoldiers 总士兵数
     * @param Columns 列数
     * @param Rows 行数
     * @return 相对于将领的本地偏移
     */
    FVector2D CalculateSlotLocalOffset(int32 SlotIndex, int32 TotalSoldiers, int32 Columns, int32 Rows) const;
};
