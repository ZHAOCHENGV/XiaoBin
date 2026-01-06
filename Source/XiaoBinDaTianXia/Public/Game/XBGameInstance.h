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
    
	/** 保存游戏配置 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save", meta = (DisplayName = "保存游戏配置"))
	bool SaveGameConfig(int32 SlotIndex = 0);

	/** 加载游戏配置 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save", meta = (DisplayName = "加载游戏配置"))
	bool LoadGameConfig(int32 SlotIndex = 0);

	/**
	 * @brief  使用自定义名称保存游戏配置
	 * @param  SlotName 存档名称
	 * @return 是否保存成功
	 * @note   详细流程分析: 保障存档对象存在 -> 以名称写入存档
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save", meta = (DisplayName = "保存游戏配置(名称)"))
	bool SaveGameConfigByName(const FString& SlotName);

	/**
	 * @brief  使用自定义名称加载游戏配置
	 * @param  SlotName 存档名称
	 * @return 是否加载成功
	 * @note   详细流程分析: 检查存档存在 -> 读取后更新当前存档对象
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save", meta = (DisplayName = "加载游戏配置(名称)"))
	bool LoadGameConfigByName(const FString& SlotName);

	/** 保存场景摆放 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save", meta = (DisplayName = "保存场景摆放"))
	bool SaveSceneLayout(int32 SlotIndex = 0);

	/** 加载场景摆放 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save", meta = (DisplayName = "加载场景摆放"))
	bool LoadSceneLayout(int32 SlotIndex = 0);

	/** 获取当前存档 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save", meta = (DisplayName = "获取当前存档"))
	UXBSaveGame* GetCurrentSaveGame() const { return CurrentSaveGame; }

	/**
	 * @brief  设置当前存档
	 * @param  NewSaveGame 新存档对象
	 * @return 无
	 * @note   详细流程分析: 允许外部系统切换存档实例以同步配置
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save", meta = (DisplayName = "设置当前存档"))
	void SetCurrentSaveGame(UXBSaveGame* NewSaveGame);

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
	void EnsureSaveGameInstance();
};
