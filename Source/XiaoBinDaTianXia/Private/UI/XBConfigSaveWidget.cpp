// Copyright XiaoBing Project. All Rights Reserved.

#include "UI/XBConfigSaveWidget.h"
#include "Game/XBGameInstance.h"
#include "Save/XBSaveSubsystem.h"
#include "UI/XBConfigWidget.h"
#include "Utils/XBLogCategories.h"

void UXBConfigSaveWidget::SetTargetConfigWidget(UXBConfigWidget* InConfigWidget)
{
    // 🔧 修改 - 缓存配置界面引用，保存时直接从 UI 读取最新数据
    TargetConfigWidget = InConfigWidget;
}

void UXBConfigSaveWidget::SetConfigData(const FXBGameConfigData& NewConfig, bool bSyncToUI)
{
    // 🔧 修改 - 直接覆盖当前配置数据，便于蓝图统一赋值
    ConfigData = NewConfig;

    if (bSyncToUI)
    {
        // 🔧 修改 - 优先同步到配置界面，避免保存界面维护额外 UI 状态
        if (TargetConfigWidget.IsValid())
        {
            TargetConfigWidget->SetConfigData(ConfigData, true);
        }
    }
}

FXBGameConfigData UXBConfigSaveWidget::GetConfigData() const
{
    // 🔧 修改 - 优先从配置界面读取最新数据，避免缓存过期
    if (TargetConfigWidget.IsValid())
    {
        return TargetConfigWidget->GetConfigData();
    }

    return ConfigData;
}

bool UXBConfigSaveWidget::SaveConfigByName(const FString& SlotName, bool bSaveToDisk)
{
    if (SlotName.IsEmpty())
    {
        UE_LOG(LogXBConfig, Warning, TEXT("保存配置失败：SlotName 为空"));
        return false;
    }

    UXBGameInstance* GameInstance = GetGameInstance<UXBGameInstance>();
    if (!GameInstance)
    {
        UE_LOG(LogXBConfig, Warning, TEXT("保存配置失败：GameInstance 为空"));
        return false;
    }

    // 🔧 修改 - 配置界面未绑定时拒绝保存，避免写入默认值导致数据归零
    if (!TargetConfigWidget.IsValid())
    {
        UE_LOG(LogXBConfig, Warning, TEXT("保存配置失败：配置界面引用无效，请先设置配置界面"));
        return false;
    }

    // 🔧 修改 - 保存前从配置界面同步 UI，确保保存的是当前界面最新数据
    TargetConfigWidget->SyncConfigFromUI();
    ConfigData = TargetConfigWidget->GetConfigData();

    // 🔧 修改 - 先写入配置，再进行存档保存，确保保存的是最新配置数据
    GameInstance->SetGameConfig(ConfigData, false);

    if (!bSaveToDisk)
    {
        return true;
    }

    UXBSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UXBSaveSubsystem>();
    if (!SaveSubsystem)
    {
        UE_LOG(LogXBConfig, Warning, TEXT("保存配置失败：SaveSubsystem 为空"));
        return false;
    }

    // 🔧 修改 - 若存档名已存在，则拒绝覆盖并返回失败
    if (SaveSubsystem->DoesSaveGameExist(SlotName, 0))
    {
        UE_LOG(LogXBConfig, Warning, TEXT("保存配置失败：存档名已存在 %s"), *SlotName);
        return false;
    }

    // 🔧 修改 - 将当前存档对象交给存档子系统统一保存
    SaveSubsystem->SetCurrentSaveGame(GameInstance->GetCurrentSaveGame());
    return SaveSubsystem->SaveGame(SlotName, 0);
}
