// Fill out your copyright notice in the Description page of Project Settings.


#include "Public/Game/XBGameMode.h"
#include "Player/XBPlayerController.h"
#include "Character/XBPlayerCharacter.h"
#include "Character/XBConfigCameraPawn.h"
#include "Kismet/GameplayStatics.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Soldier/Component/XBSoldierPoolSubsystem.h"
#include "Utils/XBLogCategories.h"

AXBGameMode::AXBGameMode()
{
	// 设置默认类
	DefaultPawnClass = AXBConfigCameraPawn::StaticClass();
	PlayerControllerClass = AXBPlayerController::StaticClass();
	PlayerLeaderClass = AXBPlayerCharacter::StaticClass();
	// 默认配置
	PoolWarmupCount = 100;
	bAsyncWarmup = true;
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

/**
 * @brief 生成玩家主将并切换控制
 * @param PlayerController 玩家控制器
 * @return 是否生成成功
 * @note   详细流程分析: 获取控制器Pawn -> 以当前位置信息生成主将 -> 切换控制 -> 进入游戏阶段
 *         性能/架构注意事项: 仅在配置阶段调用，避免重复生成
 */
bool AXBGameMode::SpawnPlayerLeader(APlayerController* PlayerController)
{
	// 🔧 修改 - 仅配置阶段允许生成
	if (!bIsConfigPhase)
	{
		return false;
	}

	if (!PlayerController)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// 🔧 修改 - 默认使用玩家主将类，未配置则回退到玩家角色类
	TSubclassOf<AXBPlayerCharacter> SpawnClass = PlayerLeaderClass;
	if (!SpawnClass)
	{
		SpawnClass = AXBPlayerCharacter::StaticClass();
	}

	APawn* CurrentPawn = PlayerController->GetPawn();
	const FVector SpawnLocation = CurrentPawn ? CurrentPawn->GetActorLocation() : FVector::ZeroVector;
	const FRotator SpawnRotation = CurrentPawn ? CurrentPawn->GetActorRotation() : FRotator::ZeroRotator;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnParams.Owner = PlayerController;

	AXBPlayerCharacter* NewLeader = World->SpawnActor<AXBPlayerCharacter>(SpawnClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (!NewLeader)
	{
		UE_LOG(LogXBCharacter, Warning, TEXT("生成玩家主将失败"));
		return false;
	}

	// 🔧 修改 - 切换控制器到主将
	PlayerController->Possess(NewLeader);
	UE_LOG(LogXBCharacter, Log, TEXT("已生成玩家主将: %s"), *NewLeader->GetName());

	EnterPlayPhase();
	return true;
}

void AXBGameMode::PauseGame()
{
	UGameplayStatics::SetGamePaused(GetWorld(), true);
}

void AXBGameMode::ResumeGame()
{
	UGameplayStatics::SetGamePaused(GetWorld(), false);
}

void AXBGameMode::InitializeSoldierPool()
{
}
