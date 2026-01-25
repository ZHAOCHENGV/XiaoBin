// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "XBPlacementSaveWidget.generated.h"

class UXBActorPlacementComponent;

/**
 * @brief 放置保存界面 Widget
 * @note 负责保存放置数据到指定存档名称
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBPlacementSaveWidget : public UUserWidget {
  GENERATED_BODY()

public:
  /**
   * @brief 设置放置组件引用
   * @param InComponent 放置组件
   */
  UFUNCTION(BlueprintCallable, Category = "XB|放置存档",
            meta = (DisplayName = "设置放置组件"))
  void SetPlacementComponent(UXBActorPlacementComponent *InComponent);

  /**
   * @brief 保存当前放置到指定槽位
   * @param SlotName 存档槽位名称（用于文件系统）
   * @param DisplayName 存档显示名称（用于 UI 显示）
   * @return 是否保存成功
   */
  UFUNCTION(BlueprintCallable, Category = "XB|放置存档",
            meta = (DisplayName = "保存放置(带显示名)"))
  bool SavePlacementWithDisplayName(const FString &SlotName,
                                    const FString &DisplayName);

  /**
   * @brief 保存当前放置到指定名称（简化版）
   * @param SlotName 存档名称
   * @return 是否保存成功
   */
  UFUNCTION(BlueprintCallable, Category = "XB|放置存档",
            meta = (DisplayName = "保存放置(名称)"))
  bool SavePlacementByName(const FString &SlotName);

  /**
   * @brief 同步 UI 控件数据到配置
   * @note 蓝图实现
   */
  UFUNCTION(BlueprintImplementableEvent, Category = "XB|放置存档",
            meta = (DisplayName = "同步UI到配置"))
  void SyncConfigFromUI();

  /**
   * @brief 获取放置组件
   * @return 放置组件指针
   */
  UFUNCTION(BlueprintPure, Category = "XB|放置存档",
            meta = (DisplayName = "获取放置组件"))
  UXBActorPlacementComponent *GetPlacementComponent() const;

public:
  /** 放置组件引用 */
  UPROPERTY(BlueprintReadWrite, Category = "配置",
            meta = (DisplayName = "放置组件", ExposeOnSpawn = "true"))
  UXBActorPlacementComponent *PlacementComponent;

  /** 当前存档槽位名称 */
  UPROPERTY(BlueprintReadWrite, Category = "配置",
            meta = (DisplayName = "存档槽位名称"))
  FString CurrentSlotName;

  /** 当前存档显示名称 */
  UPROPERTY(BlueprintReadWrite, Category = "配置",
            meta = (DisplayName = "存档显示名称"))
  FString CurrentDisplayName;
};
