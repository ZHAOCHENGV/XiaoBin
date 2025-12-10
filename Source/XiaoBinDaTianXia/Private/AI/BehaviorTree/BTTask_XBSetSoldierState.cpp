/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTTask_XBSetSoldierState.cpp

/**
 * @file BTTask_XBSetSoldierState.cpp
 * @brief 行为树任务 - 设置士兵状态实现
 * 
 * @note ✨ 新增文件
 */

#include "AI/BehaviorTree/BTTask_XBSetSoldierState.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Soldier/XBSoldierActor.h"
#include "AI/XBSoldierAIController.h"

UBTTask_XBSetSoldierState::UBTTask_XBSetSoldierState()
{
    // 设置节点名称
    NodeName = TEXT("设置士兵状态");
    
    // 配置黑板键过滤器
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBSetSoldierState, TargetKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_XBSetSoldierState::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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
    
    // 设置士兵状态
    // 说明: 调用士兵的状态设置方法，会触发状态变化委托
    Soldier->SetSoldierState(NewState);
    
    // 更新黑板
    if (BlackboardComp)
    {
        BlackboardComp->SetValueAsEnum(XBSoldierBBKeys::SoldierState, static_cast<uint8>(NewState));
        
        // 更新战斗状态标志
        bool bInCombat = (NewState == EXBSoldierState::Combat);
        BlackboardComp->SetValueAsBool(XBSoldierBBKeys::IsInCombat, bInCombat);
        
        // 清除目标（如果需要）
        // 说明: 退出战斗时通常需要清除目标
        if (bClearTarget && TargetKey.SelectedKeyName != NAME_None)
        {
            BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, nullptr);
            BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
        }
    }
    
    // 根据状态执行额外操作
    switch (NewState)
    {
    case EXBSoldierState::Combat:
        // 进入战斗状态
        Soldier->EnterCombat();
        break;
        
    case EXBSoldierState::Returning:
    case EXBSoldierState::Following:
        // 退出战斗/返回时停止追踪
        if (Soldier->GetSoldierState() == EXBSoldierState::Combat)
        {
            Soldier->ExitCombat();
        }
        break;
        
    default:
        break;
    }
    
    UE_LOG(LogTemp, Log, TEXT("士兵 %s 状态设置为: %d"), 
        *Soldier->GetName(), static_cast<int32>(NewState));
    
    return EBTNodeResult::Succeeded;
}

FString UBTTask_XBSetSoldierState::GetStaticDescription() const
{
    FString StateString;
    switch (NewState)
    {
    case EXBSoldierState::Idle:
        StateString = TEXT("待机");
        break;
    case EXBSoldierState::Following:
        StateString = TEXT("跟随");
        break;
    case EXBSoldierState::Combat:
        StateString = TEXT("战斗");
        break;
    case EXBSoldierState::Seeking:
        StateString = TEXT("搜索");
        break;
    case EXBSoldierState::Returning:
        StateString = TEXT("返回");
        break;
    case EXBSoldierState::Dead:
        StateString = TEXT("死亡");
        break;
    default:
        StateString = TEXT("未知");
        break;
    }
    
    return FString::Printf(TEXT("设置状态: %s%s"),
        *StateString,
        bClearTarget ? TEXT("\n[清除目标]") : TEXT(""));
}
