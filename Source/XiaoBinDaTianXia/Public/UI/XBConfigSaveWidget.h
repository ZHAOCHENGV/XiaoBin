// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Save/XBSaveGame.h"
#include "XBConfigSaveWidget.generated.h"

class UXBSaveSubsystem;
class UXBGameInstance;

/**
 * @brief 配置保存界面 Widget
 * @note  负责保存配置数据到指定存档名称
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBConfigSaveWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief  设置配置数据
     * @param  NewConfig 新配置数据
     * @param  bSyncToUI 是否同步到 UI
     * @return 无
     * @note   详细流程分析: 覆盖 ConfigData -> 可选同步到 UI
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "设置配置数据"))
    void SetConfigData(const FXBGameConfigData& NewConfig, bool bSyncToUI = true);

    /**
     * @brief  获取当前配置数据
     * @return 当前配置数据
     * @note   详细流程分析: 直接返回 ConfigData，便于蓝图读取
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "获取配置数据"))
    FXBGameConfigData GetConfigData() const;

    /**
     * @brief  保存配置到指定名称
     * @param  SlotName 存档名称
     * @param  bSaveToDisk 是否保存到磁盘
     * @return 是否保存成功
     * @note   详细流程分析: 同步 UI -> 写入 GameConfig -> 保存到槽位
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "保存配置(名称)"))
    bool SaveConfigByName(const FString& SlotName, bool bSaveToDisk = true);

    /**
     * @brief  同步 UI 控件数据到配置数据
     * @note   详细流程分析: 由蓝图实现，将当前 UI 值写回 ConfigData
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "XB|Config", meta = (DisplayName = "同步UI到配置"))
    void SyncConfigFromUI();

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
};
