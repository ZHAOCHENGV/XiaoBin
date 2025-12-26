/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTDecorator_XBIsInRange.cpp

/**
 * @file BTDecorator_XBIsInRange.cpp
 * @brief 行为树装饰器 - 检查是否在范围内实现
 *
 * @note ✨ 新增文件
 */

#include "AI/BehaviorTree/BTDecorator_XBIsInRange.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Soldier/XBSoldierCharacter.h"

// 🔧 修改 - 按要求补充构造函数头部注释与逐行注释
/**
 * @brief 构造范围检测装饰器并初始化键过滤器
 * @param 无
 * @return 无
 * 功能说明: 设置节点名称并配置目标/范围键过滤
 * 详细流程: 设置显示名称 -> 配置目标键 -> 配置范围键
 * 注意事项: 范围键需为 Float 类型
 */
UBTDecorator_XBIsInRange::UBTDecorator_XBIsInRange()
{
    // 设置装饰器在行为树中的显示名称
    NodeName = TEXT("在范围内?");
    
    // 配置黑板目标键的对象类型过滤
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_XBIsInRange, TargetKey), AActor::StaticClass());
    // 配置黑板范围键的浮点类型过滤
    RangeKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_XBIsInRange, RangeKey));
}

// 🔧 修改 - 按要求补充条件计算头部注释与逐行注释
/**
 * @brief 计算范围检测条件是否成立
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @return 条件是否满足
 * 功能说明: 使用攻击范围或黑板范围判断目标距离
 * 详细流程: 获取控制器 -> 获取Pawn -> 获取黑板 -> 获取目标 -> 计算范围与距离 -> 判断类型
 * 注意事项: 任一关键对象为空则直接失败
 */
bool UBTDecorator_XBIsInRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    // 获取 AI 控制器
    AAIController* AIController = OwnerComp.GetAIOwner();
    // 控制器为空则条件不成立
    if (!AIController)
    {
        // 返回失败
        return false;
    }
    
    // 获取当前受控 Pawn
    APawn* ControlledPawn = AIController->GetPawn();
    // Pawn 为空则条件不成立
    if (!ControlledPawn)
    {
        // 返回失败
        return false;
    }
    
    // 获取黑板组件
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    // 黑板为空则条件不成立
    if (!BlackboardComp)
    {
        // 返回失败
        return false;
    }
    
    // 从黑板获取目标
    AActor* Target = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
    // 目标为空则条件不成立
    if (!Target)
    {
        // 返回失败
        return false;
    }
    
    // 使用默认范围作为初始值
    float Range = DefaultRange;
    // 若 Pawn 是士兵则优先使用士兵攻击范围
    if (const AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(ControlledPawn))
    {
        // 读取士兵攻击范围
        Range = Soldier->GetAttackRange();
    }
    // 否则尝试从黑板读取范围
    else if (RangeKey.SelectedKeyName != NAME_None)
    {
        // 读取黑板范围值
        float BBRange = BlackboardComp->GetValueAsFloat(RangeKey.SelectedKeyName);
        // 黑板范围合法时替换
        if (BBRange > 0.0f)
        {
            // 覆盖检测范围
            Range = BBRange;
        }
    }
    
    // 计算与目标的距离
    float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), Target->GetActorLocation());
    
    // 判断是否在范围内
    bool bInRange = (Distance <= Range);
    
    // 根据检测类型返回结果
    switch (CheckType)
    {
    case EXBRangeCheckType::InRange:
        // 在范围内时返回真
        return bInRange;
        
    case EXBRangeCheckType::OutOfRange:
        // 超出范围时返回真
        return !bInRange;
        
    default:
        // 未知类型直接失败
        return false;
    }
}

// 🔧 修改 - 按要求补充描述函数头部注释与逐行注释
/**
 * @brief 获取装饰器静态描述
 * @param 无
 * @return 描述字符串
 * 功能说明: 展示范围检测类型、目标键与默认范围
 * 详细流程: 生成检测类型文本 -> 拼接描述
 * 注意事项: 仅用于编辑器显示
 */
FString UBTDecorator_XBIsInRange::GetStaticDescription() const
{
    // 根据检测类型生成描述文本
    FString CheckTypeStr = (CheckType == EXBRangeCheckType::InRange) ? TEXT("在范围内") : TEXT("超出范围");
    
    // 返回包含目标键与范围的描述字符串
    return FString::Printf(TEXT("检查%s\n目标键: %s\n范围: %.1f"),
        *CheckTypeStr,
        *TargetKey.SelectedKeyName.ToString(),
        DefaultRange);
}
