/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTService_XBUpdateSoldierState.cpp

/**
 * @file BTService_XBUpdateSoldierState.cpp
 * @brief 行为树服务 - 更新士兵状态
 * 
 * @note 🔧 重构 - 使用感知子系统和行为接口
 */

#include "AI/BehaviorTree/BTService_XBUpdateSoldierState.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Soldier/Component/XBSoldierBehaviorInterface.h"  // ✨ 新增
#include "Character/XBCharacterBase.h"
#include "AI/XBSoldierAIController.h"

UBTService_XBUpdateSoldierState::UBTService_XBUpdateSoldierState()
{
    NodeName = TEXT("更新士兵状态");
    Interval = 0.2f;
    RandomDeviation = 0.05f;
    
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_XBUpdateSoldierState, TargetKey), AActor::StaticClass());
    LeaderKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_XBUpdateSoldierState, LeaderKey), AActor::StaticClass());
}

void UBTService_XBUpdateSoldierState::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    Super::OnBecomeRelevant(OwnerComp, NodeMemory);
    TickNode(OwnerComp, NodeMemory, 0.0f);
}

/**
 * @brief 定期更新黑板
 * @note 🔧 重构 - 使用行为接口获取状态
 */
void UBTService_XBUpdateSoldierState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
    
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return;
    }
    
    AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(AIController->GetPawn());
    if (!Soldier)
    {
        return;
    }
    
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return;
    }
    
    // ✨ 获取行为接口
    UXBSoldierBehaviorInterface* BehaviorInterface = Soldier->GetBehaviorInterface();
    
    FVector SoldierLocation = Soldier->GetActorLocation();
    
    // ==================== 更新目标状态 ====================
    
    AActor* CurrentTarget = nullptr;
    if (TargetKey.SelectedKeyName != NAME_None)
    {
        CurrentTarget = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
    }
    
    // 🔧 修改 - 通过行为接口检查目标有效性
    bool bTargetValid = false;
    if (bCheckTargetValidity && CurrentTarget && BehaviorInterface)
    {
        bTargetValid = BehaviorInterface->IsTargetValid(CurrentTarget);
        
        if (!bTargetValid)
        {
            BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, nullptr);
            CurrentTarget = nullptr;
            Soldier->CurrentAttackTarget = nullptr;
            UE_LOG(LogTemp, Verbose, TEXT("士兵 %s 的目标已失效"), *Soldier->GetName());
        }
    }
    
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, CurrentTarget != nullptr);
    
    if (CurrentTarget)
    {
        Soldier->CurrentAttackTarget = CurrentTarget;
        float DistToTarget = FVector::Dist(SoldierLocation, CurrentTarget->GetActorLocation());
        BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToTarget, DistToTarget);
        BlackboardComp->SetValueAsVector(XBSoldierBBKeys::TargetLocation, CurrentTarget->GetActorLocation());

        // 🔧 修改 - 目标有效时更新“看见敌人时间”，避免战斗中过早脱离
        if (BehaviorInterface)
        {
            BehaviorInterface->RecordEnemySeen();
        }
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
    
    if (!Leader)
    {
        Leader = Soldier->GetFollowTarget();
        if (Leader && LeaderKey.SelectedKeyName != NAME_None)
        {
            BlackboardComp->SetValueAsObject(LeaderKey.SelectedKeyName, Leader);
        }
    }
    
    if (Leader)
    {
        float DistToLeader = FVector::Dist(SoldierLocation, Leader->GetActorLocation());
        BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToLeader, DistToLeader);
        
        // 🔧 修改 - 通过行为接口判断是否应该撤退
        float DisengageDistanceValue = Soldier->GetDisengageDistance();
        bool bShouldRetreat = false;
        if (BehaviorInterface)
        {
            bShouldRetreat = BehaviorInterface->ShouldDisengage();
            bShouldRetreat = bShouldRetreat && (DistToLeader >= DisengageDistanceValue);
        }
        else
        {
            bShouldRetreat = (DistToLeader >= DisengageDistanceValue);
        }
        BlackboardComp->SetValueAsBool(XBSoldierBBKeys::ShouldRetreat, bShouldRetreat);
    }
    
    // ==================== 更新攻击状态 ====================
    
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::CanAttack, Soldier->CanAttack());
    BlackboardComp->SetValueAsEnum(XBSoldierBBKeys::SoldierState, static_cast<uint8>(Soldier->GetSoldierState()));
    
    bool bInCombat = (Soldier->GetSoldierState() == EXBSoldierState::Combat);
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::IsInCombat, bInCombat);
    
    // ==================== 自动寻找目标 ====================
    
    if (bAutoFindTarget && bInCombat && !CurrentTarget && BehaviorInterface)
    {
        AActor* NewTarget = nullptr;
        if (BehaviorInterface->SearchForEnemy(NewTarget))
        {
            if (NewTarget && TargetKey.SelectedKeyName != NAME_None)
            {
                BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, NewTarget);
                BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, true);
                
                UE_LOG(LogTemp, Log, TEXT("士兵 %s 自动找到新目标 %s"), 
                    *Soldier->GetName(), *NewTarget->GetName());
            }
        }
    }
}

FString UBTService_XBUpdateSoldierState::GetStaticDescription() const
{
    return FString::Printf(TEXT("更新士兵状态（使用感知子系统）\n目标键: %s\n将领键: %s"),
        *TargetKey.SelectedKeyName.ToString(),
        *LeaderKey.SelectedKeyName.ToString());
}
