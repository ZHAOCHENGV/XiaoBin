// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "XBSoundManagerSubsystem.generated.h"

class UXBSoundDatabase;
class UAudioComponent;

/**
 * 音效管理子系统（简化版原型）
 * 提供统一的音效播放接口
 */
UCLASS(Config = Game, meta = (DisplayName = "小兵系统音效管理器"))
class XIAOBINDATIANXIA_API UXBSoundManagerSubsystem
    : public UGameInstanceSubsystem {
  GENERATED_BODY()

public:
  virtual void Initialize(FSubsystemCollectionBase &Collection) override;

  /**
   * 播放 2D 音效（不受空间影响）
   * @param SoundTag 音效的 Gameplay Tag
   * @param VolumeMultiplier 额外的音量倍率（可选）
   * @param PitchMultiplier 额外的音调倍率（可选）
   * @return 音频组件（可用于控制音效）
   */
  UFUNCTION(BlueprintCallable, Category = "Sound",
            meta = (DisplayName = "播放2D音效"))
  UAudioComponent *PlaySound2D(FGameplayTag SoundTag,
                               float VolumeMultiplier = 1.0f,
                               float PitchMultiplier = 1.0f);

  /**
   * 播放 3D 音效（在指定位置）
   * @param SoundTag 音效的 Gameplay Tag
   * @param Location 播放位置
   * @param VolumeMultiplier 额外的音量倍率（可选）
   * @param PitchMultiplier 额外的音调倍率（可选）
   * @return 音频组件（可用于控制音效）
   */
  UFUNCTION(BlueprintCallable, Category = "Sound",
            meta = (DisplayName = "播放3D音效（位置）"))
  UAudioComponent *PlaySoundAtLocation(FGameplayTag SoundTag, FVector Location,
                                       float VolumeMultiplier = 1.0f,
                                       float PitchMultiplier = 1.0f);

  /**
   * 播放 3D 音效（附加到组件）
   * @param SoundTag 音效的 Gameplay Tag
   * @param AttachToComponent 要附加到的场景组件
   * @param SocketName 插槽名称（可选）
   * @param VolumeMultiplier 额外的音量倍率（可选）
   * @param PitchMultiplier 额外的音调倍率（可选）
   * @return 音频组件（可用于控制音效）
   */
  UFUNCTION(BlueprintCallable, Category = "Sound",
            meta = (DisplayName = "播放3D音效（附加）"))
  UAudioComponent *PlaySoundAttached(FGameplayTag SoundTag,
                                     USceneComponent *AttachToComponent,
                                     FName SocketName = NAME_None,
                                     float VolumeMultiplier = 1.0f,
                                     float PitchMultiplier = 1.0f);

protected:
  /** 音效数据库引用（可在蓝图中直接指定） */
  UPROPERTY(EditDefaultsOnly, Category = "Config",
            meta = (DisplayName = "音效数据库（直接引用）"))
  TObjectPtr<UXBSoundDatabase> SoundDatabase;

  /** 音效数据库路径（可在 DefaultEngine.ini 中配置） */
  UPROPERTY(Config, EditDefaultsOnly, Category = "Config",
            meta = (DisplayName = "音效数据库路径（配置文件）"))
  FSoftObjectPath SoundDatabasePath;
};
