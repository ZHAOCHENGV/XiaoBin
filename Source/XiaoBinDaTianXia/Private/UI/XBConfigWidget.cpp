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
#include "Data/XBSoldierDataTable.h"


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

  // 🔧 调试日志 - 输出同步后的配置数据
  UE_LOG(LogTemp, Warning,
         TEXT("[XBConfigWidget::ApplyConfig] ===== 调试开始 ====="));
  UE_LOG(LogTemp, Warning,
         TEXT("[XBConfigWidget] ConfigData.LeaderConfigRowName = %s"),
         *ConfigData.LeaderConfigRowName.ToString());
  UE_LOG(LogTemp, Warning,
         TEXT("[XBConfigWidget] ConfigData.SelectedSoldierType = %d"),
         static_cast<int32>(ConfigData.SelectedSoldierType));
  UE_LOG(LogTemp, Warning,
         TEXT("[XBConfigWidget] ConfigData.InitialSoldierRowName (解析前) = %s"),
         *ConfigData.InitialSoldierRowName.ToString());
  UE_LOG(LogTemp, Warning,
         TEXT("[XBConfigWidget] SoldierConfigDataTable 是否有效 = %s"),
         SoldierConfigDataTable ? TEXT("是") : TEXT("否"));

  // ✨ 新增 - 根据主将名称和士兵类型自动解析士兵行名
  const FName LeaderRowName = ConfigData.LeaderConfigRowName;
  const EXBSoldierType SoldierType = ConfigData.SelectedSoldierType;

  UE_LOG(LogTemp, Warning,
         TEXT("[XBConfigWidget] 准备解析: LeaderRowName=%s, SoldierType=%d"),
         *LeaderRowName.ToString(), static_cast<int32>(SoldierType));

  if (!LeaderRowName.IsNone() && SoldierType != EXBSoldierType::None) {
    FName ResolvedSoldierRowName = GetSoldierRowNameByType(LeaderRowName, SoldierType);
    UE_LOG(LogTemp, Warning,
           TEXT("[XBConfigWidget] GetSoldierRowNameByType 返回: %s"),
           *ResolvedSoldierRowName.ToString());
    if (!ResolvedSoldierRowName.IsNone()) {
      ConfigData.InitialSoldierRowName = ResolvedSoldierRowName;
      UE_LOG(LogTemp, Log,
             TEXT("[XBConfigWidget] 自动解析士兵行名: %s + 类型%d -> %s"),
             *LeaderRowName.ToString(), static_cast<int32>(SoldierType),
             *ResolvedSoldierRowName.ToString());
    } else {
      UE_LOG(LogTemp, Warning,
             TEXT("[XBConfigWidget] 无法解析士兵行名"));
    }
  } else {
    UE_LOG(LogTemp, Warning,
           TEXT("[XBConfigWidget] 跳过解析: LeaderRowName.IsNone()=%s, SoldierType==None=%s"),
           LeaderRowName.IsNone() ? TEXT("true") : TEXT("false"),
           SoldierType == EXBSoldierType::None ? TEXT("true") : TEXT("false"));
  }

  UE_LOG(LogTemp, Warning,
         TEXT("[XBConfigWidget] ConfigData.InitialSoldierRowName (解析后) = %s"),
         *ConfigData.InitialSoldierRowName.ToString());
  UE_LOG(LogTemp, Warning,
         TEXT("[XBConfigWidget::ApplyConfig] ===== 调试结束 ====="));

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

  // 🔧 调试日志 - 输出同步后的配置数据
  UE_LOG(LogTemp, Warning,
         TEXT("[XBConfigWidget::StartGame] ===== 调试开始 ====="));
  UE_LOG(LogTemp, Warning,
         TEXT("[XBConfigWidget] ConfigData.LeaderConfigRowName = %s"),
         *ConfigData.LeaderConfigRowName.ToString());
  UE_LOG(LogTemp, Warning,
         TEXT("[XBConfigWidget] ConfigData.SelectedSoldierType = %d"),
         static_cast<int32>(ConfigData.SelectedSoldierType));
  UE_LOG(LogTemp, Warning,
         TEXT("[XBConfigWidget] SoldierConfigDataTable 是否有效 = %s"),
         SoldierConfigDataTable ? TEXT("是") : TEXT("否"));

  // ✨ 新增 - 根据主将名称和士兵类型自动解析士兵行名
  const FName LeaderRowName = ConfigData.LeaderConfigRowName;
  const EXBSoldierType SoldierType = ConfigData.SelectedSoldierType;

  if (!LeaderRowName.IsNone() && SoldierType != EXBSoldierType::None) {
    FName ResolvedSoldierRowName = GetSoldierRowNameByType(LeaderRowName, SoldierType);
    UE_LOG(LogTemp, Warning,
           TEXT("[XBConfigWidget] GetSoldierRowNameByType 返回: %s"),
           *ResolvedSoldierRowName.ToString());
    if (!ResolvedSoldierRowName.IsNone()) {
      ConfigData.InitialSoldierRowName = ResolvedSoldierRowName;
      UE_LOG(LogTemp, Log,
             TEXT("[XBConfigWidget] 开始游戏自动解析士兵行名: %s + 类型%d -> %s"),
             *LeaderRowName.ToString(), static_cast<int32>(SoldierType),
             *ResolvedSoldierRowName.ToString());
    } else {
      UE_LOG(LogTemp, Warning,
             TEXT("[XBConfigWidget] 无法解析士兵行名"));
    }
  } else {
    UE_LOG(LogTemp, Warning,
           TEXT("[XBConfigWidget] 跳过解析: LeaderRowName=%s, SoldierType=%d"),
           *LeaderRowName.ToString(), static_cast<int32>(SoldierType));
  }

  UE_LOG(LogTemp, Warning,
         TEXT("[XBConfigWidget] ConfigData.InitialSoldierRowName (解析后) = %s"),
         *ConfigData.InitialSoldierRowName.ToString());
  UE_LOG(LogTemp, Warning,
         TEXT("[XBConfigWidget::StartGame] ===== 调试结束 ====="));

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

