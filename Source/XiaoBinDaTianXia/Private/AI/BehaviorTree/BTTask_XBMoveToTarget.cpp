/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTTask_XBMoveToTarget.cpp

/**
 * @file BTTask_XBMoveToTarget.cpp
 * @brief 行为树任务 - 移动到目标实现
 * 
 * @note ✨ 新增文件
 */

#include "AI/BehaviorTree/BTTask_XBMoveToTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Soldier/XBSoldierActor.h"
#include "AI/XBSoldierAIController.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_XBMoveToTarget::UBTTask_XBMoveToTarget()
{
    // 设置节点名称
    NodeName = TEXT("移动到目标");
    
    // 启用Tick用于动态追踪目标
    bNotifyTick = true;
    bNotifyTaskFinished = true;
    
    // 配置黑板键过滤器
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBMoveToTarget, TargetKey), AActor::StaticClass());
    AttackRangeKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBMoveToTarget, AttackRangeKey));
}

EBTNodeResult::Type UBTTask_XBMoveToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // 获取AI控制器
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return EBTNodeResult::Failed;
    }
    
    // 获取士兵Actor
    AXBSoldierActor* Soldier = Cast<AXBSoldierActor>(AIController->GetPawn());
    if (!Soldier)
    {
        return EBTNodeResult::Failed;
    }
    
    // 获取黑板组件
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return EBTNodeResult::Failed;
    }
    
    // 获取目标Actor
    AActor* Target = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
    if (!Target)
    {
        UE_LOG(LogTemp, Verbose, TEXT("BTTask_MoveToTarget: 没有目标"));
        return EBTNodeResult::Failed;
    }
    
    // 获取攻击范围（停止距离）
    // 说明: 到达攻击范围后停止移动
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
        // 从士兵配置获取攻击范围
        StopDistance = Soldier->GetSoldierConfig().AttackRange;
    }
    
    // 弓手特殊处理 - 保持更远的距离
    // 说明: 弓手需要在远距离攻击，过近会后撤
    if (Soldier->GetSoldierType() == EXBSoldierType::Archer)
    {
        // 弓手的停止距离更远
        StopDistance = FMath::Max(StopDistance, StopDistance * 0.9f);
    }
    
    // 检查是否已经在范围内
    float CurrentDistance = FVector::Dist(Soldier->GetActorLocation(), Target->GetActorLocation());
    if (CurrentDistance <= StopDistance)
    {
        UE_LOG(LogTemp, Verbose, TEXT("士兵 %s 已在目标攻击范围内"), *Soldier->GetName());
        return EBTNodeResult::Succeeded;
    }
    
    // 发起移动请求
    // 说明: 移动到目标，停止在攻击范围边缘
    EPathFollowingRequestResult::Type MoveResult = AIController->MoveToActor(
        Target,
        StopDistance - 10.0f,  // 稍微近一点确保在范围内
        true,  // bUsePathfinding
        true   // bAllowStrafe
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
    // 获取AI控制器
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    // 获取士兵Actor
    AXBSoldierActor* Soldier = Cast<AXBSoldierActor>(AIController->GetPawn());
    if (!Soldier)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    // 获取黑板组件
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    // 获取目标
    AActor* Target = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
    if (!Target)
    {
        // 目标丢失
        AIController->StopMovement();
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    // 获取攻击范围
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
        StopDistance = Soldier->GetSoldierConfig().AttackRange;
    }
    
    // 检查是否到达攻击范围
    float CurrentDistance = FVector::Dist(Soldier->GetActorLocation(), Target->GetActorLocation());
    if (CurrentDistance <= StopDistance)
    {
        // 到达攻击范围
        AIController->StopMovement();
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }
    
    // 弓手过近时后撤
    // 说明: 如果敌人靠近弓手，弓手需要后撤保持距离
    if (Soldier->GetSoldierType() == EXBSoldierType::Archer)
    {
        float MinDistance = ArcherSafeDistance;
        if (CurrentDistance < MinDistance)
        {
            // 计算后撤位置
            FVector RetreatDirection = (Soldier->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();
            FVector RetreatTarget = Soldier->GetActorLocation() + RetreatDirection * 150.0f;
            
            AIController->MoveToLocation(RetreatTarget, 10.0f, true, true, true, true);
            return;
        }
    }
    
    // 定期更新目标位置
    // 说明: 目标可能在移动，需要动态追踪
    TargetUpdateTimer += DeltaSeconds;
    if (TargetUpdateTimer >= TargetUpdateInterval)
    {
        TargetUpdateTimer = 0.0f;
        
        // 重新发起移动请求
        AIController->MoveToActor(Target, StopDistance - 10.0f, true, true);
    }
}

EBTNodeResult::Type UBTTask_XBMoveToTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // 中止时停止移动
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
