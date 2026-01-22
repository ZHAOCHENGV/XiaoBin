// Copyright Epic Games, Inc. All Rights Reserved.

#include "Sound/XBSoundManagerSubsystem.h"

#include "AudioDevice.h"
#include "Components/AudioComponent.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/XBSoundDatabase.h"
#include "Sound/XBSoundSettings.h"
#include "Sound/XBSoundTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogXBSound, Log, All);

void UXBSoundManagerSubsystem::Initialize(
    FSubsystemCollectionBase &Collection) {
  Super::Initialize(Collection);

  UE_LOG(LogXBSound, Log, TEXT("[XBSoundManager] 开始初始化音效管理器..."));

  // 🔧 修改 - 优先使用数据表，其次才使用数据资产
  if (!SoundDataTable) {
    // 🔧 修改 - 优先从项目设置中读取数据表路径
    const UXBSoundSettings *Settings = UXBSoundSettings::Get();
    if (Settings && Settings->SoundDataTablePath.IsValid()) {
      UE_LOG(LogXBSound, Log,
             TEXT("[XBSoundManager] 从项目设置加载音效数据表：%s"),
             *Settings->SoundDataTablePath.ToString());
      SoundDataTable = Cast<UDataTable>(Settings->SoundDataTablePath.TryLoad());
    }

    // 🔧 修改 - 如果项目设置未配置，尝试使用配置文件中的路径
    if (!SoundDataTable && SoundDataTablePath.IsValid()) {
      UE_LOG(LogXBSound, Log,
             TEXT("[XBSoundManager] 从配置文件加载音效数据表：%s"),
             *SoundDataTablePath.ToString());
      SoundDataTable = Cast<UDataTable>(SoundDataTablePath.TryLoad());
    }

    // 🔧 修改 - 如果配置路径无效，尝试默认路径
    if (!SoundDataTable) {
      const FSoftObjectPath DefaultPath(
          TEXT("/Game/Data/DT_SoundDatabase.DT_SoundDatabase"));
      UE_LOG(LogXBSound, Log,
             TEXT("[XBSoundManager] 尝试从默认路径加载音效数据表：%s"),
             *DefaultPath.ToString());
      SoundDataTable = Cast<UDataTable>(DefaultPath.TryLoad());
    }
  }

  // 🔧 修改 - 如果未设置数据库，尝试加载数据资产
  if (!SoundDatabase) {
    // 🔧 修改 - 优先从项目设置中读取
    const UXBSoundSettings *Settings = UXBSoundSettings::Get();
    if (Settings && Settings->SoundDatabasePath.IsValid()) {
      UE_LOG(LogXBSound, Log,
             TEXT("[XBSoundManager] 从项目设置加载音效数据库：%s"),
             *Settings->SoundDatabasePath.ToString());
      SoundDatabase =
          Cast<UXBSoundDatabase>(Settings->SoundDatabasePath.TryLoad());
    }

    // 🔧 修改 - 如果项目设置未配置，尝试使用配置文件中的路径
    if (!SoundDatabase && SoundDatabasePath.IsValid()) {
      UE_LOG(LogXBSound, Log,
             TEXT("[XBSoundManager] 从配置文件加载音效数据库：%s"),
             *SoundDatabasePath.ToString());
      SoundDatabase = Cast<UXBSoundDatabase>(SoundDatabasePath.TryLoad());
    }

    // 🔧 修改 - 如果配置路径无效，尝试默认路径
    if (!SoundDatabase) {
      const FSoftObjectPath DefaultPath(
          TEXT("/Game/Data/DA_SoundDatabase.DA_SoundDatabase"));
      UE_LOG(LogXBSound, Log, TEXT("[XBSoundManager] 尝试从默认路径加载：%s"),
             *DefaultPath.ToString());
      SoundDatabase = Cast<UXBSoundDatabase>(DefaultPath.TryLoad());
    }
  }

  // 🔧 修改 - 如果两者都不存在，直接提示配置
  if (!SoundDataTable && !SoundDatabase) {
    UE_LOG(LogXBSound, Error,
           TEXT("[XBSoundManager] ❌ 音效数据未加载成功！"));
    UE_LOG(LogXBSound, Warning, TEXT("[XBSoundManager] 解决方法："));
    UE_LOG(LogXBSound, Warning,
           TEXT("  1. 在项目设置中配置：Project Settings → Plugins → XiaoBin "
                "Sound Settings"));
    UE_LOG(LogXBSound, Warning,
           TEXT("  2. 或在 /Game/Data/ 下创建 DT_SoundDatabase 或 "
                "DA_SoundDatabase"));
    return;
  }

  if (SoundDataTable) {
    UE_LOG(LogXBSound, Log,
           TEXT("[XBSoundManager] ✅ 已加载音效数据表，行数：%d"),
           SoundDataTable->GetRowNames().Num());
  }

  if (SoundDatabase) {
    UE_LOG(LogXBSound, Log,
           TEXT("[XBSoundManager] ✅ 已加载音效数据库，数量：%d"),
           SoundDatabase->SoundEntries.Num());
  }
}

