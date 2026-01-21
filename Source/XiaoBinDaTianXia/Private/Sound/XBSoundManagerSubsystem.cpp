// Copyright Epic Games, Inc. All Rights Reserved.

#include "Sound/XBSoundManagerSubsystem.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/XBSoundDatabase.h"
#include "Sound/XBSoundTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogXBSound, Log, All);

void UXBSoundManagerSubsystem::Initialize(
    FSubsystemCollectionBase &Collection) {
  Super::Initialize(Collection);

  // 如果未设置数据库，尝试加载
  if (!SoundDatabase) {
    // 优先使用配置文件中的路径
    if (SoundDatabasePath.IsValid()) {
      SoundDatabase = Cast<UXBSoundDatabase>(SoundDatabasePath.TryLoad());
    }

    // 如果配置路径无效，尝试默认路径
    if (!SoundDatabase) {
      const FSoftObjectPath DefaultPath(
          TEXT("/Game/Data/DA_SoundDatabase.DA_SoundDatabase"));
      SoundDatabase = Cast<UXBSoundDatabase>(DefaultPath.TryLoad());
    }

    if (!SoundDatabase) {
      UE_LOG(LogXBSound, Warning, TEXT("[XBSoundManager] 音效数据库未找到！"));
      UE_LOG(LogXBSound, Warning,
             TEXT("[XBSoundManager] 请在 /Game/Data/ 下创建 DA_SoundDatabase"));
      UE_LOG(LogXBSound, Warning,
             TEXT("[XBSoundManager] 或在 DefaultEngine.ini 添加配置："));
      UE_LOG(LogXBSound, Warning,
             TEXT("  [/Script/XiaoBinDaTianXia.XBSoundManagerSubsystem]"));
      UE_LOG(
          LogXBSound, Warning,
          TEXT("  SoundDatabasePath=/Game/YourPath/YourDatabase.YourDatabase"));
      return;
    }
  }

  UE_LOG(LogXBSound, Log,
         TEXT("[XBSoundManager] 音效管理器初始化成功，已加载 %d 个音效"),
         SoundDatabase->SoundMap.Num());
}

UAudioComponent *UXBSoundManagerSubsystem::PlaySound2D(FGameplayTag SoundTag,
                                                       float VolumeMultiplier,
                                                       float PitchMultiplier) {
  // 检查数据库
  if (!SoundDatabase) {
    UE_LOG(LogXBSound, Error,
           TEXT("[XBSoundManager] PlaySound2D 失败：音效数据库未设置"));
    return nullptr;
  }

  // 查找音效配置
  FXBSoundEntry Entry;
  if (!SoundDatabase->GetSoundEntry(SoundTag, Entry)) {
    UE_LOG(LogXBSound, Warning, TEXT("[XBSoundManager] 未找到音效：%s"),
           *SoundTag.ToString());
    return nullptr;
  }

  // 检查音效资源
  if (!Entry.Sound) {
    UE_LOG(LogXBSound, Warning, TEXT("[XBSoundManager] 音效 %s 的资源为空"),
           *SoundTag.ToString());
    return nullptr;
  }

  // 计算最终音量和音调
  const float FinalVolume = Entry.Volume * VolumeMultiplier;
  const float FinalPitch = Entry.Pitch * PitchMultiplier;

  // 播放音效
  UAudioComponent *AudioComp = UGameplayStatics::CreateSound2D(
      GetWorld(), Entry.Sound, FinalVolume, FinalPitch,
      0.0f, // StartTime
      Entry.Concurrency,
      false, // bPersist
      true   // bAutoDestroy
  );

  if (AudioComp) {
    UE_LOG(LogXBSound, Verbose,
           TEXT("[XBSoundManager] 播放2D音效：%s (Volume: %.2f, Pitch: %.2f)"),
           *SoundTag.ToString(), FinalVolume, FinalPitch);
  }

  return AudioComp;
}

