/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Config/XBActorPlacementComponent.cpp

/**
 * @file XBActorPlacementComponent.cpp
 * @brief 配置阶段 Actor 放置管理组件实现
 *
 * @note ✨ 新增文件
 */

#include "Config/XBActorPlacementComponent.h"
#include "Character/Components/XBMagnetFieldComponent.h"
#include "Character/XBCharacterBase.h"
#include "Character/XBDummyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Config/XBLeaderSpawnConfigData.h"
#include "Config/XBPlacementConfigAsset.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Save/XBSaveGame.h"
#include "UI/XBBatchPlacementConfigWidget.h"
#include "UI/XBLeaderSpawnConfigWidget.h"
#include "UI/XBWorldHealthBarComponent.h"
#include "Utils/XBLogCategories.h"
#include "XBCollisionChannels.h"
#include "Environment/XBBushVolume.h"

/**
 * @brief 构造函数
 * @note  初始化 Tick 设置：允许 Tick 但默认禁用
 *        实际启用时机在 BeginPlay 中根据状态决定
 */
UXBActorPlacementComponent::UXBActorPlacementComponent() {
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = false;
}

/**
 * @brief 组件开始运行时调用
 * @note  详细流程:
 *        1. 缓存玩家控制器引用（避免每帧查询）
 *        2. 初始化为 Idle 状态
 *        3. 启用 Tick 以支持悬停检测
 *        性能注意: 玩家控制器仅缓存一次，后续使用弱引用检查有效性
 */
void UXBActorPlacementComponent::BeginPlay() {
  Super::BeginPlay();

  // 缓存玩家控制器（避免每帧查询 GetPlayerController）
  CachedPlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

  // 初始化为 Idle 状态，启用 Tick 以支持悬停检测
  CurrentState = EXBPlacementState::Idle;
  SetComponentTickEnabled(true);

  UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 初始化完成，PlacementConfig: %s"),
         PlacementConfig ? *PlacementConfig->GetName() : TEXT("None"));
}

/**
 * @brief 组件结束运行时调用
 * @param EndPlayReason 结束原因枚举
 * @note  清理预览 Actor 防止内存泄漏
 */
void UXBActorPlacementComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason) {
  DestroyPreviewActor();
  Super::EndPlay(EndPlayReason);
}

void UXBActorPlacementComponent::TickComponent(
    float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  // 预览状态时更新预览位置
  if (CurrentState == EXBPlacementState::Previewing) {
    UpdatePreviewLocation();
  }

  // 空闲状态时更新悬停状态（用于高亮显示）
  if (CurrentState == EXBPlacementState::Idle) {
    UpdateHoverState();
  }

  // ✨ 调试 - 每2秒输出一次当前状态
#if WITH_EDITOR
  static float DebugTimer = 0.0f;
  DebugTimer += DeltaTime;
  if (DebugTimer >= 2.0f) {
    DebugTimer = 0.0f;
    const TCHAR *StateStr =
        CurrentState == EXBPlacementState::Idle         ? TEXT("Idle")
        : CurrentState == EXBPlacementState::Previewing ? TEXT("Previewing")
        : CurrentState == EXBPlacementState::Editing    ? TEXT("Editing")
                                                        : TEXT("Unknown");
    UE_LOG(LogXBConfig, Log,
           TEXT("[放置组件调试] 当前状态: %s, PlacedActors数量: %d"), StateStr,
           PlacedActors.Num());
  }
#endif
}

/**
 * @brief 处理鼠标左键点击输入
 * @return 是否成功处理点击
 * @note  详细流程（状态机模式）:
 *        - Idle 状态:
 *          1. 检测是否点击已放置 Actor -> 进入 Editing 状态
 *          2. 否则广播 OnRequestShowMenu 事件 -> UI 显示放置菜单
 *        - Previewing 状态:
 *          1. 位置有效 -> 调用 ConfirmPlacement 确认放置
 *        - Editing 状态:
 *          1. 点击其他 Actor -> 切换选中
 *          2. 点击空白 -> 取消选中，回到 Idle
 */
bool UXBActorPlacementComponent::HandleClick() {
  switch (CurrentState) {
  case EXBPlacementState::Idle: {
    /*// 空闲状态：检测是否点击已放置 Actor 或请求显示菜单
    AActor *HitActor = nullptr;
    if (GetHitPlacedActor(HitActor)) {
      SelectActor(HitActor);
      return true;
    }*/

    // 未点击到已放置 Actor，广播显示菜单事件
    FVector HitLocation;
    FVector HitNormal;
    if (GetMouseHitLocation(HitLocation, HitNormal)) {
      LastClickLocation = HitLocation;
      OnRequestShowMenu.Broadcast(HitLocation);
      return true;
    }
    return false;
  }

  case EXBPlacementState::Previewing: {
    // 预览状态：确认放置
    if (bIsPreviewLocationValid) {
      ConfirmPlacement();
      return true;
    }
    return false;
  }

  case EXBPlacementState::Editing: {
    // 编辑状态：检测是否点击其他 Actor 或取消选中
    AActor *HitActor = nullptr;
    if (GetHitPlacedActor(HitActor)) {
      if (HitActor != SelectedActor.Get()) {
        SelectActor(HitActor);
      }
      return true;
    }

    // 点击空白区域，取消选中
    DeselectActor();
    return true;
  }

  default:
    return false;
  }
}

/**
 * @brief 开始预览指定索引的 Actor
 * @param EntryIndex 配置条目索引（对应 PlacementConfig->SpawnableActors 数组）
 * @return 是否成功开始预览
 * @note  详细流程:
 *        1. 检查是否处于 Editing 状态 -> 忽略请求（防止意外触发）
 *        2. 检查是否需要配置面板（bRequiresConfig）:
 *           - 是 -> 设置 Idle 状态，广播 OnRequestShowConfigPanel 事件
 *           - 否 -> 创建预览 Actor，进入 Previewing 状态
 *        3. 根据旋转模式设置初始旋转（Manual/FacePlayer/Random）
 *        注意事项: 对于需要配置的 Actor，此函数不会创建预览 Actor，
 *                  而是等待用户在配置界面确认后由 HandleLeaderConfigConfirmed
 * 创建
 */
bool UXBActorPlacementComponent::StartPreview(int32 EntryIndex) {
  UE_LOG(LogXBConfig, Log,
         TEXT("[放置组件] 📍 StartPreview 被调用，索引: %d，当前状态: %d"),
         EntryIndex, static_cast<int32>(CurrentState));

  // 🔧 修复 - 如果当前处于 Editing 状态（用户正在编辑已选中的 Actor），
  // 不执行 StartPreview，避免意外弹出配置界面
  if (CurrentState == EXBPlacementState::Editing) {
    UE_LOG(LogXBConfig, Log,
           TEXT("[放置组件] 当前处于编辑状态，忽略 StartPreview 请求"));
    return false;
  }

  if (!PlacementConfig) {
    UE_LOG(LogXBConfig, Warning, TEXT("[放置组件] 未配置 PlacementConfig"));
    return false;
  }

  const FXBSpawnableActorEntry *Entry =
      PlacementConfig->GetEntryByIndexPtr(EntryIndex);
  if (!Entry || !Entry->ActorClass) {
    UE_LOG(LogXBConfig, Warning, TEXT("[放置组件] 无效的条目索引: %d"),
           EntryIndex);
    return false;
  }

  // ✨ 新增 - 检测是否需要放置前配置
  // 如果需要配置，则不创建预览，直接弹出配置界面
  // 🔧 修复 - 如果是批量放置且已有配置数据，则跳过配置界面直接创建预览
  if (Entry->bRequiresConfig)
    {
    // 检查是否已有批量配置数据（连续放置模式）
    if (Entry->bBatchPlacement && bHasPendingBatchConfig)
      {
      UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已有批量配置数据，跳过配置界面"));
      // 不弹出配置界面，继续创建预览
      }
    else
      {
      // 缓存待配置状态
      PendingConfigEntryIndex = EntryIndex;
      // 位置在配置确认后再获取（用户点击位置）
      PendingConfigLocation = FVector::ZeroVector;
      PendingConfigRotation = Entry->DefaultRotation;

      // ✨ 先清理悬停和选中状态，避免配置界面显示时光标仍在选中 Actor
      // 必须在 SetPlacementState 之前调用，确保材质正确恢复
      if (HoveredActor.IsValid()) {
        ApplyHoverMaterial(HoveredActor.Get(), false);
        HoveredActor.Reset();
      }
      if (SelectedActor.IsValid()) {
        RestoreCachedMaterials(SelectedActor.Get());
        SelectedActor.Reset();
        UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已清理选中的 Actor"));
      }

      // 🔧 修复 - 确保状态为 Idle，因为此时没有实际的预览 Actor
      SetPlacementState(EXBPlacementState::Idle);

      UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 需要配置面板，缓存索引: %d"),
             PendingConfigEntryIndex);

      // 设置配置界面正在显示标志，禁用悬停检测
      bIsConfigUIShowing = true;

      // 广播请求显示配置面板事件
      OnRequestShowConfigPanel.Broadcast(PendingConfigEntryIndex,
                                         Entry->ConfigWidgetClass);
      return true; // 返回 true 表示处理成功（但没有创建预览）
     }
   }

  // 销毁旧的预览 Actor
  DestroyPreviewActor();

  // 创建预览 Actor
  if (!CreatePreviewActor(EntryIndex)) {
    return false;
  }

  CurrentPreviewEntryIndex = EntryIndex;

  // 根据旋转模式设置初始旋转
  switch (Entry->RotationMode) {
  case EXBPlacementRotationMode::Manual:
    // 手动模式使用默认旋转
    PreviewRotation = Entry->DefaultRotation;
    break;

  case EXBPlacementRotationMode::FacePlayer:
    // 朝向玩家模式：计算面朝玩家 Pawn 的方向
    if (APawn *PlayerPawn = CachedPlayerController.IsValid()
                                ? CachedPlayerController->GetPawn()
                                : nullptr) {
      FVector PawnLocation = PlayerPawn->GetActorLocation();
      FVector PreviewDir = PawnLocation - PreviewLocation;
      PreviewDir.Z = 0.0f;
      if (!PreviewDir.IsNearlyZero()) {
        PreviewRotation = PreviewDir.Rotation();
      } else {
        PreviewRotation = Entry->DefaultRotation;
      }
    } else {
      PreviewRotation = Entry->DefaultRotation;
    }
    break;

  case EXBPlacementRotationMode::Random:
    // 随机模式：随机 Yaw 角度
    PreviewRotation = Entry->DefaultRotation;
    PreviewRotation.Yaw = FMath::FRandRange(0.0f, 360.0f);
    break;

  default:
    PreviewRotation = Entry->DefaultRotation;
    break;
  }

  // 切换到预览状态
  SetPlacementState(EXBPlacementState::Previewing);

  return true;
}

