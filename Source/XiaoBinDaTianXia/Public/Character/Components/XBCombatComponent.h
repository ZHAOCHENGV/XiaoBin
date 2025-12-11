/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Character/Components/XBCombatComponent.h

/**
 * @file XBCombatComponent.h
 * @brief 战斗组件 - 管理角色攻击和技能
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
 * @brief 战斗组件
 * @note 管理角色的普攻和技能系统
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent, DisplayName = "XB战斗组件"))
class XIAOBINDATIANXIA_API UXBCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UXBCombatComponent();

    /**
     * @brief 从数据表初始化
     * @param DataTable 数据表资源
     * @param RowName 行名称
     */
    UFUNCTION(BlueprintCallable, Category = "战斗")
    void InitializeFromDataTable(UDataTable* DataTable, FName RowName);

    /**
     * @brief 执行普通攻击
     * @return 是否成功触发
     */
    UFUNCTION(BlueprintCallable, Category = "战斗")
    bool PerformBasicAttack();

    /**
     * @brief 执行技能攻击
     * @return 是否成功触发
     */
    UFUNCTION(BlueprintCallable, Category = "战斗")
    bool PerformSpecialSkill();

    /**
     * @brief 重置攻击状态
     */
    UFUNCTION(BlueprintCallable, Category = "战斗")
    void ResetAttackState();

    /**
     * @brief 获取是否正在攻击
     */
    UFUNCTION(BlueprintPure, Category = "战斗")
    bool IsAttacking() const { return bIsAttacking; }

    /**
     * @brief 获取普攻冷却剩余时间
     */
    UFUNCTION(BlueprintPure, Category = "战斗")
    float GetBasicAttackCooldownRemaining() const { return BasicAttackCooldownTimer; }

    /**
     * @brief 获取技能冷却剩余时间
     */
    UFUNCTION(BlueprintPure, Category = "战斗")
    float GetSkillCooldownRemaining() const { return SkillCooldownTimer; }

    /**
     * @brief 检查普攻是否在冷却中
     */
    UFUNCTION(BlueprintPure, Category = "战斗")
    bool IsBasicAttackOnCooldown() const { return BasicAttackCooldownTimer > 0.0f; }

    /**
     * @brief 检查技能是否在冷却中
     */
    UFUNCTION(BlueprintPure, Category = "战斗")
    bool IsSkillOnCooldown() const { return SkillCooldownTimer > 0.0f; }

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /**
     * @brief 播放蒙太奇
     * @param Montage 蒙太奇资源
     * @param PlayRate 播放速率
     * @return 是否成功播放
     */
    bool PlayMontage(UAnimMontage* Montage, float PlayRate = 1.0f);

    /**
     * @brief 蒙太奇结束回调
     */
    UFUNCTION()
    void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    /**
     * @brief 激活GA
     * @param AbilityClass GA类
     * @return 是否成功激活
     */
    bool TryActivateAbility(TSubclassOf<UGameplayAbility> AbilityClass);

public:
    /** @brief 普攻配置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "普攻配置"))
    FXBAbilityConfig BasicAttackConfig;

    /** @brief 技能配置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "技能配置"))
    FXBAbilityConfig SpecialSkillConfig;

protected:
    /** @brief 是否正在攻击 */
    UPROPERTY(BlueprintReadOnly, Category = "状态")
    bool bIsAttacking = false;

    // ✨ 新增 - 独立的冷却计时器
    /** @brief 普攻冷却计时器 */
    UPROPERTY(BlueprintReadOnly, Category = "状态")
    float BasicAttackCooldownTimer = 0.0f;

    /** @brief 技能冷却计时器 */
    UPROPERTY(BlueprintReadOnly, Category = "状态")
    float SkillCooldownTimer = 0.0f;

    /** @brief 缓存的ASC引用 */
    UPROPERTY()
    TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

    /** @brief 缓存的动画实例 */
    UPROPERTY()
    TWeakObjectPtr<UAnimInstance> CachedAnimInstance;

    /** @brief 已加载的普攻蒙太奇 */
    UPROPERTY()
    TObjectPtr<UAnimMontage> LoadedBasicAttackMontage;

    /** @brief 已加载的技能蒙太奇 */
    UPROPERTY()
    TObjectPtr<UAnimMontage> LoadedSkillMontage;

private:
    /** @brief 是否已初始化 */
    bool bInitialized = false;


public:
    // ✨ 新增 - 范围缩放支持
    /**
     * @brief 设置攻击范围缩放倍率
     * @param ScaleMultiplier 缩放倍率
     */
    UFUNCTION(BlueprintCallable, Category = "战斗", meta = (DisplayName = "设置攻击范围缩放"))
    void SetAttackRangeScale(float ScaleMultiplier);

    /**
     * @brief 获取缩放后的攻击范围
     * @return 实际攻击范围
     */
    UFUNCTION(BlueprintPure, Category = "战斗", meta = (DisplayName = "获取缩放后攻击范围"))
    float GetScaledAttackRange() const;

    /**
     * @brief 检查目标是否在攻击范围内（考虑缩放）
     * @param Target 目标Actor
     * @return 是否在范围内
     */
    UFUNCTION(BlueprintCallable, Category = "战斗", meta = (DisplayName = "目标在攻击范围内"))
    bool IsTargetInRange(AActor* Target) const;

protected:
    // ✨ 新增 - 攻击范围缩放倍率
    UPROPERTY(BlueprintReadOnly, Category = "战斗")
    float AttackRangeScaleMultiplier = 1.0f;

    // ✨ 新增 - 基础攻击范围（从配置读取）
    UPROPERTY(BlueprintReadOnly, Category = "战斗")
    float BaseAttackRange = 150.0f;



};
