/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/UI/XBLeaderSpawnConfigWidget.cpp

/**
 * @file XBLeaderSpawnConfigWidget.cpp
 * @brief 主将放置配置界面 Widget 实现
 */

#include "UI/XBLeaderSpawnConfigWidget.h"
#include "Engine/DataTable.h"
#include "Utils/XBLogCategories.h"

void UXBLeaderSpawnConfigWidget::NativeConstruct() {
  Super::NativeConstruct();

  // 缓存并显示光标
  if (APlayerController *PC = GetOwningPlayer()) {
    bOriginalShowCursor = PC->bShowMouseCursor;
    PC->bShowMouseCursor = true;
    PC->SetInputMode(FInputModeUIOnly().SetWidgetToFocus(TakeWidget()));

    UE_LOG(LogXBConfig, Log, TEXT("[主将配置界面] 已显示光标"));
  }
}

void UXBLeaderSpawnConfigWidget::NativeDestruct() {
  // 🔧 修复 - 恢复到 GameAndUI 模式，保持放置菜单可用
  // 原问题：配置界面关闭后恢复到 GameOnly 模式，导致放置菜单点击无响应
  if (APlayerController *PC = GetOwningPlayer()) {
    // 保持光标可见，允许继续放置操作
    PC->bShowMouseCursor = true;
    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PC->SetInputMode(InputMode);

    UE_LOG(LogXBConfig, Log, TEXT("[主将配置界面] 已恢复到放置模式"));
  }

  Super::NativeDestruct();
}

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
      EntryIndex, *ConfigData.GameConfig.LeaderConfigRowName.ToString(),
      ConfigData.GameConfig.InitialSoldierCount);

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
