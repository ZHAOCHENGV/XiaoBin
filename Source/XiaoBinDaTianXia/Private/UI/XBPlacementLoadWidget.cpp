// Copyright XiaoBing Project. All Rights Reserved.

#include "UI/XBPlacementLoadWidget.h"
#include "Config/XBActorPlacementComponent.h"
#include "Utils/XBLogCategories.h"

void UXBPlacementLoadWidget::SetPlacementComponent(
    UXBActorPlacementComponent *InComponent) {
  PlacementComponent = InComponent;

  if (PlacementComponent) {
    UE_LOG(LogXBConfig, Log, TEXT("[放置读取Widget] 放置组件已设置: %s"),
           *PlacementComponent->GetOwner()->GetName());
  } else {
    UE_LOG(LogXBConfig, Warning,
           TEXT("[放置读取Widget] 放置组件设置失败：传入的组件为空"));
  }
}

UXBActorPlacementComponent *
UXBPlacementLoadWidget::GetPlacementComponent() const {
  return PlacementComponent;
}

bool UXBPlacementLoadWidget::RefreshSaveSlotNames() {
  UE_LOG(LogXBConfig, Log, TEXT("[放置读取Widget] 开始刷新存档列表..."));

  if (!PlacementComponent) {
    UE_LOG(LogXBConfig, Error,
           TEXT("[放置读取Widget] 刷新失败：放置组件未设置"));
    return false;
  }

  SlotNames = PlacementComponent->GetPlacementSaveSlotNames();

  // 清空详细信息列表
  SlotInfoList.Empty();

  // 为每个槽位创建详细信息（目前简化处理）
  for (const FString &SlotName : SlotNames) {
    FXBPlacementSlotInfo Info;
    Info.SlotName = SlotName;
    Info.DisplayName = SlotName;      // 默认显示名 = 槽位名
    Info.SaveTime = FDateTime::Now(); // TODO: 从存档读取实际时间
    Info.ActorCount = 0;              // TODO: 从存档读取实际数量
    SlotInfoList.Add(Info);
  }

  UE_LOG(LogXBConfig, Log, TEXT("[放置读取Widget] 刷新完成，共 %d 个存档槽位"),
         SlotNames.Num());

  // 通知蓝图刷新 UI
  OnSlotListRefreshed();

  return true;
}

bool UXBPlacementLoadWidget::LoadPlacementByName(const FString &SlotName) {
  UE_LOG(LogXBConfig, Log, TEXT("[放置读取Widget] 开始读取存档: %s"),
         *SlotName);

  if (!PlacementComponent) {
    UE_LOG(LogXBConfig, Error,
           TEXT("[放置读取Widget] 读取失败：放置组件未设置"));
    return false;
  }

  bool bSuccess = PlacementComponent->LoadPlacementFromSlot(SlotName);

  if (bSuccess) {
    UE_LOG(LogXBConfig, Log, TEXT("[放置读取Widget] 读取成功: %s"), *SlotName);
    SyncUIFromConfig();
  } else {
    UE_LOG(LogXBConfig, Error, TEXT("[放置读取Widget] 读取失败: %s"),
           *SlotName);
  }

  return bSuccess;
}

bool UXBPlacementLoadWidget::DeletePlacementByName(const FString &SlotName) {
  UE_LOG(LogXBConfig, Log, TEXT("[放置读取Widget] 开始删除存档: %s"),
         *SlotName);

  if (!PlacementComponent) {
    UE_LOG(LogXBConfig, Error,
           TEXT("[放置读取Widget] 删除失败：放置组件未设置"));
    return false;
  }

  bool bSuccess = PlacementComponent->DeletePlacementSave(SlotName);

  if (bSuccess) {
    UE_LOG(LogXBConfig, Log, TEXT("[放置读取Widget] 删除成功: %s"), *SlotName);
    // 刷新列表
    RefreshSaveSlotNames();
  } else {
    UE_LOG(LogXBConfig, Error, TEXT("[放置读取Widget] 删除失败: %s"),
           *SlotName);
  }

  return bSuccess;
}
