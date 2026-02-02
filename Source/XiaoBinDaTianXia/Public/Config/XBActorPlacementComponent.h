/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Config/XBActorPlacementComponent.h

/**
 * @file XBActorPlacementComponent.h
 * @brief 配置阶段 Actor 放置管理组件
 *
 * @note ✨ 新增文件 - 负责射线检测、预览、放置、编辑功能
 */

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "XBLeaderSpawnConfigData.h"
#include "XBPlacementTypes.h"
#include "XBActorPlacementComponent.generated.h"

class UXBPlacementConfigAsset;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UUserWidget;
class UXBLeaderSpawnConfigWidget;
struct FXBLeaderSpawnConfigData;

// ============ 代理声明 ============

/** 放置状态变更代理 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlacementStateChanged,
                                            EXBPlacementState, NewState);

/** 请求显示选择菜单代理 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRequestShowMenu, FVector,
                                            ClickLocation);

/** Actor 放置完成代理 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActorPlaced, AActor *,
                                             PlacedActor, int32, EntryIndex);

/** Actor 被删除代理 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorDeleted, AActor *,
                                            DeletedActor);

/** 选中 Actor 变更代理 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectionChanged, AActor *,
                                            SelectedActor);

/** 请求显示配置面板代理（主将类型 Actor） */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRequestShowConfigPanel, int32,
                                             EntryIndex,
                                             TSubclassOf<UUserWidget>,
                                             WidgetClass);

/**
 * @brief 配置阶段 Actor 放置管理组件
 * @note 挂载在 AXBConfigCameraPawn 上，负责放置系统的核心逻辑
 */
UCLASS(ClassGroup = (Custom),
       meta = (BlueprintSpawnableComponent, DisplayName = "Actor放置组件"))
class XIAOBINDATIANXIA_API UXBActorPlacementComponent : public UActorComponent {
  GENERATED_BODY()

public:
  UXBActorPlacementComponent();

protected:
  virtual void BeginPlay() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
  virtual void
  TickComponent(float DeltaTime, ELevelTick TickType,
                FActorComponentTickFunction *ThisTickFunction) override;

  // ============ 公共接口 ============

  /**
   * @brief 处理点击输入
   * @return 是否成功处理
   * @note   详细流程分析:
   * 根据当前状态决定行为（空闲->显示菜单，预览->确认放置，编辑->取消选中或选中新目标）
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统",
            meta = (DisplayName = "处理点击"))
  bool HandleClick();

  /**
   * @brief 开始预览指定索引的 Actor
   * @param EntryIndex 配置条目索引
   * @return 是否成功开始预览
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统",
            meta = (DisplayName = "开始预览"))
  bool StartPreview(int32 EntryIndex);

  /**
   * @brief 确认放置当前预览的 Actor
   * @return 放置的 Actor 指针
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统",
            meta = (DisplayName = "确认放置"))
  AActor *ConfirmPlacement();

  /**
   * @brief 取消当前操作（预览或编辑）
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统",
            meta = (DisplayName = "取消操作"))
  void CancelOperation();

  /**
   * @brief 清理所有选中和悬停状态，恢复 Actor 原始材质
   * @note 游戏开始时调用，确保所有 Actor 恢复正常显示
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统",
            meta = (DisplayName = "清理选中和悬停状态"))
  void ClearAllSelectionAndHover();

  /**
   * @brief 删除当前选中的 Actor
   * @return 是否成功删除
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统",
            meta = (DisplayName = "删除选中"))
  bool DeleteSelectedActor();

  /**
   * @brief 删除当前悬停的 Actor（光标下的 Actor）
   * @return 是否成功删除
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统",
            meta = (DisplayName = "删除悬停"))
  bool DeleteHoveredActor();

  /**
   * @brief 处理右键输入（删除悬停/选中的 Actor）
   * @return 是否成功处理
   * @note   Idle 状态删除悬停 Actor，Editing 状态删除选中 Actor，Previewing
   * 状态取消预览
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统",
            meta = (DisplayName = "处理右键"))
  bool HandleRightClick();

  /**
   * @brief 获取当前悬停的 Actor
   * @return 悬停的 Actor 指针
   */
  UFUNCTION(BlueprintPure, Category = "放置系统",
            meta = (DisplayName = "获取悬停Actor"))
  AActor *GetHoveredActor() const { return HoveredActor.Get(); }

