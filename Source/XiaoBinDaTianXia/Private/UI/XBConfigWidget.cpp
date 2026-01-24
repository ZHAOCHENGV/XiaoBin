/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/UI/XBConfigWidget.cpp

/**
 * @file XBConfigWidget.cpp
 * @brief 配置界面 Widget 实现
 */

#include "UI/XBConfigWidget.h"
#include "Character/XBCharacterBase.h"
#include "Engine/DataTable.h"
#include "Game/XBGameInstance.h"
#include "Kismet/GameplayStatics.h"


void UXBConfigWidget::NativeConstruct() {
  Super::NativeConstruct();
  
}

void UXBConfigWidget::NativeDestruct() {
  // 恢复光标状态
  // 优先使用 GetOwningPlayer，如果没有则尝试获取第一个玩家控制器
  APlayerController *PC = GetOwningPlayer();
  if (!PC) {
    PC = UGameplayStatics::GetPlayerController(this, 0);
  }

  if (PC) {
    PC->bShowMouseCursor = true;
    PC->SetInputMode(FInputModeGameOnly());
  }

  Super::NativeDestruct();
}

void UXBConfigWidget::InitializeConfig(AXBCharacterBase *InLeader) {
  // 强制显示光标和设置输入模式
  if (APlayerController *PC = UGameplayStatics::GetPlayerController(this, 0)) {
    // 强制开启
    PC->bShowMouseCursor = true;

    // 设置焦点到当前 Widget
    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(TakeWidget());
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PC->SetInputMode(InputMode);
  }

  // 🔧 修改 - 缓存主将引用，便于应用时直接生效
  TargetLeader = InLeader;

  if (!bHasCachedInitialConfig) {
    // 🔧 修改 - 首次进入界面时缓存 UI 默认值，确保“重置默认”回到界面初始状态
    // InitialConfigData = ConfigData;
    SetConfigData(InitialConfigData);
    bHasCachedInitialConfig = true;
  }
}

bool UXBConfigWidget::RefreshConfigFromSave() {
  UXBGameInstance *GameInstance = GetGameInstance<UXBGameInstance>();
  if (!GameInstance) {
    UE_LOG(LogTemp, Warning, TEXT("配置界面刷新失败：GameInstance 为空"));
    return false;
  }

  // 🔧 修改 - 从 GameInstance 读取最新配置
  ConfigData = GameInstance->GetGameConfig();
  return true;
}

bool UXBConfigWidget::ApplyConfig(bool bSaveToDisk) {
  UXBGameInstance *GameInstance = GetGameInstance<UXBGameInstance>();
  if (!GameInstance) {
    UE_LOG(LogTemp, Warning, TEXT("配置界面应用失败：GameInstance 为空"));
    return false;
  }

  // 🔧 修改 - 应用前同步 UI，确保 ConfigData 使用当前控件值
  SyncConfigFromUI();

  // 🔧 修改 - 先写入配置，再应用到主将与士兵
  GameInstance->SetGameConfig(ConfigData, bSaveToDisk);

  if (TargetLeader.IsValid()) {
    GameInstance->ApplyGameConfigToLeader(TargetLeader.Get(), true);
  }

  return true;
}

bool UXBConfigWidget::StartGame(bool bSaveToDisk) {
  UXBGameInstance *GameInstance = GetGameInstance<UXBGameInstance>();
  if (!GameInstance) {
    UE_LOG(LogTemp, Warning, TEXT("开始游戏失败：GameInstance 为空"));
    return false;
  }

  // 🔧 修改 - 开始游戏前从 UI 同步最新值，确保使用当前控件配置
  SyncConfigFromUI();

  // 🔧 修改 - 先写入配置并应用到主将，确保进入地图前配置已生效
  GameInstance->SetGameConfig(ConfigData, bSaveToDisk);

  if (TargetLeader.IsValid()) {
    GameInstance->ApplyGameConfigToLeader(TargetLeader.Get(), true);
  }

  // 🔧 修改 - 使用配置中选定的地图开始游戏
  return GameInstance->LoadSelectedMap();
}

void UXBConfigWidget::SetConfigData(const FXBGameConfigData &NewConfig,
                                    bool bSyncToUI) {
  // 🔧 修改 - 直接覆盖当前配置数据，便于蓝图统一赋值
  ConfigData = NewConfig;

  if (bSyncToUI) {
    // 🔧 修改 - 同步到 UI，保证界面显示与数据一致
    SyncUIFromConfig();
  }
}

FXBGameConfigData UXBConfigWidget::GetConfigData() const {
  // 🔧 修改 - 直接返回 ConfigData，便于蓝图侧读取当前配置
  return ConfigData;
}

bool UXBConfigWidget::ResetToDefault(bool bSaveToDisk) {
  UXBGameInstance *GameInstance = GetGameInstance<UXBGameInstance>();
  if (!GameInstance) {
    UE_LOG(LogTemp, Warning, TEXT("配置界面重置失败：GameInstance 为空"));
    return false;
  }

  if (!bHasCachedInitialConfig) {
    UE_LOG(LogTemp, Warning, TEXT("配置界面重置失败：未缓存初始配置"));
    return false;
  }

  // 🔧 修改 - 使用 UI 初始默认值重置，不依赖 GameInstance 默认配置
  ConfigData = InitialConfigData;

  if (bSaveToDisk) {
    // 🔧 修改 - 将 UI 默认值同步写入存档，保证下次读取一致
    GameInstance->SetGameConfig(ConfigData, true);
  }

  // 🔧 修改 - 重置后同步 UI，保持显示一致
  SyncUIFromConfig();

  return true;
}

TArray<FName> UXBConfigWidget::GetLeaderRowNames() const {
  if (!LeaderConfigDataTable) {
    return TArray<FName>();
  }

  // 🔧 修改 - 从数据表拉取行名供 UI 下拉使用
  return LeaderConfigDataTable->GetRowNames();
}

TArray<FName> UXBConfigWidget::GetSoldierRowNames() const {
  if (!SoldierConfigDataTable) {
    return TArray<FName>();
  }

  // 🔧 修改 - 从数据表拉取行名供 UI 下拉使用
  return SoldierConfigDataTable->GetRowNames();
}
