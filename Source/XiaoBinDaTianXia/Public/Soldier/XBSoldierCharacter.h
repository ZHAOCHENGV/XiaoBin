/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Soldier/XBSoldierCharacter.h

/**
 * @file XBSoldierCharacter.h
 * @brief 士兵Actor类 - 统一角色系统（休眠态 + 激活态 + 掉落态）
 * 
 * @note 🔧 架构重构记录:
 *       1. ✨ 新增 休眠态系统（替代 XBVillagerActor）
 *       2. ✨ 新增 组件启用/禁用管理
 *       3. ✨ 新增 Zzz 特效系统
 *       4. ✨ 新增 掉落抛物线系统（支持落地自动入列）
 *       5. 🔧 修改 状态机支持 Dormant 和 Dropping 状态
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Army/XBSoldierTypes.h"
#include "Data/XBSoldierDataTable.h"
#include "XBSoldierCharacter.generated.h"

// ============================================
// 前向声明
// ============================================

class UXBSoldierFollowComponent;
class UXBSoldierDebugComponent;
class UXBSoldierDataAccessor;
class UXBSoldierBehaviorInterface;
class UXBFormationComponent;
class UBehaviorTree;
class AAIController;
class AXBSoldierAIController;
class AXBCharacterBase;
class UDataTable;
class UAnimMontage;
class UAnimSequence;
class UNiagaraComponent;
class UNiagaraSystem;
class UAbilitySystemComponent;
class UXBAbilitySystemComponent;
class UGameplayAbility;
class UMaterialInterface;
struct FXBGameConfigData;

// ============================================
// 委托声明
// ============================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSoldierStateChanged, EXBSoldierState, OldState, EXBSoldierState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSoldierDied, AXBSoldierCharacter*, Soldier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSoldierRecruited, AXBSoldierCharacter*, Soldier, AActor*, Leader);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDormantStateChanged, AXBSoldierCharacter*, Soldier, bool, bIsDormant);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDropLandingComplete, AXBSoldierCharacter*, Soldier);

// ============================================
// 士兵Actor类
// ============================================

UCLASS()
class XIAOBINDATIANXIA_API AXBSoldierCharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AXBSoldierCharacter();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void PostInitializeComponents() override;

    // ============ IAbilitySystemInterface ============
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    // ==================== 组件状态 ====================

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "组件已初始化"))
    bool bComponentsInitialized = false;

    void EnableMovementAndTick();

    // ==================== 数据访问器接口 ====================

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Data", meta = (DisplayName = "获取数据访问器"))
    UXBSoldierDataAccessor* GetDataAccessor() const { return DataAccessor; }

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Data", meta = (DisplayName = "数据访问器有效"))
    bool IsDataAccessorValid() const;

    // ==================== 初始化方法 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "从数据表初始化"))
    void InitializeFromDataTable(UDataTable* DataTable, FName RowName, EXBFaction InFaction);

    // ✨ 新增 - 完整初始化（用于掉落士兵）
    /**
     * @brief 完整初始化士兵（数据 + 组件 + 视觉）
     * @param DataTable 数据表
     * @param RowName 行名
     * @param InFaction 阵营
     * @note 用于掉落士兵，在生成时立即完成所有初始化
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "完整初始化"))
    void FullInitialize(UDataTable* DataTable, FName RowName, EXBFaction InFaction);

    /**
     * @brief  获取发射物配置
     * @return 发射物配置结构
     * @note   仅弓手有效，其余类型返回默认值
     */
    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Combat", meta = (DisplayName = "获取发射物配置"))
    FXBProjectileConfig GetProjectileConfig() const { return ProjectileConfig; }

    // ==================== 休眠系统接口 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|Dormant", meta = (DisplayName = "进入休眠态"))
    void EnterDormantState(EXBDormantType DormantType = EXBDormantType::Sleeping);

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|Dormant", meta = (DisplayName = "退出休眠态"))
    void ExitDormantState();

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Dormant", meta = (DisplayName = "是否休眠中"))
    bool IsDormant() const { return CurrentState == EXBSoldierState::Dormant; }

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|Dormant", meta = (DisplayName = "设置休眠配置"))
    void SetDormantVisualConfig(const FXBDormantVisualConfig& NewConfig);

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Dormant", meta = (DisplayName = "获取休眠配置"))
    const FXBDormantVisualConfig& GetDormantVisualConfig() const { return DormantConfig; }

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|Dormant", meta = (DisplayName = "设置Zzz特效"))
    void SetZzzEffectEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|Dormant", meta = (DisplayName = "切换休眠类型"))
    void SetDormantType(EXBDormantType NewType);

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Dormant", meta = (DisplayName = "获取休眠类型"))
    EXBDormantType GetDormantType() const { return CurrentDormantType; }

    // ==================== 掉落抛物线系统接口 ====================

    /**
     * @brief 开始掉落抛物线飞行
     * @param StartLocation 起始位置（将领死亡位置）
     * @param TargetLocation 目标落地位置
     * @param ArcConfig 抛物线配置
     * @param TargetLeader 落地后要加入的将领（可选）
     * @note 士兵会进入 Dropping 状态，飞行期间不可招募
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|Drop", meta = (DisplayName = "开始掉落飞行"))
    void StartDropFlight(const FVector& StartLocation, const FVector& TargetLocation, 
        const FXBDropArcConfig& ArcConfig, AXBCharacterBase* TargetLeader = nullptr);

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Drop", meta = (DisplayName = "是否掉落中"))
    bool IsDropping() const { return CurrentState == EXBSoldierState::Dropping; }

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Drop", meta = (DisplayName = "获取掉落进度"))
    float GetDropProgress() const;

    /**
     * @brief 绘制掉落抛物线用于调试
     * @param DurationOverride 调试持续时间（<0 使用配置）
     * @param SegmentOverride 采样段数（<=0 使用配置）
     * @note ✨ 新增 - 便于在蓝图中可视化掉落轨迹
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|Drop", meta = (DisplayName = "绘制掉落抛物线"))
    void DrawDropDebugArc(float DurationOverride = -1.0f, int32 SegmentOverride = -1) const;

    // ==================== 配置属性访问方法 ====================

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Data", meta = (DisplayName = "获取士兵类型"))
    EXBSoldierType GetSoldierType() const { return SoldierType; }

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Data", meta = (DisplayName = "获取显示名称"))
    FText GetDisplayName() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Data", meta = (DisplayName = "获取士兵标签"))
    FGameplayTagContainer GetSoldierTags() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Combat", meta = (DisplayName = "获取最大血量"))
    float GetMaxHealth() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Combat", meta = (DisplayName = "获取基础伤害"))
    float GetBaseDamage() const;

    /**
     * @brief  应用运行时配置
     * @param  GameConfig 游戏配置数据
     * @return 无
     * @note   详细流程分析: 缓存倍率/覆盖值 -> 刷新当前血量 -> 保证运行时数据一致
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|Config", meta = (DisplayName = "应用运行时配置"))
    void ApplyRuntimeConfig(const struct FXBGameConfigData& GameConfig);

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Combat", meta = (DisplayName = "获取攻击范围"))
    float GetAttackRange() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Combat", meta = (DisplayName = "获取攻击间隔"))
    float GetAttackInterval() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Movement", meta = (DisplayName = "获取移动速度"))
    float GetMoveSpeed() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Movement", meta = (DisplayName = "获取冲刺倍率"))
    float GetSprintSpeedMultiplier() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Movement", meta = (DisplayName = "获取跟随插值速度"))
    float GetFollowInterpSpeed() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Movement", meta = (DisplayName = "获取旋转速度"))
    float GetRotationSpeed() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "获取视野范围"))
    float GetVisionRange() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "获取脱离距离"))
    float GetDisengageDistance() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "获取返回延迟"))
    float GetReturnDelay() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "获取到达阈值"))
    float GetArrivalThreshold() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "获取避让半径"))
    float GetAvoidanceRadius() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "获取避让权重"))
    float GetAvoidanceWeight() const;

    // ==================== 招募系统 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "被招募"))
    void OnRecruited(AActor* NewLeader, int32 SlotIndex);

    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "是否已招募"))
    bool IsRecruited() const { return bIsRecruited; }

    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "是否可招募"))
    bool CanBeRecruited() const;

    // ==================== 跟随系统 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置跟随将领"))
    void SetFollowTarget(AActor* NewLeader, int32 SlotIndex);

    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "获取跟随将领"))
    AActor* GetFollowTarget() const { return FollowTarget.Get(); }

    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "获取将领角色"))
    AXBCharacterBase* GetLeaderCharacter() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "获取槽位索引"))
    int32 GetFormationSlotIndex() const { return FormationSlotIndex; }

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置槽位索引"))
    void SetFormationSlotIndex(int32 NewIndex);

    // ==================== 状态管理 ====================

    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "获取士兵状态"))
    EXBSoldierState GetSoldierState() const { return CurrentState; }

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置士兵状态"))
    void SetSoldierState(EXBSoldierState NewState);

    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "获取阵营"))
    EXBFaction GetFaction() const { return Faction; }

    // ✨ 新增 - 设置阵营
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置阵营"))
    void SetFaction(EXBFaction NewFaction) { Faction = NewFaction; }

    // ==================== GAS 支持 ====================

    /** @brief 士兵ASC（用于近战Tag触发GA） */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (DisplayName = "能力系统组件"))
    TObjectPtr<class UXBAbilitySystemComponent> AbilitySystemComponent;

    /** @brief 近战命中GA（由蒙太奇Tag触发） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS", meta = (DisplayName = "近战命中GA"))
    TSubclassOf<class UGameplayAbility> MeleeHitAbilityClass;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "是否已死亡"))
    bool IsDead() const { return bIsDead; }

    // ==================== 战斗系统 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "进入战斗"))
    void EnterCombat();

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "退出战斗"))
    void ExitCombat();

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "受到伤害"))
    float TakeSoldierDamage(float DamageAmount, AActor* DamageSource);

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "执行攻击"))
    bool PerformAttack(AActor* Target);

public:
    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "获取当前血量"))
    float GetCurrentHealth() const { return CurrentHealth; }

    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "是否可以攻击"))
    bool CanAttack() const;

    // ==================== AI系统 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "范围内有敌人"))
    bool HasEnemiesInRadius(float Radius) const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "获取到目标距离"))
    float GetDistanceToTarget(AActor* Target) const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "是否在攻击范围内"))
    bool IsInAttackRange(AActor* Target) const;

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "返回队列"))
    void ReturnToFormation();
    
    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|AI", meta = (DisplayName = "移动到编队位置"))
    void MoveToFormationPosition();

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "获取编队位置"))
    FVector GetFormationWorldPosition() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "获取编队位置(安全)"))
    FVector GetFormationWorldPositionSafe() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "是否到达编队位置"))
    bool IsAtFormationPosition() const;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|AI", meta = (DisplayName = "是否到达编队位置(安全)"))
    bool IsAtFormationPositionSafe() const;

    // ==================== 逃跑系统 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier", meta = (DisplayName = "设置逃跑状态"))
    void SetEscaping(bool bEscaping);

    UFUNCTION(BlueprintPure, Category = "XB|Soldier", meta = (DisplayName = "是否正在逃跑"))
    bool IsEscaping() const { return bIsEscaping; }

    // ==================== 对象池支持 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Soldier|Pool", meta = (DisplayName = "重置状态"))
    void ResetForPooling();

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Pool", meta = (DisplayName = "是否池化士兵"))
    bool IsPooledSoldier() const { return bIsPooledSoldier; }

    void MarkAsPooledSoldier() { bIsPooledSoldier = true; }

    // ==================== 委托事件 ====================

    UPROPERTY(BlueprintAssignable, Category = "XB|Soldier")
    FOnSoldierStateChanged OnSoldierStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "XB|Soldier")
    FOnSoldierDied OnSoldierDied;

    UPROPERTY(BlueprintAssignable, Category = "XB|Soldier")
    FOnSoldierRecruited OnSoldierRecruited;

    UPROPERTY(BlueprintAssignable, Category = "XB|Soldier|Dormant", meta = (DisplayName = "休眠状态变化"))
    FOnDormantStateChanged OnDormantStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "XB|Soldier|Drop", meta = (DisplayName = "掉落完成"))
    FOnDropLandingComplete OnDropLandingComplete;

    // ==================== 组件访问 ====================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "跟随组件"))
    TObjectPtr<UXBSoldierFollowComponent> FollowComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "调试组件"))
    TObjectPtr<UXBSoldierDebugComponent> DebugComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "Zzz特效"))
    TObjectPtr<UNiagaraComponent> ZzzEffectComponent;

    // ==================== 公开访问的战斗状态 ====================

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "当前攻击目标"))
    TWeakObjectPtr<AActor> CurrentAttackTarget;

    // ==================== AI系统友元 ====================

    friend class AXBSoldierAIController;

    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Behavior", meta = (DisplayName = "获取行为接口"))
    UXBSoldierBehaviorInterface* GetBehaviorInterface() const { return BehaviorInterface; }

    // ==================== ✨ 新增：动画系统接口 ====================

    /**
     * @brief 获取用于动画的移动速度
     * @return 当前移动速度，仅在合适状态下返回有效值
     * @note ✨ 新增 - 仅在以下条件满足时返回速度：
     *       1. 已被招募
     *       2. 处于锁定跟随模式或战斗状态
     *       3. 已到达编队位置（非招募过渡中）
     *       其他情况返回0，避免过渡动画异常
     */
    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Animation", meta = (DisplayName = "获取动画移动速度"))
    float GetAnimationMoveSpeed() const;

    /**
     * @brief 检查是否应该播放移动动画
     * @return 是否应该播放移动动画
     * @note ✨ 新增 - 供动画蓝图判断是否使用移动混合空间
     */
    UFUNCTION(BlueprintPure, Category = "XB|Soldier|Animation", meta = (DisplayName = "应该播放移动动画"))
    bool ShouldPlayMoveAnimation() const;

    /**
     * @brief  设置草丛隐身状态
     * @param  bHidden 是否隐身
     * @note   详细流程分析: 设置半透明 -> 调整碰撞通道
     *         性能/架构注意事项: 仅在状态变化时执行
     */
    UFUNCTION(BlueprintCallable, Category = "草丛", meta = (DisplayName = "设置草丛隐身"))
    void SetHiddenInBush(bool bEnableHidden);

    /**
     * @brief  是否处于草丛隐身
     * @return 是否隐身
     */
    UFUNCTION(BlueprintPure, Category = "草丛", meta = (DisplayName = "是否草丛隐身"))
    bool IsHiddenInBush() const { return bIsHiddenInBush; }

