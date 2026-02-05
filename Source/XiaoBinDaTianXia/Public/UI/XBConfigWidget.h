/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/UI/XBConfigWidget.h

/**
 * @file XBConfigWidget.h
 * @brief 配置界面 Widget - 负责配置数据展示与修改
 *
 * @note 架构意图:
 *       1. 仅负责 UI 层数据绑定与触发应用
 *       2. 实际存档读写与运行时应用由 GameInstance/角色负责
 */

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Save/XBSaveGame.h"
#include "XBConfigWidget.generated.h"


class AXBCharacterBase;
class UDataTable;
class UXBHealthBarColorConfig;

/**
 * @brief 配置界面 Widget
 * @note 作为 UI 数据承载层，提供蓝图可调用的刷新/应用/保存接口
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBConfigWidget : public UUserWidget {
  GENERATED_BODY()

protected:
  virtual void NativeConstruct() override;
  virtual void NativeDestruct() override;


public:
  /**
   * @brief  初始化配置数据
   * @param  InLeader 目标主将（可选）
   * @return 无
   * @note   详细流程分析: 缓存主将引用 -> 读取全局配置 -> 刷新本地数据
   */
  UFUNCTION(BlueprintCallable, Category = "XB|Config",
            meta = (DisplayName = "初始化配置"))
  void InitializeConfig(AXBCharacterBase *InLeader);

  /**
   * @brief  从存档刷新配置数据
   * @return 是否刷新成功
   * @note   详细流程分析: 获取 GameInstance -> 读取 GameConfig -> 同步到
   * ConfigData
   */
  UFUNCTION(BlueprintCallable, Category = "XB|Config",
            meta = (DisplayName = "刷新配置数据"))
  bool RefreshConfigFromSave();

  /**
   * @brief  应用配置数据
   * @param  bSaveToDisk 是否保存到存档
   * @return 是否应用成功
   * @note   详细流程分析: 保存 GameConfig -> 应用到主将与士兵 -> 可选保存
   */
  UFUNCTION(BlueprintCallable, Category = "XB|Config",
            meta = (DisplayName = "应用配置"))
  bool ApplyConfig(bool bSaveToDisk = true);

  /**
   * @brief  开始游戏
   * @param  bSaveToDisk 是否保存到存档
   * @return 是否开始成功
   * @note   详细流程分析: 写入配置 -> 应用到主将与士兵 -> 加载选定地图
   */
  UFUNCTION(BlueprintCallable, Category = "XB|Config",
            meta = (DisplayName = "开始游戏"))
  bool StartGame(bool bSaveToDisk = true);

  /**
   * @brief  设置配置数据
   * @param  NewConfig 新配置数据
   * @param  bSyncToUI 是否同步到 UI
   * @return 无
   * @note   详细流程分析: 覆盖 ConfigData -> 可选同步到 UI
   */
  UFUNCTION(BlueprintCallable, Category = "XB|Config",
            meta = (DisplayName = "设置配置数据"))
  void SetConfigData(const FXBGameConfigData &NewConfig, bool bSyncToUI = true);

  /**
   * @brief  获取当前配置数据
   * @return 当前配置数据
   * @note   详细流程分析: 直接返回 ConfigData，避免蓝图重复维护镜像变量
   */
  UFUNCTION(BlueprintCallable, Category = "XB|Config",
            meta = (DisplayName = "获取配置数据"))
  FXBGameConfigData GetConfigData() const;

  /**
   * @brief  同步 UI 控件数据到配置数据
   * @note   详细流程分析: 由蓝图实现，将当前 UI 值写回 ConfigData
   */
  UFUNCTION(BlueprintImplementableEvent, Category = "XB|Config",
            meta = (DisplayName = "同步UI到配置"))
  void SyncConfigFromUI();

  /**
   * @brief  同步配置数据到 UI 控件
   * @note   详细流程分析: 由蓝图实现，将 ConfigData 刷新到 UI
   */
  UFUNCTION(BlueprintImplementableEvent, Category = "XB|Config",
            meta = (DisplayName = "同步配置到UI"))
  void SyncUIFromConfig();

  /**
   * @brief  重置为默认配置
   * @param  bSaveToDisk 是否保存到存档
   * @return 是否重置成功
   */
  UFUNCTION(BlueprintCallable, Category = "XB|Config",
            meta = (DisplayName = "重置默认配置"))
  bool ResetToDefault(bool bSaveToDisk = true);

  /**
   * @brief  获取主将配置行列表
   * @return 行名数组
   */
  UFUNCTION(BlueprintCallable, Category = "XB|Config",
            meta = (DisplayName = "获取主将配置行列表"))
  TArray<FName> GetLeaderRowNames() const;

  /**
   * @brief  获取士兵配置行列表
   * @return 行名数组
   */
  UFUNCTION(BlueprintCallable, Category = "XB|Config",
            meta = (DisplayName = "获取士兵配置行列表"))
  TArray<FName> GetSoldierRowNames() const;

  // ✨ 新增 - 士兵类型选择接口（根据主将自动匹配）

  /**
   * @brief 获取可用士兵类型列表
   * @return 士兵类型枚举数组
   */
  UFUNCTION(BlueprintCallable, Category = "XB|Config",
            meta = (DisplayName = "获取士兵类型列表"))
  TArray<EXBSoldierType> GetSoldierTypes() const;

  /**
   * @brief 获取士兵类型的显示名称（中文）
   * @param SoldierType 士兵类型枚举
   * @return 显示名称文本
   * @note 使用 UEnum 反射 API 获取 DisplayName，确保打包后也能正确显示中文
   */
  UFUNCTION(BlueprintPure, Category = "XB|Config",
            meta = (DisplayName = "获取士兵类型显示名称"))
  static FText GetSoldierTypeDisplayName(EXBSoldierType SoldierType);

  /**
   * @brief 根据主将名称和士兵类型获取对应的士兵行名
   * @param LeaderRowName 主将行名
   * @param SoldierType 士兵类型
   * @return 匹配的士兵行名，未找到返回 NAME_None
   */
  UFUNCTION(BlueprintCallable, Category = "XB|Config",
            meta = (DisplayName = "根据类型获取士兵行名"))
  FName GetSoldierRowNameByType(FName LeaderRowName, EXBSoldierType SoldierType) const;

  // ✨ 新增 - 血条颜色选择接口

  /**
   * @brief 获取血条颜色名称列表（供 UI 下拉框使用）
   * @return 颜色名称数组
   */
  UFUNCTION(BlueprintCallable, Category = "XB|Config",
            meta = (DisplayName = "获取血条颜色名称列表"))
  TArray<FText> GetHealthBarColorNames() const;

  /**
   * @brief 根据颜色名称获取颜色值
   * @param ColorName 颜色名称
   * @return 颜色值
   */
  UFUNCTION(BlueprintCallable, Category = "XB|Config",
            meta = (DisplayName = "根据名称获取血条颜色"))
  FLinearColor GetHealthBarColorByName(const FString& ColorName) const;

