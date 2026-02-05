/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Config/XBHealthBarColorConfig.cpp

/**
 * @file XBHealthBarColorConfig.cpp
 * @brief 血条颜色配置数据资产实现
 *
 * @note ✨ 新增文件
 */

#include "Config/XBHealthBarColorConfig.h"

FLinearColor UXBHealthBarColorConfig::GetColorByIndex(int32 Index) const {
  if (Colors.IsValidIndex(Index)) {
    return Colors[Index].ColorValue;
  }
  // 索引无效时返回默认绿色
  return FLinearColor(0.0f, 0.8f, 0.2f, 1.0f);
}

bool UXBHealthBarColorConfig::GetColorByName(const FString& ColorName,
                                              FLinearColor& OutColor) const {
  for (const FXBHealthBarColorEntry& Entry : Colors) {
    if (Entry.ColorName.ToString() == ColorName) {
      OutColor = Entry.ColorValue;
      return true;
    }
  }
  return false;
}

TArray<FText> UXBHealthBarColorConfig::GetColorNames() const {
  TArray<FText> Names;
  for (const FXBHealthBarColorEntry& Entry : Colors) {
    Names.Add(Entry.ColorName);
  }
  return Names;
}

TArray<FString> UXBHealthBarColorConfig::GetColorNameStrings() const {
  TArray<FString> Names;
  for (const FXBHealthBarColorEntry& Entry : Colors) {
    Names.Add(Entry.ColorName.ToString());
  }
  return Names;
}
