/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTTask_XBMoveToTarget.cpp

#include "AI/BehaviorTree/BTTask_XBMoveToTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "AI/XBSoldierAIController.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_XBMoveToTarget::UBTTask_XBMoveToTarget()
{
    NodeName = TEXT("移动到目标");
    
    bNotifyTick = true;
    bNotifyTaskFinished = true;
    
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBMoveToTarget, TargetKey), AActor::StaticClass());
    AttackRangeKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBMoveToTarget, AttackRangeKey));
}

EBTNodeResult::Type UBTTask_XBMoveToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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
    
    AActor* CurrentTarget = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
    if (!CurrentTarget)
    {
        return EBTNodeResult::Failed;
    }
    
    float StopDistance = DefaultStopDistance;
    
    if (AttackRangeKey.SelectedKeyName != NAME_None)
    {
        float BBRange = BlackboardComp->GetValueAsFloat(AttackRangeKey.SelectedKeyName);
        if (BBRange > 0.0f)
        {
            StopDistance = BBRange;
        }
    }
    else
    {
        // 🔧 修复 - 直接调用 GetAttackRange()
        StopDistance = Soldier->GetAttackRange();
    }
    
    if (Soldier->GetSoldierType() == EXBSoldierType::Archer)
    {
        StopDistance = FMath::Max(StopDistance, StopDistance * 0.9f);
    }
    
    float CurrentDistance = FVector::Dist(Soldier->GetActorLocation(), CurrentTarget->GetActorLocation());
    if (CurrentDistance <= StopDistance)
    {
        UE_LOG(LogTemp, Verbose, TEXT("士兵 %s 已在目标攻击范围内"), *Soldier->GetName());
        return EBTNodeResult::Succeeded;
    }
    
    EPathFollowingRequestResult::Type MoveResult = AIController->MoveToActor(
        CurrentTarget,
        StopDistance - 10.0f,
        true,
        true
    );
    
    if (MoveResult == EPathFollowingRequestResult::RequestSuccessful)
    {
        TargetUpdateTimer = 0.0f;
        return EBTNodeResult::InProgress;
    }
    else if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
    {
        return EBTNodeResult::Succeeded;
    }
    
    return EBTNodeResult::Failed;
}

void UBTTask_XBMoveToTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(AIController->GetPawn());
    if (!Soldier)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    AActor* Target = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
    if (!Target)
    {
        AIController->StopMovement();
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    float StopDistance = DefaultStopDistance;
    
    if (AttackRangeKey.SelectedKeyName != NAME_None)
    {
        float BBRange = BlackboardComp->GetValueAsFloat(AttackRangeKey.SelectedKeyName);
        if (BBRange > 0.0f)
        {
            StopDistance = BBRange;
        }
    }
    else
    {
        // 🔧 修复 - 直接调用 GetAttackRange()
        StopDistance = Soldier->GetAttackRange();
    }
    
    float CurrentDistance = FVector::Dist(Soldier->GetActorLocation(), Target->GetActorLocation());
    if (CurrentDistance <= StopDistance)
    {
        AIController->StopMovement();
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }
    
    if (Soldier->GetSoldierType() == EXBSoldierType::Archer)
    {
        float MinDistance = ArcherSafeDistance;
        if (CurrentDistance < MinDistance)
        {
            FVector RetreatDirection = (Soldier->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();
            FVector RetreatTarget = Soldier->GetActorLocation() + RetreatDirection * 150.0f;
            
            AIController->MoveToLocation(RetreatTarget, 10.0f, true, true, true, true);
            return;
        }
    }
    
    TargetUpdateTimer += DeltaSeconds;
    if (TargetUpdateTimer >= TargetUpdateInterval)
    {
        TargetUpdateTimer = 0.0f;
        
        AIController->MoveToActor(Target, StopDistance - 10.0f, true, true);
    }
}

EBTNodeResult::Type UBTTask_XBMoveToTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    if (AAIController* AIController = OwnerComp.GetAIOwner())
    {
        AIController->StopMovement();
    }
    
    return EBTNodeResult::Aborted;
}

FString UBTTask_XBMoveToTarget::GetStaticDescription() const
{
    return FString::Printf(TEXT("移动到目标\n目标键: %s\n停止距离: %.1f"),
        *TargetKey.SelectedKeyName.ToString(),
        DefaultStopDistance);
}