TArray<EXBSoldierType> UXBConfigWidget::GetSoldierTypes() const {
  // 返回可用的士兵类型列表（不包含 None）
  TArray<EXBSoldierType> Types;
  Types.Add(EXBSoldierType::Infantry);
  Types.Add(EXBSoldierType::Archer);
  Types.Add(EXBSoldierType::Cavalry);
  return Types;
}

FName UXBConfigWidget::GetSoldierRowNameByType(FName LeaderRowName,
                                               EXBSoldierType SoldierType) const {
  UE_LOG(LogTemp, Warning,
         TEXT("[XBConfigWidget::GetSoldierRowNameByType] 输入: LeaderRowName=%s, SoldierType=%d"),
         *LeaderRowName.ToString(), static_cast<int32>(SoldierType));

  // 校验参数
  if (LeaderRowName.IsNone() || SoldierType == EXBSoldierType::None) {
    UE_LOG(LogTemp, Warning,
           TEXT("[XBConfigWidget::GetSoldierRowNameByType] 参数无效，返回 NAME_None"));
    return NAME_None;
  }

  if (!SoldierConfigDataTable) {
    UE_LOG(LogTemp, Warning,
           TEXT("[XBConfigWidget::GetSoldierRowNameByType] 士兵数据表未配置，返回 NAME_None"));
    return NAME_None;
  }

  // 🔧 调试日志 - 列出数据表中所有行名
  TArray<FName> AllRowNames = SoldierConfigDataTable->GetRowNames();
  UE_LOG(LogTemp, Warning,
         TEXT("[XBConfigWidget] 士兵数据表共 %d 行:"), AllRowNames.Num());
  for (const FName& RowName : AllRowNames) {
    const FXBSoldierTableRow* Row =
        SoldierConfigDataTable->FindRow<FXBSoldierTableRow>(RowName, TEXT(""));
    if (Row) {
      UE_LOG(LogTemp, Warning,
             TEXT("  - 行名: %s, 类型: %d"), *RowName.ToString(), static_cast<int32>(Row->SoldierType));
    }
  }

  // 构造主将名称前缀（如：李世民_）
  const FString LeaderPrefix = LeaderRowName.ToString() + TEXT("_");
  UE_LOG(LogTemp, Warning,
         TEXT("[XBConfigWidget] 查找前缀: %s"), *LeaderPrefix);

  // 遍历士兵数据表，查找匹配的行
  for (const FName& RowName : AllRowNames) {
    // 检查行名是否以主将名称为前缀
    bool bStartsWith = RowName.ToString().StartsWith(LeaderPrefix);

    if (!bStartsWith) {
      continue;
    }

    // 获取行数据并检查士兵类型
    if (const FXBSoldierTableRow* Row =
            SoldierConfigDataTable->FindRow<FXBSoldierTableRow>(RowName, TEXT(""))) {
      UE_LOG(LogTemp, Warning,
             TEXT("  前缀匹配! 行名=%s, 行类型=%d, 目标类型=%d"),
             *RowName.ToString(), static_cast<int32>(Row->SoldierType),
             static_cast<int32>(SoldierType));
      if (Row->SoldierType == SoldierType) {
        UE_LOG(LogTemp, Warning,
               TEXT("[XBConfigWidget::GetSoldierRowNameByType] 匹配成功! 返回: %s"),
               *RowName.ToString());
        return RowName;
      }
    }
  }

  UE_LOG(LogTemp, Warning,
         TEXT("[XBConfigWidget::GetSoldierRowNameByType] 未找到匹配，返回 NAME_None"));
  return NAME_None;
}
