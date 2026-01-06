// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Save/XBSaveGame.h"
#include "XBConfigLoadWidget.generated.h"

class UXBSaveSubsystem;
class UXBGameInstance;

/**
 * @brief 配置读取界面 Widget
 * @note  负责列举/读取/删除配置存档
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBConfigLoadWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief  刷新存档列表
     * @return 是否刷新成功
     * @note   详细流程分析: 获取存档子系统 -> 拉取槽位名称 -> 更新 SlotNames
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "刷新存档列表"))
    bool RefreshSaveSlotNames();

    /**
     * @brief  读取配置存档
     * @param  SlotName 存档名称
     * @param  bSyncToUI 是否同步到 UI
     * @return 是否读取成功
     * @note   详细流程分析: 读取存档 -> 同步到 GameInstance -> 更新 ConfigData
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "读取配置(名称)"))
    bool LoadConfigByName(const FString& SlotName, bool bSyncToUI = true);

    /**
     * @brief  删除配置存档
     * @param  SlotName 存档名称
     * @return 是否删除成功
     * @note   详细流程分析: 删除存档 -> 刷新列表
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "删除配置存档"))
    bool DeleteConfigByName(const FString& SlotName);

    /**
     * @brief  获取当前配置数据
     * @return 当前配置数据
     * @note   详细流程分析: 直接返回 ConfigData，便于蓝图读取
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "获取配置数据"))
    FXBGameConfigData GetConfigData() const;

    /**
     * @brief  同步配置数据到 UI 控件
     * @note   详细流程分析: 由蓝图实现，将 ConfigData 刷新到 UI
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "XB|Config", meta = (DisplayName = "同步配置到UI"))
    void SyncUIFromConfig();

public:
    // ==================== UI 数据 ====================

    UPROPERTY(BlueprintReadWrite, Category = "配置", meta = (DisplayName = "配置数据"))
    FXBGameConfigData ConfigData;

    UPROPERTY(BlueprintReadWrite, Category = "配置", meta = (DisplayName = "存档槽位名称列表"))
    TArray<FString> SlotNames;
};
