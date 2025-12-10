/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Soldier/AI/XBSoldierAIController.h

/**
 * @file XBSoldierAIController.h
 * @brief 士兵AI控制器 - 管理行为树和黑板
 */

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "XBSoldierAIController.generated.h"

class UBehaviorTreeComponent;
class UBlackboardComponent;
class AXBSoldierActor;

/**
 * @brief 士兵AI控制器
 * @note 管理士兵的行为树和黑板数据
 */
UCLASS()
class XIAOBINDATIANXIA_API AXBSoldierAIController : public AAIController
{
    GENERATED_BODY()

public:
    AXBSoldierAIController();

    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

    // ============ 行为树接口 ============

    /**
     * @brief 启动行为树
     * @param BehaviorTree 行为树资源
     * @return 是否成功启动
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI")
    bool StartBehaviorTree(UBehaviorTree* BehaviorTree);

    /**
     * @brief 停止行为树
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI")
    void StopBehaviorTree();

    // ============ 黑板操作 ============

    /**
     * @brief 设置黑板中的目标Actor
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI")
    void SetTargetActor(AActor* Target);

    /**
     * @brief 获取黑板中的目标Actor
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI")
    AActor* GetTargetActor() const;

    /**
     * @brief 设置士兵状态到黑板
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI")
    void SetSoldierState(uint8 State);

    /**
     * @brief 更新黑板中的编队位置
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI")
    void UpdateFormationPosition();

    // ============ 获取器 ============

    /**
     * @brief 获取控制的士兵
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI")
    AXBSoldierActor* GetSoldierPawn() const;

protected:
    /** @brief 行为树组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件")
    TObjectPtr<UBehaviorTreeComponent> BehaviorTreeComponent;

    /** @brief 黑板组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件")
    TObjectPtr<UBlackboardComponent> BlackboardComponent;

    // ============ 黑板键名 ============

    /** @brief 目标Actor键名 */
    static const FName BB_TargetActor;

    /** @brief 将领键名 */
    static const FName BB_Leader;

    /** @brief 士兵状态键名 */
    static const FName BB_SoldierState;

    /** @brief 编队位置键名 */
    static const FName BB_FormationPosition;

    /** @brief 攻击范围键名 */
    static const FName BB_AttackRange;

    /** @brief 是否可以攻击键名 */
    static const FName BB_CanAttack;
};
