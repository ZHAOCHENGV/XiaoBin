// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Army/XBSoldierTypes.h"
#include "XBCharacterBase.generated.h"

class UXBAbilitySystemComponent;
class UXBAttributeSet;
class UGameplayEffect;
class UGameplayAbility;

/**
 * @brief 角色基类 (Character Base)
 * * 所有将领（包括玩家和 AI 假人）的基类。
 * * @details 集成了以下核心系统：
 * 1. GAS (Gameplay Ability System)：处理技能、属性（血量、伤害）和状态效果。
 * 2. 阵营系统：区分玩家、敌对势力和中立单位。
 * 3. 士兵管理：提供招募、召回士兵的接口，以及根据士兵数量动态改变自身属性。
 * 4. 隐身系统：处理进入草丛后的逻辑表现。
 */
UCLASS(Abstract)
class XIAOBINDATIANXIA_API AXBCharacterBase : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    /**
     * @brief 构造函数
     * * 初始化 GAS 组件、胶囊体和默认设置
     */
    AXBCharacterBase();

    // ============ IAbilitySystemInterface ============
    
    /**
     * @brief 获取技能系统组件 (ASC)
     * * @return UAbilitySystemComponent* 组件指针
     * 实现 IAbilitySystemInterface 接口，供系统自动查找 ASC
     */
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    // ============ AActor ============
    
    /**
     * @brief 游戏开始
     * * 初始化逻辑
     */
    virtual void BeginPlay() override;

    /**
     * @brief 被控制器接管时调用
     * * @param NewController 新的控制器
     * 在服务器端初始化 ASC 的重要时机
     */
    virtual void PossessedBy(AController* NewController) override;

    /**
     * @brief PlayerState 复制回调
     * * 在客户端初始化 ASC 的重要时机
     */
    virtual void OnRep_PlayerState() override;

    // ============ 技能系统 ============

    /**
     * @brief 尝试通过 Tag 激活技能
     * * @param AbilityTag 技能标签
     * * @return bool 是否成功触发
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Ability")
    bool TryActivateAbilityByTag(const FGameplayTag& AbilityTag);

    /**
     * @brief 获取属性集
     * * @return UXBAttributeSet* 属性集指针，包含血量、攻击力等
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Attribute")
    UXBAttributeSet* GetAttributeSet() const { return AttributeSet; }

    // ============ 阵营 ============

    /**
     * @brief 获取当前阵营
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Faction")
    EXBFaction GetFaction() const { return Faction; }

    /**
     * @brief 设置当前阵营
     * * @param NewFaction 新阵营
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Faction")
    void SetFaction(EXBFaction NewFaction) { Faction = NewFaction; }

    /**
     * @brief 检查目标是否敌对
     * * @param Other 其他角色
     * * @return true 表示敌对，false 表示友善或中立
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Faction")
    bool IsHostileTo(const AXBCharacterBase* Other) const;

    // ============ 士兵管理 ============

    /**
     * @brief 获取当前携带的士兵数量
     * * @return int32 数量
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Army")
    int32 GetSoldierCount() const;

    /**
     * @brief 招募一个士兵（逻辑层）
     * * @param SoldierId 士兵唯一ID
     * 处理属性加成和队列更新
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Army")
    void RecruitSoldier(int32 SoldierId);

    /**
     * @brief 召回所有士兵
     * * 用于进入兵营或清除状态
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Army")
    void RecallAllSoldiers();

    /**
     * @brief 强制进入战斗状态
     * * 通知周围士兵开始索敌
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Combat")
    void EnterCombat();

    /**
     * @brief 强制退出战斗状态
     * * 通知士兵回归队列
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Combat")
    void ExitCombat();

    /**
     * @brief 检查是否处于战斗状态
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Combat")
    bool IsInCombat() const;

    // ============ 缩放与成长 ============

    /**
     * @brief 根据士兵数量更新角色模型缩放
     * * 公式：Base + (Count * Rate)，采用加法叠加
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Growth")
    void UpdateScaleFromSoldierCount();

    /**
     * @brief 响应士兵招募事件
     * * 增加血量上限并回血，更新缩放
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Growth")
    void OnSoldierRecruited();

    /**
     * @brief 响应士兵死亡事件
     * * 减小缩放，但不扣除血量
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Growth")
    void OnSoldierDied();

    // ============ 隐身 ============

    // 🔧 修改 - 避免与 AActor::SetHidden 参数名冲突，重命名函数明确意图
    /**
     * @brief 设置角色隐身状态（草丛逻辑）
     * * @param bNewHidden 是否隐身
     * 隐身时半透显示，取消碰撞，且被敌人忽略
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Stealth")
    void SetCharacterHidden(bool bNewHidden);  

    /**
     * @brief 检查是否因草丛而隐身
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Stealth")
    bool IsHiddenInGrass() const { return bIsHiddenInGrass; }

protected:
    // ============ 组件 ============

    
    /** 技能系统组件 (GAS)，处理所有技能交互 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "XB|Ability", meta = (DisplayName = "技能系统组件"))
    TObjectPtr<UXBAbilitySystemComponent> AbilitySystemComponent;

    /** 属性集，存储 Health, AttackPower 等数值 (GAS 内部对象，无需 DisplayName) */
    UPROPERTY()
    TObjectPtr<UXBAttributeSet> AttributeSet;

    // ============ 配置 ============

    
    /** 当前所属阵营 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Faction", meta = (DisplayName = "所属阵营"))
    EXBFaction Faction = EXBFaction::Neutral;

    
    /** 游戏开始时自动赋予的技能列表 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Ability", meta = (DisplayName = "初始技能列表"))
    TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

    
    /** 游戏开始时自动应用的 GameplayEffect（用于初始化属性） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Ability", meta = (DisplayName = "初始效果列表"))
    TArray<TSubclassOf<UGameplayEffect>> StartupEffects;

    
    /** 将领成长配置（缩放倍率、血量加成等） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Growth", meta = (DisplayName = "成长配置"))
    FXBLeaderGrowthConfig GrowthConfig;

    
    /** 默认携带的兵种类型 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Army", meta = (DisplayName = "默认携带兵种"))
    EXBSoldierType DefaultSoldierType = EXBSoldierType::Infantry;

    // ============ 状态 ============

    
    /** 是否正在草丛中隐身 */
    UPROPERTY(BlueprintReadOnly, Category = "XB|Stealth", meta = (DisplayName = "是否在草丛中"))
    bool bIsHiddenInGrass = false;
    
    
    /** 初始基础缩放值 */
    UPROPERTY(BlueprintReadOnly, Category = "XB|Growth", meta = (DisplayName = "基础缩放值"))
    float BaseScale = 1.0f;

    // ============ 初始化 ============

    /**
     * @brief 初始化 GAS 组件
     * * 绑定 ActorInfo，初始化属性
     */
    virtual void InitializeAbilitySystem();

    /**
     * @brief 授予初始技能
     */
    virtual void AddStartupAbilities();

    /**
     * @brief 应用初始 GameplayEffect
     */
    virtual void ApplyStartupEffects();

    // ============ 事件回调 ============

    /**
     * @brief GAS 属性：血量变化回调
     */
    virtual void OnHealthChanged(float OldValue, float NewValue);

    /**
     * @brief 自定义逻辑：缩放变化回调
     */
    virtual void OnScaleChanged(float OldValue, float NewValue);

    /**
     * @brief 死亡事件（可蓝图重写）
     * * 处理布娃娃、特效播放、游戏结束逻辑
     */
    UFUNCTION(BlueprintNativeEvent, Category = "XB|Death")
    void OnDeath();
    virtual void OnDeath_Implementation();
};