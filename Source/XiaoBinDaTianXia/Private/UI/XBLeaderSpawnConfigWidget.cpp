/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/UI/XBLeaderSpawnConfigWidget.cpp

/**
 * @file XBLeaderSpawnConfigWidget.cpp
 * @brief 主将放置配置界面 Widget 实现
 */

#include "UI/XBLeaderSpawnConfigWidget.h"
#include "Engine/DataTable.h"
#include "Utils/XBLogCategories.h"
#include "Data/XBSoldierDataTable.h"

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

  // 🔧 调试日志 - 输出同步后的配置数据
  UE_LOG(LogXBConfig, Warning,
         TEXT("[主将配置界面] ===== OnConfirmClicked 调试开始 ====="));
  UE_LOG(LogXBConfig, Warning,
         TEXT("[主将配置界面] ConfigData.GameConfig.LeaderConfigRowName = %s"),
         *ConfigData.GameConfig.LeaderConfigRowName.ToString());
  UE_LOG(LogXBConfig, Warning,
         TEXT("[主将配置界面] ConfigData.SelectedSoldierType = %d"),
         static_cast<int32>(ConfigData.SelectedSoldierType));
  UE_LOG(LogXBConfig, Warning,
         TEXT("[主将配置界面] ConfigData.GameConfig.InitialSoldierRowName (解析前) = %s"),
         *ConfigData.GameConfig.InitialSoldierRowName.ToString());
  UE_LOG(LogXBConfig, Warning,
         TEXT("[主将配置界面] SoldierConfigDataTable 是否有效 = %s"),
         SoldierConfigDataTable ? TEXT("是") : TEXT("否"));

  // ✨ 新增 - 根据主将名称和士兵类型自动解析士兵行名
  const FName LeaderRowName = ConfigData.GameConfig.LeaderConfigRowName;
  const EXBSoldierType SoldierType = ConfigData.SelectedSoldierType;
  
  UE_LOG(LogXBConfig, Warning,
         TEXT("[主将配置界面] 准备解析: LeaderRowName=%s, SoldierType=%d"),
         *LeaderRowName.ToString(), static_cast<int32>(SoldierType));

  if (!LeaderRowName.IsNone() && SoldierType != EXBSoldierType::None) {
    FName ResolvedSoldierRowName = GetSoldierRowNameByType(LeaderRowName, SoldierType);
    UE_LOG(LogXBConfig, Warning,
           TEXT("[主将配置界面] GetSoldierRowNameByType 返回: %s"),
           *ResolvedSoldierRowName.ToString());
    if (!ResolvedSoldierRowName.IsNone()) {
      ConfigData.GameConfig.InitialSoldierRowName = ResolvedSoldierRowName;
      UE_LOG(LogXBConfig, Log,
             TEXT("[主将配置界面] 自动解析士兵行名: %s + 类型%d -> %s"),
             *LeaderRowName.ToString(), static_cast<int32>(SoldierType),
             *ResolvedSoldierRowName.ToString());
    } else {
      UE_LOG(LogXBConfig, Warning,
             TEXT("[主将配置界面] 无法解析士兵行名: %s + 类型%d"),
             *LeaderRowName.ToString(), static_cast<int32>(SoldierType));
    }
  } else {
    UE_LOG(LogXBConfig, Warning,
           TEXT("[主将配置界面] 跳过解析: LeaderRowName.IsNone()=%s, SoldierType==None=%s"),
           LeaderRowName.IsNone() ? TEXT("true") : TEXT("false"),
           SoldierType == EXBSoldierType::None ? TEXT("true") : TEXT("false"));
  }

  UE_LOG(LogXBConfig, Warning,
         TEXT("[主将配置界面] ConfigData.GameConfig.InitialSoldierRowName (解析后) = %s"),
         *ConfigData.GameConfig.InitialSoldierRowName.ToString());
  UE_LOG(LogXBConfig, Warning,
         TEXT("[主将配置界面] ===== OnConfirmClicked 调试结束 ====="));

  // 广播确认事件
  OnConfigConfirmed.Broadcast(EntryIndex, ConfigData);

  UE_LOG(
      LogXBConfig, Log,
      TEXT("[主将配置界面] 配置确认，条目索引: %d，主将行: %s，士兵行: %s，初始士兵数: %d"),
      EntryIndex, *ConfigData.GameConfig.LeaderConfigRowName.ToString(),
      *ConfigData.GameConfig.InitialSoldierRowName.ToString(),
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

TArray<EXBSoldierType> UXBLeaderSpawnConfigWidget::GetSoldierTypes() const {
  // 返回可用的士兵类型列表（不包含 None）
  TArray<EXBSoldierType> Types;
  Types.Add(EXBSoldierType::Infantry);
  Types.Add(EXBSoldierType::Archer);
  Types.Add(EXBSoldierType::Cavalry);
  return Types;
}

FName UXBLeaderSpawnConfigWidget::GetSoldierRowNameByType(
    FName LeaderRowName, EXBSoldierType SoldierType) const {
  UE_LOG(LogXBConfig, Warning,
         TEXT("[GetSoldierRowNameByType] 输入: LeaderRowName=%s, SoldierType=%d"),
         *LeaderRowName.ToString(), static_cast<int32>(SoldierType));

  // 校验参数
  if (LeaderRowName.IsNone() || SoldierType == EXBSoldierType::None) {
    UE_LOG(LogXBConfig, Warning,
           TEXT("[GetSoldierRowNameByType] 参数无效，返回 NAME_None"));
    return NAME_None;
  }

  if (!SoldierConfigDataTable) {
    UE_LOG(LogXBConfig, Warning,
           TEXT("[GetSoldierRowNameByType] 士兵数据表未配置，返回 NAME_None"));
    return NAME_None;
  }

  // 🔧 调试日志 - 列出数据表中所有行名
  TArray<FName> AllRowNames = SoldierConfigDataTable->GetRowNames();
  UE_LOG(LogXBConfig, Warning,
         TEXT("[GetSoldierRowNameByType] 士兵数据表共 %d 行:"), AllRowNames.Num());
  for (const FName& RowName : AllRowNames) {
    const FXBSoldierTableRow* Row =
        SoldierConfigDataTable->FindRow<FXBSoldierTableRow>(RowName, TEXT(""));
    if (Row) {
      UE_LOG(LogXBConfig, Warning,
             TEXT("  - 行名: %s, 类型: %d"), *RowName.ToString(), static_cast<int32>(Row->SoldierType));
    } else {
      UE_LOG(LogXBConfig, Warning,
             TEXT("  - 行名: %s, (无法读取行数据)"), *RowName.ToString());
    }
  }

  // 构造主将名称前缀（如：李世民_）
  const FString LeaderPrefix = LeaderRowName.ToString() + TEXT("_");
  UE_LOG(LogXBConfig, Warning,
         TEXT("[GetSoldierRowNameByType] 查找前缀: %s"), *LeaderPrefix);

  // 遍历士兵数据表，查找匹配的行
  for (const FName& RowName : AllRowNames) {
    // 检查行名是否以主将名称为前缀
    bool bStartsWith = RowName.ToString().StartsWith(LeaderPrefix);
    UE_LOG(LogXBConfig, Log,
           TEXT("  检查 %s: StartsWith(%s) = %s"),
           *RowName.ToString(), *LeaderPrefix, bStartsWith ? TEXT("true") : TEXT("false"));

    if (!bStartsWith) {
      continue;
    }

    // 获取行数据并检查士兵类型
    if (const FXBSoldierTableRow* Row =
            SoldierConfigDataTable->FindRow<FXBSoldierTableRow>(RowName, TEXT(""))) {
      UE_LOG(LogXBConfig, Warning,
             TEXT("  前缀匹配! 行名=%s, 行类型=%d, 目标类型=%d, 类型匹配=%s"),
             *RowName.ToString(), static_cast<int32>(Row->SoldierType),
             static_cast<int32>(SoldierType),
             Row->SoldierType == SoldierType ? TEXT("true") : TEXT("false"));
      if (Row->SoldierType == SoldierType) {
        UE_LOG(LogXBConfig, Warning,
               TEXT("[GetSoldierRowNameByType] 匹配成功! 返回: %s"),
               *RowName.ToString());
        return RowName;
      }
    }
  }

  UE_LOG(LogXBConfig, Warning,
         TEXT("[GetSoldierRowNameByType] 未找到匹配，返回 NAME_None"));
  return NAME_None;
}
