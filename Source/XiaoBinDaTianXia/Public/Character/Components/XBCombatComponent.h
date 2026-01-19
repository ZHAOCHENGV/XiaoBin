/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Character/Components/XBCombatComponent.h

/**
 * @file XBCombatComponent.h
 * @brief 战斗组件 - 管理角色攻击和技能
 * 
 * @note 🔧 修改记录:
 *       1. ✨ 新增 攻击类型枚举
 *       2. ✨ 新增 当前攻击上下文追踪
 *       3. ✨ 新增 GetCurrentAttackDamage() 供近战检测获取伤害值
 *       4. ✨ 新增 攻击状态变化委托（用于禁用移动）
 *       5. ✨ 新增 IsAnyAttackMontagePlayingInternal() 蒙太奇互斥检查
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "Data/XBLeaderDataTable.h"
#include "XBCombatComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
class UAnimMontage;
class UDataTable;

/**
 * @brief 攻击类型
 * @note 用于近战检测确定使用哪个伤害配置
 */
UENUM(BlueprintType)
enum class EXBAttackType : uint8
{
    None            UMETA(DisplayName = "无"),
    BasicAttack     UMETA(DisplayName = "普通攻击"),
    SpecialSkill    UMETA(DisplayName = "特殊技能")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttackStateChanged, bool, bIsAttacking);

/**
 * @brief 战斗组件
 * @note 管理角色的普攻和技能系统
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent, DisplayName = "XB战斗组件"))
class XIAOBINDATIANXIA_API UXBCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UXBCombatComponent();

    UFUNCTION(BlueprintCallable, Category = "战斗")
    void InitializeFromDataTable(UDataTable* DataTable, FName RowName);

    UFUNCTION(BlueprintCallable, Category = "战斗")
    bool PerformBasicAttack();

    UFUNCTION(BlueprintCallable, Category = "战斗")
    bool PerformSpecialSkill();

    UFUNCTION(BlueprintCallable, Category = "战斗")
    void ResetAttackState();

    UFUNCTION(BlueprintPure, Category = "战斗")
    bool IsAttacking() const { return bIsAttacking; }

    UFUNCTION(BlueprintPure, Category = "战斗")
    float GetBasicAttackCooldownRemaining() const { return BasicAttackCooldownTimer; }

    UFUNCTION(BlueprintPure, Category = "战斗")
    float GetSkillCooldownRemaining() const { return SkillCooldownTimer; }

    UFUNCTION(BlueprintPure, Category = "战斗")
    bool IsBasicAttackOnCooldown() const { return BasicAttackCooldownTimer > 0.0f; }

    UFUNCTION(BlueprintPure, Category = "战斗")
    bool IsSkillOnCooldown() const { return SkillCooldownTimer > 0.0f; }

    UFUNCTION(BlueprintPure, Category = "战斗", meta = (DisplayName = "获取当前攻击类型"))
    EXBAttackType GetCurrentAttackType() const { return CurrentAttackType; }

    UFUNCTION(BlueprintPure, Category = "战斗", meta = (DisplayName = "获取当前攻击伤害"))
    float GetCurrentAttackDamage() const;

    UFUNCTION(BlueprintPure, Category = "战斗", meta = (DisplayName = "获取当前攻击最终伤害"))
    float GetCurrentAttackFinalDamage() const;

    UFUNCTION(BlueprintPure, Category = "战斗", meta = (DisplayName = "获取伤害倍率"))
    float GetDamageMultiplier() const;

    UPROPERTY(BlueprintAssignable, Category = "战斗", meta = (DisplayName = "攻击状态变化"))
    FOnAttackStateChanged OnAttackStateChanged;

    UFUNCTION(BlueprintPure, Category = "战斗", meta = (DisplayName = "是否禁止移动"))
    bool ShouldBlockMovement() const;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    bool PlayMontage(UAnimMontage* Montage, float PlayRate = 1.0f);

    UFUNCTION()
    void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    bool TryActivateAbility(TSubclassOf<UGameplayAbility> AbilityClass);

    void SetCurrentAttackType(EXBAttackType NewType);

    void SetAttackingState(bool bNewAttacking);

public:
    // ✨ 新增 - 蒙太奇互斥检查
    /**
     * @brief 检查是否有任何攻击/技能蒙太奇正在播放
     * @return 是否有蒙太奇正在播放
     * @note 用于阻止在动画播放期间触发新的攻击，或限制移动
     */
    bool IsAnyAttackMontagePlayingInternal() const;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "普攻配置"))
    FXBAbilityConfig BasicAttackConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "技能配置"))
    FXBAbilityConfig SpecialSkillConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "普攻时禁止移动"))
    bool bBlockMovementDuringBasicAttack = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "技能时禁止移动"))
    bool bBlockMovementDuringSkill = true;

protected:
    UPROPERTY(BlueprintReadOnly, Category = "状态")
    bool bIsAttacking = false;

    UPROPERTY(BlueprintReadOnly, Category = "状态")
    float BasicAttackCooldownTimer = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "状态")
    float SkillCooldownTimer = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "当前攻击类型"))
    EXBAttackType CurrentAttackType = EXBAttackType::None;

    UPROPERTY()
    TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

    UPROPERTY()
    TWeakObjectPtr<UAnimInstance> CachedAnimInstance;

    UPROPERTY()
    TObjectPtr<UAnimMontage> LoadedBasicAttackMontage;

    UPROPERTY()
    TObjectPtr<UAnimMontage> LoadedSkillMontage;

private:
    bool bInitialized = false;

public:
    UFUNCTION(BlueprintCallable, Category = "战斗", meta = (DisplayName = "设置攻击范围缩放"))
    void SetAttackRangeScale(float ScaleMultiplier);

    UFUNCTION(BlueprintPure, Category = "战斗", meta = (DisplayName = "获取缩放后攻击范围"))
    float GetScaledAttackRange() const;

    UFUNCTION(BlueprintCallable, Category = "战斗", meta = (DisplayName = "目标在攻击范围内"))
    bool IsTargetInRange(AActor* Target) const;

    // ✨ 新增 - 获取普攻攻击范围
    UFUNCTION(BlueprintPure, Category = "战斗", meta = (DisplayName = "获取普攻攻击范围"))
    float GetBasicAttackRange() const;

    // ✨ 新增 - 获取技能攻击范围
    UFUNCTION(BlueprintPure, Category = "战斗", meta = (DisplayName = "获取技能攻击范围"))
    float GetSkillAttackRange() const;

    // ✨ 新增 - 检查目标是否在普攻范围内
    UFUNCTION(BlueprintCallable, Category = "战斗", meta = (DisplayName = "目标在普攻范围内"))
    bool IsTargetInBasicAttackRange(AActor* Target) const;

    // ✨ 新增 - 检查目标是否在技能范围内
    UFUNCTION(BlueprintCallable, Category = "战斗", meta = (DisplayName = "目标在技能范围内"))
    bool IsTargetInSkillRange(AActor* Target) const;

protected:
    UPROPERTY(BlueprintReadOnly, Category = "战斗")
    float AttackRangeScaleMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "战斗")
    float BaseAttackRange = 150.0f;
};
