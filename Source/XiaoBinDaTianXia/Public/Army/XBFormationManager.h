// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Army/XBSoldierTypes.h"
#include "XBFormationManager.generated.h"

/**
 * 编队管理器
 * 静态工具类，负责计算编队槽位
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBFormationManager : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 根据士兵总数计算编队维度
	 * @param TotalSoldiers 士兵总数
	 * @param OutColumns 输出列数
	 * @param OutRows 输出行数
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|Formation")
	static void CalculateFormationDimensions(int32 TotalSoldiers, int32& OutColumns, int32& OutRows);

	/**
	 * 计算指定槽位的本地偏移
	 * @param SlotIndex 槽位索引
	 * @param TotalSoldiers 士兵总数
	 * @param Config 编队配置（可选）
	 * @return 相对于将领的本地偏移向量
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|Formation")
	static FVector CalculateSlotOffset(int32 SlotIndex, int32 TotalSoldiers, const FXBFormationConfig& Config = FXBFormationConfig());

	/**
	 * 生成完整的编队槽位数组
	 * @param TotalSoldiers 士兵总数
	 * @param Config 编队配置
	 * @return 槽位数组
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|Formation")
	static TArray<FXBFormationSlot> GenerateFormationSlots(int32 TotalSoldiers, const FXBFormationConfig& Config = FXBFormationConfig());

	/**
	 * 验证槽位索引是否有效
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|Formation")
	static bool IsValidSlotIndex(int32 SlotIndex, int32 TotalSoldiers);

private:
	/**
	 * 从槽位索引计算行列位置
	 */
	static void SlotIndexToRowColumn(int32 SlotIndex, int32 TotalSoldiers, int32& OutRow, int32& OutColumn);
};