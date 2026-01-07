// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "XBCharacterBase.h"
#include "XBDummyCharacter.generated.h"

class AXBDummyAIController;

UCLASS()
class XIAOBINDATIANXIA_API AXBDummyCharacter : public AXBCharacterBase
{
	GENERATED_BODY()

public:

	AXBDummyCharacter();

protected:

	virtual void BeginPlay() override;

public:

	virtual void Tick(float DeltaTime) override;

	/**
	 * @brief  处理假人受击逻辑
	 * @param  DamageSource 伤害来源
	 * @param  DamageAmount 伤害数值
	 * @return 无
	 * @note   详细流程分析: 触发延迟响应 -> 由行为树任务执行攻击
	 *         性能/架构注意事项: 通过随机延迟削峰，避免同时释放导致性能抖动
	 */
	virtual void HandleDamageReceived(AActor* DamageSource, float DamageAmount) override;

	/**
	 * @brief  初始化主将数据（假人）
	 * @return 无
	 * @note   详细流程分析: 使用父类通用初始化，默认从 Actor 内部与数据表读取
	 */
	virtual void InitializeLeaderData() override;

	/**
	 * @brief  执行假人受击后的攻击逻辑
	 * @return 是否成功执行攻击
	 * @note   详细流程分析: 检查战斗组件 -> 判断冷却 -> 优先释放技能或普攻
	 *         性能/架构注意事项: 仅负责释放，不处理目标选择
	 */
	bool ExecuteDamageResponseAttack();

private:
	// ✨ 新增 - 假人受击响应延迟
	/**
	 * @brief 受击响应延迟最小值
	 * @note  用于随机范围下限，避免所有假人同步出手
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "受击响应延迟最小值", ClampMin = "0.0", AllowPrivateAccess = "true"))
	float DamageResponseDelayMin = 0.2f;

	/**
	 * @brief 受击响应延迟最大值
	 * @note  用于随机范围上限
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "受击响应延迟最大值", ClampMin = "0.0", AllowPrivateAccess = "true"))
	float DamageResponseDelayMax = 0.6f;

	// ✨ 新增 - 受击响应定时器
	FTimerHandle DamageResponseTimerHandle;

	// ✨ 新增 - 延迟后触发攻击请求
	/**
	 * @brief 延迟回调：通知AI执行攻击
	 * @return 无
	 * @note   详细流程分析: 获取AI控制器 -> 写入黑板触发攻击任务
	 *         性能/架构注意事项: 若AI控制器缺失则直接返回
	 */
	void TriggerDamageResponse();

};
