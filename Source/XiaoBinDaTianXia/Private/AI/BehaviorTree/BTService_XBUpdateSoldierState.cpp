/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTService_XBUpdateSoldierState.cpp

/**
 * @file BTService_XBUpdateSoldierState.cpp
 * @brief 行为树服务 - 更新士兵状态实现
 * 
 * @note ✨ 新增文件
 */

#include "AI/BehaviorTree/BTService_XBUpdateSoldierState.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Soldier/XBSoldierActor.h"
#include "Character/XBCharacterBase.h"
#include "AI/XBSoldierAIController.h"
#include "Kismet/GameplayStatics.h"

UBTService_XBUpdateSoldierState::UBTService_XBUpdateSoldierState()
{
    // 设置节点名称
    NodeName = TEXT("更新士兵状态");
    
    // 设置默认更新间隔
    // 说明: 0.2秒更新一次，平衡性能和响应速度
    Interval = 0.2f;
    RandomDeviation = 0.05f;
    
    // 配置黑板键过滤器
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_XBUpdateSoldierState, TargetKey), AActor::StaticClass());
    LeaderKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_XBUpdateSoldierState, LeaderKey), AActor::StaticClass());
}

void UBTService_XBUpdateSoldierState::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    Super::OnBecomeRelevant(OwnerComp, NodeMemory);
    
    // 服务激活时立即更新一次
    // 说明: 确保黑板数据是最新的
    TickNode(OwnerComp, NodeMemory, 0.0f);
}

void UBTService_XBUpdateSoldierState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
    
    // 获取AI控制器
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return;
    }
    
    // 获取士兵Actor
    AXBSoldierActor* Soldier = Cast<AXBSoldierActor>(AIController->GetPawn());
    if (!Soldier)
    {
        return;
    }
    
    // 获取黑板组件
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return;
    }
    
    FVector SoldierLocation = Soldier->GetActorLocation();
    
    // ==================== 更新目标状态 ====================
    
    AActor* CurrentTarget = nullptr;
    if (TargetKey.SelectedKeyName != NAME_None)
    {
        CurrentTarget = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
    }
    
    // 检查目标有效性
    // 说明: 目标可能已死亡或被销毁
    bool bTargetValid = false;
    if (bCheckTargetValidity && CurrentTarget)
    {
        // 检查目标是否已死亡
        if (const AXBSoldierActor* TargetSoldier = Cast<AXBSoldierActor>(CurrentTarget))
        {
            bTargetValid = (TargetSoldier->GetSoldierState() != EXBSoldierState::Dead);
        }
        else if (const AXBCharacterBase* TargetChar = Cast<AXBCharacterBase>(CurrentTarget))
        {
            // 假设角色有死亡检测（可根据实际实现调整）
            bTargetValid = IsValid(TargetChar);
        }
        else
        {
            bTargetValid = IsValid(CurrentTarget);
        }
        
        // 目标失效时清除
        if (!bTargetValid)
        {
            BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, nullptr);
            CurrentTarget = nullptr;
            UE_LOG(LogTemp, Verbose, TEXT("士兵 %s 的目标已失效"), *Soldier->GetName());
        }
    }
    
    // 更新目标相关黑板值
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, CurrentTarget != nullptr);
    
    if (CurrentTarget)
    {
        float DistToTarget = FVector::Dist(SoldierLocation, CurrentTarget->GetActorLocation());
        BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToTarget, DistToTarget);
        BlackboardComp->SetValueAsVector(XBSoldierBBKeys::TargetLocation, CurrentTarget->GetActorLocation());
    }
    else
    {
        BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToTarget, MAX_FLT);
    }
    
    // ==================== 更新将领状态 ====================
    
    AActor* Leader = nullptr;
    if (LeaderKey.SelectedKeyName != NAME_None)
    {
        Leader = Cast<AActor>(BlackboardComp->GetValueAsObject(LeaderKey.SelectedKeyName));
    }
    
    // 如果黑板中没有将领，从士兵Actor获取
    if (!Leader)
    {
        Leader = Soldier->GetFollowTarget();
        if (Leader && LeaderKey.SelectedKeyName != NAME_None)
        {
            BlackboardComp->SetValueAsObject(LeaderKey.SelectedKeyName, Leader);
        }
    }
    
    // 更新到将领的距离
    if (Leader)
    {
        float DistToLeader = FVector::Dist(SoldierLocation, Leader->GetActorLocation());
        BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToLeader, DistToLeader);
        
        // 判断是否应该撤退
        // 说明: 超过脱离距离时应返回将领身边
        bool bShouldRetreat = (DistToLeader > DisengageDistance);
        BlackboardComp->SetValueAsBool(XBSoldierBBKeys::ShouldRetreat, bShouldRetreat);
    }
    
    // ==================== 更新编队位置 ====================
    
    FVector FormationPos = Soldier->GetFormationWorldPosition();
    BlackboardComp->SetValueAsVector(XBSoldierBBKeys::FormationPosition, FormationPos);
    
    // 检查是否在编队位置
    float DistToFormation = FVector::Dist2D(SoldierLocation, FormationPos);
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::IsAtFormation, DistToFormation <= 50.0f);
    
    // ==================== 更新攻击状态 ====================
    
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::CanAttack, Soldier->CanAttack());
    
    // 更新士兵状态枚举
    BlackboardComp->SetValueAsEnum(XBSoldierBBKeys::SoldierState, static_cast<uint8>(Soldier->GetSoldierState()));
    
    // 战斗状态
    bool bInCombat = (Soldier->GetSoldierState() == EXBSoldierState::Combat);
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::IsInCombat, bInCombat);
    
    // ==================== 自动寻找目标 ====================
    
    // 如果在战斗中但没有目标，自动寻找
    // 说明: 提高AI响应速度
    if (bAutoFindTarget && bInCombat && !CurrentTarget)
    {
        AActor* NewTarget = Soldier->FindNearestEnemy();
        if (NewTarget && TargetKey.SelectedKeyName != NAME_None)
        {
            BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, NewTarget);
            BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, true);
            
            UE_LOG(LogTemp, Log, TEXT("士兵 %s 自动找到新目标 %s"), 
                *Soldier->GetName(), *NewTarget->GetName());
        }
    }
}

FString UBTService_XBUpdateSoldierState::GetStaticDescription() const
{
    return FString::Printf(TEXT("更新士兵状态\n目标键: %s\n将领键: %s\n脱离距离: %.0f"),
        *TargetKey.SelectedKeyName.ToString(),
        *LeaderKey.SelectedKeyName.ToString(),
        DisengageDistance);
}
