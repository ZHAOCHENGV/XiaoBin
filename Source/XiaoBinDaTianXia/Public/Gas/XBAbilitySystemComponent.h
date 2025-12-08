// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "XBAbilitySystemComponent.generated.h"

/**
 * 自定义技能系统组件
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UXBAbilitySystemComponent();

	/** 通过 Tag 激活技能 */
	UFUNCTION(BlueprintCallable, Category = "XB|Ability")
	bool TryActivateAbilityByTag(const FGameplayTag& AbilityTag, bool bAllowRemoteActivation = true);

	/** 获取激活的技能中是否有指定 Tag */
	UFUNCTION(BlueprintCallable, Category = "XB|Ability")
	bool IsAbilityActiveByTag(const FGameplayTag& AbilityTag) const;

	/** 取消指定 Tag 的技能 */
	UFUNCTION(BlueprintCallable, Category = "XB|Ability")
	void CancelAbilityByTag(const FGameplayTag& AbilityTag);

	/** 添加初始技能（由角色调用） */
	void AddStartupAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities);

	/** 进入战斗状态 */
	UFUNCTION(BlueprintCallable, Category = "XB|Combat")
	void EnterCombat();

	/** 退出战斗状态 */
	UFUNCTION(BlueprintCallable, Category = "XB|Combat")
	void ExitCombat();

	/** 是否处于战斗状态 */
	UFUNCTION(BlueprintCallable, Category = "XB|Combat")
	bool IsInCombat() const;

protected:
	/** 初始技能是否已添加 */
	bool bStartupAbilitiesGiven = false;
};