public:
  // ==================== UI 数据 ====================

  UPROPERTY(BlueprintReadWrite, Category = "配置",
            meta = (DisplayName = "配置数据"))
  FXBGameConfigData ConfigData;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置",
            meta = (DisplayName = "主将配置数据表"))
  TObjectPtr<UDataTable> LeaderConfigDataTable;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置",
            meta = (DisplayName = "士兵配置数据表"))
  TObjectPtr<UDataTable> SoldierConfigDataTable;

  /** 地图选项标签（使用 Map 分类下的标签） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置",
            meta = (DisplayName = "地图选项", Categories = "Map"))
  FGameplayTagContainer MapOptions;

  /** 血条颜色配置数据资产 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置",
            meta = (DisplayName = "血条颜色配置"))
  TObjectPtr<UXBHealthBarColorConfig> HealthBarColorConfig;

  /**
   * @brief 从地图标签中获取地图名称
   * @param MapTag 地图标签（如 Map.01_草地）
   * @return 地图名称（如 01_草地）
   * @note 移除 "Map." 前缀后返回剩余部分作为地图名称
   */
  UFUNCTION(BlueprintPure, Category = "XB|Config",
            meta = (DisplayName = "从标签获取地图名称"))
  static FString GetMapNameFromTag(const FGameplayTag& MapTag);

  UPROPERTY(BlueprintReadWrite, Category = "配置",
            meta = (DisplayName = "目标主将"))
  TWeakObjectPtr<AXBCharacterBase> TargetLeader;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置",
            meta = (DisplayName = "初始配置缓存"))
  FXBGameConfigData InitialConfigData;

private:
  UPROPERTY(VisibleAnywhere, Category = "配置",
            meta = (DisplayName = "是否已缓存初始配置"))
  bool bHasCachedInitialConfig = false;
};