/**
 * @brief 确认放置当前预览的 Actor
 * @return 放置成功返回新生成的 Actor 指针，失败返回 nullptr
 * @note  详细流程:
 *        1. 验证状态（必须为 Previewing）和预览 Actor 有效性
 *        2. 在预览位置生成实际 Actor
 *        3. 应用配置数据到主将类型 Actor（阵营、名称等）
 *        4. 禁用磁场组件（配置阶段防止招募士兵）
 *        5. 记录放置数据到 PlacedActors 列表
 *        6. 销毁预览 Actor
 *        7. 广播 OnActorPlaced 事件
 *        8. 处理连续放置逻辑（对于不需要配置的 Actor）
 *        注意事项: 对于需要配置的 Actor，不会自动触发连续放置，
 *                  因为每次放置都需要用户手动配置
 */
AActor *UXBActorPlacementComponent::ConfirmPlacement() {
  if (CurrentState != EXBPlacementState::Previewing) {
    return nullptr;
  }

  if (!PreviewActor.IsValid() || !PlacementConfig) {
    return nullptr;
  }

  const FXBSpawnableActorEntry *Entry =
      PlacementConfig->GetEntryByIndexPtr(CurrentPreviewEntryIndex);
  if (!Entry || !Entry->ActorClass) {
    return nullptr;
  }

  UWorld *World = GetWorld();
  if (!World) {
    return nullptr;
  }

  // ========== 批量放置逻辑 ==========
  // 如果配置了批量放置，则生成网格布局的多个 Actor
  if (Entry->bBatchPlacement && Entry->BatchGridSize.X > 0 && Entry->BatchGridSize.Y > 0) {
    UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 开始批量放置: %dx%d, 间距: %.1f"),
           Entry->BatchGridSize.X, Entry->BatchGridSize.Y, Entry->BatchSpacing);

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // 计算网格中心偏移（使放置点为网格中心）
    const float HalfWidth = (Entry->BatchGridSize.X - 1) * Entry->BatchSpacing * 0.5f;
    const float HalfHeight = (Entry->BatchGridSize.Y - 1) * Entry->BatchSpacing * 0.5f;

    AActor* FirstActor = nullptr;
    int32 SpawnedCount = 0;

    for (int32 GridX = 0; GridX < Entry->BatchGridSize.X; ++GridX) {
      for (int32 GridY = 0; GridY < Entry->BatchGridSize.Y; ++GridY) {
        // 计算相对于中心的偏移
        FVector GridOffset(
            GridX * Entry->BatchSpacing - HalfWidth,
            GridY * Entry->BatchSpacing - HalfHeight,
            0.0f);

        // 应用预览旋转到偏移（保持网格随旋转方向变化）
        FVector RotatedOffset = PreviewRotation.RotateVector(GridOffset);
        FVector SpawnLocation = PreviewLocation + RotatedOffset;

        // 地面贴合检测
        if (Entry->bSnapToGround) {
          FVector GroundLocation;
          if (TraceForGround(SpawnLocation, GroundLocation)) {
            SpawnLocation = GroundLocation;
          }
        }

        // 应用位置偏移
        SpawnLocation += Entry->LocationOffset;

        // 根据旋转模式计算每个 Actor 的旋转
        FRotator ActorRotation = PreviewRotation;
        if (Entry->RotationMode == EXBPlacementRotationMode::Random) {
          ActorRotation = Entry->DefaultRotation;
          ActorRotation.Yaw = FMath::FRandRange(0.0f, 360.0f);
        }

        // 生成 Actor
        AActor* GridActor = World->SpawnActor<AActor>(
            Entry->ActorClass, SpawnLocation, ActorRotation, SpawnParams);

        if (GridActor) {
          GridActor->SetActorScale3D(Entry->DefaultScale);
          SpawnedCount++;

          // 记录第一个生成的 Actor 作为返回值
          if (!FirstActor) {
            FirstActor = GridActor;
          }

          // 记录放置数据
          FXBPlacedActorData PlacedData;
          PlacedData.PlacedActor = GridActor;
          PlacedData.EntryIndex = CurrentPreviewEntryIndex;
          PlacedData.ActorClassPath = FSoftClassPath(Entry->ActorClass);
          PlacedData.Location = SpawnLocation;
          PlacedData.Rotation = ActorRotation;
          PlacedData.Scale = Entry->DefaultScale;
          PlacedActors.Add(PlacedData);
        }
      }
    }

    UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 批量放置完成，共生成 %d 个 Actor"),
           SpawnedCount);

    // 缓存连续放置相关数据
    const int32 PlacedEntryIndex = CurrentPreviewEntryIndex;
    const bool bGlobalContinuousMode =
        PlacementConfig && PlacementConfig->bContinuousPlacementMode;
    const bool bEntryContinuousMode = Entry->bContinuousPlacement;
    const bool bShouldContinue = bGlobalContinuousMode || bEntryContinuousMode;

    // 销毁预览 Actor
    DestroyPreviewActor();

    // 广播事件（使用第一个 Actor）
    if (FirstActor) {
      OnActorPlaced.Broadcast(FirstActor, PlacedEntryIndex);
    }

    // 连续放置模式
    if (bShouldContinue) {
      StartPreview(PlacedEntryIndex);
    } else {
      SetPlacementState(EXBPlacementState::Idle);
    }

    return FirstActor;
  }

  // ========== 单个放置逻辑（原有逻辑） ==========
  // 生成实际 Actor
  FActorSpawnParameters SpawnParams;
  SpawnParams.SpawnCollisionHandlingOverride =
      ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

  AActor *NewActor = World->SpawnActor<AActor>(
      Entry->ActorClass, PreviewLocation, PreviewRotation, SpawnParams);

  if (!NewActor) {
    UE_LOG(LogXBConfig, Warning, TEXT("[放置组件] 生成 Actor 失败: %s"),
           *Entry->ActorClass->GetName());
    return nullptr;
  }

  // 应用缩放
  NewActor->SetActorScale3D(Entry->DefaultScale);

  // ✨ 修复 - 使用预览 Actor 的位置（已经在 UpdatePreviewLocation
  // 中计算过偏移） 获取预览 Actor 的当前位置作为最终放置位置
  FVector FinalLocation = PreviewActor.IsValid()
                              ? PreviewActor->GetActorLocation()
                              : PreviewLocation;
  
  // ✨ 新增 - 应用配置的位置偏移
  FinalLocation += Entry->LocationOffset;
  
  NewActor->SetActorLocation(FinalLocation);

  // ✨ 新增 - 生成后立即禁用磁场组件（防止用默认配置招募士兵）
  // 必须在 ApplyRuntimeConfig 之前禁用，否则 BeginPlay 中开启的磁场会提前招募
  UXBMagnetFieldComponent *MagnetComp =
      NewActor->FindComponentByClass<UXBMagnetFieldComponent>();
  if (MagnetComp) {
    MagnetComp->SetFieldEnabled(false);
    UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已禁用磁场组件: %s"),
           *NewActor->GetName());
  }

  // ✨ 新增 - 如果有待应用的配置数据，应用到生成的 Actor
  if (bHasPendingConfig) {
    // ✨ 新增 - 调用 XBDummyCharacter 专用的名称初始化函数
    if (AXBDummyCharacter *DummyLeader = Cast<AXBDummyCharacter>(NewActor)) {
      // 设置阵营
      DummyLeader->SetFaction(PendingConfigData.Faction);
      // 应用游戏配置（包括主将类型切换、视觉配置等）
      DummyLeader->ApplyRuntimeConfig(PendingConfigData.GameConfig, true);

      // 优先使用 LeaderDisplayName，如果为空则使用 LeaderConfigRowName
      FString DisplayName = PendingConfigData.GameConfig.LeaderDisplayName;

      // 🔧 调试 - 输出 LeaderDisplayName 的值
      UE_LOG(LogXBConfig, Log,
             TEXT("[放置组件] 📝 LeaderDisplayName='%s', "
                  "LeaderConfigRowName='%s'"),
             *DisplayName,
             *PendingConfigData.GameConfig.LeaderConfigRowName.ToString());

      if (DisplayName.IsEmpty() &&
          !PendingConfigData.GameConfig.LeaderConfigRowName.IsNone()) {
        DisplayName =
            PendingConfigData.GameConfig.LeaderConfigRowName.ToString();
      }
      DummyLeader->InitializeCharacterNameFromConfig(DisplayName);

      // 🔧 修复 - 刷新血条组件，确保显示正确的名称
      // 问题：BeginPlay 时血条组件缓存了数据表默认名称，这里需要通知刷新
      if (UXBWorldHealthBarComponent *HealthBar =
              DummyLeader->GetHealthBarComponent()) {
        HealthBar->RefreshNameDisplay();
      }

      // 🔧 调试 - 检查初始化后的 CharacterName
      UE_LOG(LogXBConfig, Log,
             TEXT("[放置组件] 📝 初始化后 CharacterName='%s'"),
             *DummyLeader->CharacterName);

      // ✨ 新增 - 应用假人移动模式
      if (!PendingConfigData.GameConfig.LeaderDummyMoveMode.IsNone()) {
        FString ModeStr =
            PendingConfigData.GameConfig.LeaderDummyMoveMode.ToString();
        EXBLeaderAIMoveMode MoveMode =
            EXBLeaderAIMoveMode::Stand; // 默认原地站立

        if (ModeStr == TEXT("Wander") || ModeStr == TEXT("范围内移动")) {
          MoveMode = EXBLeaderAIMoveMode::Wander;
        } else if (ModeStr == TEXT("Stand") || ModeStr == TEXT("原地站立")) {
          MoveMode = EXBLeaderAIMoveMode::Route;
        } else if (ModeStr == TEXT("Forward") || ModeStr == TEXT("向前行走")) {
          MoveMode = EXBLeaderAIMoveMode::Forward;
        }
        DummyLeader->SetDummyMoveMode(MoveMode);
      }

      UE_LOG(LogXBConfig, Log,
             TEXT("[放置组件] 已应用配置到主将: %s, 阵营: %d, 主将行: %s"),
             *NewActor->GetName(),
             static_cast<int32>(PendingConfigData.Faction),
             *PendingConfigData.GameConfig.LeaderConfigRowName.ToString());

      // ✨ 新增 - 配置应用完成后开启磁场，此时已使用自定义配置
      if (MagnetComp) {
        MagnetComp->SetFieldEnabled(true);
        UE_LOG(LogXBConfig, Log,
               TEXT("[放置组件] 已开启磁场组件（配置应用完成）: %s"),
               *NewActor->GetName());
      }
    }
  }

  // 记录放置数据
  FXBPlacedActorData PlacedData;
  PlacedData.PlacedActor = NewActor;
  PlacedData.EntryIndex = CurrentPreviewEntryIndex;
  PlacedData.ActorClassPath = FSoftClassPath(Entry->ActorClass);
  PlacedData.Location = FinalLocation;
  PlacedData.Rotation = PreviewRotation;
  PlacedData.Scale = Entry->DefaultScale;

  // ✨ 修复 - 在清理状态前保存主将配置数据（如果有）
  if (bHasPendingConfig) {
    PlacedData.bHasLeaderConfig = true;
    PlacedData.LeaderConfigData = PendingConfigData;
    UE_LOG(LogXBConfig, Log,
           TEXT("[放置组件] 保存主将配置: 阵营=%d, 主将行=%s, 士兵数=%d, "
                "显示名=%s"),
           static_cast<int32>(PendingConfigData.Faction),
           *PendingConfigData.GameConfig.LeaderConfigRowName.ToString(),
           PendingConfigData.GameConfig.InitialSoldierCount,
           *PendingConfigData.GameConfig.LeaderDisplayName);

    // ✨ 清理配置状态（移到这里，在记录配置后）
    bHasPendingConfig = false;
  }

  PlacedActors.Add(PlacedData);

  // ✨ 重要：在销毁预览 Actor 前保存连续放置相关数据
  const int32 PlacedEntryIndex = CurrentPreviewEntryIndex;
  const bool bGlobalContinuousMode =
      PlacementConfig && PlacementConfig->bContinuousPlacementMode;
  const bool bEntryContinuousMode = Entry->bContinuousPlacement;
  const bool bShouldContinue = bGlobalContinuousMode || bEntryContinuousMode;

  // 🔧 修复 - 对于需要配置的 Actor，不自动触发连续放置
  // 因为每次放置都需要用户手动配置，自动触发会导致状态从 Idle 变为 Previewing
  const bool bActualContinue = bShouldContinue && !Entry->bRequiresConfig;

  UE_LOG(LogXBConfig, Log,
         TEXT("[放置组件] 连续放置检查 - 全局: %s, 条目: %s, 需配置: %s, 索引: "
              "%d, 应继续: %s"),
         bGlobalContinuousMode ? TEXT("开启") : TEXT("关闭"),
         bEntryContinuousMode ? TEXT("开启") : TEXT("关闭"),
         Entry->bRequiresConfig ? TEXT("是") : TEXT("否"), PlacedEntryIndex,
         bActualContinue ? TEXT("是") : TEXT("否"));

  // 销毁预览 Actor（这会重置 CurrentPreviewEntryIndex）
  DestroyPreviewActor();

  // 广播事件
  OnActorPlaced.Broadcast(NewActor, PlacedEntryIndex);

  UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已放置 Actor: %s 位置: %s"),
         *NewActor->GetName(), *FinalLocation.ToString());

  // 连续放置模式：放置后自动继续预览同类型 Actor（仅对不需要配置的 Actor）
  if (bActualContinue) {
    // 直接调用 StartPreview，使用之前保存的索引
    const bool bStarted = StartPreview(PlacedEntryIndex);
    UE_LOG(LogXBConfig, Log,
           TEXT("[放置组件] 连续放置模式：自动开始预览索引 %d，结果: %s"),
           PlacedEntryIndex, bStarted ? TEXT("成功") : TEXT("失败"));
  } else {
    // 回到空闲状态
    SetPlacementState(EXBPlacementState::Idle);
  }

  return NewActor;
}

