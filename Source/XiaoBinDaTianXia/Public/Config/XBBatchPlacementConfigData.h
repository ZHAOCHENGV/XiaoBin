/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Config/XBBatchPlacementConfigData.h

/**
 * @file XBBatchPlacementConfigData.h
 * @brief 批量放置配置数据结构
 *
 * @note ✨ 新增文件 - 用于配置阶段批量放置时的参数配置
 */

#pragma once

#include "CoreMinimal.h"
#include "XBBatchPlacementConfigData.generated.h"

/**
 * @brief 批量放置配置数据
 * @note 用于配置阶段批量放置环境场景时的属性配置
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBBatchPlacementConfigData {
  GENERATED_BODY()

  /** 网格尺寸（X行 × Y列） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "批量配置",
            meta = (DisplayName = "网格尺寸", ClampMin = "1", ClampMax = "20"))
  FIntPoint GridSize = FIntPoint(5, 5);

  /** 网格间距（相邻 Actor 之间的距离） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "批量配置",
            meta = (DisplayName = "网格间距", ClampMin = "10.0", ClampMax = "2000.0"))
  float Spacing = 200.0f;

  /** 是否启用随机旋转（每个 Actor 随机 Yaw） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "批量配置",
            meta = (DisplayName = "随机旋转"))
  bool bRandomRotation = false;

  /** 是否启用随机缩放（在范围内随机） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "批量配置",
            meta = (DisplayName = "随机缩放"))
  bool bRandomScale = false;

  /** 随机缩放范围（最小/最大） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "批量配置",
            meta = (DisplayName = "缩放范围", EditCondition = "bRandomScale"))
  FVector2D ScaleRange = FVector2D(0.8f, 1.2f);
};
