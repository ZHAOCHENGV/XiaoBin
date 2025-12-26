/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTTask_XBAttackTarget.cpp

/**
 * @file BTTask_XBAttackTarget.cpp
 * @brief 行为树任务 - 攻击目标
 *
 * @note 🔧 重构 - 使用行为接口执行攻击
 */

#include "AI/BehaviorTree/BTTask_XBAttackTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "AI/XBSoldierAIController.h"
#include "Soldier/Component/XBSoldierBehaviorInterface.h"  // ✅新增

// 🔧 修改 - 按要求补充构造函数头部注释与逐行注释
/**
 * @brief 构造攻击任务并初始化目标键过滤器
 * @param 无
 * @return 无
 * 功能说明: 设置节点名称并限制目标键类型
 * 详细流程: 设置显示名称 -> 配置目标键过滤器
 * 注意事项: 目标键应与黑板中的对象类型一致
 */
UBTTask_XBAttackTarget::UBTTask_XBAttackTarget()
{
    // 设置任务在行为树中的显示名称
    NodeName = TEXT("攻击目标");
    
    // 配置黑板目标键的对象类型过滤
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBAttackTarget, TargetKey), AActor::StaticClass());
}

// 🔧 修改 - 按要求补充执行函数头部注释与逐行注释
/**
 * @brief 执行攻击任务
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @return 行为树执行结果
 * 功能说明: 通过行为接口对当前目标进行攻击
 * 详细流程: 获取控制器与士兵 -> 获取黑板目标 -> 校验目标 -> 调用攻击接口 -> 返回结果
 * 注意事项: 目标无效或接口为空时直接失败
 */
EBTNodeResult::Type UBTTask_XBAttackTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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
    AActor* Target = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
    // 目标为空则任务失败
    if (!Target)
    {
        // 🔧 修改 - 打印中文日志提示目标为空
        UE_LOG(LogTemp, Verbose, TEXT("攻击任务: 目标为空"));
        // 返回失败
        return EBTNodeResult::Failed;
    }

    // 同步当前攻击目标缓存
    Soldier->CurrentAttackTarget = Target;
    
    // 获取行为接口
    UXBSoldierBehaviorInterface* BehaviorInterface = Soldier->GetBehaviorInterface();
    // 行为接口为空则任务失败
    if (!BehaviorInterface)
    {
        // 返回失败
        return EBTNodeResult::Failed;
    }

    // 校验目标有效性
    if (!BehaviorInterface->IsTargetValid(Target))
    {
        // 清空黑板目标
        BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, nullptr);
        // 同步黑板标记为无目标
        BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
        // 清空当前攻击目标缓存
        Soldier->CurrentAttackTarget = nullptr;
        // 返回失败
        return EBTNodeResult::Failed;
    }

    // 再次校验目标有效性（保持原逻辑）
    if (!BehaviorInterface->IsTargetValid(Target))
    {
        // 清空黑板目标
        BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, nullptr);
        // 同步黑板标记为无目标
        BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
        // 清空当前攻击目标缓存
        Soldier->CurrentAttackTarget = nullptr;
        // 返回失败
        return EBTNodeResult::Failed;
    }
    
    // 调用行为接口执行攻击
    EXBBehaviorResult Result = BehaviorInterface->ExecuteAttack(Target);
    
    // 根据行为结果返回节点状态
    switch (Result)
    {
    case EXBBehaviorResult::Success:
        // 攻击成功
        return EBTNodeResult::Succeeded;
        
    case EXBBehaviorResult::InProgress:
        // 冷却中根据配置返回成功或失败
        if (bSucceedOnCooldown)
        {
            // 冷却中也视为成功
            return EBTNodeResult::Succeeded;
        }
        // 冷却中但不允许成功
        return EBTNodeResult::Failed;
        
    case EXBBehaviorResult::Failed:
    default:
        // 攻击失败
        return EBTNodeResult::Failed;
    }
}

// 🔧 修改 - 按要求补充描述函数头部注释与逐行注释
/**
 * @brief 获取任务静态描述
 * @param 无
 * @return 描述字符串
 * 功能说明: 展示当前任务使用的目标黑板键
 * 详细流程: 拼接固定文本与键名
 * 注意事项: 仅用于编辑器显示
 */
FString UBTTask_XBAttackTarget::GetStaticDescription() const
{
    // 返回描述字符串
    return FString::Printf(TEXT("通过行为接口执行攻击\n目标键: %s"), *TargetKey.SelectedKeyName.ToString());
}
