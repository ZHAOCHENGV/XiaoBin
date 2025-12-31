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

	/** 保存场景摆放 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save", meta = (DisplayName = "保存场景摆放"))
	bool SaveSceneLayout(int32 SlotIndex = 0);

	/** 加载场景摆放 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save", meta = (DisplayName = "加载场景摆放"))
	bool LoadSceneLayout(int32 SlotIndex = 0);

	/** 获取当前存档 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save", meta = (DisplayName = "获取当前存档"))
	UXBSaveGame* GetCurrentSaveGame() const { return CurrentSaveGame; }

	// ============ 全局配置 ============

	/** 玩家自定义名称 */
	UPROPERTY(BlueprintReadWrite, Category = "XB|Config", meta = (DisplayName = "玩家自定义名称"))
	FString PlayerCustomName = TEXT("Player");

	/** 假人自定义名称 */
	UPROPERTY(BlueprintReadWrite, Category = "XB|Config", meta = (DisplayName = "假人自定义名称"))
	TArray<FString> DummyCustomNames;

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
};
