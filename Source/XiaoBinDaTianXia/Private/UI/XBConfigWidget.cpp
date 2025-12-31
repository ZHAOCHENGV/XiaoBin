/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/UI/XBConfigWidget.cpp

/**
 * @file XBConfigWidget.cpp
 * @brief 配置界面 Widget 实现
 */

#include "UI/XBConfigWidget.h"
#include "Game/XBGameInstance.h"
#include "Character/XBCharacterBase.h"
#include "Engine/DataTable.h"

void UXBConfigWidget::InitializeConfig(AXBCharacterBase* InLeader)
{
    // 🔧 修改 - 缓存主将引用，便于应用时直接生效
    TargetLeader = InLeader;

    // 🔧 修改 - 初始化时直接从存档刷新配置数据
    RefreshConfigFromSave();
}

bool UXBConfigWidget::RefreshConfigFromSave()
{
    UXBGameInstance* GameInstance = GetGameInstance<UXBGameInstance>();
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("配置界面刷新失败：GameInstance 为空"));
        return false;
    }

    // 🔧 修改 - 从 GameInstance 读取最新配置
    ConfigData = GameInstance->GetGameConfig();
    return true;
}

bool UXBConfigWidget::ApplyConfig(bool bSaveToDisk)
{
    UXBGameInstance* GameInstance = GetGameInstance<UXBGameInstance>();
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("配置界面应用失败：GameInstance 为空"));
        return false;
    }

    // 🔧 修改 - 先写入配置，再应用到主将与士兵
    GameInstance->SetGameConfig(ConfigData, bSaveToDisk);

    if (TargetLeader.IsValid())
    {
        GameInstance->ApplyGameConfigToLeader(TargetLeader.Get(), true);
    }

    return true;
}

bool UXBConfigWidget::SaveConfig()
{
    UXBGameInstance* GameInstance = GetGameInstance<UXBGameInstance>();
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("配置界面保存失败：GameInstance 为空"));
        return false;
    }

    // 🔧 修改 - 保存配置到存档
    GameInstance->SetGameConfig(ConfigData, true);
    return true;
}

bool UXBConfigWidget::LoadConfig()
{
    UXBGameInstance* GameInstance = GetGameInstance<UXBGameInstance>();
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("配置界面读取失败：GameInstance 为空"));
        return false;
    }

    // 🔧 修改 - 读取存档并刷新数据
    const bool bLoaded = GameInstance->LoadGameConfig(0);
    RefreshConfigFromSave();
    return bLoaded;
}

bool UXBConfigWidget::ResetToDefault(bool bSaveToDisk)
{
    UXBGameInstance* GameInstance = GetGameInstance<UXBGameInstance>();
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("配置界面重置失败：GameInstance 为空"));
        return false;
    }

    // 🔧 修改 - 重置配置后同步 UI 数据
    GameInstance->ResetGameConfigToDefault(bSaveToDisk);
    RefreshConfigFromSave();
    return true;
}

TArray<FName> UXBConfigWidget::GetLeaderRowNames() const
{
    TArray<FName> RowNames;
    if (!LeaderConfigDataTable)
    {
        return RowNames;
    }

    // 🔧 修改 - 从数据表拉取行名供 UI 下拉使用
    LeaderConfigDataTable->GetRowNames(RowNames);
    return RowNames;
}

TArray<FName> UXBConfigWidget::GetSoldierRowNames() const
{
    TArray<FName> RowNames;
    if (!SoldierConfigDataTable)
    {
        return RowNames;
    }

    // 🔧 修改 - 从数据表拉取行名供 UI 下拉使用
    SoldierConfigDataTable->GetRowNames(RowNames);
    return RowNames;
}
