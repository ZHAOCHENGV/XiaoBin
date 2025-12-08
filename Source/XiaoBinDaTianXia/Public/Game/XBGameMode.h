
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "XBGameMode.generated.h"

/**
 * 游戏模式基类
 */
UCLASS()
class XIAOBINDATIANXIA_API AXBGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AXBGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;
	virtual void BeginPlay() override;

	// ============ 游戏流程控制 ============

	/** 进入配置阶段 */
	UFUNCTION(BlueprintCallable, Category = "XB|GameMode")
	void EnterConfigPhase();

	/** 进入游戏阶段 */
	UFUNCTION(BlueprintCallable, Category = "XB|GameMode")
	void EnterPlayPhase();

	/** 暂停游戏 */
	UFUNCTION(BlueprintCallable, Category = "XB|GameMode")
	void PauseGame();

	/** 恢复游戏 */
	UFUNCTION(BlueprintCallable, Category = "XB|GameMode")
	void ResumeGame();

	/** 当前是否在配置阶段 */
	UFUNCTION(BlueprintCallable, Category = "XB|GameMode")
	bool IsInConfigPhase() const { return bIsConfigPhase; }

protected:
	/** 是否处于配置阶段 */
	UPROPERTY(BlueprintReadOnly, Category = "XB|GameMode")
	bool bIsConfigPhase = true;

	/** 配置阶段开始事件 */
	UFUNCTION(BlueprintImplementableEvent, Category = "XB|GameMode")
	void OnConfigPhaseStarted();

	/** 游戏阶段开始事件 */
	UFUNCTION(BlueprintImplementableEvent, Category = "XB|GameMode")
	void OnPlayPhaseStarted();
};