/**
 * @brief 取消当前操作
 * @note  根据当前状态执行不同操作:
 *        - Previewing: 销毁预览 Actor，回到 Idle
 *        - Editing: 取消选中，回到 Idle
 */
void UXBActorPlacementComponent::CancelOperation() {
  switch (CurrentState) {
  case EXBPlacementState::Previewing:
    DestroyPreviewActor();
    SetPlacementState(EXBPlacementState::Idle);
    // 清除批量配置数据，重新选择时会再次弹出配置界面
    bHasPendingBatchConfig = false;
    UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 取消预览，已清除批量配置数据"));
    break;

  case EXBPlacementState::Editing:
    DeselectActor();
    break;

  default:
    break;
  }
}

void UXBActorPlacementComponent::ClearAllSelectionAndHover() {
  // 清理悬停状态并恢复材质
  if (HoveredActor.IsValid()) {
    ApplyHoverMaterial(HoveredActor.Get(), false);
    HoveredActor.Reset();
  }

  // 清理选中状态并恢复材质
  if (SelectedActor.IsValid()) {
    RestoreCachedMaterials(SelectedActor.Get());
    SelectedActor.Reset();
  }

  // 确保所有缓存的材质都恢复
  for (auto &Pair : OriginalMaterialsCache) {
    if (Pair.Key.IsValid()) {
      RestoreCachedMaterials(Pair.Key.Get());
    }
  }
  OriginalMaterialsCache.Empty();

  // 重置状态
  SetPlacementState(EXBPlacementState::Idle);

  UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已清理所有选中和悬停状态"));
}

bool UXBActorPlacementComponent::DeleteSelectedActor() {
  if (CurrentState != EXBPlacementState::Editing) {
    return false;
  }

  if (!SelectedActor.IsValid()) {
    return false;
  }

  AActor *ActorToDelete = SelectedActor.Get();

  // 从已放置列表中移除
  PlacedActors.RemoveAll([ActorToDelete](const FXBPlacedActorData &Data) {
    return Data.PlacedActor.Get() == ActorToDelete;
  });

  // 广播删除事件
  OnActorDeleted.Broadcast(ActorToDelete);

  // 销毁 Actor
  ActorToDelete->Destroy();

  // 清空选中
  SelectedActor.Reset();

  UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已删除选中 Actor"));

  // 回到空闲状态
  SetPlacementState(EXBPlacementState::Idle);

  return true;
}

void UXBActorPlacementComponent::RotateActor(float YawDelta) {
  if (!PlacementConfig) {
    return;
  }

  const float RotationStep = PlacementConfig->RotationSpeed * YawDelta;

  // 🔧 修复 - 仅在预览状态下允许旋转，放置后的 Actor 不再允许旋转
  if (CurrentState == EXBPlacementState::Previewing && PreviewActor.IsValid()) {
    // 预览模式下只有手动旋转模式才允许旋转
    const FXBSpawnableActorEntry *Entry =
        PlacementConfig->GetEntryByIndexPtr(CurrentPreviewEntryIndex);
    if (Entry && Entry->RotationMode == EXBPlacementRotationMode::Manual) {
      PreviewRotation.Yaw += RotationStep;
      PreviewActor->SetActorRotation(PreviewRotation);
    }
  }
  // 已移除 Editing 状态下的旋转逻辑，放置完成后不再允许旋转
}

void UXBActorPlacementComponent::RestoreFromSaveData(
    const TArray<FXBPlacedActorData> &SavedData) {
  UWorld *World = GetWorld();
  if (!World) {
    UE_LOG(LogXBConfig, Error, TEXT("[放置组件-恢复] World 为空，无法恢复"));
    return;
  }

  // ========== 恢复前日志 ==========
  UE_LOG(LogXBConfig, Log,
         TEXT("[放置组件-恢复] 开始恢复，存档中有 %d 个 Actor"),
         SavedData.Num());

  for (int32 i = 0; i < SavedData.Num(); ++i) {
    const FXBPlacedActorData &Data = SavedData[i];
    UE_LOG(LogXBConfig, Log,
           TEXT("[放置组件-恢复] 存档[%d]: 类=%s, bHasLeaderConfig=%s"), i,
           *Data.ActorClassPath.ToString(),
           Data.bHasLeaderConfig ? TEXT("true") : TEXT("false"));

    if (Data.bHasLeaderConfig) {
      UE_LOG(LogXBConfig, Log,
             TEXT("[放置组件-恢复]   配置: Faction=%d, 主将行=%s, 显示名=%s, "
                  "士兵数=%d"),
             static_cast<int32>(Data.LeaderConfigData.Faction),
             *Data.LeaderConfigData.GameConfig.LeaderConfigRowName.ToString(),
             *Data.LeaderConfigData.GameConfig.LeaderDisplayName,
             Data.LeaderConfigData.GameConfig.InitialSoldierCount);
    }
  }

  // 清空当前已放置的 Actor
  for (const FXBPlacedActorData &Data : PlacedActors) {
    if (Data.PlacedActor.IsValid()) {
      Data.PlacedActor->Destroy();
    }
  }
  PlacedActors.Empty();

  // 恢复存档中的 Actor
  for (const FXBPlacedActorData &SavedItem : SavedData) {
    UClass *ActorClass = SavedItem.ActorClassPath.TryLoadClass<AActor>();
    if (!ActorClass) {
      UE_LOG(LogXBConfig, Warning, TEXT("[放置组件-恢复] 无法加载类: %s"),
             *SavedItem.ActorClassPath.ToString());
      continue;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AActor *NewActor = World->SpawnActor<AActor>(
        ActorClass, SavedItem.Location, SavedItem.Rotation, SpawnParams);

    if (NewActor) {
      NewActor->SetActorScale3D(SavedItem.Scale);

      // ✨ 新增 - 如果有主将配置数据，应用到 DummyCharacter
      if (SavedItem.bHasLeaderConfig) {
        if (AXBDummyCharacter *DummyLeader =
                Cast<AXBDummyCharacter>(NewActor)) {
          const FXBLeaderSpawnConfigData &ConfigData =
              SavedItem.LeaderConfigData;

          // 使用 SetFaction 设置阵营
          DummyLeader->SetFaction(ConfigData.Faction);

          // 应用移动模式
          if (!ConfigData.GameConfig.LeaderDummyMoveMode.IsNone()) {
            FString ModeStr =
                ConfigData.GameConfig.LeaderDummyMoveMode.ToString();
            EXBLeaderAIMoveMode MoveMode = EXBLeaderAIMoveMode::Stand;
            if (ModeStr == TEXT("Wander") || ModeStr == TEXT("范围内移动")) {
              MoveMode = EXBLeaderAIMoveMode::Wander;
            } else if (ModeStr == TEXT("Route") ||
                       ModeStr == TEXT("固定路线")) {
              MoveMode = EXBLeaderAIMoveMode::Route;
            } else if (ModeStr == TEXT("Forward") ||
                       ModeStr == TEXT("向前行走")) {
              MoveMode = EXBLeaderAIMoveMode::Forward;
            }
            DummyLeader->SetDummyMoveMode(MoveMode);
          }

          // 使用 ApplyRuntimeConfig 应用完整配置（主将行、士兵配置等）
          DummyLeader->ApplyRuntimeConfig(ConfigData.GameConfig, true);

          // ✨ 修复 - 显示名称必须在 ApplyRuntimeConfig 之后设置，否则会被覆盖
          if (!ConfigData.GameConfig.LeaderDisplayName.IsEmpty()) {
            DummyLeader->InitializeCharacterNameFromConfig(
                ConfigData.GameConfig.LeaderDisplayName);
            UE_LOG(LogXBConfig, Log, TEXT("[放置组件-恢复] 设置显示名称: %s"),
                   *ConfigData.GameConfig.LeaderDisplayName);

            // ✨ 新增 - 刷新血条组件显示
            if (UXBWorldHealthBarComponent *HealthBar =
                    DummyLeader->GetHealthBarComponent()) {
              HealthBar->RefreshNameDisplay();
              UE_LOG(LogXBConfig, Log,
                     TEXT("[放置组件-恢复] 已刷新血条显示名称"));
            }
          }

          // 开启磁场
          if (UXBMagnetFieldComponent *MagnetComp =
                  DummyLeader
                      ->FindComponentByClass<UXBMagnetFieldComponent>()) {
            MagnetComp->SetFieldEnabled(true);
          }

          UE_LOG(LogXBConfig, Log,
                 TEXT("[放置组件] 已恢复主将配置: %s, 阵营=%d, 主将行=%s, "
                      "士兵数=%d, 显示名=%s"),
                 *NewActor->GetName(), static_cast<int32>(ConfigData.Faction),
                 *ConfigData.GameConfig.LeaderConfigRowName.ToString(),
                 ConfigData.GameConfig.InitialSoldierCount,
                 *ConfigData.GameConfig.LeaderDisplayName);
        }
      }

      FXBPlacedActorData RestoredData = SavedItem;
      RestoredData.PlacedActor = NewActor;
      PlacedActors.Add(RestoredData);

      UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已恢复 Actor: %s"),
             *NewActor->GetName());
    }
  }

  UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 存档恢复完成，共恢复 %d 个 Actor"),
         PlacedActors.Num());
}

