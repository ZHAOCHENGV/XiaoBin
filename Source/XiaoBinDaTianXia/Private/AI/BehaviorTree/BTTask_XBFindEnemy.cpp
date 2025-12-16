/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTTask_XBFindEnemy.cpp

/**
 * @file BTTask_XBFindEnemy.cpp
 * @brief 行为树任务 - 寻找敌人
 * 
 * @note 🔧 重构 - 使用感知子系统和行为接口
 */

#include "AI/BehaviorTree/BTTask_XBFindEnemy.h"
#include "Utils/XBLogCategories.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Soldier/Component/XBSoldierBehaviorInterface.h"  // ✨ 新增
#include "AI/XBSoldierAIController.h"

UBTTask_XBFindEnemy::UBTTask_XBFindEnemy()
{
    NodeName = TEXT("寻找敌人");
    
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBFindEnemy, TargetKey), AActor::StaticClass());
    DetectionRangeKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBFindEnemy, DetectionRangeKey));
}

/**
 * @brief 执行任务
 * @note 🔧 核心重构 - 通过 BehaviorInterface 执行感知查询
 */
EBTNodeResult::Type UBTTask_XBFindEnemy::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        UE_LOG(LogXBAI, Warning, TEXT("BTTask_FindEnemy: 无法获取AI控制器"));
        return EBTNodeResult::Failed;
    }
    
    APawn* ControlledPawn = AIController->GetPawn();
    AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(ControlledPawn);
    if (!Soldier)
    {
        UE_LOG(LogXBAI, Warning, TEXT("BTTask_FindEnemy: Pawn 无效或不是士兵类型"));
        return EBTNodeResult::Failed;
    }
    
    if (Soldier->GetSoldierState() == EXBSoldierState::Dead)
    {
        return EBTNodeResult::Failed;
    }
    
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        UE_LOG(LogXBAI, Warning, TEXT("BTTask_FindEnemy: 无法获取黑板组件"));
        return EBTNodeResult::Failed;
    }
    
    // ✨ 核心重构 - 通过 BehaviorInterface 搜索敌人
    UXBSoldierBehaviorInterface* BehaviorInterface = Soldier->GetBehaviorInterface();
    if (!BehaviorInterface)
    {
        UE_LOG(LogXBAI, Warning, TEXT("BTTask_FindEnemy: 无法获取行为接口"));
        return EBTNodeResult::Failed;
    }
    
    AActor* NearestEnemy = nullptr;
    bool bFound = BehaviorInterface->SearchForEnemy(NearestEnemy);
    
    // ==================== 更新黑板 ====================
    
    if (TargetKey.SelectedKeyName != NAME_None)
    {
        BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, NearestEnemy);
    }
    
    if (NearestEnemy)
    {
        BlackboardComp->SetValueAsVector(XBSoldierBBKeys::TargetLocation, NearestEnemy->GetActorLocation());
        BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, true);
        
        float Distance = FVector::Dist(Soldier->GetActorLocation(), NearestEnemy->GetActorLocation());
        UE_LOG(LogXBAI, Verbose, TEXT("士兵 %s 找到敌人 %s，距离: %.1f"), 
            *Soldier->GetName(), *NearestEnemy->GetName(), Distance);
    }
    else
    {
        BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
    }
    
    return EBTNodeResult::Succeeded;
}

FString UBTTask_XBFindEnemy::GetStaticDescription() const
{
    return FString::Printf(TEXT("通过感知子系统搜索敌人\n目标键: %s"), 
        *TargetKey.SelectedKeyName.ToString());
}