  /**
   * @brief 旋转当前预览或选中的 Actor
   * @param YawDelta Yaw 旋转增量
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统",
            meta = (DisplayName = "旋转Actor"))
  void RotateActor(float YawDelta);

  /**
   * @brief 获取当前放置状态
   * @return 状态枚举
   */
  UFUNCTION(BlueprintPure, Category = "放置系统",
            meta = (DisplayName = "获取放置状态"))
  EXBPlacementState GetPlacementState() const { return CurrentState; }

  /**
   * @brief 获取当前选中的 Actor
   * @return 选中的 Actor 指针
   */
  UFUNCTION(BlueprintPure, Category = "放置系统",
            meta = (DisplayName = "获取选中Actor"))
  AActor *GetSelectedActor() const { return SelectedActor.Get(); }

  /**
   * @brief 获取所有已放置的 Actor 数据
   * @return 已放置 Actor 数据数组
   */
  UFUNCTION(BlueprintPure, Category = "放置系统",
            meta = (DisplayName = "获取已放置数据"))
  const TArray<FXBPlacedActorData> &GetPlacedActorData() const {
    return PlacedActors;
  }

  /**
   * @brief 根据存档数据恢复放置的 Actor
   * @param SavedData 存档的 Actor 数据数组
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统",
            meta = (DisplayName = "从存档恢复"))
  void RestoreFromSaveData(const TArray<FXBPlacedActorData> &SavedData);

  // ============ 数据资产访问接口 ============

  /**
   * @brief 获取放置配置资产
   * @return 放置配置 DataAsset 指针
   */
  UFUNCTION(BlueprintPure, Category = "放置系统|数据",
            meta = (DisplayName = "获取放置配置"))
  UXBPlacementConfigAsset *GetPlacementConfig() const {
    return PlacementConfig;
  }

  /**
   * @brief 获取可放置 Actor 条目数量
   * @return 条目数量
   */
  UFUNCTION(BlueprintPure, Category = "放置系统|数据",
            meta = (DisplayName = "获取可放置数量"))
  int32 GetSpawnableActorCount() const;

  /**
   * @brief 按索引获取可放置 Actor 条目
   * @param Index 索引
   * @param OutEntry 输出条目
   * @return 是否获取成功
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统|数据",
            meta = (DisplayName = "获取可放置条目"))
  bool GetSpawnableActorEntry(int32 Index,
                              FXBSpawnableActorEntry &OutEntry) const;

  /**
   * @brief 获取所有可放置 Actor 条目数组
   * @return 条目数组引用
   * @note 用于 UI 遍历生成按钮
   */
  UFUNCTION(BlueprintPure, Category = "放置系统|数据",
            meta = (DisplayName = "获取全部可放置条目"))
  const TArray<FXBSpawnableActorEntry> &GetAllSpawnableActorEntries() const;

  /**
   * @brief 设置放置配置资产
   * @param Config 放置配置 DataAsset
   * @note 由 AXBConfigCameraPawn 在 BeginPlay 时调用
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统|数据",
            meta = (DisplayName = "设置放置配置"))
  void SetPlacementConfig(UXBPlacementConfigAsset *Config);

  /**
   * @brief 获取过滤后的可放置条目（根据当前地图标签）
   * @return 过滤后的条目数组（每个条目包含 Entry 和 OriginalIndex）
   * @note UI 应使用此方法获取列表，使用 OriginalIndex 调用 StartPreview
   */
  UFUNCTION(BlueprintPure, Category = "放置系统|数据",
            meta = (DisplayName = "获取过滤后可放置条目"))
  TArray<FXBFilteredSpawnableEntry> GetFilteredSpawnableActorEntriesWithIndices() const;

