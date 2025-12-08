// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "XBGameplayAbility.generated.h"

/**
 * 技能激活策略
 */
UENUM(BlueprintType)
enum class EXBAbilityActivationPolicy : uint8
{
	OnInputTriggered,   // 输入触发
	OnSpawn,            // 生成时自动激活
	OnGiven             // 获得时自动激活
};

/**
 * 自定义技能基类
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UXBGameplayAbility();

	/** 获取激活策略 */
	EXBAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }

	/** 获取当前缩放比例（用于技能范围调整） */
	UFUNCTION(BlueprintCallable, Category = "XB|Ability")
	float GetOwnerScale() const;

	/** 获取调整后的技能范围 */
	UFUNCTION(BlueprintCallable, Category = "XB|Ability")
	float GetScaledRadius(float BaseRadius) const;

protected:
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	/** 技能激活策略 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Ability")
	EXBAbilityActivationPolicy ActivationPolicy = EXBAbilityActivationPolicy::OnInputTriggered;

	/** 是否在命中时触发战斗状态 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Ability")
	bool bTriggerCombatOnHit = true;

	/** 基础冷却时间 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Ability")
	float BaseCooldown = 0.0f;

	/** 技能命中目标时调用（用于触发战斗状态） */
	UFUNCTION(BlueprintCallable, Category = "XB|Ability")
	void NotifyAbilityHit(AActor* HitTarget);

private:
	void TryActivateOnPolicy(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec);
};