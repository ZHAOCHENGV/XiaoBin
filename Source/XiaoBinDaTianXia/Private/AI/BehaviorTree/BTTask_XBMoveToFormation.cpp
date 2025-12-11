/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTTask_XBMoveToFormation.cpp

/**
 * @file BTTask_XBMoveToFormation.cpp
 * @brief 行为树任务 - 移动到编队位置实现
 * 
 * @note ✨ 新增文件
 */

#include "AI/BehaviorTree/BTTask_XBMoveToFormation.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "AI/XBSoldierAIController.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_XBMoveToFormation::UBTTask_XBMoveToFormation()
{
    // 设置节点名称
    NodeName = TEXT("移动到编队位置");
    
    // 启用Tick用于检测到达
    // 说明: 需要在执行期间检测是否到达目标位置
    bNotifyTick = true;
    
    // 配置黑板键过滤器
    FormationPositionKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBMoveToFormation, FormationPositionKey));
}

EBTNodeResult::Type UBTTask_XBMoveToFormation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // 获取AI控制器
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return EBTNodeResult::Failed;
    }
    
    // 获取士兵Actor
    AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(AIController->GetPawn());
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
    
    // 获取目标位置
    // 说明: 从士兵Actor计算编队位置或从黑板读取
    FVector TargetLocation;
    
    if (FormationPositionKey.SelectedKeyName != NAME_None)
    {
        TargetLocation = BlackboardComp->GetValueAsVector(FormationPositionKey.SelectedKeyName);
    }
    else
    {
        // 直接从士兵获取编队位置
        TargetLocation = Soldier->GetFormationWorldPosition();
    }
    
    // 检查是否已经到达
    // 说明: 如果已经很近就直接返回成功
    float DistanceToTarget = FVector::Dist2D(Soldier->GetActorLocation(), TargetLocation);
    if (DistanceToTarget <= AcceptableRadius)
    {
        UE_LOG(LogTemp, Verbose, TEXT("士兵 %s 已在编队位置"), *Soldier->GetName());
        return EBTNodeResult::Succeeded;
    }
    
    // 发起移动请求
    // 说明: 使用AI控制器的移动功能
    EPathFollowingRequestResult::Type MoveResult;
    
    if (bUseDirectMove)
    {
        // 直接移动（不寻路）
        // 说明: 适用于开阔地形或简单场景
        MoveResult = AIController->MoveToLocation(TargetLocation, AcceptableRadius, true, true, false, false);
    }
    else
    {
        // 使用寻路移动
        // 说明: 适用于复杂地形
        MoveResult = AIController->MoveToLocation(TargetLocation, AcceptableRadius, true, true, true, true);
    }
    
    if (MoveResult == EPathFollowingRequestResult::RequestSuccessful || 
        MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
    {
        if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
        {
            return EBTNodeResult::Succeeded;
        }
        
        UE_LOG(LogTemp, Verbose, TEXT("士兵 %s 开始移动到编队位置"), *Soldier->GetName());
        
        // 返回InProgress，等待移动完成
        // 说明: Tick中会检测是否到达
        return EBTNodeResult::InProgress;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("士兵 %s 移动请求失败"), *Soldier->GetName());
    return EBTNodeResult::Failed;
}

void UBTTask_XBMoveToFormation::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    // 获取AI控制器
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    // 获取士兵Actor
    AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(AIController->GetPawn());
    if (!Soldier)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    // 获取黑板组件更新目标位置
    // 说明: 编队位置可能随将领移动而变化
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    FVector TargetLocation;
    
    if (BlackboardComp && FormationPositionKey.SelectedKeyName != NAME_None)
    {
        TargetLocation = BlackboardComp->GetValueAsVector(FormationPositionKey.SelectedKeyName);
    }
    else
    {
        TargetLocation = Soldier->GetFormationWorldPosition();
    }
    
    // 检查是否到达
    float DistanceToTarget = FVector::Dist2D(Soldier->GetActorLocation(), TargetLocation);
    if (DistanceToTarget <= AcceptableRadius)
    {
        // 到达目标位置
        AIController->StopMovement();
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }
    
    // 检查移动状态
    // 说明: 如果移动已完成但未到达，可能需要重新请求
    UPathFollowingComponent* PathFollowComp = AIController->GetPathFollowingComponent();
    if (PathFollowComp && PathFollowComp->GetStatus() == EPathFollowingStatus::Idle)
    {
        // 移动已停止但未到达，重新发起移动
        // 说明: 这可能发生在目标位置动态变化时
        AIController->MoveToLocation(TargetLocation, AcceptableRadius, true, true, !bUseDirectMove, true);
    }
}

FString UBTTask_XBMoveToFormation::GetStaticDescription() const
{
    return FString::Printf(TEXT("移动到编队位置\n到达阈值: %.1f\n%s"), 
        AcceptableRadius,
        bUseDirectMove ? TEXT("直接移动") : TEXT("寻路移动"));
}
