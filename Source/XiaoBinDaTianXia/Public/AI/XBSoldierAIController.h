/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/XBSoldierAIController.h

/**
 * @file XBSoldierAIController.h
 * @brief 士兵AI控制器 - 支持行为树和黑板系统
 * 
 * @note ✨ 新增文件
 *       1. 专门为士兵设计的AI控制器
 *       2. 集成行为树和黑板组件
 *       3. 提供常用的黑板键名常量
 *       4. 支持数据驱动配置
 */

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "XBSoldierAIController.generated.h"

// 前向声明
class UBehaviorTreeComponent;
class UBlackboardComponent;
class UAIPerceptionComponent;
class UBehaviorTree;
class AXBSoldierActor;

/**
 * @brief 士兵黑板键名常量
 * @note 统一管理所有黑板变量名，避免字符串硬编码
 */
namespace XBSoldierBBKeys
{
    // 对象类型键
    const FName Leader = TEXT("Leader");                    // 将领Actor
    const FName CurrentTarget = TEXT("CurrentTarget");      // 当前攻击目标
    const FName Self = TEXT("Self");                        // 自身引用
    
    // 位置类型键
    const FName TargetLocation = TEXT("TargetLocation");    // 目标位置
    const FName FormationPosition = TEXT("FormationPosition"); // 编队位置
    const FName HomeLocation = TEXT("HomeLocation");        // 初始位置
    
    // 枚举/整数类型键
    const FName SoldierState = TEXT("SoldierState");        // 士兵状态枚举
    const FName FormationSlot = TEXT("FormationSlot");      // 编队槽位索引
    
    // 浮点类型键
    const FName AttackRange = TEXT("AttackRange");          // 攻击范围
    const FName DetectionRange = TEXT("DetectionRange");    // 检测范围
    const FName DistanceToTarget = TEXT("DistanceToTarget"); // 到目标的距离
    const FName DistanceToLeader = TEXT("DistanceToLeader"); // 到将领的距离
    
    // 布尔类型键
    const FName HasTarget = TEXT("HasTarget");              // 是否有目标
    const FName IsInCombat = TEXT("IsInCombat");            // 是否在战斗中
    const FName ShouldRetreat = TEXT("ShouldRetreat");      // 是否应该撤退
    const FName IsAtFormation = TEXT("IsAtFormation");      // 是否在编队位置
    const FName CanAttack = TEXT("CanAttack");              // 是否可以攻击
}

/**
 * @brief 士兵AI控制器
 * 
 * @note 功能说明:
 *       - 管理士兵的行为树和黑板
 *       - 提供黑板值的便捷更新方法
 *       - 处理感知系统回调
 *       - 支持动态切换行为树
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
     * @brief 刷新所有黑板值
     * @note 从控制的士兵Actor同步所有必要数据
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "刷新黑板"))
    void RefreshBlackboardValues();

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

    // ==================== 内部方法 ====================

    /**
     * @brief 初始化黑板
     * @param BT 行为树资产
     * @return 是否成功初始化
     */
    bool InitializeBlackboard(UBehaviorTree* BT);

    /**
     * @brief 更新距离相关的黑板值
     */
    void UpdateDistanceValues();
};
