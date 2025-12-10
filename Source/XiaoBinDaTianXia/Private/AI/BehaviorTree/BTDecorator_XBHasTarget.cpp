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

UBTDecorator_XBHasTarget::UBTDecorator_XBHasTarget()
{
    NodeName = TEXT("有目标?");
    
    // 配置黑板键过滤器
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_XBHasTarget, TargetKey), AActor::StaticClass());
}

bool UBTDecorator_XBHasTarget::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    // 获取黑板组件
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return false;
    }
    
    // 检查目标是否存在且有效
    UObject* TargetObj = BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName);
    AActor* Target = Cast<AActor>(TargetObj);
    
    return IsValid(Target);
}

FString UBTDecorator_XBHasTarget::GetStaticDescription() const
{
    return FString::Printf(TEXT("检查是否有目标\n目标键: %s"), *TargetKey.SelectedKeyName.ToString());
}
