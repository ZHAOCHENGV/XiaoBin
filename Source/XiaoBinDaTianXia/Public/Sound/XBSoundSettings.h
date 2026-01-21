// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "XBSoundSettings.generated.h"

class UXBSoundDatabase;

/**
 * 音效系统设置（会出现在项目设置中）
 * 路径：Project Settings → Plugins → XiaoBin Sound Settings
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "小兵音效设置"))
class XIAOBINDATIANXIA_API UXBSoundSettings : public UDeveloperSettings {
  GENERATED_BODY()

public:
  UXBSoundSettings() {
    CategoryName = TEXT("Plugins");
    SectionName = TEXT("XiaoBin Sound Settings");
  }

  /**
   * 音效数据库
   * 在这里直接拖拽你的 DA_SoundDatabase 资源
   */
  UPROPERTY(
      Config, EditAnywhere, Category = "Sound Database",
      meta = (DisplayName = "音效数据库",
              AllowedClasses = "/Script/XiaoBinDaTianXia.XBSoundDatabase"))
  FSoftObjectPath SoundDatabasePath;

  /**
   * 获取设置单例
   */
  static const UXBSoundSettings *Get() {
    return GetDefault<UXBSoundSettings>();
  }
};
