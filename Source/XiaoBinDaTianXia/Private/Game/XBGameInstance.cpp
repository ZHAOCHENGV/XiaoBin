// Copyright XiaoBing Project. All Rights Reserved.

#include "Game/XBGameInstance.h"
#include "Utils/XBGameplayTags.h"
#include "Save/XBSaveGame.h"
#include "Kismet/GameplayStatics.h"

UXBGameInstance::UXBGameInstance()
{
	DummyCustomNames.Add(TEXT("Dummy1"));
	DummyCustomNames.Add(TEXT("Dummy2"));
}

void UXBGameInstance::Init()
{
	Super::Init();

	// 初始化 GameplayTags
	InitializeGameplayTags();

	// 尝试加载默认存档
	LoadGameConfig(0);
}

void UXBGameInstance::Shutdown()
{
	Super::Shutdown();
}

void UXBGameInstance::InitializeGameplayTags()
{
	FXBGameplayTags::InitializeNativeTags();
}

bool UXBGameInstance::SaveGameConfig(int32 SlotIndex)
{
	if (!CurrentSaveGame)
	{
		CurrentSaveGame = Cast<UXBSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UXBSaveGame::StaticClass()));
	}

	if (CurrentSaveGame)
	{
		CurrentSaveGame->PlayerName = PlayerCustomName;
		CurrentSaveGame->DummyNames = DummyCustomNames;
        
		FString SlotName = FString::Printf(TEXT("XBConfig_%d"), SlotIndex);
		return UGameplayStatics::SaveGameToSlot(CurrentSaveGame, SlotName, 0);
	}

	return false;
}

bool UXBGameInstance::LoadGameConfig(int32 SlotIndex)
{
	FString SlotName = FString::Printf(TEXT("XBConfig_%d"), SlotIndex);
    
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		CurrentSaveGame = Cast<UXBSaveGame>(
			UGameplayStatics::LoadGameFromSlot(SlotName, 0));
        
		if (CurrentSaveGame)
		{
			PlayerCustomName = CurrentSaveGame->PlayerName;
			DummyCustomNames = CurrentSaveGame->DummyNames;
			return true;
		}
	}

	return false;
}

bool UXBGameInstance::SaveSceneLayout(int32 SlotIndex)
{
	// TODO: 实现场景摆放存档
	return false;
}

bool UXBGameInstance::LoadSceneLayout(int32 SlotIndex)
{
	// TODO: 实现场景摆放加载
	return false;
}