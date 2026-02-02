/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/UI/XBBatchPlacementConfigWidget.cpp

/**
 * @file XBBatchPlacementConfigWidget.cpp
 * @brief 批量放置配置界面 Widget 实现
 *
 * @note ✨ 新增文件
 */

#include "UI/XBBatchPlacementConfigWidget.h"
#include "Character/XBConfigCameraPawn.h"
#include "Config/XBActorPlacementComponent.h"
#include "Config/XBPlacementConfigAsset.h"
#include "GameFramework/Pawn.h"
#include "Utils/XBLogCategories.h"

void UXBBatchPlacementConfigWidget::InitializeWithEntry(int32 InEntryIndex) {
  UE_LOG(LogXBConfig, Log, TEXT("[批量配置界面] InitializeWithEntry 被调用，索引: %d"),
         InEntryIndex);
  
  EntryIndex = InEntryIndex;
  
  // 从 PlacementConfig 获取默认值
  FIntPoint DefaultGridSize = FIntPoint(5, 5);
  float DefaultSpacing = 200.0f;
  
  // 自动绑定到放置组件并获取配置
  APlayerController* PC = GetOwningPlayer();
  if (PC) {
    APawn* Pawn = PC->GetPawn();
    if (Pawn) {
      AXBConfigCameraPawn* ConfigPawn = Cast<AXBConfigCameraPawn>(Pawn);
      if (ConfigPawn) {
        UXBActorPlacementComponent* PlacementComp = ConfigPawn->GetPlacementComponent();
        if (PlacementComp) {
          // 绑定事件
          PlacementComp->SetBatchConfigWidget(this);
          UE_LOG(LogXBConfig, Log, TEXT("[批量配置界面] 自动绑定到放置组件成功"));
          
          // 获取配置数据
          UXBPlacementConfigAsset* PlacementConfig = PlacementComp->GetPlacementConfig();
          if (PlacementConfig) {
            const FXBSpawnableActorEntry* Entry = PlacementConfig->GetEntryByIndexPtr(InEntryIndex);
            if (Entry) {
              DefaultGridSize = Entry->BatchGridSize;
              DefaultSpacing = Entry->BatchSpacing;
              UE_LOG(LogXBConfig, Log, TEXT("[批量配置界面] 从配置读取默认值: 网格 %dx%d, 间距 %.1f"),
                     DefaultGridSize.X, DefaultGridSize.Y, DefaultSpacing);
            }
          }
        } else {
          UE_LOG(LogXBConfig, Warning, TEXT("[批量配置界面] GetPlacementComponent 返回空"));
        }
      } else {
        UE_LOG(LogXBConfig, Warning, TEXT("[批量配置界面] Cast 到 AXBConfigCameraPawn 失败"));
      }
    } else {
      UE_LOG(LogXBConfig, Warning, TEXT("[批量配置界面] PC->GetPawn() 返回空"));
    }
  } else {
    UE_LOG(LogXBConfig, Warning, TEXT("[批量配置界面] GetOwningPlayer 返回空"));
  }
  
  // 使用默认值初始化配置数据
  ConfigData.GridSize = DefaultGridSize;
  ConfigData.Spacing = DefaultSpacing;
  
  // 同步到 UI
  SyncUIFromConfig();
  
  UE_LOG(LogXBConfig, Log, TEXT("[批量配置界面] 初始化完成，条目索引: %d, 网格: %dx%d, 间距: %.1f"),
         EntryIndex, ConfigData.GridSize.X, ConfigData.GridSize.Y, ConfigData.Spacing);
}
  
FXBBatchPlacementConfigData UXBBatchPlacementConfigWidget::GetConfigData() const {
  return ConfigData;
}

void UXBBatchPlacementConfigWidget::SetConfigData(
    const FXBBatchPlacementConfigData& InConfigData, bool bSyncToUI) {
  ConfigData = InConfigData;
  
  if (bSyncToUI) {
    SyncUIFromConfig();
  }
}

void UXBBatchPlacementConfigWidget::OnConfirmClicked() {
  // 从 UI 同步数据
  SyncConfigFromUI();
  
  UE_LOG(LogXBConfig, Log, TEXT("[批量配置界面] 确认配置，网格: %dx%d, 间距: %.1f"),
         ConfigData.GridSize.X, ConfigData.GridSize.Y, ConfigData.Spacing);
  
  // 广播确认事件
  OnConfigConfirmed.Broadcast(EntryIndex, ConfigData);
  
  // 移除界面
  RemoveFromParent();
}

void UXBBatchPlacementConfigWidget::OnCancelClicked() {
  UE_LOG(LogXBConfig, Log, TEXT("[批量配置界面] 取消配置"));
  
  // 广播取消事件
  OnConfigCancelled.Broadcast();
  
  // 移除界面
  RemoveFromParent();
}

void UXBBatchPlacementConfigWidget::NativeConstruct() {
  Super::NativeConstruct();
  
  // 缓存并设置光标可见
  if (APlayerController* PC = GetOwningPlayer()) {
    bOriginalShowCursor = PC->bShowMouseCursor;
    PC->bShowMouseCursor = true;
  }
}

void UXBBatchPlacementConfigWidget::NativeDestruct() {
  // 恢复光标状态
  if (APlayerController* PC = GetOwningPlayer()) {
    PC->bShowMouseCursor = bOriginalShowCursor;
  }
  
  Super::NativeDestruct();
}
