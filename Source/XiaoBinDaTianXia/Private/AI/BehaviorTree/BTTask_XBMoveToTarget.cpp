/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTTask_XBMoveToTarget.cpp

#include "AI/BehaviorTree/BTTask_XBMoveToTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "AI/XBSoldierAIController.h"
#include "Character/XBCharacterBase.h"
#include "Navigation/PathFollowingComponent.h"

// 🔧 修改 - 按要求补充构造函数头部注释与逐行注释
/**
 * @brief 构造移动任务并初始化键过滤器
 * @param 无
 * @return 无
 * 功能说明: 设置节点名称并启用Tick更新
 * 详细流程: 设置显示名称 -> 开启Tick通知 -> 配置目标/范围键过滤
 * 注意事项: 目标键必须为对象类型
 */
UBTTask_XBMoveToTarget::UBTTask_XBMoveToTarget()
{
    // 设置任务在行为树中的显示名称
    NodeName = TEXT("移动到目标");
    
    // 开启Tick更新
    bNotifyTick = true;
    // 开启任务结束通知
    bNotifyTaskFinished = true;
    
    // 配置目标键对象过滤
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBMoveToTarget, TargetKey), AActor::StaticClass());
    // 配置攻击范围键浮点过滤
    AttackRangeKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBMoveToTarget, AttackRangeKey));
}

// 🔧 修改 - 按要求补充执行函数头部注释与逐行注释
/**
 * @brief 执行移动到目标任务
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @return 行为树执行结果
 * 功能说明: 通过寻路移动到目标攻击范围内
 * 详细流程: 获取控制器与目标 -> 校验目标有效性 -> 计算停止距离 -> 下发移动请求
 * 注意事项: 目标死亡时会清空黑板并失败
 */
EBTNodeResult::Type UBTTask_XBMoveToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // 获取 AI 控制器
    AAIController* AIController = OwnerComp.GetAIOwner();
    // 控制器为空则任务失败
    if (!AIController)
    {
        // 返回失败
        return EBTNodeResult::Failed;
    }
    
    // 获取受控士兵
    AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(AIController->GetPawn());
    // 士兵为空则任务失败
    if (!Soldier)
    {
        // 返回失败
        return EBTNodeResult::Failed;
    }
    
    // 获取黑板组件
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    // 黑板为空则任务失败
    if (!BlackboardComp)
    {
        // 返回失败
        return EBTNodeResult::Failed;
    }
    
    // 从黑板读取当前目标
    AActor* CurrentTarget = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
    // 目标为空则任务失败
    if (!CurrentTarget)
    {
        // 返回失败
        return EBTNodeResult::Failed;
    }

    // 若目标为士兵则检查死亡状态
    if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(CurrentTarget))
    {
        // 目标士兵死亡则清理目标
        if (TargetSoldier->GetSoldierState() == EXBSoldierState::Dead)
        {
            // 清空黑板目标
            BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, nullptr);
            // 更新黑板为无目标
            BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
            // 清空当前攻击目标缓存
            Soldier->CurrentAttackTarget = nullptr;
            // 返回失败
            return EBTNodeResult::Failed;
        }
    }
    // 若目标为主将则检查死亡状态
    else if (AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(CurrentTarget))
    {
        // 目标主将死亡则清理目标
        if (TargetLeader->IsDead())
        {
            // 清空黑板目标
            BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, nullptr);
            // 更新黑板为无目标
            BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
            // 清空当前攻击目标缓存
            Soldier->CurrentAttackTarget = nullptr;
            // 返回失败
            return EBTNodeResult::Failed;
        }
    }
    
    // 设置移动时的视觉焦点为当前目标
    AIController->SetFocus(CurrentTarget);
    
    // 使用士兵攻击范围作为停止距离
    float StopDistance = Soldier->GetAttackRange();
    
    // 计算与目标的当前距离
    float CurrentDistance = FVector::Dist(Soldier->GetActorLocation(), CurrentTarget->GetActorLocation());
    // 若已进入攻击范围则直接成功
    if (CurrentDistance <= StopDistance)
    {
        // 清理焦点，避免残留
        AIController->ClearFocus(EAIFocusPriority::Gameplay);
        // 🔧 修改 - 打印中文日志提示已在范围内
        UE_LOG(LogTemp, Verbose, TEXT("士兵 %s 已在目标攻击范围内"), *Soldier->GetName());
        // 返回成功
        return EBTNodeResult::Succeeded;
    }
    
    // 下发移动请求
    EPathFollowingRequestResult::Type MoveResult = AIController->MoveToActor(
        CurrentTarget,
        StopDistance - 10.0f,
        true,
        true
    );
    
    // 请求成功则进入进行中
    if (MoveResult == EPathFollowingRequestResult::RequestSuccessful)
    {
        // 重置目标更新计时器
        TargetUpdateTimer = 0.0f;
        // 返回进行中
        return EBTNodeResult::InProgress;
    }
    // 已在目标处则成功
    else if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
    {
        // 返回成功
        return EBTNodeResult::Succeeded;
    }
    
    // 其它情况视为失败
    return EBTNodeResult::Failed;
}

