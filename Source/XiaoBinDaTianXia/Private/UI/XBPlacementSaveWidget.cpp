// Copyright XiaoBing Project. All Rights Reserved.

#include "UI/XBPlacementSaveWidget.h"
#include "Config/XBActorPlacementComponent.h"
#include "Utils/XBLogCategories.h"

void UXBPlacementSaveWidget::SetPlacementComponent(
    UXBActorPlacementComponent *InComponent) {
  PlacementComponent = InComponent;

  if (PlacementComponent) {
    UE_LOG(LogXBConfig, Log, TEXT("[放置保存Widget] 放置组件已设置: %s"),
           *PlacementComponent->GetOwner()->GetName());
  } else {
    UE_LOG(LogXBConfig, Warning,
           TEXT("[放置保存Widget] 放置组件设置失败：传入的组件为空"));
  }
}

UXBActorPlacementComponent *
UXBPlacementSaveWidget::GetPlacementComponent() const {
  return PlacementComponent;
}

bool UXBPlacementSaveWidget::SavePlacementByName(const FString &SlotName) {
  return SavePlacementWithDisplayName(SlotName, SlotName);
}

bool UXBPlacementSaveWidget::SavePlacementWithDisplayName(
    const FString &SlotName, const FString &DisplayName) {
  UE_LOG(LogXBConfig, Log,
         TEXT("[放置保存Widget] 开始保存，槽位: %s，显示名: %s"), *SlotName,
         *DisplayName);

  // 先同步 UI 数据
  SyncConfigFromUI();

  if (!PlacementComponent) {
    UE_LOG(LogXBConfig, Error,
           TEXT("[放置保存Widget] 保存失败：放置组件未设置"));
    return false;
  }

  // 更新当前名称
  CurrentSlotName = SlotName;
  CurrentDisplayName = DisplayName;

  // 调用组件保存方法
  bool bSuccess = PlacementComponent->SavePlacementToSlot(SlotName);

  if (bSuccess) {
    UE_LOG(LogXBConfig, Log,
           TEXT("[放置保存Widget] 保存成功！槽位: %s，显示名: %s"), *SlotName,
           *DisplayName);
  } else {
    UE_LOG(LogXBConfig, Error, TEXT("[放置保存Widget] 保存失败！槽位: %s"),
           *SlotName);
  }

  return bSuccess;
}
