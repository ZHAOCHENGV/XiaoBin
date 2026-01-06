// Copyright XiaoBing Project. All Rights Reserved.

#include "UI/XBConfigSaveWidget.h"
#include "Game/XBGameInstance.h"
#include "Save/XBSaveSubsystem.h"
#include "Utils/XBLogCategories.h"

void UXBConfigSaveWidget::SetConfigData(const FXBGameConfigData& NewConfig, bool bSyncToUI)
{
    // 🔧 修改 - 直接覆盖当前配置数据，便于蓝图统一赋值
    ConfigData = NewConfig;

    if (bSyncToUI)
    {
        // 🔧 修改 - 同步到 UI，保证界面显示与数据一致
        SyncUIFromConfig();
    }
}

FXBGameConfigData UXBConfigSaveWidget::GetConfigData() const
{
    // 🔧 修改 - 直接返回 ConfigData，便于蓝图读取当前配置
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

    // 🔧 修改 - 先从 GameInstance 拉取最新配置作为基准，避免保存旧数据
    ConfigData = GameInstance->GetGameConfig();

    // 🔧 修改 - 保存前同步 UI，保证写入当前控件值
    SyncConfigFromUI();

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
