
#include "Save/XBSaveGame.h"

UXBSaveGame::UXBSaveGame()
{
	SaveSlotName = TEXT("DefaultSlot");
	SaveTime = FDateTime::Now();

	// 初始化默认假人名称
	DummyNames.Add(TEXT("Dummy1"));
	DummyNames.Add(TEXT("Dummy2"));
}