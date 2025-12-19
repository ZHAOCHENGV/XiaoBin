
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "XBGameMode.generated.h"

class AXBSoldierCharacter;
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
	// ==================== 对象池配置（保留用于未来扩展） ====================

	/**
	 * @brief 默认士兵类
	 * @note 用于动态生成士兵时（如掉落）
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XB|对象池", meta = (DisplayName = "默认士兵类"))
	TSubclassOf<AXBSoldierCharacter> DefaultSoldierClass;

	/**
	 * @brief 预热数量（当前未使用）
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XB|对象池", meta = (DisplayName = "预热数量", ClampMin = "0"))
	int32 PoolWarmupCount = 100;

	/**
	 * @brief 是否异步预热（当前未使用）
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XB|对象池", meta = (DisplayName = "异步预热"))
	bool bAsyncWarmup = true;

	/**
	 * @brief 初始化对象池（当前为空实现）
	 */
	void InitializeSoldierPool();
	
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