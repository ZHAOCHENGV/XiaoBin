// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "XBSaveSlotIndex.generated.h"

/**
 * 存档槽位索引存档类
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBSaveSlotIndex : public USaveGame
{
	GENERATED_BODY()

public:
	UXBSaveSlotIndex();

	/** 存档槽位列表 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Save", meta = (DisplayName = "存档槽位列表"))
	TArray<FString> SlotNames;
};
