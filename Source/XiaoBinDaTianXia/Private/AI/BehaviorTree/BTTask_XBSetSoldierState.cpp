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
#include "Soldier/XBSoldierCharacter.h"
#include "AI/XBSoldierAIController.h"

// 🔧 修改 - 按要求补充构造函数头部注释与逐行注释
/**
 * @brief 构造设置状态任务并初始化目标键过滤器
 * @param 无
 * @return 无
 * 功能说明: 设置节点名称并配置目标键类型
 * 详细流程: 设置显示名称 -> 配置目标键过滤器
 * 注意事项: 目标键需为对象类型
 */
UBTTask_XBSetSoldierState::UBTTask_XBSetSoldierState()
{
    // 设置节点显示名称
    NodeName = TEXT("设置士兵状态");
    
    // 配置黑板目标键过滤器
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBSetSoldierState, TargetKey), AActor::StaticClass());
}

// 🔧 修改 - 按要求补充执行函数头部注释与逐行注释
/**
 * @brief 执行设置状态任务
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @return 行为树执行结果
 * 功能说明: 设置士兵状态并同步黑板
 * 详细流程: 获取控制器与士兵 -> 设置状态 -> 更新黑板 -> 执行额外状态逻辑
 * 注意事项: 清理目标受 bClearTarget 控制
 */
EBTNodeResult::Type UBTTask_XBSetSoldierState::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // 获取 AI 控制器
    AAIController* AIController = OwnerComp.GetAIOwner();
    // 控制器为空则失败
    if (!AIController)
    {
        // 返回失败
        return EBTNodeResult::Failed;
    }
    
    // 获取士兵对象
    AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(AIController->GetPawn());
    // 士兵为空则失败
    if (!Soldier)
    {
        // 返回失败
        return EBTNodeResult::Failed;
    }
    
    // 获取黑板组件
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    
    // 设置士兵状态
    Soldier->SetSoldierState(NewState);
    
    // 更新黑板数据
    if (BlackboardComp)
    {
        // 写入士兵状态
        BlackboardComp->SetValueAsEnum(XBSoldierBBKeys::SoldierState, static_cast<uint8>(NewState));
        
        // 写入战斗标记
        bool bInCombat = (NewState == EXBSoldierState::Combat);
        BlackboardComp->SetValueAsBool(XBSoldierBBKeys::IsInCombat, bInCombat);
        
        // 按需清理目标
        if (bClearTarget && TargetKey.SelectedKeyName != NAME_None)
        {
            // 清空目标对象
            BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, nullptr);
            // 标记无目标
            BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
        }
    }
    
    // 根据状态执行额外逻辑
    switch (NewState)
    {
    case EXBSoldierState::Combat:
        // 进入战斗
        Soldier->EnterCombat();
        break;
        
    case EXBSoldierState::Returning:
    case EXBSoldierState::Following:
        // 从战斗状态退出
        if (Soldier->GetSoldierState() == EXBSoldierState::Combat)
        {
            // 退出战斗
            Soldier->ExitCombat();
        }
        break;
        
    default:
        // 其它状态不处理
        break;
    }
    
    // 🔧 修改 - 打印中文日志提示状态变化
    UE_LOG(LogTemp, Log, TEXT("士兵 %s 状态设置为: %d"),
        *Soldier->GetName(), static_cast<int32>(NewState));
    
    // 返回成功
    return EBTNodeResult::Succeeded;
}

// 🔧 修改 - 按要求补充描述函数头部注释与逐行注释
/**
 * @brief 获取任务静态描述
 * @param 无
 * @return 描述字符串
 * 功能说明: 展示当前设置的状态与清理目标选项
 * 详细流程: 根据状态转换为文本 -> 拼接清理标记
 * 注意事项: 仅用于编辑器显示
 */
FString UBTTask_XBSetSoldierState::GetStaticDescription() const
{
    // 定义状态字符串
    FString StateString;
    // 根据状态枚举转换文本
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
    
    // 返回描述字符串
    return FString::Printf(TEXT("设置状态: %s%s"),
        *StateString,
        bClearTarget ? TEXT("\n[清理目标]") : TEXT(""));
}
