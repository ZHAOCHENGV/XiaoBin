/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTService_XBSoldierAttackRange.cpp

/**
 * @file BTService_XBSoldierAttackRange.cpp
 * @brief 行为树服务 - 士兵攻击范围检测实现
 *
 * @note ✨ 新增 - 根据士兵技能攻击范围判断目标是否在攻击范围内
 */

#include "AI/BehaviorTree/BTService_XBSoldierAttackRange.h"
#include "AI/XBSoldierAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Character/XBCharacterBase.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Soldier/Component/XBSoldierBehaviorInterface.h"

UBTService_XBSoldierAttackRange::UBTService_XBSoldierAttackRange()
{
    NodeName = TEXT("士兵攻击范围检测");

    Interval = 0.1f;
    RandomDeviation = 0.0f;

    TargetKey.SelectedKeyName = XBSoldierBBKeys::CurrentTarget;
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_XBSoldierAttackRange, TargetKey), AActor::StaticClass());

    IsInAttackRangeKey.SelectedKeyName = TEXT("IsInAttackRange");
    IsInAttackRangeKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_XBSoldierAttackRange, IsInAttackRangeKey));
}

void UBTService_XBSoldierAttackRange::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return;
    }

    AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(AIController->GetPawn());
    if (!Soldier || Soldier->IsDead())
    {
        return;
    }

    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    if (!Blackboard)
    {
        return;
    }

    const FName TargetKeyName = TargetKey.SelectedKeyName.IsNone()
        ? XBSoldierBBKeys::CurrentTarget
        : TargetKey.SelectedKeyName;
    AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(TargetKeyName));

    const FName IsInAttackRangeKeyName = IsInAttackRangeKey.SelectedKeyName.IsNone()
        ? FName(TEXT("IsInAttackRange"))
        : IsInAttackRangeKey.SelectedKeyName;

    if (!Target)
    {
        Blackboard->SetValueAsBool(IsInAttackRangeKeyName, false);
        return;
    }

    if (UXBSoldierBehaviorInterface* BehaviorInterface = Soldier->GetBehaviorInterface())
    {
        if (!BehaviorInterface->IsTargetValid(Target))
        {
            Blackboard->SetValueAsObject(TargetKeyName, nullptr);
            Blackboard->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
            Blackboard->SetValueAsBool(IsInAttackRangeKeyName, false);
            return;
        }
    }
    else if (const AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(Target))
    {
        if (TargetSoldier->IsDead() || TargetSoldier->GetSoldierState() == EXBSoldierState::Dead)
        {
            Blackboard->SetValueAsObject(TargetKeyName, nullptr);
            Blackboard->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
            Blackboard->SetValueAsBool(IsInAttackRangeKeyName, false);
            return;
        }
    }
    else if (const AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Target))
    {
        if (TargetLeader->IsDead())
        {
            Blackboard->SetValueAsObject(TargetKeyName, nullptr);
            Blackboard->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
            Blackboard->SetValueAsBool(IsInAttackRangeKeyName, false);
            return;
        }
    }

    bool bInRange = false;
    if (UXBSoldierBehaviorInterface* BehaviorInterface = Soldier->GetBehaviorInterface())
    {
        bInRange = BehaviorInterface->IsInAttackRange(Target);
    }
    else
    {
        const float AttackRange = Soldier->GetAttackRange();
        const float SelfRadius = Soldier->GetSimpleCollisionRadius();
        const float TargetRadius = Target->GetSimpleCollisionRadius();
        const float CenterDistance = FVector::Dist2D(Soldier->GetActorLocation(), Target->GetActorLocation());
        const float EdgeDistance = CenterDistance - SelfRadius - TargetRadius;
        bInRange = (EdgeDistance <= AttackRange);
    }

    const bool bCurrentValue = Blackboard->GetValueAsBool(IsInAttackRangeKeyName);
    if (bCurrentValue != bInRange)
    {
        Blackboard->SetValueAsBool(IsInAttackRangeKeyName, bInRange);
    }
}
