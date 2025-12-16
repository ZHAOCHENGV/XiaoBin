/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/GAS/XBAttributeSet.h

// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "XBAttributeSet.generated.h"

// 使用宏简化属性访问器定义
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 属性集 - 定义所有游戏属性
 * @note 🔧 修改 - 移除 BaseDamage 属性（现在由技能配置提供）
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UXBAttributeSet();

    // 属性复制
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 属性修改前的钳制
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

    // 属性修改后的处理
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    // ============ 核心属性 ============

    /** 当前血量 */
    UPROPERTY(BlueprintReadOnly, Category = "XB|Attribute|Health", ReplicatedUsing = OnRep_Health)
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UXBAttributeSet, Health)

    /** 最大血量 */
    UPROPERTY(BlueprintReadOnly, Category = "XB|Attribute|Health", ReplicatedUsing = OnRep_MaxHealth)
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UXBAttributeSet, MaxHealth)

    /** 生命值倍率 */
    UPROPERTY(BlueprintReadOnly, Category = "XB|Attribute|Health", ReplicatedUsing = OnRep_HealthMultiplier)
    FGameplayAttributeData HealthMultiplier;
    ATTRIBUTE_ACCESSORS(UXBAttributeSet, HealthMultiplier)

    // ❌ 删除 - BaseDamage（现在由技能配置提供）

    /** 伤害倍率 */
    UPROPERTY(BlueprintReadOnly, Category = "XB|Attribute|Damage", ReplicatedUsing = OnRep_DamageMultiplier)
    FGameplayAttributeData DamageMultiplier;
    ATTRIBUTE_ACCESSORS(UXBAttributeSet, DamageMultiplier)

    /** 移动速度 */
    UPROPERTY(BlueprintReadOnly, Category = "XB|Attribute|Movement", ReplicatedUsing = OnRep_MoveSpeed)
    FGameplayAttributeData MoveSpeed;
    ATTRIBUTE_ACCESSORS(UXBAttributeSet, MoveSpeed)

    /** 缩放比例（将领专属） */
    UPROPERTY(BlueprintReadOnly, Category = "XB|Attribute|Scale", ReplicatedUsing = OnRep_Scale)
    FGameplayAttributeData Scale;
    ATTRIBUTE_ACCESSORS(UXBAttributeSet, Scale)

    // ============ 元属性（不复制，用于计算） ============

    /** 受到的伤害（Meta Attribute） */
    UPROPERTY(BlueprintReadOnly, Category = "XB|Attribute|Meta")
    FGameplayAttributeData IncomingDamage;
    ATTRIBUTE_ACCESSORS(UXBAttributeSet, IncomingDamage)

    /** 受到的治疗（Meta Attribute） */
    UPROPERTY(BlueprintReadOnly, Category = "XB|Attribute|Meta")
    FGameplayAttributeData IncomingHealing;
    ATTRIBUTE_ACCESSORS(UXBAttributeSet, IncomingHealing)

protected:
    UFUNCTION()
    virtual void OnRep_Health(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    virtual void OnRep_HealthMultiplier(const FGameplayAttributeData& OldValue);

    // ❌ 删除 - OnRep_BaseDamage

    UFUNCTION()
    virtual void OnRep_DamageMultiplier(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    virtual void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    virtual void OnRep_Scale(const FGameplayAttributeData& OldValue);

private:
    void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
    void HandleHealthChanged(const FGameplayEffectModCallbackData& Data);
    void HandleScaleChanged(const FGameplayEffectModCallbackData& Data);
};