// 🔧 修改 - 按要求补充Tick函数头部注释与逐行注释
/**
 * @brief Tick 更新移动过程
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @param DeltaSeconds 帧间隔
 * @return 无
 * 功能说明: 持续检查目标有效性与距离，必要时更新移动请求
 * 详细流程: 获取控制器/士兵/黑板 -> 校验目标 -> 更新焦点 -> 判断距离 -> 定期更新移动
 * 注意事项: 目标死亡或丢失会终止任务
 */
void UBTTask_XBMoveToTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    // 获取 AI 控制器
    AAIController* AIController = OwnerComp.GetAIOwner();
    // 控制器为空则结束任务
    if (!AIController)
    {
        // 结束任务并标记失败
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        // 退出Tick
        return;
    }
    
    // 获取受控士兵
    AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(AIController->GetPawn());
    // 士兵为空则结束任务
    if (!Soldier)
    {
        // 结束任务并标记失败
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        // 退出Tick
        return;
    }
    
    // 获取黑板组件
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    // 黑板为空则结束任务
    if (!BlackboardComp)
    {
        // 结束任务并标记失败
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        // 退出Tick
        return;
    }
    
    // 从黑板读取目标
    AActor* Target = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
    // 目标为空则停止移动并失败
    if (!Target)
    {
        // 停止移动
        AIController->StopMovement();
        // 清理焦点
        AIController->ClearFocus(EAIFocusPriority::Gameplay);
        // 结束任务并标记失败
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        // 退出Tick
        return;
    }

    // 若目标为士兵则检查死亡状态
    if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(Target))
    {
        // 目标士兵死亡则清理并结束
        if (TargetSoldier->GetSoldierState() == EXBSoldierState::Dead)
        {
            // 清空黑板目标
            BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, nullptr);
            // 更新黑板为无目标
            BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
            // 清空当前攻击目标缓存
            Soldier->CurrentAttackTarget = nullptr;
            // 停止移动
            AIController->StopMovement();
            // 清理焦点
            AIController->ClearFocus(EAIFocusPriority::Gameplay);
            // 结束任务并标记失败
            FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
            // 退出Tick
            return;
        }
    }
    // 若目标为主将则检查死亡状态
    else if (AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Target))
    {
        // 目标主将死亡则清理并结束
        if (TargetLeader->IsDead())
        {
            // 清空黑板目标
            BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, nullptr);
            // 更新黑板为无目标
            BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
            // 清空当前攻击目标缓存
            Soldier->CurrentAttackTarget = nullptr;
            // 停止移动
            AIController->StopMovement();
            // 清理焦点
            AIController->ClearFocus(EAIFocusPriority::Gameplay);
            // 结束任务并标记失败
            FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
            // 退出Tick
            return;
        }
    }
    
    // 设置移动时焦点为目标
    AIController->SetFocus(Target);
    
    // 使用士兵攻击范围作为停止距离
    float StopDistance = Soldier->GetAttackRange();
    
    // 计算当前距离
    float CurrentDistance = FVector::Dist(Soldier->GetActorLocation(), Target->GetActorLocation());
    // 若进入攻击范围则成功
    if (CurrentDistance <= StopDistance)
    {
        // 停止移动
        AIController->StopMovement();
        // 清理焦点
        AIController->ClearFocus(EAIFocusPriority::Gameplay);
        // 结束任务并标记成功
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        // 退出Tick
        return;
    }
    
    // 累加目标更新计时器
    TargetUpdateTimer += DeltaSeconds;
    // 达到更新间隔则刷新移动
    if (TargetUpdateTimer >= TargetUpdateInterval)
    {
        // 重置计时器
        TargetUpdateTimer = 0.0f;
        
        // 重新下发移动请求
        AIController->MoveToActor(Target, StopDistance - 10.0f, true, true);
    }
}

// 🔧 修改 - 按要求补充中止函数头部注释与逐行注释
/**
 * @brief 中止任务并清理移动状态
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @return 中止结果
 * 功能说明: 停止移动并清理焦点
 * 详细流程: 获取控制器 -> 停止移动 -> 清理焦点 -> 返回中止
 * 注意事项: 仅在任务被打断时调用
 */
EBTNodeResult::Type UBTTask_XBMoveToTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // 获取 AI 控制器并停止移动
    if (AAIController* AIController = OwnerComp.GetAIOwner())
    {
        // 停止移动
        AIController->StopMovement();
        // 清理焦点
        AIController->ClearFocus(EAIFocusPriority::Gameplay);
    }
    
    // 返回中止结果
    return EBTNodeResult::Aborted;
}

// 🔧 修改 - 按要求补充描述函数头部注释与逐行注释
/**
 * @brief 获取任务静态描述
 * @param 无
 * @return 描述字符串
 * 功能说明: 展示目标键与默认停止距离
 * 详细流程: 拼接固定文本与参数
 * 注意事项: 仅用于编辑器显示
 */
FString UBTTask_XBMoveToTarget::GetStaticDescription() const
{
    // 返回描述字符串
    return FString::Printf(TEXT("移动到目标\n目标键: %s\n停止距离: %.1f"),
        *TargetKey.SelectedKeyName.ToString(),
        DefaultStopDistance);
}
