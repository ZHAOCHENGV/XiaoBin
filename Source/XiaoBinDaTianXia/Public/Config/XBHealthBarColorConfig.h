/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Config/XBHealthBarColorConfig.h

/**
 * @file XBHealthBarColorConfig.h
 * @brief 血条颜色配置数据资产
 *
 * @note ✨ 新增文件 - 用于定义可选的血条颜色列表
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "XBHealthBarColorConfig.generated.h"

/**
 * @brief 血条颜色条目
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBHealthBarColorEntry {
  GENERATED_BODY()

  /** 颜色名称（用于 UI 显示和存档） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "颜色",
            meta = (DisplayName = "颜色名称"))
  FText ColorName;

  /** 颜色值 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "颜色",
            meta = (DisplayName = "颜色值"))
  FLinearColor ColorValue = FLinearColor::Green;
};

/**
 * @brief 血条颜色配置数据资产
 * @note 在编辑器中创建此资产，定义可选的血条颜色列表
 */
UCLASS(BlueprintType)
class XIAOBINDATIANXIA_API UXBHealthBarColorConfig : public UPrimaryDataAsset {
  GENERATED_BODY()

public:
  /** 颜色列表 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置",
            meta = (DisplayName = "颜色列表"))
  TArray<FXBHealthBarColorEntry> Colors;

  /**
   * @brief 根据索引获取颜色
   * @param Index 索引
   * @return 颜色值，索引无效时返回绿色
   */
  UFUNCTION(BlueprintCallable, Category = "XB|颜色",
            meta = (DisplayName = "根据索引获取颜色"))
  FLinearColor GetColorByIndex(int32 Index) const;

  /**
   * @brief 根据名称获取颜色
   * @param ColorName 颜色名称
   * @param OutColor 输出颜色
   * @return 是否找到
   */
  UFUNCTION(BlueprintCallable, Category = "XB|颜色",
            meta = (DisplayName = "根据名称获取颜色"))
  bool GetColorByName(const FString& ColorName, FLinearColor& OutColor) const;

  /**
   * @brief 获取颜色名称列表（供 UI 下拉框使用）
   * @return 名称数组
   */
  UFUNCTION(BlueprintCallable, Category = "XB|颜色",
            meta = (DisplayName = "获取颜色名称列表"))
  TArray<FText> GetColorNames() const;

  /**
   * @brief 获取颜色名称字符串列表（用于存档）
   * @return 名称字符串数组
   */
  UFUNCTION(BlueprintCallable, Category = "XB|颜色",
            meta = (DisplayName = "获取颜色名称字符串列表"))
  TArray<FString> GetColorNameStrings() const;
};
