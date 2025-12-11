/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTTask_XBFindEnemy.cpp

/**
 * @file BTTask_XBFindEnemy.cpp
 * @brief 行为树任务 - 寻找敌人实现
 * 
 * @note ✨ 新增文件
 */

#include "AI/BehaviorTree/BTTask_XBFindEnemy.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Character/XBCharacterBase.h"
#include "Kismet/GameplayStatics.h"
#include "AI/XBSoldierAIController.h"

UBTTask_XBFindEnemy::UBTTask_XBFindEnemy()
{
    // 设置节点名称
    // 说明: 在行为树编辑器中显示的节点名称
    NodeName = TEXT("寻找敌人");
    
    // 配置黑板键过滤器
    // 说明: 限制目标键只能选择Actor类型
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBFindEnemy, TargetKey), AActor::StaticClass());
    DetectionRangeKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBFindEnemy, DetectionRangeKey));
}

EBTNodeResult::Type UBTTask_XBFindEnemy::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // 获取AI控制器
    // 说明: 从行为树组件获取关联的AI控制器
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        UE_LOG(LogTemp, Warning, TEXT("BTTask_FindEnemy: 无法获取AI控制器"));
        return EBTNodeResult::Failed;
    }
    
    // 获取控制的Pawn
    // 说明: AI控制器控制的士兵Actor
    APawn* ControlledPawn = AIController->GetPawn();
    AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(ControlledPawn);
    if (!Soldier)
    {
        UE_LOG(LogTemp, Warning, TEXT("BTTask_FindEnemy: 被控制的Pawn不是士兵类型"));
        return EBTNodeResult::Failed;
    }
    
    // 获取黑板组件
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("BTTask_FindEnemy: 无法获取黑板组件"));
        return EBTNodeResult::Failed;
    }
    
    // 获取检测范围
    // 说明: 优先从黑板读取，否则使用默认值
    float DetectionRange = DefaultDetectionRange;
    if (DetectionRangeKey.SelectedKeyName != NAME_None)
    {
        DetectionRange = BlackboardComp->GetValueAsFloat(DetectionRangeKey.SelectedKeyName);
        if (DetectionRange <= 0.0f)
        {
            DetectionRange = DefaultDetectionRange;
        }
    }
    
    // 获取士兵的阵营
    // 说明: 用于判断哪些目标是敌对的
    EXBFaction SoldierFaction = Soldier->GetFaction();
    FVector SoldierLocation = Soldier->GetActorLocation();
    
    // 收集潜在目标
    // 说明: 搜索所有角色和士兵Actor
    TArray<AActor*> PotentialTargets;
    
    // 获取所有角色类敌人
    TArray<AActor*> Characters;
    UGameplayStatics::GetAllActorsOfClass(Soldier->GetWorld(), AXBCharacterBase::StaticClass(), Characters);
    PotentialTargets.Append(Characters);
    
    // 获取所有士兵敌人
    TArray<AActor*> Soldiers;
    UGameplayStatics::GetAllActorsOfClass(Soldier->GetWorld(), AXBSoldierCharacter::StaticClass(), Soldiers);
    PotentialTargets.Append(Soldiers);
    
    // 寻找最近的敌人
    // 说明: 遍历所有潜在目标，筛选敌对且最近的
    AActor* NearestEnemy = nullptr;
    float NearestDistance = DetectionRange;
    
    for (AActor* Target : PotentialTargets)
    {
        // 跳过自身
        if (Target == Soldier)
        {
            continue;
        }
        
        // 检查是否为敌对目标
        // 说明: 根据阵营判断敌对关系
        bool bIsEnemy = false;
        
        if (const AXBCharacterBase* CharTarget = Cast<AXBCharacterBase>(Target))
        {
            // 判断角色是否敌对
            // 说明: 玩家/友军 vs 敌人
            if (SoldierFaction == EXBFaction::Player || SoldierFaction == EXBFaction::Ally)
            {
                bIsEnemy = (CharTarget->GetFaction() == EXBFaction::Enemy);
            }
            else if (SoldierFaction == EXBFaction::Enemy)
            {
                bIsEnemy = (CharTarget->GetFaction() == EXBFaction::Player || 
                           CharTarget->GetFaction() == EXBFaction::Ally);
            }
        }
        else if (const AXBSoldierCharacter* SoldierTarget = Cast<AXBSoldierCharacter>(Target))
        {
            // 跳过死亡的士兵
            if (bIgnoreDeadTargets && SoldierTarget->GetSoldierState() == EXBSoldierState::Dead)
            {
                continue;
            }
            
            // 判断士兵是否敌对
            if (SoldierFaction == EXBFaction::Player || SoldierFaction == EXBFaction::Ally)
            {
                bIsEnemy = (SoldierTarget->GetFaction() == EXBFaction::Enemy);
            }
            else if (SoldierFaction == EXBFaction::Enemy)
            {
                bIsEnemy = (SoldierTarget->GetFaction() == EXBFaction::Player || 
                           SoldierTarget->GetFaction() == EXBFaction::Ally);
            }
        }
        
        if (!bIsEnemy)
        {
            continue;
        }
        
        // 计算距离
        float Distance = FVector::Dist(SoldierLocation, Target->GetActorLocation());
        
        // 检查是否在检测范围内且更近
        if (Distance < NearestDistance)
        {
            NearestDistance = Distance;
            NearestEnemy = Target;
        }
    }
    
    // 更新黑板
    // 说明: 将找到的目标（或nullptr）写入黑板
    if (TargetKey.SelectedKeyName != NAME_None)
    {
        BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, NearestEnemy);
    }
    
    // 同时更新目标位置
    // 说明: 便于移动任务使用
    if (NearestEnemy)
    {
        BlackboardComp->SetValueAsVector(XBSoldierBBKeys::TargetLocation, NearestEnemy->GetActorLocation());
        BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, true);
        
        UE_LOG(LogTemp, Verbose, TEXT("士兵 %s 找到敌人 %s，距离: %.1f"), 
            *Soldier->GetName(), *NearestEnemy->GetName(), NearestDistance);
        
        return EBTNodeResult::Succeeded;
    }
    else
    {
        BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
        
        UE_LOG(LogTemp, Verbose, TEXT("士兵 %s 未找到敌人"), *Soldier->GetName());
        
        // 没找到敌人也算成功（任务执行完毕）
        // 说明: 让行为树继续执行其他逻辑
        return EBTNodeResult::Succeeded;
    }
}

FString UBTTask_XBFindEnemy::GetStaticDescription() const
{
    return FString::Printf(TEXT("在 %.0f 范围内搜索敌人\n目标键: %s"), 
        DefaultDetectionRange, 
        *TargetKey.SelectedKeyName.ToString());
}