protected:
    /**
     * @brief  刷新近战命中GA配置
     * @note   详细流程分析: 读取数据表普攻配置 -> 覆盖近战GA -> 初始化ASC并授予能力
     *         性能/架构注意事项: 仅在初始化阶段调用，避免运行时重复授予
     */
    void RefreshMeleeHitAbilityFromData();
protected:

    // ✨ 新增 - 配置跟随并开始移动
    /**
     * @brief 配置跟随组件并开始移动到槽位
     * @param Leader 将领
     * @param SlotIndex 槽位索引
     */
    void SetupFollowingAndStartMoving(AXBCharacterBase* Leader, int32 SlotIndex);

    /**
     * @brief 当槽位变化时触发补位移动
     * @param bForceRecruitTransition 是否强制使用招募过渡模式
     * @note ✨ 新增 - 防止槽位变化时瞬移
     */
    void RequestRelocateToSlot(bool bForceRecruitTransition = false);

    /**
     * @brief 绑定将领编队事件
     * @param Leader 将领指针
     * @note 🔧 确保队形更新时触发平滑补位
     */
    void BindLeaderFormationEvents(AXBCharacterBase* Leader);

    /**
     * @brief 解除编队事件绑定
     * @note 🔧 防止更换将领或销毁时遗留委托
     */
    void UnbindLeaderFormationEvents();

    /**
     * @brief 处理编队更新回调
     * @note ✨ 槽位按序延迟插值，形成“蛇尾”式补位
     */
    UFUNCTION()
    void HandleFormationUpdated();
    
    // ==================== 数据访问器组件 ====================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "数据访问器"))
    TObjectPtr<UXBSoldierDataAccessor> DataAccessor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "行为接口"))
    TObjectPtr<UXBSoldierBehaviorInterface> BehaviorInterface;

    // ==================== 休眠配置 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Soldier|Dormant", meta = (DisplayName = "休眠配置"))
    FXBDormantVisualConfig DormantConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Soldier|Dormant", meta = (DisplayName = "Zzz特效资源"))
    TSoftObjectPtr<UNiagaraSystem> ZzzEffectAsset;

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "休眠类型"))
    EXBDormantType CurrentDormantType = EXBDormantType::Sleeping;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Soldier|Dormant", meta = (DisplayName = "初始休眠态"))
    bool bStartAsDormant = false;

    // ==================== 掉落飞行状态 ====================

    UPROPERTY(BlueprintReadOnly, Category = "XB|Soldier|Drop")
    FVector DropStartLocation;

    UPROPERTY(BlueprintReadOnly, Category = "XB|Soldier|Drop")
    FVector DropTargetLocation;

    UPROPERTY(BlueprintReadOnly, Category = "XB|Soldier|Drop")
    float DropFlightDuration = 0.6f;

    UPROPERTY(BlueprintReadOnly, Category = "XB|Soldier|Drop")
    float DropArcHeight = 200.0f;

    UPROPERTY(BlueprintReadOnly, Category = "XB|Soldier|Drop")
    float DropElapsedTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "XB|Soldier|Drop")
    bool bPlayDropLandingEffect = true;

    // ✨ 新增 - 落地后自动入列的目标将领
    UPROPERTY(BlueprintReadOnly, Category = "XB|Soldier|Drop", meta = (DisplayName = "落地目标将领"))
    TWeakObjectPtr<AXBCharacterBase> DropTargetLeader;

    // ✨ 新增 - 是否落地后自动入列
    UPROPERTY(BlueprintReadOnly, Category = "XB|Soldier|Drop")
    bool bAutoRecruitOnLanding = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Soldier|Drop", meta = (DisplayName = "落地特效"))
    TSoftObjectPtr<UNiagaraSystem> DropLandingEffectAsset;

    // ✨ 新增 - 当前抛物线配置（用于蓝图调试）
    UPROPERTY(BlueprintReadOnly, Category = "XB|Soldier|Drop", meta = (DisplayName = "当前抛物线配置"))
    FXBDropArcConfig ActiveDropArcConfig;

    // ==================== 运行时状态 ====================

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "士兵类型"))
    EXBSoldierType SoldierType = EXBSoldierType::Infantry;

    // ✨ 新增 - 弓手发射物配置缓存
    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "发射物配置"))
    FXBProjectileConfig ProjectileConfig;

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "阵营"))
    EXBFaction Faction = EXBFaction::Neutral;

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "当前状态"))
    EXBSoldierState CurrentState = EXBSoldierState::Idle;

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "跟随目标"))
    TWeakObjectPtr<AActor> FollowTarget;

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "槽位索引"))
    int32 FormationSlotIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "当前血量"))
    float CurrentHealth = 100.0f;

    // ==================== 运行时配置缓存 ====================

    UPROPERTY(BlueprintReadOnly, Category = "配置", meta = (DisplayName = "士兵生命倍率"))
    float CachedHealthMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "配置", meta = (DisplayName = "士兵伤害倍率"))
    float CachedDamageMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "配置", meta = (DisplayName = "士兵血量覆盖值"))
    float CachedHealthOverride = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "正在逃跑"))
    bool bIsEscaping = false;

    UPROPERTY(BlueprintReadOnly, Category = "状态")
    float AttackCooldownTimer = 0.0f;

    float TargetSearchTimer = 0.0f;
    float LastEnemySeenTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "是否已招募"))
    bool bIsRecruited = false;

    UPROPERTY(BlueprintReadOnly, Category = "状态", meta = (DisplayName = "是否已死亡"))
    bool bIsDead = false;

    // ==================== 草丛隐身 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "草丛", meta = (DisplayName = "草丛覆层材质"))
    TObjectPtr<UMaterialInterface> BushOverlayMaterial;

    UPROPERTY(BlueprintReadOnly, Category = "草丛", meta = (DisplayName = "是否草丛隐身"))
    bool bIsHiddenInBush = false;

    UPROPERTY()
    bool bCachedBushCollisionResponse = false;

    UPROPERTY()
    TEnumAsByte<ECollisionResponse> CachedLeaderCollisionResponse = ECR_Block;

    UPROPERTY()
    TEnumAsByte<ECollisionResponse> CachedSoldierCollisionResponse = ECR_Block;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> CachedOverlayMaterial;

    UPROPERTY(BlueprintReadOnly, Category = "状态")
    bool bIsPooledSoldier = false;

    // ✨ 新增 - 队形尾随配置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Soldier|Formation", meta = (DisplayName = "启用编队尾随插值"))
    bool bEnableFormationTailDelay = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Soldier|Formation", meta = (DisplayName = "尾随延迟/槽位", ClampMin = "0.0"))
    float FormationTailDelayPerSlot = 0.05f;

    // ==================== AI配置 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "行为树"))
    TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "AI控制器类"))
    TSubclassOf<AXBSoldierAIController> SoldierAIControllerClass;

    // ✨ 新增 - 跟随状态自动寻敌配置
    /**
     * @brief 跟随/待机状态下的自动寻敌检查间隔
     * @note   详细流程分析: 仅在跟随/待机状态按间隔触发扫描，降低性能开销
     *         性能/架构注意事项: 间隔过低会增加扫描成本，建议 >= 0.1s
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "自动寻敌检查间隔", ClampMin = "0.05"))
    float AutoEngageCheckInterval = 0.25f;

    // ✨ 新增 - 自动反击开关
    /**
     * @brief 是否启用跟随/待机状态的自动反击
     * @note   详细流程分析: 受击或视野内发现敌人时触发进入战斗
     *         性能/架构注意事项: 关闭后士兵仅随主将触发战斗
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "启用自动反击"))
    bool bEnableAutoEngage = true;

    // ==================== 内部方法 ====================

    void HandleDeath();
    bool PlayAttackMontage();
    void ApplyVisualConfig();
    void FaceTarget(AActor* Target, float DeltaTime);
    FVector CalculateAvoidanceDirection(const FVector& DesiredDirection);

    // ✨ 新增 - 跟随/待机自动反击入口
    /**
     * @brief 跟随/待机状态下自动进入战斗
     * @param DeltaTime 帧间隔
     * @note   详细流程分析: 校验主将战斗状态 -> 累计计时 -> 触发寻敌 -> 若命中则进入战斗并锁定目标
     *         性能/架构注意事项: 仅在跟随/待机且主将已命中敌方主将时执行，避免无意义扫描
     */
    void TryAutoEngage(float DeltaTime);

    // 休眠系统内部方法
    void EnableActiveComponents();
    void DisableActiveComponents();
    void UpdateDormantAnimation();
    void UpdateZzzEffect();
    void PlayAnimationSequence(UAnimSequence* Animation, bool bLoop = true);
    void LoadDormantAnimations();

    // 掉落飞行内部方法
    void UpdateDropFlight(float DeltaTime);
    FVector CalculateArcPosition(float Progress) const;
    void OnDropLanded();
    void PlayLandingEffect();
    FVector ComputeGroundSnappedLocation(const FVector& DesiredLocation, const FXBDropArcConfig& ArcConfig) const;

    // ✨ 新增 - 落地后自动入列
    /**
     * @brief 落地后自动加入将领队伍
     * @note 在 OnDropLanded 中调用，将士兵添加到 DropTargetLeader 的队伍
     */
    void AutoRecruitToLeader();

private:
    void SpawnAndPossessAIController();
    void InitializeAI();
    FTimerHandle DelayedAIStartTimerHandle;

    // ✨ 新增 - 自动反击计时器
    float AutoEngageCheckTimer = 0.0f;

    // ✨ 新增 - 超距强制跟随锁定，避免战斗/跟随反复切换
    bool bForceFollowByDistance = false;

    UPROPERTY()
    TObjectPtr<UAnimSequence> LoadedSleepingAnimation;
    UPROPERTY()
    TObjectPtr<UAnimSequence> LoadedStandingAnimation;

    // ✨ 新增 - 编队事件绑定缓存
    UPROPERTY()
    TWeakObjectPtr<UXBFormationComponent> CachedLeaderFormation;

    FTimerHandle FormationRealignTimerHandle;
};
