/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/UI/XBBatchPlacementConfigWidget.h

/**
 * @file XBBatchPlacementConfigWidget.h
 * @brief 批量放置配置界面 Widget 基类
 *
 * @note ✨ 新增文件 - 用于配置阶段批量放置时弹出的配置界面
 */

#pragma once

#include "Blueprint/UserWidget.h"
#include "Config/XBBatchPlacementConfigData.h"
#include "CoreMinimal.h"
#include "XBBatchPlacementConfigWidget.generated.h"

// ============ 代理声明 ============

/** 批量配置确认代理 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBatchConfigConfirmed, int32, EntryIndex, FXBBatchPlacementConfigData, ConfigData);

/** 批量配置取消代理 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBatchConfigCancelled);

/**
 * @brief 批量放置配置界面 Widget 基类
 * @note C++ 定义接口，蓝图实现 UI
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBBatchPlacementConfigWidget : public UUserWidget {
  GENERATED_BODY()

public:
  // ============ 初始化 ============

  /**
   * @brief 初始化配置界面
   * @param InEntryIndex 放置条目索引
   * @note 从 PlacementConfig 中自动获取默认的网格尺寸和间距
   */
  UFUNCTION(BlueprintCallable, Category = "XB|批量配置",
            meta = (DisplayName = "初始化配置界面"))
  void InitializeWithEntry(int32 InEntryIndex);

  // ============ 配置数据访问 ============

  /**
   * @brief 获取当前配置数据
   * @return 配置数据
   */
  UFUNCTION(BlueprintCallable, Category = "XB|批量配置",
            meta = (DisplayName = "获取配置数据"))
  FXBBatchPlacementConfigData GetConfigData() const;

  /**
   * @brief 设置配置数据
   * @param InConfigData 配置数据
   * @param bSyncToUI 是否同步到 UI
   */
  UFUNCTION(BlueprintCallable, Category = "XB|批量配置",
            meta = (DisplayName = "设置配置数据"))
  void SetConfigData(const FXBBatchPlacementConfigData& InConfigData, bool bSyncToUI = true);

  // ============ 蓝图可实现事件 ============

  /**
   * @brief 同步配置数据到 UI 控件
   * @note 蓝图实现，将 ConfigData 刷新到 UI
   */
  UFUNCTION(BlueprintImplementableEvent, Category = "XB|批量配置",
            meta = (DisplayName = "同步配置到UI"))
  void SyncUIFromConfig();

  /**
   * @brief 同步 UI 控件数据到配置数据
   * @note 蓝图实现，将当前 UI 值写回 ConfigData
   */
  UFUNCTION(BlueprintImplementableEvent, Category = "XB|批量配置",
            meta = (DisplayName = "同步UI到配置"))
  void SyncConfigFromUI();

  // ============ 按钮回调 ============

  /**
   * @brief 确认按钮回调
   * @note 蓝图中绑定确认按钮后调用此函数
   */
  UFUNCTION(BlueprintCallable, Category = "XB|批量配置",
            meta = (DisplayName = "确认配置"))
  void OnConfirmClicked();

  /**
   * @brief 取消按钮回调
   * @note 蓝图中绑定取消按钮后调用此函数
   */
  UFUNCTION(BlueprintCallable, Category = "XB|批量配置",
            meta = (DisplayName = "取消配置"))
  void OnCancelClicked();

  // ============ 代理事件 ============

  /** 配置确认事件 */
  UPROPERTY(BlueprintAssignable, Category = "XB|批量配置|事件")
  FOnBatchConfigConfirmed OnConfigConfirmed;

  /** 配置取消事件 */
  UPROPERTY(BlueprintAssignable, Category = "XB|批量配置|事件")
  FOnBatchConfigCancelled OnConfigCancelled;

public:
  // ============ 配置属性 ============

  /** 当前配置数据 */
  UPROPERTY(BlueprintReadWrite, Category = "配置",
            meta = (DisplayName = "配置数据"))
  FXBBatchPlacementConfigData ConfigData;

protected:
  // ============ 生命周期 ============

  virtual void NativeConstruct() override;
  virtual void NativeDestruct() override;

private:
  /** 放置条目索引 */
  int32 EntryIndex = -1;

  /** 缓存原始光标状态 */
  bool bOriginalShowCursor = false;
};