int32 UXBActorPlacementComponent::GetSpawnableActorCount() const {
  if (!PlacementConfig) {
    return 0;
  }
  return PlacementConfig->GetEntryCount();
}

bool UXBActorPlacementComponent::GetSpawnableActorEntry(
    int32 Index, FXBSpawnableActorEntry &OutEntry) const {
  if (!PlacementConfig) {
    return false;
  }
  return PlacementConfig->GetEntryByIndex(Index, OutEntry);
}

const TArray<FXBSpawnableActorEntry> &
UXBActorPlacementComponent::GetAllSpawnableActorEntries() const {
  // 返回空数组的静态引用作为 fallback
  static const TArray<FXBSpawnableActorEntry> EmptyArray;

  if (!PlacementConfig) {
    return EmptyArray;
  }
  return PlacementConfig->SpawnableActors;
}

void UXBActorPlacementComponent::SetPlacementConfig(
    UXBPlacementConfigAsset *Config) {
  PlacementConfig = Config;
  UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已设置放置配置: %s"),
         Config ? *Config->GetName() : TEXT("None"));
}

TArray<FXBFilteredSpawnableEntry>
UXBActorPlacementComponent::GetFilteredSpawnableActorEntriesWithIndices() const {
  if (!PlacementConfig) {
    return TArray<FXBFilteredSpawnableEntry>();
  }
  return PlacementConfig->GetFilteredEntriesWithIndices(CurrentMapTag);
}

bool UXBActorPlacementComponent::AutoDetectCurrentMapTag() {
  // 获取当前地图名称
  FString MapName = GetCurrentMapName();
  if (MapName.IsEmpty()) {
    UE_LOG(LogXBConfig, Warning, TEXT("[放置组件] 无法获取当前地图名称"));
    return false;
  }

  // 构造标签名称：Map.地图名
  FString TagName = FString::Printf(TEXT("Map.%s"), *MapName);

  // 尝试请求标签
  CurrentMapTag = FGameplayTag::RequestGameplayTag(FName(*TagName), false);

  if (CurrentMapTag.IsValid()) {
    UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已自动设置地图标签: %s"),
           *CurrentMapTag.ToString());
    return true;
  } else {
    UE_LOG(LogXBConfig, Warning,
           TEXT("[放置组件] 无法找到对应的地图标签: %s"), *TagName);
    return false;
  }
}

/**
 * @brief 获取鼠标射线检测命中位置
 * @param OutLocation 输出命中位置
 * @param OutNormal 输出命中面法线
 * @return 是否命中
 * @note  用于预览位置更新和放置位置确定
 *        使用 GroundTraceObjectTypes 配置的对象类型检测
 *        自动忽略 Owner Actor 和预览 Actor
 */
bool UXBActorPlacementComponent::GetMouseHitLocation(FVector &OutLocation,
                                                     FVector &OutNormal) const {
  if (!CachedPlayerController.IsValid()) {
    return false;
  }

  // 获取鼠标位置
  float MouseX, MouseY;
  if (!CachedPlayerController->GetMousePosition(MouseX, MouseY)) {
    return false;
  }

  // 将屏幕坐标转换为世界射线
  FVector WorldLocation, WorldDirection;
  if (!CachedPlayerController->DeprojectScreenPositionToWorld(
          MouseX, MouseY, WorldLocation, WorldDirection)) {
    return false;
  }

  // 执行射线检测
  const float TraceDistance =
      PlacementConfig ? PlacementConfig->TraceDistance : 50000.0f;
  const FVector TraceEnd = WorldLocation + WorldDirection * TraceDistance;

  // 构建忽略 Actor 列表
  TArray<AActor *> ActorsToIgnore;
  ActorsToIgnore.Add(GetOwner()->GetOwner()); // Owner 的 Owner（Pawn）
  if (GetOwner()) {
    ActorsToIgnore.Add(GetOwner());
  }
  if (PreviewActor.IsValid()) {
    ActorsToIgnore.Add(PreviewActor.Get());
  }

  FHitResult HitResult;

  // 使用 LineTraceSingleForObjects 进行检测
  // 如果未配置 ObjectTypes，默认检测 WorldStatic 和 WorldDynamic
  TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = GroundTraceObjectTypes;
  if (ObjectTypes.Num() == 0) {
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
  }

  const bool bHit = UKismetSystemLibrary::LineTraceSingleForObjects(
      GetOwner(), WorldLocation, TraceEnd, ObjectTypes, false, ActorsToIgnore,
      TraceDebugType, HitResult, true, TraceDebugHitColor.ToFColor(true),
      TraceDebugMissColor.ToFColor(true), TraceDebugDuration);

  if (bHit) {
    OutLocation = HitResult.Location;
    OutNormal = HitResult.ImpactNormal;
    return true;
  }

  return false;
}

void UXBActorPlacementComponent::UpdatePreviewLocation() {
  if (!PreviewActor.IsValid()) {
    return;
  }

  FVector HitLocation;
  FVector HitNormal;
  if (GetMouseHitLocation(HitLocation, HitNormal)) {
    // 检测地面
    FVector GroundLocation;
    const FXBSpawnableActorEntry *Entry =
        PlacementConfig
            ? PlacementConfig->GetEntryByIndexPtr(CurrentPreviewEntryIndex)
            : nullptr;

    if (Entry && Entry->bSnapToGround &&
        TraceForGround(HitLocation, GroundLocation)) {
      PreviewLocation = GroundLocation;
    } else {
      PreviewLocation = HitLocation;
    }

    bIsPreviewLocationValid = true;

    // ========== 批量预览位置更新 ==========
    if (Entry && Entry->bBatchPlacement && BatchPreviewActors.Num() > 0) {
      // 计算网格中心偏移
      const float HalfWidth = (Entry->BatchGridSize.X - 1) * Entry->BatchSpacing * 0.5f;
      const float HalfHeight = (Entry->BatchGridSize.Y - 1) * Entry->BatchSpacing * 0.5f;

      int32 ActorIndex = 0;
      for (int32 GridX = 0; GridX < Entry->BatchGridSize.X; ++GridX) {
        for (int32 GridY = 0; GridY < Entry->BatchGridSize.Y; ++GridY) {
          if (ActorIndex >= BatchPreviewActors.Num()) {
            break;
          }

          TWeakObjectPtr<AActor>& BatchActor = BatchPreviewActors[ActorIndex];
          if (!BatchActor.IsValid()) {
            ActorIndex++;
            continue;
          }

          // 计算相对于中心的偏移
          FVector GridOffset(
              GridX * Entry->BatchSpacing - HalfWidth,
              GridY * Entry->BatchSpacing - HalfHeight,
              0.0f);

          // 应用预览旋转到偏移
          FVector RotatedOffset = PreviewRotation.RotateVector(GridOffset);
          FVector SpawnLocation = PreviewLocation + RotatedOffset;

          // 地面贴合检测（每个网格点单独检测）
          if (Entry->bSnapToGround) {
            FVector GridGroundLocation;
            if (TraceForGround(SpawnLocation, GridGroundLocation)) {
              SpawnLocation = GridGroundLocation;
            }
          }

          // 计算 Z 偏移
          const float ZOffset = CalculateActorBottomOffset(BatchActor.Get());
          SpawnLocation.Z += ZOffset;

          // 设置旋转（随机模式下每个 Actor 旋转不同，这里保持预览时的一致性）
          FRotator ActorRotation = PreviewRotation;

          BatchActor->SetActorLocation(SpawnLocation);
          BatchActor->SetActorRotation(ActorRotation);

          // 更新预览材质
          ApplyPreviewMaterial(BatchActor.Get(), true);

          ActorIndex++;
        }
      }
    } else {
      // ========== 单个预览位置更新（原有逻辑） ==========
      const float ZOffset = CalculateActorBottomOffset(PreviewActor.Get());
      FVector AdjustedLocation = PreviewLocation;
      AdjustedLocation.Z += ZOffset;

      PreviewActor->SetActorLocation(AdjustedLocation);

      // 更新预览材质颜色
      ApplyPreviewMaterial(PreviewActor.Get(), true);
    }
  } else {
    bIsPreviewLocationValid = false;
    
    // 批量预览时更新所有 Actor 材质
    if (BatchPreviewActors.Num() > 0) {
      for (TWeakObjectPtr<AActor>& BatchActor : BatchPreviewActors) {
        if (BatchActor.IsValid()) {
          ApplyPreviewMaterial(BatchActor.Get(), false);
        }
      }
    } else {
      ApplyPreviewMaterial(PreviewActor.Get(), false);
    }
  }
}

