/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTTask_XBAttackTarget.cpp

/**
 * @file BTTask_XBAttackTarget.cpp
 * @brief 行为树任务 - 攻击目标
 * 
 * @note 🔧 重构 - 使用行为接口执行攻击
 */

#include "AI/BehaviorTree/BTTask_XBAttackTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Soldier/Component/XBSoldierBehaviorInterface.h"  // ✨ 新增

UBTTask_XBAttackTarget::UBTTask_XBAttackTarget()
{
    NodeName = TEXT("攻击目标");
    
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBAttackTarget, TargetKey), AActor::StaticClass());
}

/**
 * @brief 执行任务
 * @note 🔧 核心重构 - 通过 BehaviorInterface 执行攻击
 */
EBTNodeResult::Type UBTTask_XBAttackTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
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
    
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return EBTNodeResult::Failed;
    }
    
    AActor* Target = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
    if (!Target)
    {
        UE_LOG(LogTemp, Verbose, TEXT("BTTask_AttackTarget: 目标为空"));
        return EBTNodeResult::Failed;
    }
    
    // ✨ 核心重构 - 通过 BehaviorInterface 执行攻击
    UXBSoldierBehaviorInterface* BehaviorInterface = Soldier->GetBehaviorInterface();
    if (!BehaviorInterface)
    {
        return EBTNodeResult::Failed;
    }
    
    EXBBehaviorResult Result = BehaviorInterface->ExecuteAttack(Target);
    
    switch (Result)
    {
    case EXBBehaviorResult::Success:
        return EBTNodeResult::Succeeded;
        
    case EXBBehaviorResult::InProgress:
        // 冷却中，返回成功让行为树继续
        if (bSucceedOnCooldown)
        {
            return EBTNodeResult::Succeeded;
        }
        return EBTNodeResult::Failed;
        
    case EXBBehaviorResult::Failed:
    default:
        return EBTNodeResult::Failed;
    }
}

FString UBTTask_XBAttackTarget::GetStaticDescription() const
{
    return FString::Printf(TEXT("通过行为接口执行攻击\n目标键: %s"), *TargetKey.SelectedKeyName.ToString());
}
