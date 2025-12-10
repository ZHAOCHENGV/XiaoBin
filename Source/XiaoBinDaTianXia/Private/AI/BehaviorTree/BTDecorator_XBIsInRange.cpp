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

UBTDecorator_XBIsInRange::UBTDecorator_XBIsInRange()
{
    NodeName = TEXT("在范围内?");
    
    // 配置黑板键过滤器
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_XBIsInRange, TargetKey), AActor::StaticClass());
    RangeKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_XBIsInRange, RangeKey));
}

bool UBTDecorator_XBIsInRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    // 获取AI控制器和Pawn
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return false;
    }
    
    APawn* ControlledPawn = AIController->GetPawn();
    if (!ControlledPawn)
    {
        return false;
    }
    
    // 获取黑板组件
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return false;
    }
    
    // 获取目标
    AActor* Target = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
    if (!Target)
    {
        return false;
    }
    
    // 获取范围值
    float Range = DefaultRange;
    if (RangeKey.SelectedKeyName != NAME_None)
    {
        float BBRange = BlackboardComp->GetValueAsFloat(RangeKey.SelectedKeyName);
        if (BBRange > 0.0f)
        {
            Range = BBRange;
        }
    }
    
    // 计算距离
    float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), Target->GetActorLocation());
    
    // 根据检查类型返回结果
    bool bInRange = (Distance <= Range);
    
    switch (CheckType)
    {
    case EXBRangeCheckType::InRange:
        return bInRange;
        
    case EXBRangeCheckType::OutOfRange:
        return !bInRange;
        
    default:
        return false;
    }
}

FString UBTDecorator_XBIsInRange::GetStaticDescription() const
{
    FString CheckTypeStr = (CheckType == EXBRangeCheckType::InRange) ? TEXT("在范围内") : TEXT("超出范围");
    
    return FString::Printf(TEXT("检查%s\n目标键: %s\n范围: %.1f"),
        *CheckTypeStr,
        *TargetKey.SelectedKeyName.ToString(),
        DefaultRange);
}
