/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTTask_XBFindEnemy.cpp

/**
 * @file BTTask_XBFindEnemy.cpp
 * @brief 行为树任务 - 寻找敌人实现
 * 
 * @note 🔧 修改记录:
 *       1. 使用通用函数库的球形检测替代全量Actor搜索
 *       2. 增强空指针检查
 *       3. 使用项目专用日志类别
 *       4. 从数据表读取视野范围
 */

#include "AI/BehaviorTree/BTTask_XBFindEnemy.h"
#include "Utils/XBLogCategories.h"
#include "Utils/XBBlueprintFunctionLibrary.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Character/XBCharacterBase.h"
#include "AI/XBSoldierAIController.h"

UBTTask_XBFindEnemy::UBTTask_XBFindEnemy()
{
    NodeName = TEXT("寻找敌人");
    
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBFindEnemy, TargetKey), AActor::StaticClass());
    DetectionRangeKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBFindEnemy, DetectionRangeKey));
}

EBTNodeResult::Type UBTTask_XBFindEnemy::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // ==================== 安全检查 ====================
    
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        UE_LOG(LogXBAI, Warning, TEXT("BTTask_FindEnemy: 无法获取AI控制器"));
        return EBTNodeResult::Failed;
    }
    
    APawn* ControlledPawn = AIController->GetPawn();
    // ✨ 新增 - 增强空指针检查
    if (!ControlledPawn || !IsValid(ControlledPawn))
    {
        UE_LOG(LogXBAI, Warning, TEXT("BTTask_FindEnemy: Pawn 无效"));
        return EBTNodeResult::Failed;
    }
    
    AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(ControlledPawn);
    if (!Soldier)
    {
        UE_LOG(LogXBAI, Warning, TEXT("BTTask_FindEnemy: 被控制的Pawn不是士兵类型"));
        return EBTNodeResult::Failed;
    }
    
    // ✨ 新增 - 检查士兵是否存活
    if (Soldier->GetSoldierState() == EXBSoldierState::Dead)
    {
        UE_LOG(LogXBAI, Verbose, TEXT("BTTask_FindEnemy: 士兵已死亡"));
        return EBTNodeResult::Failed;
    }
    
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        UE_LOG(LogXBAI, Warning, TEXT("BTTask_FindEnemy: 无法获取黑板组件"));
        return EBTNodeResult::Failed;
    }
    
    // ==================== 获取检测范围 ====================
    
    float DetectionRange = DefaultDetectionRange;
    
    // 优先从黑板读取
    if (DetectionRangeKey.SelectedKeyName != NAME_None)
    {
        float BBRange = BlackboardComp->GetValueAsFloat(DetectionRangeKey.SelectedKeyName);
        if (BBRange > 0.0f)
        {
            DetectionRange = BBRange;
        }
    }
    
    // 🔧 修改 - 从士兵数据表读取视野范围
    if (Soldier->IsInitializedFromDataTable())
    {
        float VisionRange = Soldier->GetVisionRange();
        if (VisionRange > 0.0f)
        {
            DetectionRange = VisionRange;
        }
    }
    
    // ==================== 使用球形检测寻找敌人 ====================
    
    EXBFaction SoldierFaction = Soldier->GetFaction();
    FVector SoldierLocation = Soldier->GetActorLocation();
    
    // 🔧 修改 - 使用通用函数库的球形检测
    AActor* NearestEnemy = UXBBlueprintFunctionLibrary::FindNearestEnemy(
        Soldier,                    // WorldContext
        SoldierLocation,            // Origin
        DetectionRange,             // Radius
        SoldierFaction,             // SourceFaction
        bIgnoreDeadTargets          // bIgnoreDead
    );
    
    // ==================== 更新黑板 ====================
    
    if (TargetKey.SelectedKeyName != NAME_None)
    {
        BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, NearestEnemy);
    }
    
    if (NearestEnemy)
    {
        BlackboardComp->SetValueAsVector(XBSoldierBBKeys::TargetLocation, NearestEnemy->GetActorLocation());
        BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, true);
        
        float Distance = FVector::Dist(SoldierLocation, NearestEnemy->GetActorLocation());
        UE_LOG(LogXBAI, Verbose, TEXT("士兵 %s 找到敌人 %s，距离: %.1f"), 
            *Soldier->GetName(), *NearestEnemy->GetName(), Distance);
        
        return EBTNodeResult::Succeeded;
    }
    else
    {
        BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
        
        UE_LOG(LogXBAI, Verbose, TEXT("士兵 %s 未找到敌人（范围: %.0f）"), *Soldier->GetName(), DetectionRange);
        
        return EBTNodeResult::Succeeded;
    }
}

FString UBTTask_XBFindEnemy::GetStaticDescription() const
{
    return FString::Printf(TEXT("在 %.0f 范围内搜索敌人\n目标键: %s\n使用球形检测"), 
        DefaultDetectionRange, 
        *TargetKey.SelectedKeyName.ToString());
}
