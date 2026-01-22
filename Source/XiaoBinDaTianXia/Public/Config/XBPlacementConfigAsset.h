/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Config/XBPlacementConfigAsset.h

/**
 * @file XBPlacementConfigAsset.h
 * @brief 放置系统配置 DataAsset
 * 
 * @note ✨ 新增文件 - 集中管理可放置 Actor 列表
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "XBPlacementTypes.h"
#include "XBPlacementConfigAsset.generated.h"

class UMaterialInterface;

/**
 * @brief 放置系统配置 DataAsset
 * @note 用于在编辑器中配置可放置的 Actor 列表
 */
UCLASS(BlueprintType)
class XIAOBINDATIANXIA_API UXBPlacementConfigAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// ============ 可放置 Actor 配置 ============

	/** 可放置 Actor 列表 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置", meta = (DisplayName = "可放置Actor列表", TitleProperty = "DisplayName"))
	TArray<FXBSpawnableActorEntry> SpawnableActors;

	// ============ 预览效果配置 ============

	/** 预览材质（半透明） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "预览效果", meta = (DisplayName = "预览材质"))
	TSoftObjectPtr<UMaterialInterface> PreviewMaterial;

	/** 预览有效颜色（可放置时） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "预览效果", meta = (DisplayName = "有效预览颜色"))
	FLinearColor ValidPreviewColor = FLinearColor(0.0f, 1.0f, 0.0f, 0.5f);

	/** 预览无效颜色（不可放置时） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "预览效果", meta = (DisplayName = "无效预览颜色"))
	FLinearColor InvalidPreviewColor = FLinearColor(1.0f, 0.0f, 0.0f, 0.5f);

	// ============ 选中效果配置 ============

	/** 选中高亮材质 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "选中效果", meta = (DisplayName = "选中高亮材质"))
	TSoftObjectPtr<UMaterialInterface> SelectionHighlightMaterial;

	/** 选中高亮颜色 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "选中效果", meta = (DisplayName = "选中高亮颜色"))
	FLinearColor SelectionColor = FLinearColor(0.0f, 0.5f, 1.0f, 1.0f);

	// ============ 操作配置 ============

	/** 旋转速度（度/滚轮单位） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "操作配置", meta = (DisplayName = "旋转速度", ClampMin = "1.0", ClampMax = "90.0"))
	float RotationSpeed = 15.0f;

	/** 射线检测距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "操作配置", meta = (DisplayName = "射线检测距离", ClampMin = "1000.0"))
	float TraceDistance = 50000.0f;

	/** 地面检测偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "操作配置", meta = (DisplayName = "地面检测偏移", ClampMin = "0.0"))
	float GroundTraceOffset = 500.0f;

public:
	/**
	 * @brief 根据索引获取可放置 Actor 配置
	 * @param Index 索引
	 * @param OutEntry 输出配置条目
	 * @return 是否获取成功
	 */
	UFUNCTION(BlueprintCallable, Category = "放置配置", meta = (DisplayName = "获取放置条目"))
	bool GetEntryByIndex(int32 Index, FXBSpawnableActorEntry& OutEntry) const;

	/**
	 * @brief 根据索引获取可放置 Actor 配置（C++ 内部使用）
	 * @param Index 索引
	 * @return 配置条目指针，无效索引返回 nullptr
	 */
	const FXBSpawnableActorEntry* GetEntryByIndexPtr(int32 Index) const;

	/**
	 * @brief 获取可放置 Actor 数量
	 * @return 数量
	 */
	UFUNCTION(BlueprintCallable, Category = "放置配置", meta = (DisplayName = "获取条目数量"))
	int32 GetEntryCount() const { return SpawnableActors.Num(); }
};
