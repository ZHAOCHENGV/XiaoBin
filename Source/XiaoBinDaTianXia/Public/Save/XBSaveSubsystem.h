// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "XBSaveSubsystem.generated.h"

class UXBSaveGame;

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
	UFUNCTION(BlueprintCallable, Category = "XB|Save")
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
	UPROPERTY()
	TObjectPtr<UXBSaveGame> CurrentSaveGame;

	/** 存档槽位前缀 */
	UPROPERTY()
	FString SaveSlotPrefix = TEXT("XBSave_");
};