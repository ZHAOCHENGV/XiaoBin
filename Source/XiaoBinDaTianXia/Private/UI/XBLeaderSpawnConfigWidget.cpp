/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/UI/XBLeaderSpawnConfigWidget.cpp

/**
 * @file XBLeaderSpawnConfigWidget.cpp
 * @brief 主将放置配置界面 Widget 实现
 */

#include "UI/XBLeaderSpawnConfigWidget.h"
#include "Engine/DataTable.h"
#include "Utils/XBLogCategories.h"

void UXBLeaderSpawnConfigWidget::InitializeWithEntry(int32 InEntryIndex) {
  EntryIndex = InEntryIndex;

  // 使用初始配置缓存初始化
  ConfigData = InitialConfigData;

  // 同步到 UI
  SyncUIFromConfig();

  UE_LOG(LogXBConfig, Log, TEXT("[主将配置界面] 初始化完成，条目索引: %d"),
         EntryIndex);
}

FXBLeaderSpawnConfigData UXBLeaderSpawnConfigWidget::GetConfigData() const {
  return ConfigData;
}

void UXBLeaderSpawnConfigWidget::SetConfigData(
    const FXBLeaderSpawnConfigData &InConfigData, bool bSyncToUI) {
  ConfigData = InConfigData;

  if (bSyncToUI) {
    SyncUIFromConfig();
  }
}

void UXBLeaderSpawnConfigWidget::OnConfirmClicked() {
  // 从 UI 同步最新值
  SyncConfigFromUI();

  // 广播确认事件
  OnConfigConfirmed.Broadcast(EntryIndex, ConfigData);

  UE_LOG(
      LogXBConfig, Log,
      TEXT("[主将配置界面] 配置确认，条目索引: %d，主将行: %s，初始士兵数: %d"),
      EntryIndex, *ConfigData.LeaderConfigRowName.ToString(),
      ConfigData.InitialSoldierCount);

  // 关闭界面
  RemoveFromParent();
}

void UXBLeaderSpawnConfigWidget::OnCancelClicked() {
  // 广播取消事件
  OnConfigCancelled.Broadcast();

  UE_LOG(LogXBConfig, Log, TEXT("[主将配置界面] 配置取消，条目索引: %d"),
         EntryIndex);

  // 关闭界面
  RemoveFromParent();
}

TArray<FName> UXBLeaderSpawnConfigWidget::GetLeaderRowNames() const {
  if (!LeaderConfigDataTable) {
    return TArray<FName>();
  }

  return LeaderConfigDataTable->GetRowNames();
}

TArray<FName> UXBLeaderSpawnConfigWidget::GetSoldierRowNames() const {
  if (!SoldierConfigDataTable) {
    return TArray<FName>();
  }

  return SoldierConfigDataTable->GetRowNames();
}
