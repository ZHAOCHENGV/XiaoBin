// Source/XiaoBinDaTianXia/Public/GAS/Executions/XBDamageExecutionCalculation.h
/* --- 完整文件代码 --- */

// Copyright XiaoBing Project. All Rights Reserved.

/**
 * @file XBDamageExecutionCalculation.h
 * @brief 伤害计算执行器
 * 
 * 功能说明：
 * - 从源属性集读取 BaseDamage 和 DamageMultiplier
 * - 计算最终伤害并应用到目标的 IncomingDamage 元属性
 * - 支持缩放系数影响伤害
 */

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "XBDamageExecutionCalculation.generated.h"

/**
 * @brief 伤害计算执行器
 * @note 此类在 GameplayEffect 中作为 Execution 使用，负责核心伤害公式计算
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBDamageExecutionCalculation : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	/**
	 * @brief 构造函数
	 * @note 在此声明需要捕获的属性
	 */
	UXBDamageExecutionCalculation();

	/**
	 * @brief 执行伤害计算
	 * @param ExecutionParams 执行参数，包含源和目标的ASC
	 * @param OutExecutionOutput 输出修改器
	 * @note 伤害公式: FinalDamage = (BaseDamage * DamageMultiplier * Scale) + SetByCallerDamage
	 */
	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
