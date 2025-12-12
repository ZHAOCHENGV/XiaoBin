/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Debug/XBDebugSubsystem.h

/**
 * @file XBDebugSubsystem.h
 * @brief 调试子系统 - 提供全局调试控制
 * 
 * @note ✨ 新增文件
 *       功能：
 *       1. 全局开关士兵调试显示
 *       2. 控制台命令支持
 *       3. 运行时调试控制
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "XBDebugSubsystem.generated.h"

/**
 * @brief 调试子系统
 * @note 提供游戏范围的调试功能控制
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBDebugSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ==================== 士兵调试控制 ====================

	/**
	 * @brief 切换士兵调试显示
	 * @note 控制台命令: XB.Debug.Soldier.Toggle
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|Debug", meta = (DisplayName = "切换士兵调试"))
	void ToggleSoldierDebug();

	/**
	 * @brief 启用士兵调试显示
	 * @param bEnable 是否启用
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|Debug", meta = (DisplayName = "设置士兵调试"))
	void SetSoldierDebugEnabled(bool bEnable);

	/**
	 * @brief 获取士兵调试状态
	 */
	UFUNCTION(BlueprintPure, Category = "XB|Debug", meta = (DisplayName = "获取士兵调试状态"))
	bool IsSoldierDebugEnabled() const;

	// ==================== 编队调试控制 ====================

	/**
	 * @brief 切换编队调试显示
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|Debug", meta = (DisplayName = "切换编队调试"))
	void ToggleFormationDebug();

	/**
	 * @brief 设置编队调试
	 * @param bEnable 是否启用
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|Debug", meta = (DisplayName = "设置编队调试"))
	void SetFormationDebugEnabled(bool bEnable);

private:
	/** @brief 编队调试状态 */
	bool bFormationDebugEnabled = false;

	/** @brief 注册控制台命令 */
	void RegisterConsoleCommands();
};
