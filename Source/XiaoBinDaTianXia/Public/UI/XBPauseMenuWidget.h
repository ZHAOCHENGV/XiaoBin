// Copyright XiaoBing Project. All Rights Reserved.

/**
 * @file XBPauseMenuWidget.h
 * @brief 暂停菜单 Widget 头文件
 * 
 * 功能说明：
 * - 暂停游戏时显示的菜单界面
 * - 提供"继续游戏"、"返回主菜单"、"退出游戏"功能
 * - 按钮布局由蓝图子类实现，C++ 提供逻辑接口
 */

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "XBPauseMenuWidget.generated.h"

UCLASS()
class XIAOBINDATIANXIA_API UXBPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ==================== 菜单操作接口 ====================

	/**
	 * @brief 继续游戏（关闭菜单并取消暂停）
	 * @note  由蓝图按钮 OnClicked 事件调用
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|PauseMenu", meta = (DisplayName = "继续游戏"))
	void ResumeGame();

	/**
	 * @brief 返回主菜单（配置关卡）
	 * @note  取消暂停后加载配置关卡
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|PauseMenu", meta = (DisplayName = "返回主菜单"))
	void ReturnToMainMenu();

	/**
	 * @brief 退出游戏
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|PauseMenu", meta = (DisplayName = "退出游戏"))
	void QuitGame();

	// ==================== 蓝图事件 ====================

	/**
	 * @brief 菜单打开时触发（用于蓝图播放动画/音效）
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "XB|PauseMenu", meta = (DisplayName = "菜单打开时"))
	void OnMenuOpened();

	/**
	 * @brief 菜单关闭时触发（用于蓝图播放动画/音效）
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "XB|PauseMenu", meta = (DisplayName = "菜单关闭时"))
	void OnMenuClosed();

	// ==================== 配置 ====================

	/**
	 * @brief 主菜单关卡标签（如 Map.Lv_Config）
	 * @note  在蓝图子类中设置，用于“返回主菜单”功能
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XB|PauseMenu", meta = (DisplayName = "主菜单关卡标签", Categories = "Map"))
	FGameplayTag MainMenuMapTag;
};
