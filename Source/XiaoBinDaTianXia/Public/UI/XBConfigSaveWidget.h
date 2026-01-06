// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Save/XBSaveGame.h"
#include "XBConfigSaveWidget.generated.h"

class UXBSaveSubsystem;
class UXBGameInstance;
class UXBConfigWidget;

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
     * @brief  设置配置界面引用
     * @param  InConfigWidget 配置界面
     * @return 无
     * @note   详细流程分析: 保存界面通过配置界面获取最新 UI 数据
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "设置配置界面引用"))
    void SetTargetConfigWidget(UXBConfigWidget* InConfigWidget);

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

public:
    // ==================== UI 数据 ====================

    UPROPERTY(BlueprintReadWrite, Category = "配置", meta = (DisplayName = "配置数据"))
    FXBGameConfigData ConfigData;

    UPROPERTY(BlueprintReadWrite, Category = "配置", meta = (DisplayName = "配置界面引用"))
    TWeakObjectPtr<UXBConfigWidget> TargetConfigWidget;
};