  /** 当前地图标签（用于过滤可放置条目） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置",
            meta = (DisplayName = "当前地图标签", Categories = "Map"))
  FGameplayTag CurrentMapTag;

  /**
   * @brief 自动检测并设置当前地图标签
   * @return 是否成功设置
   * @note 根据当前地图名称（如 "01_草地"）构造对应标签（如 "Map.01_草地"）
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统|数据",
            meta = (DisplayName = "自动检测地图标签"))
  bool AutoDetectCurrentMapTag();

  // ============ 存档系统接口 ============

  /**
   * @brief 保存当前放置数据到指定槽位
   * @param SlotName 存档槽位名称
   * @return 是否保存成功
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统|存档",
            meta = (DisplayName = "保存放置数据"))
  bool SavePlacementToSlot(const FString &SlotName);

  /**
   * @brief 从指定槽位读取放置数据
   * @param SlotName 存档槽位名称
   * @return 是否读取成功
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统|存档",
            meta = (DisplayName = "读取放置数据"))
  bool LoadPlacementFromSlot(const FString &SlotName);

  /**
   * @brief 获取所有放置存档槽位名称
   * @return 槽位名称数组
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统|存档",
            meta = (DisplayName = "获取放置存档列表"))
  TArray<FString> GetPlacementSaveSlotNames() const;

  /**
   * @brief 删除指定放置存档
   * @param SlotName 存档槽位名称
   * @return 是否删除成功
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统|存档",
            meta = (DisplayName = "删除放置存档"))
  bool DeletePlacementSave(const FString &SlotName);

  /**
   * @brief 清除当前所有放置的 Actor
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统|存档",
            meta = (DisplayName = "清除所有放置"))
  void ClearAllPlacedActors();

  // ============ 代理事件 ============

  /** 放置状态变更事件 */
  UPROPERTY(BlueprintAssignable, Category = "放置系统|事件")
  FOnPlacementStateChanged OnPlacementStateChanged;

  /** 请求显示选择菜单事件 */
  UPROPERTY(BlueprintAssignable, Category = "放置系统|事件")
  FOnRequestShowMenu OnRequestShowMenu;

  /** Actor 放置完成事件 */
  UPROPERTY(BlueprintAssignable, Category = "放置系统|事件")
  FOnActorPlaced OnActorPlaced;

  /** Actor 被删除事件 */
  UPROPERTY(BlueprintAssignable, Category = "放置系统|事件")
  FOnActorDeleted OnActorDeleted;

  /** 选中 Actor 变更事件 */
  UPROPERTY(BlueprintAssignable, Category = "放置系统|事件")
  FOnSelectionChanged OnSelectionChanged;

  /** 请求显示配置面板事件（主将类型 Actor） */
  UPROPERTY(BlueprintAssignable, Category = "放置系统|事件")
  FOnRequestShowConfigPanel OnRequestShowConfigPanel;

  // ============ 配置放置接口 ============

  /**
   * @brief 配置后确认放置（主将类型 Actor）
   * @param ConfigData 主将配置数据
   * @param SpawnLocation 生成位置（如果为 ZeroVector 则使用鼠标点击位置）
   * @return 放置的 Actor 指针
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统",
            meta = (DisplayName = "配置后确认放置"))
  AActor *
  ConfirmPlacementWithConfig(const FXBLeaderSpawnConfigData &ConfigData,
                             FVector SpawnLocation = FVector::ZeroVector);

  /**
   * @brief 取消待配置状态
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统",
            meta = (DisplayName = "取消待配置"))
  void CancelPendingConfig();

  /**
   * @brief 是否有待配置的放置
   * @return 是否待配置
   */
  UFUNCTION(BlueprintPure, Category = "放置系统",
            meta = (DisplayName = "是否待配置"))
  bool HasPendingConfig() const { return PendingConfigEntryIndex >= 0; }

  /**
   * @brief 设置当前配置界面引用
   * @param Widget 配置界面指针
   * @note 蓝图端创建 Widget 后调用此函数绑定事件
   */
  UFUNCTION(BlueprintCallable, Category = "放置系统",
            meta = (DisplayName = "设置配置界面"))
  void SetConfigWidget(UXBLeaderSpawnConfigWidget *Widget);

protected:
  // ============ 事件处理 ============

