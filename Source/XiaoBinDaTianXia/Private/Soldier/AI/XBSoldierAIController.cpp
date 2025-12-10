/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Soldier/AI/XBSoldierAIController.cpp

/**
 * @file XBSoldierAIController.cpp
 * @brief 士兵AI控制器实现
 */

#include "Soldier/AI/XBSoldierAIController.h"
#include "Soldier/XBSoldierActor.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

// 黑板键名定义
const FName AXBSoldierAIController::BB_TargetActor = TEXT("TargetActor");
const FName AXBSoldierAIController::BB_Leader = TEXT("Leader");
const FName AXBSoldierAIController::BB_SoldierState = TEXT("SoldierState");
const FName AXBSoldierAIController::BB_FormationPosition = TEXT("FormationPosition");
const FName AXBSoldierAIController::BB_AttackRange = TEXT("AttackRange");
const FName AXBSoldierAIController::BB_CanAttack = TEXT("CanAttack");

AXBSoldierAIController::AXBSoldierAIController()
{
    // 创建行为树组件
    BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));

    // 创建黑板组件
    BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));

    // 设置感知组件（可选）
    // 如需使用AIPerception，在此处创建
}

void AXBSoldierAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    AXBSoldierActor* Soldier = Cast<AXBSoldierActor>(InPawn);
    if (!Soldier)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("士兵AI控制器附身: %s"), *Soldier->GetName());

    // 初始化黑板
    if (BlackboardComponent && Soldier->BehaviorTreeAsset)
    {
        if (Soldier->BehaviorTreeAsset->BlackboardAsset)
        {
            BlackboardComponent->InitializeBlackboard(*Soldier->BehaviorTreeAsset->BlackboardAsset);
        }
    }

    // 设置初始黑板值
    if (BlackboardComponent)
    {
        BlackboardComponent->SetValueAsObject(BB_Leader, Soldier->GetFollowTarget());
        BlackboardComponent->SetValueAsEnum(BB_SoldierState, static_cast<uint8>(Soldier->GetSoldierState()));
        BlackboardComponent->SetValueAsFloat(BB_AttackRange, Soldier->GetSoldierConfig().AttackRange);
    }

    // 启动行为树
    if (Soldier->BehaviorTreeAsset)
    {
        StartBehaviorTree(Soldier->BehaviorTreeAsset);
    }
}

void AXBSoldierAIController::OnUnPossess()
{
    StopBehaviorTree();
    Super::OnUnPossess();
}

bool AXBSoldierAIController::StartBehaviorTree(UBehaviorTree* BehaviorTree)
{
    if (!BehaviorTree)
    {
        UE_LOG(LogTemp, Warning, TEXT("士兵AI: 无法启动空的行为树"));
        return false;
    }

    if (!BehaviorTreeComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("士兵AI: 行为树组件无效"));
        return false;
    }

    // 初始化黑板（如果尚未初始化）
    if (BlackboardComponent && BehaviorTree->BlackboardAsset)
    {
        if (!BlackboardComponent->IsInitialized())
        {
            BlackboardComponent->InitializeBlackboard(*BehaviorTree->BlackboardAsset);
        }
    }

    // 启动行为树
    bool bSuccess = BehaviorTreeComponent->StartTree(*BehaviorTree);
    
    UE_LOG(LogTemp, Log, TEXT("士兵AI: 行为树启动 %s - %s"), 
        *BehaviorTree->GetName(), bSuccess ? TEXT("成功") : TEXT("失败"));

    return bSuccess;
}

void AXBSoldierAIController::StopBehaviorTree()
{
    if (BehaviorTreeComponent)
    {
        BehaviorTreeComponent->StopTree();
        UE_LOG(LogTemp, Log, TEXT("士兵AI: 行为树已停止"));
    }
}

void AXBSoldierAIController::SetTargetActor(AActor* Target)
{
    if (BlackboardComponent)
    {
        BlackboardComponent->SetValueAsObject(BB_TargetActor, Target);
    }
}

AActor* AXBSoldierAIController::GetTargetActor() const
{
    if (BlackboardComponent)
    {
        return Cast<AActor>(BlackboardComponent->GetValueAsObject(BB_TargetActor));
    }
    return nullptr;
}

void AXBSoldierAIController::SetSoldierState(uint8 State)
{
    if (BlackboardComponent)
    {
        BlackboardComponent->SetValueAsEnum(BB_SoldierState, State);
    }
}

void AXBSoldierAIController::UpdateFormationPosition()
{
    AXBSoldierActor* Soldier = GetSoldierPawn();
    if (!Soldier)
    {
        return;
    }

    FVector FormationPos = Soldier->GetFormationWorldPosition();
    
    if (BlackboardComponent)
    {
        BlackboardComponent->SetValueAsVector(BB_FormationPosition, FormationPos);
    }
}

AXBSoldierActor* AXBSoldierAIController::GetSoldierPawn() const
{
    return Cast<AXBSoldierActor>(GetPawn());
}
