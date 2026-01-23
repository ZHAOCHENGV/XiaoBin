/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/UI/XBLeaderSpawnConfigWidget.h

/**
 * @file XBLeaderSpawnConfigWidget.h
 * @brief 主将放置配置界面 Widget 基类
 *
 * @note ✨ 新增文件 - 用于配置阶段放置主将时弹出的配置界面
 */

#pragma once

#include "Blueprint/UserWidget.h"
#include "Config/XBLeaderSpawnConfigData.h"
#include "CoreMinimal.h"
#include "XBLeaderSpawnConfigWidget.generated.h"


class UDataTable;

// ============ 代理声明 ============

/** 配置确认代理 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLeaderConfigConfirmed, int32,
                                             EntryIndex,
                                             FXBLeaderSpawnConfigData,
                                             ConfigData);

/** 配置取消代理 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeaderConfigCancelled);

/**
 * @brief 主将放置配置界面 Widget 基类
 * @note C++ 定义接口，蓝图实现 UI
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBLeaderSpawnConfigWidget : public UUserWidget {
  GENERATED_BODY()

public:
  // ============ 初始化 ============

  /**
   * @brief 初始化配置界面
   * @param InEntryIndex 放置条目索引
   */
  UFUNCTION(BlueprintCallable, Category = "XB|主将配置",
            meta = (DisplayName = "初始化配置界面"))
  void InitializeWithEntry(int32 InEntryIndex);

  // ============ 配置数据访问 ============

  /**
   * @brief 获取当前配置数据
   * @return 配置数据
   */
  UFUNCTION(BlueprintCallable, Category = "XB|主将配置",
            meta = (DisplayName = "获取配置数据"))
  FXBLeaderSpawnConfigData GetConfigData() const;

  /**
   * @brief 设置配置数据
   * @param InConfigData 配置数据
   * @param bSyncToUI 是否同步到 UI
   */
  UFUNCTION(BlueprintCallable, Category = "XB|主将配置",
            meta = (DisplayName = "设置配置数据"))
  void SetConfigData(const FXBLeaderSpawnConfigData &InConfigData,
                     bool bSyncToUI = true);

  // ============ 蓝图可实现事件 ============

  /**
   * @brief 同步配置数据到 UI 控件
   * @note 蓝图实现，将 ConfigData 刷新到 UI
   */
  UFUNCTION(BlueprintImplementableEvent, Category = "XB|主将配置",
            meta = (DisplayName = "同步配置到UI"))
  void SyncUIFromConfig();

  /**
   * @brief 同步 UI 控件数据到配置数据
   * @note 蓝图实现，将当前 UI 值写回 ConfigData
   */
  UFUNCTION(BlueprintImplementableEvent, Category = "XB|主将配置",
            meta = (DisplayName = "同步UI到配置"))
  void SyncConfigFromUI();

  // ============ 按钮回调 ============

  /**
   * @brief 确认按钮回调
   * @note 蓝图中绑定确认按钮后调用此函数
   */
  UFUNCTION(BlueprintCallable, Category = "XB|主将配置",
            meta = (DisplayName = "确认配置"))
  void OnConfirmClicked();

  /**
   * @brief 取消按钮回调
   * @note 蓝图中绑定取消按钮后调用此函数
   */
  UFUNCTION(BlueprintCallable, Category = "XB|主将配置",
            meta = (DisplayName = "取消配置"))
  void OnCancelClicked();

  // ============ 数据表访问 ============

  /**
   * @brief 获取主将配置行列表
   * @return 行名数组
   */
  UFUNCTION(BlueprintCallable, Category = "XB|主将配置",
            meta = (DisplayName = "获取主将配置行列表"))
  TArray<FName> GetLeaderRowNames() const;

  /**
   * @brief 获取士兵配置行列表
   * @return 行名数组
   */
  UFUNCTION(BlueprintCallable, Category = "XB|主将配置",
            meta = (DisplayName = "获取士兵配置行列表"))
  TArray<FName> GetSoldierRowNames() const;

  // ============ 代理事件 ============

  /** 配置确认事件 */
  UPROPERTY(BlueprintAssignable, Category = "XB|主将配置|事件")
  FOnLeaderConfigConfirmed OnConfigConfirmed;

  /** 配置取消事件 */
  UPROPERTY(BlueprintAssignable, Category = "XB|主将配置|事件")
  FOnLeaderConfigCancelled OnConfigCancelled;

public:
  // ============ 配置属性 ============

  /** 当前配置数据 */
  UPROPERTY(BlueprintReadWrite, Category = "配置",
            meta = (DisplayName = "配置数据"))
  FXBLeaderSpawnConfigData ConfigData;

  /** 主将配置数据表 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置",
            meta = (DisplayName = "主将配置数据表"))
  TObjectPtr<UDataTable> LeaderConfigDataTable;

  /** 士兵配置数据表 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置",
            meta = (DisplayName = "士兵配置数据表"))
  TObjectPtr<UDataTable> SoldierConfigDataTable;

  /** 初始配置数据缓存（用于重置） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置",
            meta = (DisplayName = "初始配置缓存"))
  FXBLeaderSpawnConfigData InitialConfigData;

private:
  /** 放置条目索引 */
  int32 EntryIndex = -1;
};
