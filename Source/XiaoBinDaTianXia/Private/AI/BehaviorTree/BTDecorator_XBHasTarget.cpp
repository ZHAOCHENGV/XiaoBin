/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTDecorator_XBHasTarget.cpp

/**
 * @file BTDecorator_XBHasTarget.cpp
 * @brief 行为树装饰器 - 检查是否有目标实现
 *
 * @note ✨ 新增文件
 */

#include "AI/BehaviorTree/BTDecorator_XBHasTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

// 🔧 修改 - 按要求补充构造函数头部注释与逐行注释
/**
 * @brief 构造装饰器并初始化目标键过滤器
 * @param 无
 * @return 无
 * 功能说明: 设置节点名称并限制黑板键类型
 * 详细流程: 设置显示名称 -> 配置目标键过滤器
 * 注意事项: 过滤器必须与黑板目标类型匹配
 */
UBTDecorator_XBHasTarget::UBTDecorator_XBHasTarget()
{
    // 设置装饰器在行为树中的显示名称
    NodeName = TEXT("有目标");
    
    // 配置黑板目标键的对象类型过滤
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_XBHasTarget, TargetKey), AActor::StaticClass());
}

// 🔧 修改 - 按要求补充条件计算头部注释与逐行注释
/**
 * @brief 计算装饰器条件是否成立
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @return 条件是否满足
 * 功能说明: 判断黑板中是否存在有效目标
 * 详细流程: 获取黑板 -> 读取对象 -> 转为 Actor -> 校验有效性
 * 注意事项: 黑板为空时直接返回失败
 */
bool UBTDecorator_XBHasTarget::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    // 获取黑板组件用于读取目标
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    // 黑板组件为空则条件不成立
    if (!BlackboardComp)
    {
        // 返回失败，阻止该分支执行
        return false;
    }
    
    // 从黑板读取目标对象
    UObject* TargetObj = BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName);
    // 将目标对象转换为 Actor
    AActor* Target = Cast<AActor>(TargetObj);
    
    // 返回目标是否有效
    return IsValid(Target);
}

// 🔧 修改 - 按要求补充描述函数头部注释与逐行注释
/**
 * @brief 获取装饰器静态描述
 * @param 无
 * @return 描述字符串
 * 功能说明: 展示当前装饰器的目标黑板键
 * 详细流程: 拼接固定文本与键名
 * 注意事项: 仅用于编辑器显示
 */
FString UBTDecorator_XBHasTarget::GetStaticDescription() const
{
    // 组合描述文本并带上目标键名
    return FString::Printf(TEXT("检查是否有目标\n目标键: %s"), *TargetKey.SelectedKeyName.ToString());
}
