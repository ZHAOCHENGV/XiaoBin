// Fill out your copyright notice in the Description page of Project Settings.


#include "Public/Game/XBGameMode.h"
#include "Player/XBPlayerController.h"
#include "Character/XBPlayerCharacter.h"
#include "Kismet/GameplayStatics.h"

AXBGameMode::AXBGameMode()
{
	// 设置默认类
	DefaultPawnClass = AXBPlayerCharacter::StaticClass();
	PlayerControllerClass = AXBPlayerController::StaticClass();
}

void AXBGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
}

void AXBGameMode::StartPlay()
{
	Super::StartPlay();
}

void AXBGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 默认进入配置阶段
	EnterConfigPhase();
}

void AXBGameMode::EnterConfigPhase()
{
	bIsConfigPhase = true;
    
	// 暂停游戏逻辑但允许输入
	if (UWorld* World = GetWorld())
	{
		// 配置阶段：暂停 AI 和战斗逻辑
	}

	OnConfigPhaseStarted();
}

void AXBGameMode::EnterPlayPhase()
{
	bIsConfigPhase = false;

	OnPlayPhaseStarted();
}

void AXBGameMode::PauseGame()
{
	UGameplayStatics::SetGamePaused(GetWorld(), true);
}

void AXBGameMode::ResumeGame()
{
	UGameplayStatics::SetGamePaused(GetWorld(), false);
}