bool UXBActorPlacementComponent::CreatePreviewActor(int32 EntryIndex) {
  if (!PlacementConfig) {
    return false;
  }

  const FXBSpawnableActorEntry *Entry =
      PlacementConfig->GetEntryByIndexPtr(EntryIndex);
  if (!Entry || !Entry->ActorClass) {
    return false;
  }

  UWorld *World = GetWorld();
  if (!World) {
    return false;
  }

  FActorSpawnParameters SpawnParams;
  SpawnParams.SpawnCollisionHandlingOverride =
      ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

  // ========== 批量预览逻辑 ==========
  if (Entry->bBatchPlacement && Entry->BatchGridSize.X > 0 && Entry->BatchGridSize.Y > 0) {
    UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 创建批量预览: %dx%d"),
           Entry->BatchGridSize.X, Entry->BatchGridSize.Y);

    // 清理旧的批量预览 Actor
    for (TWeakObjectPtr<AActor>& BatchActor : BatchPreviewActors) {
      if (BatchActor.IsValid()) {
        BatchActor->Destroy();
      }
    }
    BatchPreviewActors.Empty();

    // 创建网格预览 Actor
    for (int32 GridX = 0; GridX < Entry->BatchGridSize.X; ++GridX) {
      for (int32 GridY = 0; GridY < Entry->BatchGridSize.Y; ++GridY) {
        AActor* GridPreview = World->SpawnActor<AActor>(
            Entry->ActorClass, FVector::ZeroVector,
            Entry->DefaultRotation, SpawnParams);

        if (GridPreview) {
          GridPreview->SetActorEnableCollision(false);
          GridPreview->SetActorScale3D(Entry->DefaultScale);
          ApplyPreviewMaterial(GridPreview, true);
          BatchPreviewActors.Add(GridPreview);
        }
      }
    }

    // 将第一个 Actor 作为主预览 Actor（用于兼容现有逻辑）
    if (BatchPreviewActors.Num() > 0 && BatchPreviewActors[0].IsValid()) {
      PreviewActor = BatchPreviewActors[0];
    }

    return BatchPreviewActors.Num() > 0;
  }

  // ========== 单个预览逻辑（原有逻辑） ==========
  AActor *NewPreview =
      World->SpawnActor<AActor>(Entry->ActorClass, FVector::ZeroVector,
                                Entry->DefaultRotation, SpawnParams);

  if (!NewPreview) {
    return false;
  }

  // 禁用碰撞
  NewPreview->SetActorEnableCollision(false);

  // 应用缩放
  NewPreview->SetActorScale3D(Entry->DefaultScale);

  // 应用预览材质
  ApplyPreviewMaterial(NewPreview, true);

  PreviewActor = NewPreview;

  return true;
}

void UXBActorPlacementComponent::DestroyPreviewActor() {
  // 清理批量预览 Actor
  for (TWeakObjectPtr<AActor>& BatchActor : BatchPreviewActors) {
    if (BatchActor.IsValid()) {
      BatchActor->Destroy();
    }
  }
  BatchPreviewActors.Empty();

  // 清理主预览 Actor（如果不在批量列表中）
  if (PreviewActor.IsValid()) {
    PreviewActor->Destroy();
    PreviewActor.Reset();
  }

  CurrentPreviewEntryIndex = -1;
  bIsPreviewLocationValid = false;
}

void UXBActorPlacementComponent::ApplyPreviewMaterial(AActor *Actor,
                                                      bool bValid) {
  if (!Actor || !PlacementConfig) {
    return;
  }

  // 加载预览材质
  UMaterialInterface *PreviewMat =
      PlacementConfig->PreviewMaterial.LoadSynchronous();
  if (!PreviewMat) {
    return;
  }

  // 创建或更新动态材质实例
  if (!CachedPreviewMID) {
    CachedPreviewMID = UMaterialInstanceDynamic::Create(PreviewMat, this);
  }

  // 设置颜色
  const FLinearColor &Color = bValid ? PlacementConfig->ValidPreviewColor
                                     : PlacementConfig->InvalidPreviewColor;
  CachedPreviewMID->SetVectorParameterValue(TEXT("Color"), Color);

  // 应用到所有 Mesh 组件
  TArray<UPrimitiveComponent *> PrimitiveComps;
  Actor->GetComponents<UPrimitiveComponent>(PrimitiveComps);

  for (UPrimitiveComponent *PrimComp : PrimitiveComps) {
    if (PrimComp) {
      const int32 NumMaterials = PrimComp->GetNumMaterials();
      for (int32 i = 0; i < NumMaterials; ++i) {
        PrimComp->SetMaterial(i, CachedPreviewMID);
      }
    }
  }
}

void UXBActorPlacementComponent::RestoreOriginalMaterials(AActor *Actor) {
  // 注意：此函数需要缓存原始材质才能恢复
  // 当前实现中预览 Actor 是销毁重建的，不需要恢复
  // 选中高亮功能可以在此扩展
}

/**
 * @brief 检测射线命中的已放置 Actor
 * @param OutActor 输出命中的 Actor 指针
 * @return 是否命中已放置的 Actor
 * @note  用于悬停检测和点击选中
 *        使用多个碰撞通道检测（Leader/Pawn/Visibility）
 *        仅返回 PlacedActors 列表中的 Actor
 *        包含调试可视化（绿色=命中，红色=未命中）
 */
bool UXBActorPlacementComponent::GetHitPlacedActor(AActor *&OutActor) const {
  if (!CachedPlayerController.IsValid()) {
    return false;
  }

  float MouseX, MouseY;
  if (!CachedPlayerController->GetMousePosition(MouseX, MouseY)) {
    return false;
  }

  FVector WorldLocation, WorldDirection;
  if (!CachedPlayerController->DeprojectScreenPositionToWorld(
          MouseX, MouseY, WorldLocation, WorldDirection)) {
    return false;
  }

  const float TraceDistance =
      PlacementConfig ? PlacementConfig->TraceDistance : 50000.0f;
  const FVector TraceEnd = WorldLocation + WorldDirection * TraceDistance;

  FHitResult HitResult;
  FCollisionQueryParams QueryParams;
  QueryParams.AddIgnoredActor(GetOwner());

  if (PreviewActor.IsValid()) {
    QueryParams.AddIgnoredActor(PreviewActor.Get());
  }

  UWorld *World = GetWorld();
  if (!World) {
    return false;
  }

  // 射线检测已放置 Actor（使用 ObjectType 检测多个碰撞类型）
  bool bHit = false;
  
  if (PlacementConfig && PlacementConfig->SelectionTraceChannels.Num() > 0) {
    // 构建 ObjectQueryParams，包含配置的所有碰撞通道
    FCollisionObjectQueryParams ObjectQueryParams;
    for (const TEnumAsByte<ECollisionChannel>& Channel : PlacementConfig->SelectionTraceChannels) {
      ObjectQueryParams.AddObjectTypesToQuery(Channel);
    }
    
    // 使用 ObjectType 检测，同时检测所有配置的类型
    bHit = World->LineTraceSingleByObjectType(HitResult, WorldLocation, TraceEnd,
                                               ObjectQueryParams, QueryParams);
  } else {
    // 默认：检测 Leader, Pawn, Visibility, WorldDynamic
    FCollisionObjectQueryParams DefaultObjectParams;
    DefaultObjectParams.AddObjectTypesToQuery(XBCollision::Leader);
    DefaultObjectParams.AddObjectTypesToQuery(ECC_Pawn);
    DefaultObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
    
    bHit = World->LineTraceSingleByObjectType(HitResult, WorldLocation, TraceEnd,
                                               DefaultObjectParams, QueryParams);
  }

  if (bHit) {
    AActor *HitActor = HitResult.GetActor();

    UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 射线命中 Actor: %s, Component: %s"),
           HitActor ? *HitActor->GetName() : TEXT("None"),
           HitResult.GetComponent() ? *HitResult.GetComponent()->GetName() : TEXT("None"));

    // 检查是否是已放置的 Actor
    for (const FXBPlacedActorData &Data : PlacedActors) {
      if (Data.PlacedActor.Get() == HitActor) {
        OutActor = HitActor;
        UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 检测到已放置的 Actor: %s"),
               *HitActor->GetName());
        return true;
      }
    }

    UE_LOG(
        LogXBConfig, Log,
        TEXT("[放置组件] 命中的 Actor 不在已放置列表中，PlacedActors 数量: %d, 命中Actor类: %s"),
        PlacedActors.Num(), HitActor ? *HitActor->GetClass()->GetName() : TEXT("None"));
  } else {
    UE_LOG(LogXBConfig, Verbose, TEXT("[放置组件] 射线未命中任何 Actor"));
  }

  return false;
}

void UXBActorPlacementComponent::SelectActor(AActor *Actor) {
  if (!Actor) {
    return;
  }

  // 取消之前的选中
  if (SelectedActor.IsValid() && SelectedActor.Get() != Actor) {
    RestoreOriginalMaterials(SelectedActor.Get());
  }

  SelectedActor = Actor;

  // TODO: 应用选中高亮效果

  SetPlacementState(EXBPlacementState::Editing);

  OnSelectionChanged.Broadcast(Actor);

  UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 选中 Actor: %s"),
         *Actor->GetName());
}

void UXBActorPlacementComponent::DeselectActor() {
  if (SelectedActor.IsValid()) {
    RestoreOriginalMaterials(SelectedActor.Get());
  }

  SelectedActor.Reset();

  SetPlacementState(EXBPlacementState::Idle);

  OnSelectionChanged.Broadcast(nullptr);
}

void UXBActorPlacementComponent::SetPlacementState(EXBPlacementState NewState) {
  if (CurrentState == NewState) {
    return;
  }

  const EXBPlacementState OldState = CurrentState;
  CurrentState = NewState;

  // 根据状态启用/禁用 Tick
  // Idle 状态需要 Tick 用于悬停检测，Previewing 状态需要 Tick 用于更新预览位置
  const bool bNeedsTick = (NewState == EXBPlacementState::Idle ||
                           NewState == EXBPlacementState::Previewing);
  SetComponentTickEnabled(bNeedsTick);

  // 广播状态变更事件
  OnPlacementStateChanged.Broadcast(NewState);

  // ✨ 调试 - 详细日志
  auto StateToString = [](EXBPlacementState State) -> const TCHAR * {
    switch (State) {
    case EXBPlacementState::Idle:
      return TEXT("Idle");
    case EXBPlacementState::Previewing:
      return TEXT("Previewing");
    case EXBPlacementState::Editing:
      return TEXT("Editing");
    default:
      return TEXT("Unknown");
    }
  };
  UE_LOG(LogXBConfig, Log, TEXT("[放置组件] ⚠️ 状态变更: %s -> %s"),
         StateToString(OldState), StateToString(NewState));
}

bool UXBActorPlacementComponent::TraceForGround(
    const FVector &InLocation, FVector &OutGroundLocation) const {
  UWorld *World = GetWorld();
  if (!World) {
    return false;
  }

  const float Offset =
      PlacementConfig ? PlacementConfig->GroundTraceOffset : 500.0f;
  const FVector TraceStart = InLocation + FVector(0.0f, 0.0f, Offset);
  const FVector TraceEnd = InLocation - FVector(0.0f, 0.0f, Offset * 10.0f);

  FHitResult HitResult;
  FCollisionQueryParams QueryParams;
  QueryParams.AddIgnoredActor(GetOwner());

  if (PreviewActor.IsValid()) {
    QueryParams.AddIgnoredActor(PreviewActor.Get());
  }

  if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd,
                                      ECC_WorldStatic, QueryParams)) {
    OutGroundLocation = HitResult.Location;
    return true;
  }

  OutGroundLocation = InLocation;
  return false;
}

