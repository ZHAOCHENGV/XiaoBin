// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundConcurrency.h"
#include "XBSoundTypes.generated.h"

/**
 * 音效配置条目
 * 每个音效可以单独配置其属性
 *
 * @note TitleProperty 让 SoundName 字段显示为数组行的折叠标题
 * @note 方案A：此结构可直接作为 DataTable 的行结构使用
 */
USTRUCT(BlueprintType, meta = (TitleProperty = "SoundName"))
struct FXBSoundEntry : public FTableRowBase {
  GENERATED_BODY()

  /** 音效标签（用于查找和引用） */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound",
            meta = (DisplayName = "音效标签", Categories = "Sound"))
  FGameplayTag SoundTag;

  /** 音效名称（用于调试和识别，同时作为编辑器显示标题） */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound",
            meta = (DisplayName = "音效名称"))
  FString SoundName;

  /** 音效资源（支持 MetaSound 或普通 Sound） */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound",
            meta = (DisplayName = "音效资源"))
  TObjectPtr<USoundBase> Sound = nullptr;

  /** 音量（0.0 - 1.0） */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound",
            meta = (DisplayName = "音量", ClampMin = "0.0", ClampMax = "1.0"))
  float Volume = 1.0f;

  /** 音调（0.5 - 2.0，1.0 为正常） */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound",
            meta = (DisplayName = "音调", ClampMin = "0.5", ClampMax = "2.0"))
  float Pitch = 1.0f;

  /** 是否启用衰减（3D 音效需要） */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound|3D",
            meta = (DisplayName = "启用衰减"))
  bool bEnableAttenuation = true;

  /** 衰减设置（控制音效随距离衰减） */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound|3D",
            meta = (DisplayName = "衰减设置",
                    EditCondition = "bEnableAttenuation"))
  TObjectPtr<USoundAttenuation> Attenuation = nullptr;

  /** 并发限制（防止同一音效同时播放过多） */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound|Advanced",
            meta = (DisplayName = "并发限制"))
  TObjectPtr<USoundConcurrency> Concurrency = nullptr;

  /** 是否循环播放 */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound|Advanced",
            meta = (DisplayName = "循环播放"))
  bool bLooping = false;
};
