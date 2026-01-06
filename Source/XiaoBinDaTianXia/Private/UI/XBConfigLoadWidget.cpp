// Copyright XiaoBing Project. All Rights Reserved.

#include "UI/XBConfigLoadWidget.h"
#include "Game/XBGameInstance.h"
#include "Save/XBSaveSubsystem.h"
#include "Utils/XBLogCategories.h"

bool UXBConfigLoadWidget::RefreshSaveSlotNames()
{
    UXBGameInstance* GameInstance = GetGameInstance<UXBGameInstance>();
    if (!GameInstance)
    {
        UE_LOG(LogXBConfig, Warning, TEXT("刷新存档列表失败：GameInstance 为空"));
        return false;
    }

    UXBSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UXBSaveSubsystem>();
    if (!SaveSubsystem)
    {
        UE_LOG(LogXBConfig, Warning, TEXT("刷新存档列表失败：SaveSubsystem 为空"));
        return false;
    }

    // 🔧 修改 - 通过存档子系统获取全部槽位名称
    SlotNames = SaveSubsystem->GetAllSaveSlotNames();
    return true;
}

bool UXBConfigLoadWidget::LoadConfigByName(const FString& SlotName, bool bSyncToUI)
{
    if (SlotName.IsEmpty())
    {
        UE_LOG(LogXBConfig, Warning, TEXT("读取配置失败：SlotName 为空"));
        return false;
    }

    UXBGameInstance* GameInstance = GetGameInstance<UXBGameInstance>();
    if (!GameInstance)
    {
        UE_LOG(LogXBConfig, Warning, TEXT("读取配置失败：GameInstance 为空"));
        return false;
    }

    UXBSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UXBSaveSubsystem>();
    if (!SaveSubsystem)
    {
        UE_LOG(LogXBConfig, Warning, TEXT("读取配置失败：SaveSubsystem 为空"));
        return false;
    }

    // 🔧 修改 - 使用存档子系统加载并同步到 GameInstance
    if (!SaveSubsystem->LoadGame(SlotName, 0))
    {
        UE_LOG(LogXBConfig, Warning, TEXT("读取配置失败：加载存档失败 %s"), *SlotName);
        return false;
    }

    GameInstance->SetCurrentSaveGame(SaveSubsystem->GetCurrentSaveGame());
    ConfigData = GameInstance->GetGameConfig();

    if (bSyncToUI)
    {
        SyncUIFromConfig();
    }

    return true;
}

bool UXBConfigLoadWidget::DeleteConfigByName(const FString& SlotName)
{
    if (SlotName.IsEmpty())
    {
        UE_LOG(LogXBConfig, Warning, TEXT("删除配置失败：SlotName 为空"));
        return false;
    }

    UXBGameInstance* GameInstance = GetGameInstance<UXBGameInstance>();
    if (!GameInstance)
    {
        UE_LOG(LogXBConfig, Warning, TEXT("删除配置失败：GameInstance 为空"));
        return false;
    }

    UXBSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UXBSaveSubsystem>();
    if (!SaveSubsystem)
    {
        UE_LOG(LogXBConfig, Warning, TEXT("删除配置失败：SaveSubsystem 为空"));
        return false;
    }

    // 🔧 修改 - 删除后刷新列表，保证 UI 一致
    const bool bDeleted = SaveSubsystem->DeleteSaveGame(SlotName, 0);
    RefreshSaveSlotNames();
    return bDeleted;
}

FXBGameConfigData UXBConfigLoadWidget::GetConfigData() const
{
    // 🔧 修改 - 直接返回 ConfigData，便于蓝图读取当前配置
    return ConfigData;
}
