// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "XBSaveSubsystem.generated.h"

class UXBSaveGame;
class UXBSaveSlotIndex;

/**
 * 存档子系统
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ============ 存档操作 ============

	/** 保存游戏 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save")
	bool SaveGame(const FString& SlotName, int32 UserIndex = 0);

	/** 加载游戏 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save")
	bool LoadGame(const FString& SlotName, int32 UserIndex = 0);

	/** 删除存档 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save")
	bool DeleteSaveGame(const FString& SlotName, int32 UserIndex = 0);

	/** 检查存档是否存在 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save")
	bool DoesSaveGameExist(const FString& SlotName, int32 UserIndex = 0) const;

	/** 获取所有存档槽位名称 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save", meta = (DisplayName = "获取全部存档槽位名称"))
	TArray<FString> GetAllSaveSlotNames() const;

	// ============ 当前存档访问 ============

	/** 获取当前存档 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save")
	UXBSaveGame* GetCurrentSaveGame() const { return CurrentSaveGame; }

	/** 创建新存档 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save")
	UXBSaveGame* CreateNewSaveGame();

	/** 设置当前存档 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save")
	void SetCurrentSaveGame(UXBSaveGame* SaveGame);

protected:
	/** 当前存档 */
	UPROPERTY(meta = (DisplayName = "当前存档"))
	TObjectPtr<UXBSaveGame> CurrentSaveGame;

	/** 存档槽位前缀 */
	UPROPERTY(EditDefaultsOnly, Category = "XB|Save", meta = (DisplayName = "存档槽位前缀"))
	FString SaveSlotPrefix = TEXT("XBSave_");

	/** 存档索引槽位名 */
	UPROPERTY(EditDefaultsOnly, Category = "XB|Save", meta = (DisplayName = "存档索引槽位名"))
	FString SaveSlotIndexName = TEXT("XBSaveIndex");

	/** 默认存档槽位数量 */
	UPROPERTY(EditDefaultsOnly, Category = "XB|Save", meta = (DisplayName = "默认存档槽位数量"))
	int32 DefaultSaveSlotCount = 0;

	/** 存档槽位索引 */
	UPROPERTY(meta = (DisplayName = "存档槽位索引"))
	TObjectPtr<UXBSaveSlotIndex> SaveSlotIndex;

	/** 缓存的地图名称（用于检测地图切换） */
	FString CachedMapName;

private:
	/**
	 * @brief  初始化存档槽位索引
	 * @note   确保存档槽位列表可用，并在首次运行时生成默认存档槽位。
	 */
	void InitializeSaveSlotIndex();

	/**
	 * @brief  保存存档槽位索引
	 * @note   将当前槽位列表持久化，便于后续列举。
	 */
	void SaveSlotIndexToDisk() const;

	/**
	 * @brief  构建完整存档槽位名称
	 * @param  SlotName 逻辑槽位名称
	 * @return 完整槽位名称
	 * @note   用于避免外部直接依赖前缀格式。
	 */
	FString BuildFullSlotName(const FString& SlotName) const;

	/**
	 * @brief  获取当前地图名称
	 * @return 当前地图名称（不含路径和后缀）
	 * @note   用于按场景分离存档数据
	 */
	FString GetCurrentMapName() const;

	/**
	 * @brief  获取地图特定的索引名称
	 * @return 包含地图名称的索引槽位名
	 * @note   确保每个地图有独立的存档列表
	 */
	FString GetMapSpecificIndexName() const;
};