bool UXBSoundManagerSubsystem::GetSoundEntryByTag(
    FGameplayTag SoundTag, FXBSoundEntry &OutEntry) const {
  // 🔧 修改 - 方案A：优先从数据表读取，RowName 需与 SoundTag 字符串一致
  if (SoundDataTable) {
    const FName RowName(*SoundTag.ToString());
    const FXBSoundEntry *Row = SoundDataTable->FindRow<FXBSoundEntry>(
        RowName, TEXT("XBSoundManager_GetSoundEntryByTag"));
    if (Row) {
      OutEntry = *Row;
      // 🔧 修改 - 如果数据表未填 SoundTag，则使用请求的 Tag 进行补全
      if (!OutEntry.SoundTag.IsValid()) {
        OutEntry.SoundTag = SoundTag;
      }
      return true;
    }

    // 🔧 修改 - RowName 未命中时，回退为遍历匹配 SoundTag 字段
    // 说明：允许行名与 Tag 不一致，但要求行内 SoundTag 正确配置
    const TArray<FName> RowNames = SoundDataTable->GetRowNames();
    for (const FName &FallbackRowName : RowNames) {
      const FXBSoundEntry *FallbackRow =
          SoundDataTable->FindRow<FXBSoundEntry>(
              FallbackRowName, TEXT("XBSoundManager_GetSoundEntryByTag"));
      if (!FallbackRow) {
        continue;
      }

      // 🔧 修改 - 使用 GameplayTag 精准匹配，避免字符串误差
      if (FallbackRow->SoundTag == SoundTag) {
        OutEntry = *FallbackRow;
        return true;
      }
    }

    UE_LOG(LogXBSound, Warning,
           TEXT("[XBSoundManager] 数据表未找到音效：RowName=%s，Tag=%s"),
           *RowName.ToString(), *SoundTag.ToString());
  }

  // 🔧 修改 - 兼容旧数据资产
  if (SoundDatabase) {
    return SoundDatabase->GetSoundEntry(SoundTag, OutEntry);
  }

  return false;
}

UAudioComponent *UXBSoundManagerSubsystem::PlaySound2D(FGameplayTag SoundTag,
                                                       float VolumeMultiplier,
                                                       float PitchMultiplier) {
  // 检查数据库
  if (!SoundDataTable && !SoundDatabase) {
    UE_LOG(LogXBSound, Error,
           TEXT("[XBSoundManager] PlaySound2D 失败：音效数据未设置"));
    return nullptr;
  }

  // 查找音效配置
  FXBSoundEntry Entry;
  if (!GetSoundEntryByTag(SoundTag, Entry)) {
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
    const UObject *WorldContextObject, FGameplayTag SoundTag, FVector Location,
    float VolumeMultiplier, float PitchMultiplier) {
  if (!SoundDataTable && !SoundDatabase) {
    UE_LOG(LogXBSound, Error,
           TEXT("[XBSoundManager] PlaySoundAtLocation 失败：音效数据未设置"));
    return nullptr;
  }

  FXBSoundEntry Entry;
  if (!GetSoundEntryByTag(SoundTag, Entry)) {
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

  // 🔧 修复 - 使用 SpawnSoundAtLocation 创建独立的 AudioComponent
  // 这样音效不会因为调用者（如发射物）销毁而中断
  UWorld *World =
      WorldContextObject ? WorldContextObject->GetWorld() : GetWorld();

  // 🔧 使用 SpawnSoundAtLocation 替代 PlaySoundAtLocation
  // SpawnSoundAtLocation 会创建一个独立的 AudioComponent，
  // 即使调用者（如 Projectile）被销毁，音效也会完整播放
  UAudioComponent *AudioComp = UGameplayStatics::SpawnSoundAtLocation(
      World, Entry.Sound, Location, FRotator::ZeroRotator, FinalVolume,
      FinalPitch,
      0.0f, // StartTime
      Entry.bEnableAttenuation ? Entry.Attenuation : nullptr, Entry.Concurrency,
      true // bAutoDestroy - 音效播放完毕后自动销毁组件
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
  if (!SoundDataTable && !SoundDatabase) {
    UE_LOG(LogXBSound, Error,
           TEXT("[XBSoundManager] PlaySoundAttached 失败：音效数据未设置"));
    return nullptr;
  }

  if (!AttachToComponent) {
    UE_LOG(LogXBSound, Error,
           TEXT("[XBSoundManager] PlaySoundAttached 失败：附加组件为空"));
    return nullptr;
  }

  FXBSoundEntry Entry;
  if (!GetSoundEntryByTag(SoundTag, Entry)) {
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
