// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Save/XBSaveGame.h"
#include "XBGameInstance.generated.h"

class UXBSaveGame;
class AXBCharacterBase;

/**
 * 游戏实例
 * 负责全局配置、存档管理、跨关卡数据持久化
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UXBGameInstance();

	virtual void Init() override;
	virtual void Shutdown() override;

	// ============ 存档系统 ============
    
	/**
	 * @brief  保存游戏配置
	 * @param  SlotIndex 存档槽位索引
	 * @return 是否保存成功
	 * @note   详细流程分析: 校验槽位索引 -> 获取存档对象 -> 写入磁盘
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save", meta = (DisplayName = "保存游戏配置"))
	bool SaveGameConfig(int32 SlotIndex = 0);

	/**
	 * @brief  加载游戏配置
	 * @param  SlotIndex 存档槽位索引
	 * @return 是否加载成功
	 * @note   详细流程分析: 校验槽位索引 -> 查找存档 -> 读取并绑定
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save", meta = (DisplayName = "加载游戏配置"))
	bool LoadGameConfig(int32 SlotIndex = 0);

	/**
	 * @brief  保存场景摆放
	 * @param  SlotIndex 存档槽位索引
	 * @return 是否保存成功
	 * @note   详细流程分析: 预留接口，后续扩展场景摆放数据保存
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save", meta = (DisplayName = "保存场景摆放"))
	bool SaveSceneLayout(int32 SlotIndex = 0);

	/**
	 * @brief  加载场景摆放
	 * @param  SlotIndex 存档槽位索引
	 * @return 是否加载成功
	 * @note   详细流程分析: 预留接口，后续扩展场景摆放数据加载
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save", meta = (DisplayName = "加载场景摆放"))
	bool LoadSceneLayout(int32 SlotIndex = 0);

	/**
	 * @brief  获取配置存档槽位上限
	 * @return 最大槽位数
	 * @note   详细流程分析: 用于 UI 或系统侧构建存档列表
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save", meta = (DisplayName = "获取配置存档槽位上限"))
	int32 GetMaxConfigSaveSlots() const;

	/**
	 * @brief  获取可用的配置存档槽位索引
	 * @return 槽位索引列表（默认返回 1~Max）
	 * @note   详细流程分析: 用于 UI 显示可选择的存档列表
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save", meta = (DisplayName = "获取配置存档槽位列表"))
	TArray<int32> GetAllConfigSlotIndices() const;

	/**
	 * @brief  判断配置存档是否存在
	 * @param  SlotIndex 存档槽位索引
	 * @return 是否存在
	 * @note   详细流程分析: 校验槽位索引 -> 查询磁盘
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save", meta = (DisplayName = "判断配置存档是否存在"))
	bool DoesGameConfigExist(int32 SlotIndex) const;

	/** 获取当前存档 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save", meta = (DisplayName = "获取当前存档"))
	UXBSaveGame* GetCurrentSaveGame() const { return CurrentSaveGame; }

	// ============ 全局配置 ============
	
	/** 获取游戏配置 */
	UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "获取游戏配置"))
	FXBGameConfigData GetGameConfig() const;

	/** 设置游戏配置 */
	UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "设置游戏配置"))
	void SetGameConfig(const FXBGameConfigData& NewConfig, bool bSaveToDisk = true);

	/** 重置游戏配置 */
	UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "重置游戏配置"))
	void ResetGameConfigToDefault(bool bSaveToDisk = true);

	/** 应用配置到主将 */
	UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "应用配置到主将"))
	void ApplyGameConfigToLeader(AXBCharacterBase* Leader, bool bApplyToSoldiers = true);

	/** 加载配置地图 */
	UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "加载配置地图"))
	bool LoadSelectedMap();

protected:
	UPROPERTY()
	TObjectPtr<UXBSaveGame> CurrentSaveGame;

private:
	void InitializeGameplayTags();
	void EnsureSaveGameInstance();
	bool IsValidConfigSlotIndex(int32 SlotIndex) const;
	FString BuildConfigSlotName(int32 SlotIndex) const;

	static constexpr int32 MaxConfigSaveSlots = 5;
};