  /** 处理配置确认事件 */
  UFUNCTION()
  void HandleLeaderConfigConfirmed(int32 EntryIndex,
                                   FXBLeaderSpawnConfigData ConfigData);

  /** 处理配置取消事件 */
  UFUNCTION()
  void HandleLeaderConfigCancelled();

  // ============ 配置引用 ============

  /** 放置配置 DataAsset */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置",
            meta = (DisplayName = "放置配置"))
  TObjectPtr<UXBPlacementConfigAsset> PlacementConfig;

  // ============ 射线检测配置 ============

  /** 地面检测使用的对象类型 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置|射线检测",
            meta = (DisplayName = "地面检测对象类型"))
  TArray<TEnumAsByte<EObjectTypeQuery>> GroundTraceObjectTypes;

  /** 射线检测调试绘制类型 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置|射线检测",
            meta = (DisplayName = "调试绘制类型"))
  TEnumAsByte<EDrawDebugTrace::Type> TraceDebugType = EDrawDebugTrace::None;

  /** 调试绘制持续时间（秒） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置|射线检测",
            meta = (DisplayName = "调试绘制时间", EditCondition = "TraceDebugType != EDrawDebugTrace::None"))
  float TraceDebugDuration = 2.0f;

  /** 调试绘制颜色 - 命中 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置|射线检测",
            meta = (DisplayName = "命中颜色", EditCondition = "TraceDebugType != EDrawDebugTrace::None"))
  FLinearColor TraceDebugHitColor = FLinearColor::Green;

  /** 调试绘制颜色 - 未命中 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置|射线检测",
            meta = (DisplayName = "未命中颜色", EditCondition = "TraceDebugType != EDrawDebugTrace::None"))
  FLinearColor TraceDebugMissColor = FLinearColor::Red;

private:
  // ============ 内部方法 ============

  /**
   * @brief 获取鼠标射线检测命中位置
   * @param OutLocation 输出命中位置
   * @param OutNormal 输出命中法线
   * @return 是否命中
   */
  bool GetMouseHitLocation(FVector &OutLocation, FVector &OutNormal) const;

  /**
   * @brief 更新预览 Actor 位置
   */
  void UpdatePreviewLocation();

  /**
   * @brief 创建预览 Actor
   * @param EntryIndex 配置条目索引
   * @return 是否成功创建
   */
  bool CreatePreviewActor(int32 EntryIndex);

  /**
   * @brief 销毁预览 Actor
   */
  void DestroyPreviewActor();

  /**
   * @brief 应用预览材质到 Actor
   * @param Actor 目标 Actor
   * @param bValid 是否有效位置
   */
  void ApplyPreviewMaterial(AActor *Actor, bool bValid);

  /**
   * @brief 恢复 Actor 原始材质
   * @param Actor 目标 Actor
   */
  void RestoreOriginalMaterials(AActor *Actor);

  /**
   * @brief 检测射线命中的已放置 Actor
   * @param OutActor 输出命中的 Actor
   * @return 是否命中
   */
  bool GetHitPlacedActor(AActor *&OutActor) const;

  /**
   * @brief 选中指定 Actor
   * @param Actor 要选中的 Actor
   */
  void SelectActor(AActor *Actor);

  /**
   * @brief 取消选中
   */
  void DeselectActor();

  /**
   * @brief 设置放置状态
   * @param NewState 新状态
   */
  void SetPlacementState(EXBPlacementState NewState);

  /**
   * @brief 对指定位置进行地面检测
   * @param InLocation 输入位置
   * @param OutGroundLocation 输出地面位置
   * @return 是否检测到地面
   */
  bool TraceForGround(const FVector &InLocation,
                      FVector &OutGroundLocation) const;

  /**
   * @brief 获取当前地图名称
   * @return 当前地图名称（不含路径和后缀）
   * @note 用于按场景分离存档数据
   */
  FString GetCurrentMapName() const;

  /**
   * @brief 构建放置存档完整槽位名称
   * @param SlotName 逻辑槽位名称
   * @return 包含地图名称的完整槽位名
   * @note 格式: XBPlacement_地图名_槽位名
   */
  FString BuildPlacementSlotName(const FString& SlotName) const;

  /**
   * @brief 获取地图特定的索引槽位名称
   * @return 包含地图名称的索引槽位名
   * @note 格式: XBPlacement_Index_地图名
   */
  FString GetPlacementIndexSlotName() const;

  // ============ 内部状态 ============

  /** 当前放置状态 */
  EXBPlacementState CurrentState = EXBPlacementState::Idle;

  /** 当前预览的配置条目索引 */
  int32 CurrentPreviewEntryIndex = -1;

  /** 预览 Actor 引用 */
  UPROPERTY()
  TWeakObjectPtr<AActor> PreviewActor;

  /** 批量预览 Actor 数组（网格放置时使用） */
  UPROPERTY()
  TArray<TWeakObjectPtr<AActor>> BatchPreviewActors;

  /** 当前选中的 Actor */
  UPROPERTY()
  TWeakObjectPtr<AActor> SelectedActor;

  /** 当前悬停的 Actor（光标下的已放置 Actor） */
  UPROPERTY()
  TWeakObjectPtr<AActor> HoveredActor;

  /** 预览 Actor 当前位置 */
  FVector PreviewLocation = FVector::ZeroVector;

  /** 预览 Actor 当前旋转 */
  FRotator PreviewRotation = FRotator::ZeroRotator;

  /** 预览位置是否有效 */
  bool bIsPreviewLocationValid = false;

  /** 已放置的 Actor 数据列表 */
  UPROPERTY()
  TArray<FXBPlacedActorData> PlacedActors;

  /** 缓存的预览动态材质实例 */
  UPROPERTY()
  TObjectPtr<UMaterialInstanceDynamic> CachedPreviewMID;

  /** 缓存的高亮动态材质实例 */
  UPROPERTY()
  TObjectPtr<UMaterialInstanceDynamic> CachedHoverMID;

  /** 缓存的玩家控制器 */
  UPROPERTY()
  TWeakObjectPtr<APlayerController> CachedPlayerController;

  /** 上一次点击的世界位置（用于显示菜单） */
  FVector LastClickLocation = FVector::ZeroVector;

  // ============ 待配置状态（主将类型） ============

  /** 待配置的条目索引 */
  int32 PendingConfigEntryIndex = -1;

  /** 待配置的位置 */
  FVector PendingConfigLocation = FVector::ZeroVector;

  /** 待配置的旋转 */
  FRotator PendingConfigRotation = FRotator::ZeroRotator;

  /** 当前配置界面引用 */
  UPROPERTY()
  TWeakObjectPtr<UXBLeaderSpawnConfigWidget> CurrentConfigWidget;

  /** 待应用的配置数据（配置确认后保存，确认放置时应用） */
  FXBLeaderSpawnConfigData PendingConfigData;

  /** 是否有待应用的配置数据 */
  bool bHasPendingConfig = false;

  // ============ 悬停相关内部方法 ============

  /**
   * @brief 更新悬停状态（在 Tick 中调用）
   */
  void UpdateHoverState();

  /**
   * @brief 应用悬停高亮材质
   * @param Actor 目标 Actor
   * @param bHovered 是否悬停
   */
  void ApplyHoverMaterial(AActor *Actor, bool bHovered);

  /**
   * @brief 计算 Actor 底部到原点的 Z 偏移
   * @param Actor 目标 Actor
   * @return Z 偏移量（使 Actor 底部贴地）
   * @note   角色类型使用胶囊体半高，普通 Actor 使用 Bounds
   */
  float CalculateActorBottomOffset(AActor *Actor) const;

  /**
   * @brief 保存 Actor 的原始材质
   * @param Actor 目标 Actor
   */
  void CacheOriginalMaterials(AActor *Actor);

  /**
   * @brief 恢复缓存的原始材质（带清理）
   * @param Actor 目标 Actor
   */
  void RestoreCachedMaterials(AActor *Actor);

  // ============ 原始材质缓存 ============

  /** 缓存的原始材质（Actor -> (组件索引, 材质槽索引, 材质)） */
  TMap<TWeakObjectPtr<AActor>,
       TArray<TPair<int32, TArray<TObjectPtr<UMaterialInterface>>>>>
      OriginalMaterialsCache;
};
