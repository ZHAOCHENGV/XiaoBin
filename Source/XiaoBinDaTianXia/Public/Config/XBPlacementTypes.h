/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Config/XBPlacementTypes.h

/**
 * @file XBPlacementTypes.h
 * @brief 配置阶段放置系统类型定义
 *
 * @note ✨ 新增文件 - 定义可放置 Actor 的配置结构
 */

#pragma once

#include "Config/XBLeaderSpawnConfigData.h"
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameplayTagContainer.h"
#include "UMG.h"
#include "XBPlacementTypes.generated.h"

class UTexture2D;

/**
 * @brief 放置系统状态枚举
 * @note 用于控制放置组件的行为模式
 */
UENUM(BlueprintType)
enum class EXBPlacementState : uint8 {
  /** 空闲状态 - 未进行任何放置操作 */
  Idle UMETA(DisplayName = "空闲"),

  /** 预览状态 - 正在预览待放置的 Actor */
  Previewing UMETA(DisplayName = "预览中"),

  /** 编辑状态 - 正在编辑已放置的 Actor */
  Editing UMETA(DisplayName = "编辑中")
};

/**
 * @brief 放置时旋转模式枚举
 * @note 控制 Actor 在放置时的旋转行为
 */
UENUM(BlueprintType)
enum class EXBPlacementRotationMode : uint8 {
  /** 手动旋转 - 使用鼠标滚轮手动控制旋转 */
  Manual UMETA(DisplayName = "手动旋转"),

  /** 固定旋转 - 生成时面朝玩家操控的 Pawn */
  FacePlayer UMETA(DisplayName = "朝向玩家"),

  /** 随机旋转 - 生成时随机 Yaw 角度 */
  Random UMETA(DisplayName = "随机旋转")
};

/**
 * @brief 可生成 Actor 条目配置
 * @note 用于 DataAsset 中配置可放置的 Actor 类型
 */
USTRUCT(BlueprintType)
struct FXBSpawnableActorEntry {
  GENERATED_BODY()

  /** 显示名称（中文，用于UI） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置",
            meta = (DisplayName = "显示名称"))
  FText DisplayName;

  /** Actor 类（蓝图或 C++ 类） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置",
            meta = (DisplayName = "Actor类"))
  TSubclassOf<AActor> ActorClass;

  /** 缩略图/图标（用于UI显示） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置",
            meta = (DisplayName = "图标"))
  TObjectPtr<UTexture2D> Icon;

  /** 分类标签（用于UI分组，仅显示 Placement 下的标签） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置",
            meta = (DisplayName = "分类标签", Categories = "Placement"))
  FGameplayTag Category;

  /** 默认缩放 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置",
            meta = (DisplayName = "默认缩放"))
  FVector DefaultScale = FVector::OneVector;

  /** 默认旋转偏移 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置",
            meta = (DisplayName = "默认旋转"))
  FRotator DefaultRotation = FRotator::ZeroRotator;

  /** 是否贴地（自动检测地面高度） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置",
            meta = (DisplayName = "贴地放置"))
  bool bSnapToGround = true;

  /** 旋转模式（手动、朝向玩家、随机） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置",
            meta = (DisplayName = "旋转模式"))
  EXBPlacementRotationMode RotationMode = EXBPlacementRotationMode::Manual;

  /** 是否可移动 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置",
            meta = (DisplayName = "允许移动"))
  bool bAllowMove = true;

  /** 连续放置模式（放置后自动继续预览同类型 Actor，右键取消预览） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置",
            meta = (DisplayName = "连续放置"))
  bool bContinuousPlacement = false;

  /** 生成位置偏移（相对于放置点） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置",
            meta = (DisplayName = "位置偏移"))
  FVector LocationOffset = FVector::ZeroVector;

  /** 适用地图标签（仅当设置后有效，为空时适用所有地图） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置",
            meta = (DisplayName = "适用地图", Categories = "Map"))
  FGameplayTagContainer ApplicableMaps;

  /** 是否需要放置前配置（主将类型） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置",
            meta = (DisplayName = "放置前配置"))
  bool bRequiresConfig = false;

  /** 配置界面 Widget 类（仅 bRequiresConfig=true 时使用） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置",
            meta = (DisplayName = "配置界面类",
                    EditCondition = "bRequiresConfig"))
  TSubclassOf<UUserWidget> ConfigWidgetClass;

  // ============ 批量放置配置 ============

  /** 是否支持批量网格放置（环境场景等） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置|批量",
            meta = (DisplayName = "批量放置"))
  bool bBatchPlacement = false;

  /** 批量放置网格尺寸（X行 × Y列） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置|批量",
            meta = (DisplayName = "网格尺寸", EditCondition = "bBatchPlacement"))
  FIntPoint BatchGridSize = FIntPoint(5, 5);

  /** 批量放置间距（相邻 Actor 之间的距离） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置|批量",
            meta = (DisplayName = "网格间距", EditCondition = "bBatchPlacement", ClampMin = "10.0"))
  float BatchSpacing = 200.0f;

  FXBSpawnableActorEntry()
      : DefaultScale(FVector::OneVector),
        DefaultRotation(FRotator::ZeroRotator), bSnapToGround(true),
        RotationMode(EXBPlacementRotationMode::Manual), bAllowMove(true),
        bContinuousPlacement(false), LocationOffset(FVector::ZeroVector),
        bRequiresConfig(false), bBatchPlacement(false),
        BatchGridSize(FIntPoint(5, 5)), BatchSpacing(200.0f) {}
};

/**
 * @brief 过滤后的可放置条目（携带原始索引）
 * @note 用于 UI 显示过滤后的条目列表，同时保留原始索引用于调用 StartPreview
 */
USTRUCT(BlueprintType)
struct FXBFilteredSpawnableEntry {
  GENERATED_BODY()

