/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Core/XBLogCategories.h

/**
 * @file XBLogCategories.h
 * @brief 项目专用日志类别定义
 * 
 * @note ✨ 新增文件
 *       统一管理所有日志类别，便于过滤和调试
 */

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

// ============================================
// 日志类别声明
// ============================================

/** @brief 士兵系统日志 */
DECLARE_LOG_CATEGORY_EXTERN(LogXBSoldier, Log, All);

/** @brief 战斗系统日志 */
DECLARE_LOG_CATEGORY_EXTERN(LogXBCombat, Log, All);

/** @brief AI系统日志 */
DECLARE_LOG_CATEGORY_EXTERN(LogXBAI, Log, All);

/** @brief 编队系统日志 */
DECLARE_LOG_CATEGORY_EXTERN(LogXBFormation, Log, All);

/** @brief 招募系统日志 */
DECLARE_LOG_CATEGORY_EXTERN(LogXBRecruit, Log, All);

/** @brief 角色系统日志 */
DECLARE_LOG_CATEGORY_EXTERN(LogXBCharacter, Log, All);

/** @brief 玩家输入日志 */
DECLARE_LOG_CATEGORY_EXTERN(LogXBInput, Log, All);
