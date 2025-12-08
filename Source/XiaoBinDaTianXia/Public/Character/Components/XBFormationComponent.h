// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Army/XBSoldierTypes.h"
#include "XBFormationComponent.generated.h"

/**
 * @brief 编队组件 (Formation Component)
 * * 挂载于将领（Leader）身上，负责计算和管理跟随士兵的目标位置。
 * * @details 核心逻辑：
 * 1. 根据当前士兵总数动态计算阵型结构（基于横向=2倍纵向的规则）。
 * 2. 维护一组 Slot（槽位），每个 Slot 对应一个相对于将领的本地坐标。
 * 3. 提供槽位分配（Occupy）和释放（Release）接口，供 SoldierAgent 调用。
 * * @note 该组件只负责计算“位置”，士兵的实际移动由 AI 或 Simulation 系统处理。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName = "XB Formation Component"))
class XIAOBINDATIANXIA_API UXBFormationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    /**
     * @brief 构造函数
     * * 设置默认 Tick 启用状态
     */
    UXBFormationComponent();

    /**
     * @brief 游戏开始
     * * 初始化逻辑
     */
    virtual void BeginPlay() override;

    /**
     * @brief 每帧更新
     * * @param DeltaTime 帧时间
     * 用于处理调试绘制或动态阵型平滑调整
     */
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ============ 编队管理 ============

    /**
     * @brief 获取当前所有的编队槽位数据
     * * @return const TArray<FXBFormationSlot>& 槽位数组引用
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    const TArray<FXBFormationSlot>& GetFormationSlots() const { return FormationSlots; }

    /**
     * @brief 根据士兵数量重新生成编队槽位
     * * @param SoldierCount 当前士兵总数
     * @details 根据“横向数量 = 纵向数量 * 2”的规则（如 4x2, 6x3, 8x4...）
     * 计算所需的行列数，并生成对应的本地坐标偏移。
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    void RegenerateFormation(int32 SoldierCount);

    /**
     * @brief 获取指定槽位的世界空间位置
     * * @param SlotIndex 槽位索引
     * * @return FVector 世界坐标
     * @details 将槽位的 LocalOffset 变换为 WorldLocation，考虑将领当前的旋转。
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    FVector GetSlotWorldPosition(int32 SlotIndex) const;

    /**
     * @brief 获取第一个空闲（未被占用）的槽位索引
     * * @return int32 索引值，若无空闲则返回 INDEX_NONE (-1)
     * 通常用于新招募士兵的入列分配。
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    int32 GetFirstAvailableSlot() const;

    /**
     * @brief 尝试占用指定槽位
     * * @param SlotIndex 目标槽位索引
     * * @param SoldierId 占用者的士兵ID
     * * @return bool 是否占用成功（若已被占用则失败）
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    bool OccupySlot(int32 SlotIndex, int32 SoldierId);

    /**
     * @brief 释放指定槽位
     * * @param SlotIndex 要释放的槽位索引
     * * @return bool 是否释放成功
     * 士兵死亡或脱离战斗队列时调用。
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    bool ReleaseSlot(int32 SlotIndex);

    /**
     * @brief 获取当前的编队配置参数
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    const FXBFormationConfig& GetFormationConfig() const { return FormationConfig; }

    /**
     * @brief 更新编队配置参数
     * * @param NewConfig 新配置
     * 调用后通常需要手动调用 RegenerateFormation 以应用更改。
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation")
    void SetFormationConfig(const FXBFormationConfig& NewConfig);

    // ============ 调试 ============

    /**
     * @brief 绘制编队调试信息
     * * @param Duration 调试线持续时间
     * 在世界空间绘制出每个 Slot 的位置和占用状态（绿色=空闲，红色=占用）。
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Formation|Debug")
    void DrawDebugFormation(float Duration = 0.0f);

protected:
    
    /** 编队配置参数（间距、跟随速度等） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Formation", meta = (DisplayName = "编队配置"))
    FXBFormationConfig FormationConfig;

    
    /** 运行时存储的编队槽位数组 */
    UPROPERTY(BlueprintReadOnly, Category = "XB|Formation", meta = (DisplayName = "编队槽位列表"))
    TArray<FXBFormationSlot> FormationSlots;

    
    /** 是否在编辑器运行时自动绘制调试图形 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Formation|Debug", meta = (DisplayName = "编辑器内绘制调试"))
    bool bDrawDebugInEditor = false;
};