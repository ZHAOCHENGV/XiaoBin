/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Config/XBPlacementConfigAsset.cpp

/**
 * @file XBPlacementConfigAsset.cpp
 * @brief 放置系统配置 DataAsset 实现
 * 
 * @note ✨ 新增文件
 */

#include "Config/XBPlacementConfigAsset.h"

bool UXBPlacementConfigAsset::GetEntryByIndex(int32 Index, FXBSpawnableActorEntry& OutEntry) const
{
	// 校验索引有效性
	if (Index < 0 || Index >= SpawnableActors.Num())
	{
		return false;
	}

	OutEntry = SpawnableActors[Index];
	return true;
}

const FXBSpawnableActorEntry* UXBPlacementConfigAsset::GetEntryByIndexPtr(int32 Index) const
{
	// 校验索引有效性
	if (Index < 0 || Index >= SpawnableActors.Num())
	{
		return nullptr;
	}

	return &SpawnableActors[Index];
}
