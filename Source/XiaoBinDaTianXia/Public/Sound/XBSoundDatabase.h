// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Sound/XBSoundTypes.h"
#include "XBSoundDatabase.generated.h"

/**
 * 音效数据库
 * 使用 Gameplay Tag 映射音效配置
 * 在蓝图中编辑，每个音效可单独设置属性
 */
UCLASS(BlueprintType, meta = (DisplayName = "音效数据库"))
class XIAOBINDATIANXIA_API UXBSoundDatabase : public UPrimaryDataAsset {
  GENERATED_BODY()

public:
  /** 音效映射表（Tag → Sound Entry） */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound Database",
            meta = (DisplayName = "音效配置表"))
  TMap<FGameplayTag, FXBSoundEntry> SoundMap;

  /**
   * 根据 Tag 查找音效配置
   * @param SoundTag 音效的 Gameplay Tag
   * @param OutEntry 输出的音效配置
   * @return 是否找到
   */
  UFUNCTION(BlueprintCallable, Category = "Sound")
  bool GetSoundEntry(FGameplayTag SoundTag, FXBSoundEntry &OutEntry) const {
    if (const FXBSoundEntry *Entry = SoundMap.Find(SoundTag)) {
      OutEntry = *Entry;
      return true;
    }
    return false;
  }

  /**
   * 检查是否包含指定 Tag 的音效
   */
  UFUNCTION(BlueprintCallable, Category = "Sound")
  bool HasSound(FGameplayTag SoundTag) const {
    return SoundMap.Contains(SoundTag);
  }
};
