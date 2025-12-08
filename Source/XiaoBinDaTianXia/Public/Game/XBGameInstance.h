// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "XBGameInstance.generated.h"

class UXBSaveGame;

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
	UFUNCTION(BlueprintCallable, Category = "XB|Save")
	bool SaveGameConfig(int32 SlotIndex = 0);

	/** 加载游戏配置 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save")
	bool LoadGameConfig(int32 SlotIndex = 0);

	/** 保存场景摆放 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save")
	bool SaveSceneLayout(int32 SlotIndex = 0);

	/** 加载场景摆放 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save")
	bool LoadSceneLayout(int32 SlotIndex = 0);

	/** 获取当前存档 */
	UFUNCTION(BlueprintCallable, Category = "XB|Save")
	UXBSaveGame* GetCurrentSaveGame() const { return CurrentSaveGame; }

	// ============ 全局配置 ============

	/** 玩家自定义名称 */
	UPROPERTY(BlueprintReadWrite, Category = "XB|Config")
	FString PlayerCustomName = TEXT("Player");

	/** 假人自定义名称 */
	UPROPERTY(BlueprintReadWrite, Category = "XB|Config")
	TArray<FString> DummyCustomNames;

protected:
	UPROPERTY()
	TObjectPtr<UXBSaveGame> CurrentSaveGame;

private:
	void InitializeGameplayTags();
};