UAudioComponent *UXBSoundManagerSubsystem::PlaySoundAtLocation(
    FGameplayTag SoundTag, FVector Location, float VolumeMultiplier,
    float PitchMultiplier) {
  if (!SoundDatabase) {
    UE_LOG(LogXBSound, Error,
           TEXT("[XBSoundManager] PlaySoundAtLocation 失败：音效数据库未设置"));
    return nullptr;
  }

  FXBSoundEntry Entry;
  if (!SoundDatabase->GetSoundEntry(SoundTag, Entry)) {
    UE_LOG(LogXBSound, Warning, TEXT("[XBSoundManager] 未找到音效：%s"),
           *SoundTag.ToString());
    return nullptr;
  }

  if (!Entry.Sound) {
    UE_LOG(LogXBSound, Warning, TEXT("[XBSoundManager] 音效 %s 的资源为空"),
           *SoundTag.ToString());
    return nullptr;
  }

  const float FinalVolume = Entry.Volume * VolumeMultiplier;
  const float FinalPitch = Entry.Pitch * PitchMultiplier;

  // 播放3D音效
  UAudioComponent *AudioComp = UGameplayStatics::SpawnSoundAtLocation(
      GetWorld(), Entry.Sound, Location, FRotator::ZeroRotator, FinalVolume,
      FinalPitch,
      0.0f, // StartTime
      Entry.bEnableAttenuation ? Entry.Attenuation : nullptr, Entry.Concurrency,
      true // bAutoDestroy
  );

  if (AudioComp) {
    UE_LOG(LogXBSound, Verbose,
           TEXT("[XBSoundManager] 播放3D音效：%s at (%.1f, %.1f, %.1f)"),
           *SoundTag.ToString(), Location.X, Location.Y, Location.Z);
  }

  return AudioComp;
}

UAudioComponent *UXBSoundManagerSubsystem::PlaySoundAttached(
    FGameplayTag SoundTag, USceneComponent *AttachToComponent, FName SocketName,
    float VolumeMultiplier, float PitchMultiplier) {
  if (!SoundDatabase) {
    UE_LOG(LogXBSound, Error,
           TEXT("[XBSoundManager] PlaySoundAttached 失败：音效数据库未设置"));
    return nullptr;
  }

  if (!AttachToComponent) {
    UE_LOG(LogXBSound, Error,
           TEXT("[XBSoundManager] PlaySoundAttached 失败：附加组件为空"));
    return nullptr;
  }

  FXBSoundEntry Entry;
  if (!SoundDatabase->GetSoundEntry(SoundTag, Entry)) {
    UE_LOG(LogXBSound, Warning, TEXT("[XBSoundManager] 未找到音效：%s"),
           *SoundTag.ToString());
    return nullptr;
  }

  if (!Entry.Sound) {
    UE_LOG(LogXBSound, Warning, TEXT("[XBSoundManager] 音效 %s 的资源为空"),
           *SoundTag.ToString());
    return nullptr;
  }

  const float FinalVolume = Entry.Volume * VolumeMultiplier;
  const float FinalPitch = Entry.Pitch * PitchMultiplier;

  // 播放附加音效
  UAudioComponent *AudioComp = UGameplayStatics::SpawnSoundAttached(
      Entry.Sound, AttachToComponent, SocketName, FVector::ZeroVector,
      FRotator::ZeroRotator, EAttachLocation::SnapToTarget,
      false, // bStopWhenAttachedToDestroyed
      FinalVolume, FinalPitch,
      0.0f, // StartTime
      Entry.bEnableAttenuation ? Entry.Attenuation : nullptr, Entry.Concurrency,
      true // bAutoDestroy
  );

  if (AudioComp) {
    const FString OwnerName = AttachToComponent->GetOwner()
                                  ? AttachToComponent->GetOwner()->GetName()
                                  : TEXT("None");
    UE_LOG(LogXBSound, Verbose,
           TEXT("[XBSoundManager] 播放附加音效：%s on %s (Socket: %s)"),
           *SoundTag.ToString(), *OwnerName, *SocketName.ToString());
  }

  return AudioComp;
}
