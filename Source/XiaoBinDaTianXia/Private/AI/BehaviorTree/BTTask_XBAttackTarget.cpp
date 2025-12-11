/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTTask_XBAttackTarget.cpp

/**
 * @file BTTask_XBAttackTarget.cpp
 * @brief 行为树任务 - 攻击目标实现
 * 
 * @note ✨ 新增文件
 */

#include "AI/BehaviorTree/BTTask_XBAttackTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Soldier/XBSoldierCharacter.h"

UBTTask_XBAttackTarget::UBTTask_XBAttackTarget()
{
    // 设置节点名称
    NodeName = TEXT("攻击目标");
    
    // 配置黑板键过滤器
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBAttackTarget, TargetKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_XBAttackTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // 获取AI控制器和士兵
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return EBTNodeResult::Failed;
    }
    
    AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(AIController->GetPawn());
    if (!Soldier)
    {
        return EBTNodeResult::Failed;
    }
    
    // 获取黑板中的目标
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return EBTNodeResult::Failed;
    }
    
    // 从黑板获取目标
    // 说明: 目标应该在寻敌任务中被设置
    AActor* Target = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
    if (!Target)
    {
        UE_LOG(LogTemp, Verbose, TEXT("BTTask_AttackTarget: 目标为空"));
        return EBTNodeResult::Failed;
    }
    
    // 检查是否可以攻击
    // 说明: 士兵Actor内部管理攻击冷却
    if (!Soldier->CanAttack())
    {
        // 在冷却中
        if (bSucceedOnCooldown)
        {
            // 冷却中返回成功，让行为树继续运行
            // 说明: 这样可以避免频繁切换状态
            return EBTNodeResult::Succeeded;
        }
        return EBTNodeResult::Failed;
    }
    
    // 检查是否在攻击范围内
    // 说明: 超出范围应该先移动
    if (!Soldier->IsInAttackRange(Target))
    {
        UE_LOG(LogTemp, Verbose, TEXT("BTTask_AttackTarget: 目标不在攻击范围内"));
        return EBTNodeResult::Failed;
    }
    
    // 执行攻击
    // 说明: 调用士兵的攻击方法，处理伤害和动画
    bool bAttacked = Soldier->PerformAttack(Target);
    
    if (bAttacked)
    {
        UE_LOG(LogTemp, Log, TEXT("士兵 %s 攻击目标 %s"), *Soldier->GetName(), *Target->GetName());
        return EBTNodeResult::Succeeded;
    }
    
    return EBTNodeResult::Failed;
}

FString UBTTask_XBAttackTarget::GetStaticDescription() const
{
    return FString::Printf(TEXT("攻击目标\n目标键: %s"), *TargetKey.SelectedKeyName.ToString());
}
