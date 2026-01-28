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

TArray<FXBSpawnableActorEntry> UXBPlacementConfigAsset::GetFilteredEntries(const FGameplayTag& CurrentMapTag) const
{
	TArray<FXBSpawnableActorEntry> FilteredEntries;

	for (const FXBSpawnableActorEntry& Entry : SpawnableActors)
	{
		// 如果 ApplicableMaps 为空，表示适用于所有地图
		if (Entry.ApplicableMaps.IsEmpty())
		{
			FilteredEntries.Add(Entry);
			continue;
		}

		// 检查当前地图标签是否在适用列表中
		if (CurrentMapTag.IsValid() && Entry.ApplicableMaps.HasTag(CurrentMapTag))
		{
			FilteredEntries.Add(Entry);
		}
	}

	return FilteredEntries;
}