// ============ 悬停与高亮相关实现 ============

void UXBActorPlacementComponent::UpdateHoverState() {
  // 如果配置界面正在显示，跳过悬停检测
  if (bIsConfigUIShowing) {
    return;
  }

  // 获取当前光标下的已放置 Actor
  AActor *NewHovered = nullptr;
  GetHitPlacedActor(NewHovered);

  // 悬停对象变化时更新材质
  if (NewHovered != HoveredActor.Get()) {
    // 移除旧的高亮
    if (HoveredActor.IsValid()) {
      ApplyHoverMaterial(HoveredActor.Get(), false);
    }

    // 更新悬停引用
    HoveredActor = NewHovered;

    // 应用新的高亮
    if (HoveredActor.IsValid()) {
      ApplyHoverMaterial(HoveredActor.Get(), true);
    }
  }
}

void UXBActorPlacementComponent::ApplyHoverMaterial(AActor *Actor,
                                                    bool bHovered) {
  if (!Actor || !PlacementConfig) {
    return;
  }

  if (bHovered) {
    // 缓存原始材质（如果尚未缓存）
    CacheOriginalMaterials(Actor);

    // 加载高亮材质
    UMaterialInterface *HighlightMat =
        PlacementConfig->SelectionHighlightMaterial.LoadSynchronous();
    if (!HighlightMat) {
      UE_LOG(LogXBConfig, Warning, TEXT("[放置组件] 未配置选中高亮材质"));
      return;
    }

    // 创建或复用动态材质实例
    if (!CachedHoverMID) {
      CachedHoverMID = UMaterialInstanceDynamic::Create(HighlightMat, this);
    }

    // 设置高亮颜色
    CachedHoverMID->SetVectorParameterValue(TEXT("Color"),
                                            PlacementConfig->SelectionColor);

    // 应用到所有 Mesh 组件
    TArray<UPrimitiveComponent *> PrimitiveComps;
    Actor->GetComponents<UPrimitiveComponent>(PrimitiveComps);

    for (UPrimitiveComponent *PrimComp : PrimitiveComps) {
      if (PrimComp) {
        const int32 NumMaterials = PrimComp->GetNumMaterials();
        for (int32 i = 0; i < NumMaterials; ++i) {
          PrimComp->SetMaterial(i, CachedHoverMID);
        }
      }
    }

    UE_LOG(LogXBConfig, Verbose, TEXT("[放置组件] 应用悬停高亮: %s"),
           *Actor->GetName());
  } else {
    // 恢复原始材质
    RestoreCachedMaterials(Actor);

    UE_LOG(LogXBConfig, Verbose, TEXT("[放置组件] 移除悬停高亮: %s"),
           *Actor->GetName());
  }
}

bool UXBActorPlacementComponent::HandleRightClick() {
  switch (CurrentState) {
  case EXBPlacementState::Idle: {
    // 空闲状态 -> 删除悬停的 Actor
    if (HoveredActor.IsValid()) {
      return DeleteHoveredActor();
    }
    return false;
  }

  case EXBPlacementState::Previewing: {
    // 预览状态 -> 取消预览
    CancelOperation();
    return true;
  }

  case EXBPlacementState::Editing: {
    // 编辑状态 -> 删除选中的 Actor
    return DeleteSelectedActor();
  }

  default:
    return false;
  }
}

bool UXBActorPlacementComponent::DeleteHoveredActor() {
  if (!HoveredActor.IsValid()) {
    return false;
  }

  AActor *ActorToDelete = HoveredActor.Get();

  // 移除悬停高亮
  ApplyHoverMaterial(ActorToDelete, false);

  // 从已放置列表中移除
  PlacedActors.RemoveAll([ActorToDelete](const FXBPlacedActorData &Data) {
    return Data.PlacedActor.Get() == ActorToDelete;
  });

  // 广播删除事件
  OnActorDeleted.Broadcast(ActorToDelete);

  // 销毁 Actor
  ActorToDelete->Destroy();

  // 清空悬停引用
  HoveredActor.Reset();

  UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已删除悬停 Actor"));

  return true;
}

float UXBActorPlacementComponent::CalculateActorBottomOffset(
    AActor *Actor) const {
  if (!Actor) {
    return 0.0f;
  }

  // 只对角色类型使用胶囊体半高偏移（使角色脚底贴地）
  if (ACharacter *CharActor = Cast<ACharacter>(Actor)) {
    if (UCapsuleComponent *Capsule = CharActor->GetCapsuleComponent()) {
      return Capsule->GetScaledCapsuleHalfHeight();
    }
  }

  // 普通 Actor 不需要偏移，保持原点在地面
  return 0.0f;
}

void UXBActorPlacementComponent::CacheOriginalMaterials(AActor *Actor) {
  if (!Actor) {
    return;
  }

  // 如果已缓存则跳过
  TWeakObjectPtr<AActor> WeakActor = Actor;
  if (OriginalMaterialsCache.Contains(WeakActor)) {
    return;
  }

  TArray<TPair<int32, TArray<TObjectPtr<UMaterialInterface>>>>
      ComponentMaterials;

  TArray<UPrimitiveComponent *> PrimitiveComps;
  Actor->GetComponents<UPrimitiveComponent>(PrimitiveComps);

  for (int32 CompIdx = 0; CompIdx < PrimitiveComps.Num(); ++CompIdx) {
    UPrimitiveComponent *PrimComp = PrimitiveComps[CompIdx];
    if (!PrimComp) {
      continue;
    }

    TArray<TObjectPtr<UMaterialInterface>> Materials;
    const int32 NumMaterials = PrimComp->GetNumMaterials();
    for (int32 MatIdx = 0; MatIdx < NumMaterials; ++MatIdx) {
      Materials.Add(PrimComp->GetMaterial(MatIdx));
    }

    ComponentMaterials.Add(TPair<int32, TArray<TObjectPtr<UMaterialInterface>>>(
        CompIdx, Materials));
  }

  OriginalMaterialsCache.Add(WeakActor, ComponentMaterials);
}

void UXBActorPlacementComponent::RestoreCachedMaterials(AActor *Actor) {
  if (!Actor) {
    return;
  }

  TWeakObjectPtr<AActor> WeakActor = Actor;
  TArray<TPair<int32, TArray<TObjectPtr<UMaterialInterface>>>> *CachedData =
      OriginalMaterialsCache.Find(WeakActor);
  if (!CachedData) {
    return;
  }

  TArray<UPrimitiveComponent *> PrimitiveComps;
  Actor->GetComponents<UPrimitiveComponent>(PrimitiveComps);

  for (const TPair<int32, TArray<TObjectPtr<UMaterialInterface>>> &CompData :
       *CachedData) {
    const int32 CompIdx = CompData.Key;
    const TArray<TObjectPtr<UMaterialInterface>> &Materials = CompData.Value;

    if (CompIdx < PrimitiveComps.Num() && PrimitiveComps[CompIdx]) {
      UPrimitiveComponent *PrimComp = PrimitiveComps[CompIdx];
      for (int32 MatIdx = 0; MatIdx < Materials.Num(); ++MatIdx) {
        if (MatIdx < PrimComp->GetNumMaterials()) {
          PrimComp->SetMaterial(MatIdx, Materials[MatIdx]);
        }
      }
    }
  }

  // 清理缓存
  OriginalMaterialsCache.Remove(WeakActor);
}

// ============ 配置放置接口实现 ============

AActor *UXBActorPlacementComponent::ConfirmPlacementWithConfig(
    const FXBLeaderSpawnConfigData &ConfigData, FVector SpawnLocation) {
  if (PendingConfigEntryIndex < 0) {
    UE_LOG(LogXBConfig, Warning,
           TEXT("[放置组件] 配置后确认放置失败：无待配置条目"));
    return nullptr;
  }

  if (!PlacementConfig) {
    UE_LOG(LogXBConfig, Warning,
           TEXT("[放置组件] 配置后确认放置失败：未配置 PlacementConfig"));
    PendingConfigEntryIndex = -1;
    return nullptr;
  }

  const FXBSpawnableActorEntry *Entry =
      PlacementConfig->GetEntryByIndexPtr(PendingConfigEntryIndex);
  if (!Entry || !Entry->ActorClass) {
    UE_LOG(LogXBConfig, Warning,
           TEXT("[放置组件] 配置后确认放置失败：无效的条目索引 %d"),
           PendingConfigEntryIndex);
    PendingConfigEntryIndex = -1;
    return nullptr;
  }

  UWorld *World = GetWorld();
  if (!World) {
    PendingConfigEntryIndex = -1;
    return nullptr;
  }

  // 确定生成位置
  FVector FinalSpawnLocation = SpawnLocation;
  if (FinalSpawnLocation.IsNearlyZero()) {
    // 如果未指定位置，尝试从鼠标射线获取
    FVector HitLocation, HitNormal;
    if (GetMouseHitLocation(HitLocation, HitNormal)) {
      FinalSpawnLocation = HitLocation;
    } else {
      // 如果无法获取鼠标位置，使用默认位置
      FinalSpawnLocation = FVector(0, 0, 100);
      UE_LOG(LogXBConfig, Warning,
             TEXT("[放置组件] 无法获取鼠标位置，使用默认位置"));
    }
  }

  // 生成 Actor
  FActorSpawnParameters SpawnParams;
  SpawnParams.SpawnCollisionHandlingOverride =
      ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

  AActor *NewActor =
      World->SpawnActor<AActor>(Entry->ActorClass, FinalSpawnLocation,
                                PendingConfigRotation, SpawnParams);

  if (!NewActor) {
    UE_LOG(LogXBConfig, Warning, TEXT("[放置组件] 配置后生成 Actor 失败: %s"),
           *Entry->ActorClass->GetName());
    PendingConfigEntryIndex = -1;
    return nullptr;
  }

  // 应用缩放
  NewActor->SetActorScale3D(Entry->DefaultScale);

  // 应用配置到主将
  if (AXBCharacterBase *Leader = Cast<AXBCharacterBase>(NewActor)) {
    // 设置阵营
    Leader->SetFaction(ConfigData.Faction);

    // 直接使用 GameConfig 成员应用配置
    Leader->ApplyRuntimeConfig(ConfigData.GameConfig, true);

    UE_LOG(LogXBConfig, Log,
           TEXT("[放置组件] 已应用配置到主将: %s, 阵营: %d, 初始士兵: %d"),
           *NewActor->GetName(), static_cast<int32>(ConfigData.Faction),
           ConfigData.GameConfig.InitialSoldierCount);
  }

  // 配置阶段禁用磁场组件（防止提前招募士兵）
  if (UXBMagnetFieldComponent *MagnetComp =
          NewActor->FindComponentByClass<UXBMagnetFieldComponent>()) {
    MagnetComp->SetFieldEnabled(false);
  }

  // 记录放置数据
  FXBPlacedActorData PlacedData;
  PlacedData.PlacedActor = NewActor;
  PlacedData.EntryIndex = PendingConfigEntryIndex;
  PlacedData.ActorClassPath = FSoftClassPath(Entry->ActorClass);
  PlacedData.Location = PendingConfigLocation;
  PlacedData.Rotation = PendingConfigRotation;
  PlacedData.Scale = Entry->DefaultScale;
  PlacedActors.Add(PlacedData);

  // 保存索引用于广播
  const int32 PlacedEntryIndex = PendingConfigEntryIndex;

  // 清理待配置状态
  PendingConfigEntryIndex = -1;
  PendingConfigLocation = FVector::ZeroVector;
  PendingConfigRotation = FRotator::ZeroRotator;

  // 广播放置完成事件
  OnActorPlaced.Broadcast(NewActor, PlacedEntryIndex);

  UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 配置后已放置 Actor: %s 位置: %s"),
         *NewActor->GetName(), *PlacedData.Location.ToString());

  return NewActor;
}

