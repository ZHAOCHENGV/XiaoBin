/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/XBSoldierAIController.h

/**
 * @file XBSoldierAIController.h
 * @brief 士兵AI控制器 - 支持行为树和黑板系统
 * 
 * @note 🔧 修改记录:
 *       1. 修复 OnPossess 中访问未初始化组件导致的崩溃
 *       2. 将所有行为树初始化延迟到 OnPossess 完成后
 *       3. 添加安全的黑板更新方法
 */

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "XBSoldierAIController.generated.h"

// 前向声明
class UBehaviorTreeComponent;
class UBlackboardComponent;
class UBehaviorTree;
class AXBSoldierActor;

/**
 * @brief 士兵黑板键名常量
 * @note 统一管理所有黑板变量名，避免字符串硬编码
 */
namespace XBSoldierBBKeys
{
    // 对象类型键
    const FName Leader = TEXT("Leader");
    const FName CurrentTarget = TEXT("CurrentTarget");
    const FName Self = TEXT("Self");
    
    // 位置类型键
    const FName TargetLocation = TEXT("TargetLocation");
    const FName FormationPosition = TEXT("FormationPosition");
    const FName HomeLocation = TEXT("HomeLocation");
    
    // 枚举/整数类型键
    const FName SoldierState = TEXT("SoldierState");
    const FName FormationSlot = TEXT("FormationSlot");
    
    // 浮点类型键
    const FName AttackRange = TEXT("AttackRange");
    const FName DetectionRange = TEXT("DetectionRange");
    const FName DistanceToTarget = TEXT("DistanceToTarget");
    const FName DistanceToLeader = TEXT("DistanceToLeader");
    
    // 布尔类型键
    const FName HasTarget = TEXT("HasTarget");
    const FName IsInCombat = TEXT("IsInCombat");
    const FName ShouldRetreat = TEXT("ShouldRetreat");
    const FName IsAtFormation = TEXT("IsAtFormation");
    const FName CanAttack = TEXT("CanAttack");
}

/**
 * @brief 士兵AI控制器
 * 
 * @note 功能说明:
 *       - 管理士兵的行为树和黑板
 *       - 提供黑板值的便捷更新方法
 *       - 支持延迟初始化，避免组件未就绪时崩溃
 */
UCLASS()
class XIAOBINDATIANXIA_API AXBSoldierAIController : public AAIController
{
    GENERATED_BODY()

public:
    AXBSoldierAIController();

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

public:
    virtual void Tick(float DeltaTime) override;

    // ==================== 行为树控制 ====================

    /**
     * @brief 启动行为树
     * @param BehaviorTreeAsset 行为树资产
     * @return 是否成功启动
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "启动行为树"))
    bool StartBehaviorTree(UBehaviorTree* BehaviorTreeAsset);

    /**
     * @brief 停止行为树
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "停止行为树"))
    void StopBehaviorTreeLogic();

    /**
     * @brief 暂停/恢复行为树
     * @param bPause 是否暂停
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "暂停行为树"))
    void PauseBehaviorTree(bool bPause);

    // ==================== 黑板值更新 ====================

    /**
     * @brief 设置目标Actor
     * @param Target 目标Actor
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "设置目标"))
    void SetTargetActor(AActor* Target);

    /**
     * @brief 设置将领
     * @param Leader 将领Actor
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "设置将领"))
    void SetLeader(AActor* Leader);

    /**
     * @brief 设置士兵状态
     * @param NewState 新状态
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "设置状态"))
    void SetSoldierState(uint8 NewState);

    /**
     * @brief 设置编队位置
     * @param Position 世界坐标位置
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "设置编队位置"))
    void SetFormationPosition(const FVector& Position);

    /**
     * @brief 设置攻击范围
     * @param Range 攻击范围
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "设置攻击范围"))
    void SetAttackRange(float Range);

    /**
     * @brief 更新战斗状态黑板值
     * @param bInCombat 是否在战斗中
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "更新战斗状态"))
    void UpdateCombatState(bool bInCombat);

    /**
     * @brief 刷新所有黑板值（安全版本）
     * @note 不访问可能触发移动组件的函数
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "刷新黑板"))
    void RefreshBlackboardValues();

    /**
     * @brief 安全地刷新黑板值
     * @note 用于初始化阶段，避免访问未就绪的组件
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "安全刷新黑板"))
    void RefreshBlackboardValuesSafe();

    // ==================== 访问器 ====================

    /**
     * @brief 获取控制的士兵Actor
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "获取士兵"))
    AXBSoldierActor* GetSoldierActor() const;

    /**
     * @brief 获取行为树组件
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "获取行为树组件"))
    UBehaviorTreeComponent* GetBehaviorTreeComponent() const { return BehaviorTreeComp; }

    /**
     * @brief 获取黑板组件
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "获取黑板组件"))
    UBlackboardComponent* GetSoldierBlackboard() const { return BlackboardComp; }

protected:
    // ==================== 组件 ====================

    /** @brief 行为树组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "行为树组件"))
    TObjectPtr<UBehaviorTreeComponent> BehaviorTreeComp;

    /** @brief 黑板组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "黑板组件"))
    TObjectPtr<UBlackboardComponent> BlackboardComp;

    // ==================== 配置 ====================

    /** @brief 默认行为树（可在蓝图中覆盖） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI配置", meta = (DisplayName = "默认行为树"))
    TObjectPtr<UBehaviorTree> DefaultBehaviorTree;

    /** @brief 黑板更新间隔 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI配置", meta = (DisplayName = "黑板更新间隔", ClampMin = "0.05"))
    float BlackboardUpdateInterval = 0.1f;

private:
    // ==================== 内部变量 ====================

    /** @brief 缓存的士兵引用 */
    TWeakObjectPtr<AXBSoldierActor> CachedSoldier;

    /** @brief 黑板更新计时器 */
    float BlackboardUpdateTimer = 0.0f;

    /** @brief 是否已完成初始化 */
    bool bIsInitialized = false;

    // ==================== 内部方法 ====================

    /**
     * @brief 初始化士兵黑板
     * @param BT 行为树资产
     * @return 是否成功初始化
     */
    bool SetupSoldierBlackboard(UBehaviorTree* BT);

    /**
     * @brief 安全地更新距离值
     * @note 只使用简单的位置计算，不触发移动组件
     */
    void UpdateDistanceValuesSafe();

    /**
     * @brief 延迟的 OnPossess 初始化
     * @note 在 Possess 完成后的下一帧执行，确保组件就绪
     */
    UFUNCTION()
    void DelayedOnPossess();
};
