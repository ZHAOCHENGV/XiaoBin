/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/XBDummyAIType.h

/**
 * @file XBDummyAIType.h
 * @brief 假人主将AI相关枚举定义
 */

#pragma once

#include "CoreMinimal.h"
#include "XBDummyAIType.generated.h"

/**
 * @brief 假人主将选择的能力类型
 */
UENUM(BlueprintType)
enum class EXBDummyLeaderAbilityType : uint8
{
	None UMETA(DisplayName = "无"),
	BasicAttack UMETA(DisplayName = "普攻"),
	SpecialSkill UMETA(DisplayName = "技能")
};
