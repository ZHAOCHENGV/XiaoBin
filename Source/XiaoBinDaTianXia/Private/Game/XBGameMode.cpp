// Fill out your copyright notice in the Description page of Project Settings.


#include "Public/Game/XBGameMode.h"
#include "Player/XBPlayerController.h"
#include "Character/XBPlayerCharacter.h"
#include "Character/XBConfigCameraPawn.h"
#include "Components/CapsuleComponent.h"
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
	const FVector SpawnOrigin = CurrentPawn ? CurrentPawn->GetActorLocation() : FVector::ZeroVector;
	// 🔧 修改 - 使用控制器朝向作为主将朝向，避免PlayerStart朝向覆盖
	const FRotator SpawnRotation = PlayerController ? PlayerController->GetControlRotation() : FRotator::ZeroRotator;

	// 🔧 修改 - 向下检测地面，确保主将落地踩地
	FVector SpawnLocation = SpawnOrigin;
	float CapsuleHalfHeight = 0.0f;
	if (SpawnClass)
	{
		if (const AXBPlayerCharacter* LeaderCDO = SpawnClass->GetDefaultObject<AXBPlayerCharacter>())
		{
			if (const UCapsuleComponent* CapsuleComp = LeaderCDO->GetCapsuleComponent())
			{
				CapsuleHalfHeight = CapsuleComp->GetScaledCapsuleHalfHeight();
			}
		}
	}

	// ✨ 新增 - 使用射线检测，找到配置相机正下方的地面高度
	if (CurrentPawn)
	{
		FHitResult HitResult;
		const FVector TraceStart = SpawnOrigin + FVector(0.0f, 0.0f, 500.0f);
		const FVector TraceEnd = SpawnOrigin - FVector(0.0f, 0.0f, 5000.0f);
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(CurrentPawn);

		const bool bHit = World->LineTraceSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			ECC_WorldStatic,
			QueryParams
		);

		if (bHit)
		{
			// 🔧 修改 - 加上胶囊半高，保证角色底部落在地面
			SpawnLocation = HitResult.Location + FVector(0.0f, 0.0f, CapsuleHalfHeight);
		}
	}

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

	// 🔧 修改 - 同步主将与控制器朝向，确保继承配置相机最终朝向
	NewLeader->SetActorRotation(SpawnRotation);
	PlayerController->SetControlRotation(SpawnRotation);

	// 🔧 修改 - 生成后销毁配置相机Pawn，避免重复占用
	if (AXBConfigCameraPawn* ConfigPawn = Cast<AXBConfigCameraPawn>(CurrentPawn))
	{
		ConfigPawn->Destroy();
	}

	// 🔧 修改 - 切回主将视角时重置镜头旋转到背后
	if (AXBPlayerController* XBPlayerController = Cast<AXBPlayerController>(PlayerController))
	{
		XBPlayerController->ResetCameraAfterSpawnLeader();
	}

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
