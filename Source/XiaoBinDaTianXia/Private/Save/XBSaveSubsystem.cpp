// Copyright XiaoBing Project. All Rights Reserved.

#include "Save/XBSaveSubsystem.h"
#include "Save/XBSaveGame.h"
#include "Kismet/GameplayStatics.h"

void UXBSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 尝试加载默认存档
    if (DoesSaveGameExist(TEXT("Default"), 0))
    {
        LoadGame(TEXT("Default"), 0);
    }
    else
    {
        CreateNewSaveGame();
    }

    UE_LOG(LogTemp, Log, TEXT("XBSaveSubsystem Initialized"));
}

void UXBSaveSubsystem::Deinitialize()
{
    // 自动保存
    if (CurrentSaveGame)
    {
        SaveGame(TEXT("AutoSave"), 0);
    }

    Super::Deinitialize();
}

bool UXBSaveSubsystem::SaveGame(const FString& SlotName, int32 UserIndex)
{
    if (!CurrentSaveGame)
    {
        UE_LOG(LogTemp, Warning, TEXT("XBSaveSubsystem::SaveGame - No current save game!"));
        return false;
    }

    FString FullSlotName = SaveSlotPrefix + SlotName;
    CurrentSaveGame->SaveSlotName = SlotName;
    CurrentSaveGame->SaveTime = FDateTime::Now();

    if (UGameplayStatics::SaveGameToSlot(CurrentSaveGame, FullSlotName, UserIndex))
    {
        UE_LOG(LogTemp, Log, TEXT("Game saved to slot: %s"), *FullSlotName);
        return true;
    }

    UE_LOG(LogTemp, Error, TEXT("Failed to save game to slot: %s"), *FullSlotName);
    return false;
}

bool UXBSaveSubsystem::LoadGame(const FString& SlotName, int32 UserIndex)
{
    FString FullSlotName = SaveSlotPrefix + SlotName;

    if (!UGameplayStatics::DoesSaveGameExist(FullSlotName, UserIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("Save game does not exist: %s"), *FullSlotName);
        return false;
    }

    UXBSaveGame* LoadedGame = Cast<UXBSaveGame>(
        UGameplayStatics::LoadGameFromSlot(FullSlotName, UserIndex));

    if (LoadedGame)
    {
        CurrentSaveGame = LoadedGame;
        UE_LOG(LogTemp, Log, TEXT("Game loaded from slot: %s"), *FullSlotName);
        return true;
    }

    UE_LOG(LogTemp, Error, TEXT("Failed to load game from slot: %s"), *FullSlotName);
    return false;
}

bool UXBSaveSubsystem::DeleteSaveGame(const FString& SlotName, int32 UserIndex)
{
    FString FullSlotName = SaveSlotPrefix + SlotName;

    if (UGameplayStatics::DeleteGameInSlot(FullSlotName, UserIndex))
    {
        UE_LOG(LogTemp, Log, TEXT("Save game deleted: %s"), *FullSlotName);
        return true;
    }

    return false;
}

bool UXBSaveSubsystem::DoesSaveGameExist(const FString& SlotName, int32 UserIndex) const
{
    FString FullSlotName = SaveSlotPrefix + SlotName;
    return UGameplayStatics::DoesSaveGameExist(FullSlotName, UserIndex);
}

TArray<FString> UXBSaveSubsystem::GetAllSaveSlotNames() const
{
    // TODO: 实现获取所有存档槽位名称
    // UE5 没有内置方法列举所有存档，需要自己维护一个列表
    TArray<FString> SlotNames;
    return SlotNames;
}

UXBSaveGame* UXBSaveSubsystem::CreateNewSaveGame()
{
    CurrentSaveGame = Cast<UXBSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UXBSaveGame::StaticClass()));

    return CurrentSaveGame;
}

void UXBSaveSubsystem::SetCurrentSaveGame(UXBSaveGame* SaveGame)
{
    CurrentSaveGame = SaveGame;
}