void UXBActorPlacementComponent::CancelPendingConfig() {
  if (PendingConfigEntryIndex >= 0) {
    UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 取消待配置，索引: %d"),
           PendingConfigEntryIndex);

    PendingConfigEntryIndex = -1;
    PendingConfigLocation = FVector::ZeroVector;
    PendingConfigRotation = FRotator::ZeroRotator;
  }
}

// ============ 配置界面绑定实现 ============

void UXBActorPlacementComponent::SetConfigWidget(
    UXBLeaderSpawnConfigWidget *Widget) {
  // 解绑旧 Widget
  if (CurrentConfigWidget.IsValid()) {
    CurrentConfigWidget->OnConfigConfirmed.RemoveDynamic(
        this, &UXBActorPlacementComponent::HandleLeaderConfigConfirmed);
    CurrentConfigWidget->OnConfigCancelled.RemoveDynamic(
        this, &UXBActorPlacementComponent::HandleLeaderConfigCancelled);
  }

  CurrentConfigWidget = Widget;

  // 绑定新 Widget 事件
  if (Widget) {
    Widget->OnConfigConfirmed.AddDynamic(
        this, &UXBActorPlacementComponent::HandleLeaderConfigConfirmed);
    Widget->OnConfigCancelled.AddDynamic(
        this, &UXBActorPlacementComponent::HandleLeaderConfigCancelled);

    UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已绑定配置界面事件"));
  }
}

void UXBActorPlacementComponent::HandleLeaderConfigConfirmed(
    int32 EntryIndex, FXBLeaderSpawnConfigData ConfigData) {
  UE_LOG(LogXBConfig, Log,
         TEXT("[放置组件] 收到配置确认，索引: %d，主将行: %s"), EntryIndex,
         *ConfigData.GameConfig.LeaderConfigRowName.ToString());

  // 保存配置数据，等待用户确认位置后再应用
  PendingConfigData = ConfigData;
  bHasPendingConfig = true;

  // 清理 Widget 引用，恢复悬停检测
  CurrentConfigWidget.Reset();
  bIsConfigUIShowing = false;

  // 创建预览 Actor（跟随光标）
  if (CreatePreviewActor(EntryIndex)) {
    CurrentPreviewEntryIndex = EntryIndex;
    SetPlacementState(EXBPlacementState::Previewing);

    // ✨ 新增 - 在预览时就初始化假人主将的 CharacterName
    if (PreviewActor.IsValid()) {
      if (AXBDummyCharacter *DummyPreview =
              Cast<AXBDummyCharacter>(PreviewActor.Get())) {
        // 优先使用 LeaderDisplayName，如果为空则使用 LeaderConfigRowName
        FString DisplayName = ConfigData.GameConfig.LeaderDisplayName;
        if (DisplayName.IsEmpty() &&
            !ConfigData.GameConfig.LeaderConfigRowName.IsNone()) {
          DisplayName = ConfigData.GameConfig.LeaderConfigRowName.ToString();
        }
        DummyPreview->InitializeCharacterNameFromConfig(DisplayName);
        UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 预览 Actor 名称已初始化: %s"),
               *DisplayName);
      }
    }

    UE_LOG(LogXBConfig, Log,
           TEXT("[放置组件] 配置确认后进入预览模式，索引: %d"), EntryIndex);
  } else {
    UE_LOG(LogXBConfig, Warning,
           TEXT("[放置组件] 配置确认后创建预览失败，索引: %d"), EntryIndex);
    bHasPendingConfig = false;
  }

  // 清理待配置状态
  PendingConfigEntryIndex = -1;
}

void UXBActorPlacementComponent::HandleLeaderConfigCancelled() {
  UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 配置已取消"));

  // 取消待配置状态
  CancelPendingConfig();

  // 恢复悬停检测
  bIsConfigUIShowing = false;

  // 清理 Widget 引用
  CurrentConfigWidget.Reset();

  // 清理配置数据
  bHasPendingConfig = false;
}

// ============ 批量配置界面绑定实现 ============

void UXBActorPlacementComponent::SetBatchConfigWidget(
    UXBBatchPlacementConfigWidget *Widget) {
  // 解绑旧 Widget
  if (CurrentBatchConfigWidget.IsValid()) {
    CurrentBatchConfigWidget->OnConfigConfirmed.RemoveDynamic(
        this, &UXBActorPlacementComponent::HandleBatchConfigConfirmed);
    CurrentBatchConfigWidget->OnConfigCancelled.RemoveDynamic(
        this, &UXBActorPlacementComponent::HandleBatchConfigCancelled);
  }

  CurrentBatchConfigWidget = Widget;

  // 绑定新 Widget 事件
  if (Widget) {
    Widget->OnConfigConfirmed.AddDynamic(
        this, &UXBActorPlacementComponent::HandleBatchConfigConfirmed);
    Widget->OnConfigCancelled.AddDynamic(
        this, &UXBActorPlacementComponent::HandleBatchConfigCancelled);

    UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已绑定批量配置界面事件"));
  }
}

void UXBActorPlacementComponent::HandleBatchConfigConfirmed(
    int32 EntryIndex, FXBBatchPlacementConfigData ConfigData) {
  UE_LOG(LogXBConfig, Log,
         TEXT("[放置组件] 收到批量配置确认，索引: %d，网格: %dx%d，间距: %.1f"),
         EntryIndex, ConfigData.GridSize.X, ConfigData.GridSize.Y, ConfigData.Spacing);

  // 保存批量配置数据
  PendingBatchConfigData = ConfigData;
  bHasPendingBatchConfig = true;

  // 清理 Widget 引用，恢复悬停检测
  CurrentBatchConfigWidget.Reset();
  bIsConfigUIShowing = false;

  // 获取并更新条目配置
  if (!PlacementConfig) {
    UE_LOG(LogXBConfig, Warning, TEXT("[放置组件] PlacementConfig 无效"));
    bHasPendingBatchConfig = false;
    return;
  }

  FXBSpawnableActorEntry* Entry = 
      const_cast<FXBSpawnableActorEntry*>(PlacementConfig->GetEntryByIndexPtr(EntryIndex));
  if (!Entry || !Entry->ActorClass) {
    UE_LOG(LogXBConfig, Warning, TEXT("[放置组件] 条目无效，索引: %d"), EntryIndex);
    bHasPendingBatchConfig = false;
    return;
  }

  // 应用用户配置的网格尺寸和间距到条目
  Entry->BatchGridSize = ConfigData.GridSize;
  Entry->BatchSpacing = ConfigData.Spacing;

  // 创建预览 Actor（批量预览）
  if (CreatePreviewActor(EntryIndex)) {
    CurrentPreviewEntryIndex = EntryIndex;
    SetPlacementState(EXBPlacementState::Previewing);

    UE_LOG(LogXBConfig, Log,
           TEXT("[放置组件] 批量配置确认后进入预览模式，索引: %d"), EntryIndex);
  } else {
    UE_LOG(LogXBConfig, Warning,
           TEXT("[放置组件] 批量配置确认后创建预览失败，索引: %d"), EntryIndex);
    bHasPendingBatchConfig = false;
  }

  // 清理待配置状态
  PendingConfigEntryIndex = -1;
}

void UXBActorPlacementComponent::HandleBatchConfigCancelled() {
  UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 批量配置已取消"));

  // 取消待配置状态
  CancelPendingConfig();

  // 清理 Widget 引用，恢复悬停检测
  CurrentBatchConfigWidget.Reset();
  bIsConfigUIShowing = false;

  // 清理配置数据
  bHasPendingBatchConfig = false;
}

// ============ 存档系统实现 ============

/**
 * @brief 保存当前放置数据到指定槽位
 */
