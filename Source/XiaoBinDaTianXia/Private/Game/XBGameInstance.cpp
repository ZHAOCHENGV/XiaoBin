// Copyright XiaoBing Project. All Rights Reserved.

#include "Game/XBGameInstance.h"
#include "Save/XBSaveGame.h"
#include "Character/XBCharacterBase.h"
#include "Character/XBPlayerCharacter.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Kismet/GameplayStatics.h"

UXBGameInstance::UXBGameInstance()
{
	
}

void UXBGameInstance::Init()
{
	Super::Init();

	// 🔧 修改 - NativeGameplayTags 已自动注册，无需手动初始化

	// 尝试加载默认存档
	if (!LoadGameConfig(0))
	{
		// 🔧 修改 - 没有存档时创建默认配置
		EnsureSaveGameInstance();
	}
}

void UXBGameInstance::Shutdown()
{
	Super::Shutdown();
}

bool UXBGameInstance::SaveGameConfig(int32 SlotIndex)
{
	const FString SlotName = FString::Printf(TEXT("XBConfig_%d"), SlotIndex);
	return SaveGameConfigByName(SlotName);
}

bool UXBGameInstance::LoadGameConfig(int32 SlotIndex)
{
	const FString SlotName = FString::Printf(TEXT("XBConfig_%d"), SlotIndex);
	return LoadGameConfigByName(SlotName);
}

bool UXBGameInstance::SaveGameConfigByName(const FString& SlotName)
{
	if (SlotName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("保存配置失败：SlotName 为空"));
		return false;
	}

	EnsureSaveGameInstance();

	if (!CurrentSaveGame)
	{
		UE_LOG(LogTemp, Warning, TEXT("保存配置失败：CurrentSaveGame 为空"));
		return false;
	}

	// 🔧 修改 - 使用自定义名称保存配置，以支持多存档插槽
	return UGameplayStatics::SaveGameToSlot(CurrentSaveGame, SlotName, 0);
}

bool UXBGameInstance::LoadGameConfigByName(const FString& SlotName)
{
	if (SlotName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("加载配置失败：SlotName 为空"));
		return false;
	}

	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		CurrentSaveGame = Cast<UXBSaveGame>(
			UGameplayStatics::LoadGameFromSlot(SlotName, 0));

		if (!CurrentSaveGame)
		{
			UE_LOG(LogTemp, Warning, TEXT("加载配置失败：存档对象无效"));
			return false;
		}

		return true;
	}

	// 🔧 修改 - 没有存档时创建默认配置
	EnsureSaveGameInstance();
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

FXBGameConfigData UXBGameInstance::GetGameConfig() const
{
	if (CurrentSaveGame)
	{
		return CurrentSaveGame->GameConfig;
	}

	// 🔧 修改 - 未加载存档时返回默认配置
	UXBSaveGame* DefaultSave = Cast<UXBSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UXBSaveGame::StaticClass()));
	return DefaultSave ? DefaultSave->GameConfig : FXBGameConfigData();
}

void UXBGameInstance::SetGameConfig(const FXBGameConfigData& NewConfig, bool bSaveToDisk)
{
	EnsureSaveGameInstance();
	if (!CurrentSaveGame)
	{
		UE_LOG(LogTemp, Warning, TEXT("设置配置失败：CurrentSaveGame 为空"));
		return;
	}

	CurrentSaveGame->GameConfig = NewConfig;

	if (bSaveToDisk)
	{
		SaveGameConfig(0);
	}
}

void UXBGameInstance::ResetGameConfigToDefault(bool bSaveToDisk)
{
	EnsureSaveGameInstance();
	if (!CurrentSaveGame)
	{
		UE_LOG(LogTemp, Warning, TEXT("重置配置失败：CurrentSaveGame 为空"));
		return;
	}

	UXBSaveGame* DefaultSave = Cast<UXBSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UXBSaveGame::StaticClass()));
	if (!DefaultSave)
	{
		UE_LOG(LogTemp, Warning, TEXT("重置配置失败：无法创建默认存档对象"));
		return;
	}

	CurrentSaveGame->GameConfig = DefaultSave->GameConfig;

	if (bSaveToDisk)
	{
		SaveGameConfig(0);
	}
}

void UXBGameInstance::ApplyGameConfigToLeader(AXBCharacterBase* Leader, bool bApplyToSoldiers)
{
	if (!Leader)
	{
		UE_LOG(LogTemp, Warning, TEXT("应用配置失败：Leader 为空"));
		return;
	}

	const FXBGameConfigData GameConfig = GetGameConfig();
	if (AXBPlayerCharacter* PlayerLeader = Cast<AXBPlayerCharacter>(Leader))
	{
		// 🔧 修改 - 仅玩家主将应用配置界面数据
		PlayerLeader->ApplyConfigFromGameConfig(GameConfig, true);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("应用配置跳过：目标非玩家主将 %s"), *Leader->GetName());
		return;
	}

	if (!bApplyToSoldiers)
	{
		return;
	}

	for (AXBSoldierCharacter* Soldier : Leader->GetSoldiers())
	{
		if (Soldier)
		{
			Soldier->ApplyRuntimeConfig(GameConfig);
		}
	}
}

bool UXBGameInstance::LoadSelectedMap()
{
	const FXBGameConfigData GameConfig = GetGameConfig();
	if (GameConfig.SelectedMapName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("加载地图失败：未配置地图选项"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("加载配置地图：%s"), *GameConfig.SelectedMapName.ToString());
	UGameplayStatics::OpenLevel(this, GameConfig.SelectedMapName);
	return true;
}

void UXBGameInstance::EnsureSaveGameInstance()
{
	if (!CurrentSaveGame)
	{
		CurrentSaveGame = Cast<UXBSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UXBSaveGame::StaticClass()));
	}
}
