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
#include "Army/XBSoldierTypes.h"
#include "CoreMinimal.h"
#include "XBLeaderSpawnConfigWidget.generated.h"

class UDataTable;

// ============ 代理声明 ============

/** 配置确认代理 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLeaderConfigConfirmed, int32,EntryIndex,FXBLeaderSpawnConfigData,ConfigData);

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

  // ✨ 新增 - 士兵类型选择接口（根据主将自动匹配）

  /**
   * @brief 获取可用士兵类型列表
   * @return 士兵类型枚举数组
   * @note 返回 Infantry/Archer/Cavalry 供 UI 选择
   */
  UFUNCTION(BlueprintCallable, Category = "XB|主将配置",
            meta = (DisplayName = "获取士兵类型列表"))
  TArray<EXBSoldierType> GetSoldierTypes() const;

  /**
   * @brief 获取士兵类型的显示名称（中文）
   * @param SoldierType 士兵类型枚举
   * @return 显示名称文本
   * @note 使用 UEnum 反射 API 获取 DisplayName，确保打包后也能正确显示中文
   */
  UFUNCTION(BlueprintPure, Category = "XB|主将配置",
            meta = (DisplayName = "获取士兵类型显示名称"))
  static FText GetSoldierTypeDisplayName(EXBSoldierType SoldierType);

  /**
   * @brief 根据主将名称和士兵类型获取对应的士兵行名
   * @param LeaderRowName 主将行名（如：李世民）
   * @param SoldierType 士兵类型（如：Cavalry）
   * @return 匹配的士兵行名（如：李世民_玄甲铁骑），未找到返回 NAME_None
   * @note 士兵行名格式：{主将名称}_{士兵名称}
   */
  UFUNCTION(BlueprintCallable, Category = "XB|主将配置",
            meta = (DisplayName = "根据类型获取士兵行名"))
  FName GetSoldierRowNameByType(FName LeaderRowName, EXBSoldierType SoldierType) const;

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