bool UXBActorPlacementComponent::SavePlacementToSlot(const FString &SlotName) {
  if (SlotName.IsEmpty()) {
    UE_LOG(LogXBConfig, Warning, TEXT("[放置组件] 保存失败：槽位名称为空"));
    return false;
  }

  // 🔧 修改 - 使用地图特定的存档路径
  const FString FullSlotName = BuildPlacementSlotName(SlotName);

  // ✨ 新增 - 检测同名存档是否已存在
  if (UGameplayStatics::DoesSaveGameExist(FullSlotName, 0)) {
    UE_LOG(LogXBConfig, Warning,
           TEXT("[放置组件] 保存失败：同名存档已存在 "
                "'%s'，请使用其他名称或删除旧存档"),
           *SlotName);
    return false;
  }

  UXBPlacementSaveGame *SaveGame =
      Cast<UXBPlacementSaveGame>(UGameplayStatics::CreateSaveGameObject(
          UXBPlacementSaveGame::StaticClass()));

  if (!SaveGame) {
    UE_LOG(LogXBConfig, Error, TEXT("[放置组件] 保存失败：无法创建存档对象"));
    return false;
  }

  // 创建存档数据
  FXBPlacementSaveData SaveData;
  SaveData.SaveName = SlotName;
  SaveData.SaveTime = FDateTime::Now();
  SaveData.PlacedActors = PlacedActors;

  // ========== 详细调试日志 ==========
  UE_LOG(LogXBConfig, Log,
         TEXT("[放置组件-保存] 开始保存槽位: %s，Actor数量: %d"), *SlotName,
         PlacedActors.Num());

  for (int32 i = 0; i < PlacedActors.Num(); ++i) {
    const FXBPlacedActorData &Data = PlacedActors[i];
    UE_LOG(LogXBConfig, Log,
           TEXT("[放置组件-保存] Actor[%d]: 类=%s, 位置=(%0.1f, %0.1f, %0.1f), "
                "bHasLeaderConfig=%s"),
           i, *Data.ActorClassPath.ToString(), Data.Location.X, Data.Location.Y,
           Data.Location.Z,
           Data.bHasLeaderConfig ? TEXT("true") : TEXT("false"));

    if (Data.bHasLeaderConfig) {
      UE_LOG(LogXBConfig, Log,
             TEXT("[放置组件-保存]   配置: Faction=%d, 主将行=%s, 显示名=%s, "
                  "士兵数=%d"),
             static_cast<int32>(Data.LeaderConfigData.Faction),
             *Data.LeaderConfigData.GameConfig.LeaderConfigRowName.ToString(),
             *Data.LeaderConfigData.GameConfig.LeaderDisplayName,
             Data.LeaderConfigData.GameConfig.InitialSoldierCount);
    }
  }

  // 添加或更新存档
  bool bFound = false;
  for (FXBPlacementSaveData &Existing : SaveGame->PlacementSaves) {
    if (Existing.SaveName == SlotName) {
      Existing = SaveData;
      bFound = true;
      break;
    }
  }

  if (!bFound) {
    SaveGame->PlacementSaves.Add(SaveData);
    SaveGame->SlotNames.AddUnique(SlotName);
  }

  // 保存到磁盘
  if (!UGameplayStatics::SaveGameToSlot(SaveGame, FullSlotName, 0)) {
    UE_LOG(LogXBConfig, Error, TEXT("[放置组件] 保存失败：写入磁盘失败"));
    return false;
  }

  // 🔧 修改 - 使用地图特定的索引文件名
  const FString IndexSlotName = GetPlacementIndexSlotName();

  UXBPlacementSaveGame *IndexSave = Cast<UXBPlacementSaveGame>(
      UGameplayStatics::LoadGameFromSlot(IndexSlotName, 0));

  if (!IndexSave) {
    IndexSave =
        Cast<UXBPlacementSaveGame>(UGameplayStatics::CreateSaveGameObject(
            UXBPlacementSaveGame::StaticClass()));
  }

  if (IndexSave) {
    // 添加新槽位名到索引
    IndexSave->SlotNames.AddUnique(SlotName);

    // 保存索引文件
    if (UGameplayStatics::SaveGameToSlot(IndexSave, IndexSlotName, 0)) {
      UE_LOG(LogXBConfig, Log,
             TEXT("[放置组件] 索引文件已更新，当前槽位数: %d"),
             IndexSave->SlotNames.Num());
    } else {
      UE_LOG(LogXBConfig, Warning, TEXT("[放置组件] 索引文件更新失败"));
    }
  }

  UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 保存成功：%s，共 %d 个 Actor"),
         *SlotName, PlacedActors.Num());
  return true;
}

/**
 * @brief 从指定槽位读取放置数据
 */
bool UXBActorPlacementComponent::LoadPlacementFromSlot(
    const FString &SlotName) {
  if (SlotName.IsEmpty()) {
    UE_LOG(LogXBConfig, Warning, TEXT("[放置组件] 读取失败：槽位名称为空"));
    return false;
  }

  // 🔧 修改 - 使用地图特定的存档路径
  const FString FullSlotName = BuildPlacementSlotName(SlotName);

  UXBPlacementSaveGame *SaveGame = Cast<UXBPlacementSaveGame>(
      UGameplayStatics::LoadGameFromSlot(FullSlotName, 0));

  if (!SaveGame) {
    UE_LOG(LogXBConfig, Warning, TEXT("[放置组件] 读取失败：存档不存在 %s"),
           *SlotName);
    return false;
  }

  // 查找对应的存档数据
  const FXBPlacementSaveData *FoundData = nullptr;
  for (const FXBPlacementSaveData &Data : SaveGame->PlacementSaves) {
    if (Data.SaveName == SlotName) {
      FoundData = &Data;
      break;
    }
  }

  if (!FoundData) {
    UE_LOG(LogXBConfig, Warning, TEXT("[放置组件] 读取失败：未找到存档数据 %s"),
           *SlotName);
    return false;
  }

  // 清除当前放置的 Actor
  ClearAllPlacedActors();

  // 恢复放置的 Actor
  RestoreFromSaveData(FoundData->PlacedActors);

  UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 读取成功：%s，共恢复 %d 个 Actor"),
         *SlotName, FoundData->PlacedActors.Num());
  return true;
}

/**
 * @brief 获取所有放置存档槽位名称
 */
TArray<FString> UXBActorPlacementComponent::GetPlacementSaveSlotNames() const {
  TArray<FString> SlotNames;

  // 遍历所有可能的存档槽位
  // 🔧 修改 - 使用地图特定的索引文件名
  const FString IndexSlotName = GetPlacementIndexSlotName();

  UXBPlacementSaveGame *IndexSave = Cast<UXBPlacementSaveGame>(
      UGameplayStatics::LoadGameFromSlot(IndexSlotName, 0));

  if (IndexSave) {
    SlotNames = IndexSave->SlotNames;
  }

  return SlotNames;
}

/**
 * @brief 删除指定放置存档
 */
bool UXBActorPlacementComponent::DeletePlacementSave(const FString &SlotName) {
  if (SlotName.IsEmpty()) {
    return false;
  }

  // 🔧 修改 - 使用地图特定的存档路径
  const FString FullSlotName = BuildPlacementSlotName(SlotName);

  if (!UGameplayStatics::DeleteGameInSlot(FullSlotName, 0)) {
    UE_LOG(LogXBConfig, Warning, TEXT("[放置组件] 删除存档失败：%s"),
           *SlotName);
    return false;
  }

  UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已删除存档：%s"), *SlotName);

  // 🔧 修改 - 使用地图特定的索引文件名
  const FString IndexSlotName = GetPlacementIndexSlotName();

  UXBPlacementSaveGame *IndexSave = Cast<UXBPlacementSaveGame>(
      UGameplayStatics::LoadGameFromSlot(IndexSlotName, 0));

  if (IndexSave) {
    // 从索引中移除槽位名
    IndexSave->SlotNames.Remove(SlotName);

    // 保存索引文件
    if (UGameplayStatics::SaveGameToSlot(IndexSave, IndexSlotName, 0)) {
      UE_LOG(LogXBConfig, Log,
             TEXT("[放置组件] 索引文件已更新，剩余槽位数: %d"),
             IndexSave->SlotNames.Num());
    }
  }

  return true;
}

/**
 * @brief 清除当前所有放置的 Actor
 * @note 不仅清除 PlacedActors 数组中的 Actor，还搜索场景中所有匹配配置的 Actor
 */
void UXBActorPlacementComponent::ClearAllPlacedActors() {
  UWorld *World = GetWorld();
  if (!World) {
    UE_LOG(LogXBConfig, Warning,
           TEXT("[放置组件] ClearAllPlacedActors: World 为空"));
    return;
  }

  int32 DestroyedCount = 0;
  int32 SkippedCount = 0;

  // 首先清除 PlacedActors 数组中记录的 Actor
  for (FXBPlacedActorData &Data : PlacedActors) {
    if (Data.PlacedActor.IsValid()) {
      AActor *PlacedActor = Data.PlacedActor.Get();

      // ✨ 新增 - 检查是否需要排除存档（如 AXBBushVolume）
      if (AXBBushVolume *BushVolume = Cast<AXBBushVolume>(PlacedActor)) {
        if (BushVolume->ShouldExcludeFromSave()) {
          UE_LOG(LogXBConfig, Log,
                 TEXT("[放置组件] 跳过排除存档的 Actor: %s"),
                 *PlacedActor->GetName());
          ++SkippedCount;
          continue;
        }
      }

      PlacedActor->Destroy();
      ++DestroyedCount;
    }
  }

  // 从数组中移除已销毁的 Actor（保留排除存档的）
  PlacedActors.RemoveAll([](const FXBPlacedActorData &Data) {
    return !Data.PlacedActor.IsValid();
  });

  // 然后根据配置搜索场景中所有匹配的 Actor 并删除
  if (PlacementConfig) {
    for (const FXBSpawnableActorEntry &Entry :
         PlacementConfig->SpawnableActors) {
      if (!Entry.ActorClass) {
        continue;
      }

      // 使用 GetAllActorsOfClass 搜索场景中所有此类的 Actor
      TArray<AActor *> FoundActors;
      UGameplayStatics::GetAllActorsOfClass(World, Entry.ActorClass,
                                            FoundActors);

      for (AActor *FoundActor : FoundActors) {
        if (FoundActor && !FoundActor->IsPendingKillPending()) {
          // ✨ 新增 - 检查是否需要排除存档（如 AXBBushVolume）
          if (AXBBushVolume *BushVolume = Cast<AXBBushVolume>(FoundActor)) {
            if (BushVolume->ShouldExcludeFromSave()) {
              UE_LOG(LogXBConfig, Log,
                     TEXT("[放置组件] 跳过排除存档的场景 Actor: %s"),
                     *FoundActor->GetName());
              ++SkippedCount;
              continue;
            }
          }

          UE_LOG(LogXBConfig, Log,
                 TEXT("[放置组件] 清除场景中的 Actor: %s (类: %s)"),
                 *FoundActor->GetName(), *Entry.ActorClass->GetName());
          FoundActor->Destroy();
          ++DestroyedCount;
        }
      }
    }
  }

  UE_LOG(LogXBConfig, Log,
         TEXT("[放置组件] 已清除放置的 Actor，销毁 %d 个，跳过 %d 个（排除存档）"),
         DestroyedCount, SkippedCount);
}

// ============ 地图存档辅助函数 ============

FString UXBActorPlacementComponent::GetCurrentMapName() const {
  if (UWorld* World = GetWorld()) {
    // 获取当前地图名称（不含路径和后缀）
    FString MapName = World->GetMapName();
    // 移除 UEDPIE 前缀（编辑器 PIE 模式下会有这个前缀）
    MapName.RemoveFromStart(World->StreamingLevelsPrefix);
    return MapName;
  }
  return TEXT("Default");
}

FString UXBActorPlacementComponent::BuildPlacementSlotName(const FString& SlotName) const {
  // 🔧 修改 - 加入地图名称，实现按场景分离存档
  // 格式: XBPlacement_地图名_槽位名
  return FString::Printf(TEXT("XBPlacement_%s_%s"), *GetCurrentMapName(), *SlotName);
}

FString UXBActorPlacementComponent::GetPlacementIndexSlotName() const {
  // 格式: XBPlacement_Index_地图名
  return FString::Printf(TEXT("XBPlacement_Index_%s"), *GetCurrentMapName());
}

