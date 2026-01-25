// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "XBPlacementLoadWidget.generated.h"

class UXBActorPlacementComponent;

/**
 * @brief 存档信息结构（用于 UI 显示）
 */
USTRUCT(BlueprintType)
struct FXBPlacementSlotInfo {
  GENERATED_BODY()

  /** 槽位名称（文件系统名） */
  UPROPERTY(BlueprintReadWrite, Category = "存档信息",
            meta = (DisplayName = "槽位名称"))
  FString SlotName;

  /** 显示名称（用户自定义） */
  UPROPERTY(BlueprintReadWrite, Category = "存档信息",
            meta = (DisplayName = "显示名称"))
  FString DisplayName;

  /** 存档时间 */
  UPROPERTY(BlueprintReadWrite, Category = "存档信息",
            meta = (DisplayName = "存档时间"))
  FDateTime SaveTime;

  /** Actor 数量 */
  UPROPERTY(BlueprintReadWrite, Category = "存档信息",
            meta = (DisplayName = "Actor数量"))
  int32 ActorCount = 0;
};

/**
 * @brief 放置读取界面 Widget
 * @note 负责列举/读取/删除放置存档
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBPlacementLoadWidget : public UUserWidget {
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
   * @brief 刷新存档列表
   * @return 是否刷新成功
   */
  UFUNCTION(BlueprintCallable, Category = "XB|放置存档",
            meta = (DisplayName = "刷新存档列表"))
  bool RefreshSaveSlotNames();

  /**
   * @brief 读取指定存档
   * @param SlotName 存档槽位名称
   * @return 是否读取成功
   */
  UFUNCTION(BlueprintCallable, Category = "XB|放置存档",
            meta = (DisplayName = "读取放置(名称)"))
  bool LoadPlacementByName(const FString &SlotName);

  /**
   * @brief 删除指定存档
   * @param SlotName 存档槽位名称
   * @return 是否删除成功
   */
  UFUNCTION(BlueprintCallable, Category = "XB|放置存档",
            meta = (DisplayName = "删除放置存档"))
  bool DeletePlacementByName(const FString &SlotName);

  /**
   * @brief 获取放置组件
   * @return 放置组件指针
   */
  UFUNCTION(BlueprintPure, Category = "XB|放置存档",
            meta = (DisplayName = "获取放置组件"))
  UXBActorPlacementComponent *GetPlacementComponent() const;

  /**
   * @brief 同步配置数据到 UI 控件
   * @note 蓝图实现
   */
  UFUNCTION(BlueprintImplementableEvent, Category = "XB|放置存档",
            meta = (DisplayName = "同步配置到UI"))
  void SyncUIFromConfig();

  /**
   * @brief 存档列表刷新完成事件
   * @note 蓝图实现，用于刷新 UI 列表
   */
  UFUNCTION(BlueprintImplementableEvent, Category = "XB|放置存档",
            meta = (DisplayName = "存档列表已刷新"))
  void OnSlotListRefreshed();

public:
  /** 放置组件引用 */
  UPROPERTY(BlueprintReadWrite, Category = "配置",
            meta = (DisplayName = "放置组件", ExposeOnSpawn = "true"))
  UXBActorPlacementComponent *PlacementComponent;

  /** 存档槽位名称列表（简单版） */
  UPROPERTY(BlueprintReadWrite, Category = "配置",
            meta = (DisplayName = "存档槽位名称列表"))
  TArray<FString> SlotNames;

  /** 存档信息列表（详细版） */
  UPROPERTY(BlueprintReadWrite, Category = "配置",
            meta = (DisplayName = "存档信息列表"))
  TArray<FXBPlacementSlotInfo> SlotInfoList;
};
