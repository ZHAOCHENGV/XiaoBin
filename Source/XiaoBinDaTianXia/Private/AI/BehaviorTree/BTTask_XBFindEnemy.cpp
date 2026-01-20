/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTTask_XBFindEnemy.cpp

/**
 * @file BTTask_XBFindEnemy.cpp
 * @brief 行为树任务 - 寻找敌人
 *
 * @note 🔧 重构 - 使用感知子系统与行为接口
 */

#include "AI/BehaviorTree/BTTask_XBFindEnemy.h"
#include "Utils/XBLogCategories.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Soldier/Component/XBSoldierBehaviorInterface.h"  // ✅新增
#include "AI/XBSoldierAIController.h"

// 🔧 修改 - 按要求补充构造函数头部注释与逐行注释
/**
 * @brief 构造寻敌任务并初始化黑板键过滤器
 * @param 无
 * @return 无
 * 功能说明: 设置节点名称并限制目标/范围键类型
 * 详细流程: 设置显示名称 -> 配置目标键 -> 配置范围键
 * 注意事项: 目标键需为对象类型
 */
UBTTask_XBFindEnemy::UBTTask_XBFindEnemy()
{
    // 设置任务在行为树中的显示名称
    NodeName = TEXT("寻找敌人");
    
    // 配置黑板目标键对象类型过滤
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBFindEnemy, TargetKey), AActor::StaticClass());
    // 配置黑板范围键浮点类型过滤
    DetectionRangeKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBFindEnemy, DetectionRangeKey));
}

// 🔧 修改 - 按要求补充执行函数头部注释与逐行注释
/**
 * @brief 执行寻敌任务
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @return 行为树执行结果
 * 功能说明: 检查已分配目标并更新黑板
 * 详细流程: 获取控制器与士兵 -> 获取黑板 -> 校验目标有效性 -> 写回黑板与缓存
 * 注意事项: 死亡状态下直接失败
 */
EBTNodeResult::Type UBTTask_XBFindEnemy::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // 获取 AI 控制器
    AAIController* AIController = OwnerComp.GetAIOwner();
    // 控制器为空则任务失败
    if (!AIController)
    {
        // 🔧 修改 - 打印中文日志提示控制器为空
        UE_LOG(LogXBAI, Warning, TEXT("寻敌任务: 无法获取AI控制器"));
        // 返回失败
        return EBTNodeResult::Failed;
    }
    
    // 获取受控 Pawn
    APawn* ControlledPawn = AIController->GetPawn();
    // 转换为士兵对象
    AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(ControlledPawn);
    // 士兵无效则任务失败
    if (!Soldier)
    {
        // 🔧 修改 - 打印中文日志提示Pawn无效
        UE_LOG(LogXBAI, Warning, TEXT("寻敌任务: Pawn无效或不是士兵"));
        // 返回失败
        return EBTNodeResult::Failed;
    }
    
    // 若士兵死亡则任务失败
    if (Soldier->GetSoldierState() == EXBSoldierState::Dead)
    {
        // 返回失败
        return EBTNodeResult::Failed;
    }
    
    // 获取黑板组件
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    // 黑板为空则任务失败
    if (!BlackboardComp)
    {
        // 🔧 修改 - 打印中文日志提示黑板为空
        UE_LOG(LogXBAI, Warning, TEXT("寻敌任务: 无法获取黑板组件"));
        // 返回失败
        return EBTNodeResult::Failed;
    }
    
    // 获取行为接口用于寻敌
    UXBSoldierBehaviorInterface* BehaviorInterface = Soldier->GetBehaviorInterface();
    // 行为接口为空则任务失败
    if (!BehaviorInterface)
    {
        // 🔧 修改 - 打印中文日志提示行为接口为空
        UE_LOG(LogXBAI, Warning, TEXT("寻敌任务: 无法获取行为接口"));
        // 返回失败
        return EBTNodeResult::Failed;
    }
    
    // 使用已分配目标
    AActor* NearestEnemy = Soldier->CurrentAttackTarget.Get();
    bool bFound = BehaviorInterface->IsTargetValid(NearestEnemy);
    
    // ==================== 更新黑板 ====================
    
    // 若配置了目标键则写入目标
    if (TargetKey.SelectedKeyName != NAME_None)
    {
        // 写入目标对象到黑板
        BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, NearestEnemy);
    }
    
    // 若找到目标则更新黑板与缓存
    if (bFound && NearestEnemy)
    {
        // 同步当前攻击目标
        Soldier->CurrentAttackTarget = NearestEnemy;
        // 更新目标位置
        BlackboardComp->SetValueAsVector(XBSoldierBBKeys::TargetLocation, NearestEnemy->GetActorLocation());
        // 更新是否有目标标记
        BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, true);
        
        // 计算与目标的距离
        float Distance = FVector::Dist(Soldier->GetActorLocation(), NearestEnemy->GetActorLocation());
        // 🔧 修改 - 打印中文日志提示寻敌成功
        UE_LOG(LogXBAI, Verbose, TEXT("士兵 %s 找到敌人 %s，距离 %.1f"),
            *Soldier->GetName(), *NearestEnemy->GetName(), Distance);
    }
    else
    {
        // 未找到目标时清空缓存
        Soldier->CurrentAttackTarget = nullptr;
        BlackboardComp->ClearValue(XBSoldierBBKeys::TargetLocation);
        // 更新是否有目标标记
        BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
        UE_LOG(LogXBAI, Warning, TEXT("寻敌任务: 无法寻找到目标"));
    }
    
    // 返回成功以继续行为树
    return EBTNodeResult::Succeeded;
}

// 🔧 修改 - 按要求补充描述函数头部注释与逐行注释
/**
 * @brief 获取任务静态描述
 * @param 无
 * @return 描述字符串
 * 功能说明: 展示寻敌任务的目标键
 * 详细流程: 拼接固定文本与键名
 * 注意事项: 仅用于编辑器显示
 */
FString UBTTask_XBFindEnemy::GetStaticDescription() const
{
    // 返回描述字符串
    return FString::Printf(TEXT("检查已分配目标\n目标键: %s"),
        *TargetKey.SelectedKeyName.ToString());
}
