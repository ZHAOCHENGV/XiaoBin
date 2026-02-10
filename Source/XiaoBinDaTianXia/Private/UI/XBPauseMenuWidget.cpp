// Copyright XiaoBing Project. All Rights Reserved.

/**
 * @file XBPauseMenuWidget.cpp
 * @brief 暂停菜单 Widget 实现文件
 */

#include "UI/XBPauseMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"

void UXBPauseMenuWidget::ResumeGame()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	// 通知蓝图菜单即将关闭
	OnMenuClosed();

	// 取消暂停
	UGameplayStatics::SetGamePaused(GetWorld(), false);

	// 恢复输入模式为仅游戏
	FInputModeGameOnly GameOnlyMode;
	PC->SetInputMode(GameOnlyMode);
	PC->bShowMouseCursor = false;

	// 从视口移除菜单
	RemoveFromParent();

	UE_LOG(LogTemp, Log, TEXT("[PauseMenu] 继续游戏"));
}

void UXBPauseMenuWidget::ReturnToMainMenu()
{
	// 先取消暂停，否则 OpenLevel 可能被阻止
	UGameplayStatics::SetGamePaused(GetWorld(), false);

	if (!MainMenuMapTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PauseMenu] 主菜单关卡标签未配置，无法返回"));
		return;
	}

	// 从标签提取关卡名（如 "Map.Lv_Config" → "Lv_Config"）
	FString TagName = MainMenuMapTag.ToString();
	static const FString MapPrefix = TEXT("Map.");
	if (TagName.StartsWith(MapPrefix))
	{
		TagName = TagName.RightChop(MapPrefix.Len());
	}

	UE_LOG(LogTemp, Log, TEXT("[PauseMenu] 返回主菜单: %s"), *TagName);
	UGameplayStatics::OpenLevel(GetWorld(), FName(*TagName));
}

void UXBPauseMenuWidget::QuitGame()
{
	UE_LOG(LogTemp, Log, TEXT("[PauseMenu] 退出游戏"));

	// 使用 KismetSystemLibrary 退出，支持编辑器和打包版本
	UKismetSystemLibrary::QuitGame(
		GetWorld(),
		GetOwningPlayer(),
		EQuitPreference::Quit,
		false
	);
}