  /** 条目数据 */
  UPROPERTY(BlueprintReadOnly, Category = "放置配置")
  FXBSpawnableActorEntry Entry;

  /** 在原始数组中的索引（用于调用 StartPreview） */
  UPROPERTY(BlueprintReadOnly, Category = "放置配置")
  int32 OriginalIndex = -1;

  FXBFilteredSpawnableEntry() = default;
  
  FXBFilteredSpawnableEntry(const FXBSpawnableActorEntry& InEntry, int32 InIndex)
      : Entry(InEntry), OriginalIndex(InIndex) {}
};

/**
 * @brief 已放置 Actor 的运行时数据
 * @note 用于跟踪配置阶段放置的 Actor
 */
USTRUCT(BlueprintType)
struct FXBPlacedActorData {
  GENERATED_BODY()

  /** 放置的 Actor 引用 */
  UPROPERTY(BlueprintReadOnly, Category = "放置数据")
  TWeakObjectPtr<AActor> PlacedActor;

  /** 对应的配置条目索引 */
  UPROPERTY(BlueprintReadOnly, Category = "放置数据")
  int32 EntryIndex = -1;

  /** 原始 Actor 类路径（用于存档） */
  UPROPERTY(BlueprintReadOnly, Category = "放置数据")
  FSoftClassPath ActorClassPath;

  /** 放置位置 */
  UPROPERTY(BlueprintReadOnly, Category = "放置数据")
  FVector Location = FVector::ZeroVector;

  /** 放置旋转 */
  UPROPERTY(BlueprintReadOnly, Category = "放置数据")
  FRotator Rotation = FRotator::ZeroRotator;

  /** 放置缩放 */
  UPROPERTY(BlueprintReadWrite, Category = "放置数据")
  FVector Scale = FVector::OneVector;

  /** 主将配置数据（仅主将类型有效） */
  UPROPERTY(BlueprintReadWrite, Category = "放置数据",
            meta = (DisplayName = "主将配置数据"))
  FXBLeaderSpawnConfigData LeaderConfigData;

  /** 是否包含主将配置 */
  UPROPERTY(BlueprintReadWrite, Category = "放置数据",
            meta = (DisplayName = "包含主将配置"))
  bool bHasLeaderConfig = false;
};

// ============ 放置存档数据 ============

/**
 * @brief 放置存档数据
 * @note 用于保存配置阶段放置的 Actor 数据
 */
USTRUCT(BlueprintType)
struct FXBPlacementSaveData {
  GENERATED_BODY()

  /** 存档槽位名称（用于文件系统） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置存档",
            meta = (DisplayName = "槽位名称"))
  FString SaveName;

  /** 存档显示名称（用户自定义，用于 UI 显示） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置存档",
            meta = (DisplayName = "显示名称"))
  FString DisplayName;

  /** 存档时间 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置存档",
            meta = (DisplayName = "存档时间"))
  FDateTime SaveTime;

  /** 已放置的 Actor 数据 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置存档",
            meta = (DisplayName = "放置数据"))
  TArray<FXBPlacedActorData> PlacedActors;
};

/**
 * @brief 放置存档类
 * @note 专门用于保存放置数据的存档
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBPlacementSaveGame : public USaveGame {
  GENERATED_BODY()

public:
  /** 放置存档列表 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Save",
            meta = (DisplayName = "放置存档列表"))
  TArray<FXBPlacementSaveData> PlacementSaves;

  /** 存档槽位名称列表 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Save",
            meta = (DisplayName = "槽位名称列表"))
  TArray<FString> SlotNames;
};
