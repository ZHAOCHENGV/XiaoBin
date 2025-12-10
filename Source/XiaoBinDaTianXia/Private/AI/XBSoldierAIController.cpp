/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/XBSoldierAIController.cpp

/**
 * @file XBSoldierAIController.cpp
 * @brief 士兵AI控制器实现
 * 
 * @note ✨ 新增文件
 */

#include "AI/XBSoldierAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Soldier/XBSoldierActor.h"
#include "Character/XBCharacterBase.h"
#include "Kismet/GameplayStatics.h"

AXBSoldierAIController::AXBSoldierAIController()
{
    // 创建行为树组件
    // 说明: 行为树组件用于运行和管理行为树逻辑
    BehaviorTreeComp = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
    
    // 创建黑板组件
    // 说明: 黑板组件存储行为树所需的共享数据
    BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
    
    // 启用Tick用于定期更新黑板值
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.0f;
}

void AXBSoldierAIController::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Log, TEXT("士兵AI控制器 %s BeginPlay"), *GetName());
}

void AXBSoldierAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    
    // 空指针检查
    if (!InPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("AI控制器 %s OnPossess: InPawn 为空"), *GetName());
        return;
    }
    
    // 缓存士兵引用
    // 说明: 在控制器接管Pawn时缓存引用，避免重复Cast
    AXBSoldierActor* Soldier = Cast<AXBSoldierActor>(InPawn);
    if (!Soldier)
    {
        UE_LOG(LogTemp, Warning, TEXT("AI控制器 %s 无法识别被控制的Pawn为士兵类型: %s"), 
            *GetName(), *InPawn->GetName());
        return;
    }
    
    CachedSoldier = Soldier;
    
    // 获取士兵配置的行为树
    // 说明: 优先使用士兵Actor上配置的行为树，否则使用控制器默认行为树
    UBehaviorTree* BTToUse = nullptr;
    
    // 检查士兵是否有配置行为树（从数据表加载）
    // 注意: BehaviorTreeAsset 可能为空，需要安全检查
    if (Soldier->BehaviorTreeAsset != nullptr)
    {
        BTToUse = Soldier->BehaviorTreeAsset;
        UE_LOG(LogTemp, Log, TEXT("使用士兵 %s 配置的行为树: %s"), 
            *Soldier->GetName(), *BTToUse->GetName());
    }
    else if (DefaultBehaviorTree != nullptr)
    {
        BTToUse = DefaultBehaviorTree;
        UE_LOG(LogTemp, Log, TEXT("使用默认行为树控制士兵 %s"), *Soldier->GetName());
    }
    
    // 启动行为树
    if (BTToUse)
    {
        StartBehaviorTree(BTToUse);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("士兵 %s 没有配置行为树，将使用简单状态机"), *Soldier->GetName());
    }
}

void AXBSoldierAIController::OnUnPossess()
{
    // 停止行为树
    // 说明: 在控制器释放Pawn时停止行为树，清理资源
    StopBehaviorTreeLogic();
    
    CachedSoldier.Reset();
    
    Super::OnUnPossess();
}

void AXBSoldierAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 定期更新黑板值
    // 说明: 避免每帧更新，使用间隔更新提高性能
    BlackboardUpdateTimer += DeltaTime;
    if (BlackboardUpdateTimer >= BlackboardUpdateInterval)
    {
        BlackboardUpdateTimer = 0.0f;
        UpdateDistanceValues();
    }
}

// ==================== 行为树控制实现 ====================

bool AXBSoldierAIController::StartBehaviorTree(UBehaviorTree* BehaviorTreeAsset)
{
    if (!BehaviorTreeAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("AI控制器 %s 启动行为树失败: 行为树资产为空"), *GetName());
        return false;
    }
    
    // 初始化黑板
    // 说明: 黑板必须在行为树运行之前初始化
    if (!SetupSoldierBlackboard(BehaviorTreeAsset))
    {
        UE_LOG(LogTemp, Error, TEXT("AI控制器 %s 初始化黑板失败"), *GetName());
        return false;
    }
    
    // 刷新黑板初始值
    RefreshBlackboardValues();
    
    // 启动行为树
    // 说明: 使用行为树组件运行行为树
    // 注意: StartTree 返回 void，通过检查行为树是否正在运行来判断成功
    BehaviorTreeComp->StartTree(*BehaviorTreeAsset);
    
    // 检查行为树是否成功启动
    bool bSuccess = BehaviorTreeComp->IsRunning();
    
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("AI控制器 %s 成功启动行为树 %s"), 
            *GetName(), *BehaviorTreeAsset->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AI控制器 %s 启动行为树 %s 失败"), 
            *GetName(), *BehaviorTreeAsset->GetName());
    }
    
    return bSuccess;
}

void AXBSoldierAIController::StopBehaviorTreeLogic()
{
    if (BehaviorTreeComp)
    {
        BehaviorTreeComp->StopTree(EBTStopMode::Safe);
        UE_LOG(LogTemp, Log, TEXT("AI控制器 %s 停止行为树"), *GetName());
    }
}

void AXBSoldierAIController::PauseBehaviorTree(bool bPause)
{
    if (BehaviorTreeComp)
    {
        if (bPause)
        {
            BehaviorTreeComp->PauseLogic(TEXT("Manual Pause"));
        }
        else
        {
            BehaviorTreeComp->ResumeLogic(TEXT("Manual Resume"));
        }
    }
}

bool AXBSoldierAIController::SetupSoldierBlackboard(UBehaviorTree* BT)
{
    if (!BT || !BT->BlackboardAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("行为树或黑板资产为空"));
        return false;
    }
    
    // 使用行为树的黑板资产初始化黑板组件
    // 说明: 黑板组件需要黑板资产来定义可用的键
    // 注意: UseBlackboard 第二个参数是原始指针引用
    UBlackboardComponent* BBCompRaw = BlackboardComp.Get();
    if (UseBlackboard(BT->BlackboardAsset, BBCompRaw))
    {
        // 如果 UseBlackboard 修改了指针，更新 TObjectPtr
        if (BBCompRaw != BlackboardComp.Get())
        {
            BlackboardComp = BBCompRaw;
        }
        return true;
    }
    
    UE_LOG(LogTemp, Error, TEXT("UseBlackboard 失败"));
    return false;
}

// ==================== 黑板值更新实现 ====================

void AXBSoldierAIController::SetTargetActor(AActor* Target)
{
    if (!BlackboardComp)
    {
        return;
    }
    
    BlackboardComp->SetValueAsObject(XBSoldierBBKeys::CurrentTarget, Target);
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, Target != nullptr);
    
    // 如果有目标，更新目标位置
    if (Target)
    {
        BlackboardComp->SetValueAsVector(XBSoldierBBKeys::TargetLocation, Target->GetActorLocation());
    }
}

void AXBSoldierAIController::SetLeader(AActor* Leader)
{
    if (!BlackboardComp)
    {
        return;
    }
    
    BlackboardComp->SetValueAsObject(XBSoldierBBKeys::Leader, Leader);
}

void AXBSoldierAIController::SetSoldierState(uint8 NewState)
{
    if (!BlackboardComp)
    {
        return;
    }
    
    BlackboardComp->SetValueAsEnum(XBSoldierBBKeys::SoldierState, NewState);
    
    // 更新战斗状态
    // 说明: 根据状态枚举值判断是否在战斗中
    bool bInCombat = (NewState == static_cast<uint8>(EXBSoldierState::Combat));
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::IsInCombat, bInCombat);
}

void AXBSoldierAIController::SetFormationPosition(const FVector& Position)
{
    if (!BlackboardComp)
    {
        return;
    }
    
    BlackboardComp->SetValueAsVector(XBSoldierBBKeys::FormationPosition, Position);
}

void AXBSoldierAIController::SetAttackRange(float Range)
{
    if (!BlackboardComp)
    {
        return;
    }
    
    BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::AttackRange, Range);
}

void AXBSoldierAIController::UpdateCombatState(bool bInCombat)
{
    if (!BlackboardComp)
    {
        return;
    }
    
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::IsInCombat, bInCombat);
}

void AXBSoldierAIController::RefreshBlackboardValues()
{
    if (!BlackboardComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("RefreshBlackboardValues: BlackboardComp 为空"));
        return;
    }
    
    if (!CachedSoldier.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("RefreshBlackboardValues: CachedSoldier 无效"));
        return;
    }
    
    AXBSoldierActor* Soldier = CachedSoldier.Get();
    if (!Soldier)
    {
        return;
    }
    
    // 🔧 修复 - 安全检查: 确保Actor完全初始化
    if (!IsValid(Soldier) || Soldier->IsPendingKillPending())
    {
        UE_LOG(LogTemp, Warning, TEXT("RefreshBlackboardValues: Soldier 正在销毁中"));
        return;
    }
    
    // 设置自身引用
    // 说明: 方便行为树节点访问士兵Actor
    BlackboardComp->SetValueAsObject(XBSoldierBBKeys::Self, Soldier);
    
    // 设置将领 - 安全检查
    AActor* Leader = Soldier->GetFollowTarget();
    SetLeader(Leader);
    
    // 设置状态
    SetSoldierState(static_cast<uint8>(Soldier->GetSoldierState()));
    
    // 设置编队槽位
    int32 SlotIndex = Soldier->GetFormationSlotIndex();
    BlackboardComp->SetValueAsInt(XBSoldierBBKeys::FormationSlot, SlotIndex);
    
    // 🔧 修复 - 设置编队位置 - 增强安全检查
    // 说明: 在初始化早期可能组件未就绪，使用当前位置作为回退
    FVector FormationPos = Soldier->GetActorLocation();
    if (Leader && IsValid(Leader) && SlotIndex != INDEX_NONE)
    {
        // 只有有跟随目标且有有效槽位时才计算编队位置
        FVector SafeFormationPos = Soldier->GetFormationWorldPositionSafe();
        if (!SafeFormationPos.IsZero())
        {
            FormationPos = SafeFormationPos;
        }
    }
    SetFormationPosition(FormationPos);
    
    // 设置攻击范围
    // 说明: 从士兵配置中获取攻击范围
    float AttackRange = Soldier->GetSoldierConfig().AttackRange;
    SetAttackRange(AttackRange);
    
    // 设置检测范围
    // 说明: 检测范围用于寻敌
    BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DetectionRange, 800.0f);
    
    // 检查是否可以攻击
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::CanAttack, Soldier->CanAttack());
    
    // 🔧 修复 - 检查是否在编队位置 - 增强安全检查
    bool bAtFormation = true;
    if (Leader && IsValid(Leader) && SlotIndex != INDEX_NONE)
    {
        bAtFormation = Soldier->IsAtFormationPositionSafe();
    }
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::IsAtFormation, bAtFormation);
}

void AXBSoldierAIController::UpdateDistanceValues()
{
    if (!BlackboardComp)
    {
        return;
    }
    
    if (!CachedSoldier.IsValid())
    {
        return;
    }
    
    AXBSoldierActor* Soldier = CachedSoldier.Get();
    if (!Soldier || !IsValid(Soldier))
    {
        return;
    }
    
    // 🔧 修复 - 安全检查: 确保Actor完全初始化
    if (Soldier->IsPendingKillPending())
    {
        return;
    }
    
    FVector SoldierLocation = Soldier->GetActorLocation();
    
    // 更新到目标的距离
    // 说明: 用于行为树中的距离判断
    UObject* TargetObj = BlackboardComp->GetValueAsObject(XBSoldierBBKeys::CurrentTarget);
    if (AActor* Target = Cast<AActor>(TargetObj))
    {
        if (IsValid(Target) && !Target->IsPendingKillPending())
        {
            float DistToTarget = FVector::Dist(SoldierLocation, Target->GetActorLocation());
            BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToTarget, DistToTarget);
        }
        else
        {
            BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToTarget, MAX_FLT);
            // 目标无效，清空目标引用
            BlackboardComp->SetValueAsObject(XBSoldierBBKeys::CurrentTarget, nullptr);
            BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
        }
    }
    else
    {
        BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToTarget, MAX_FLT);
    }
    
    // 更新到将领的距离
    // 说明: 用于判断是否应该脱离战斗返回
    UObject* LeaderObj = BlackboardComp->GetValueAsObject(XBSoldierBBKeys::Leader);
    if (AActor* Leader = Cast<AActor>(LeaderObj))
    {
        if (IsValid(Leader) && !Leader->IsPendingKillPending())
        {
            float DistToLeader = FVector::Dist(SoldierLocation, Leader->GetActorLocation());
            BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToLeader, DistToLeader);
            
            // 更新是否应该撤退
            // 说明: 超过1000距离时应该返回将领身边
            BlackboardComp->SetValueAsBool(XBSoldierBBKeys::ShouldRetreat, DistToLeader > 1000.0f);
        }
    }
    
    // 🔧 修复 - 更新编队位置相关 - 增强安全检查
    // 说明: 检查是否到达编队位置
    bool bAtFormation = true;
    AActor* FollowTarget = Soldier->GetFollowTarget();
    int32 SlotIndex = Soldier->GetFormationSlotIndex();
    if (FollowTarget && IsValid(FollowTarget) && SlotIndex != INDEX_NONE)
    {
        bAtFormation = Soldier->IsAtFormationPositionSafe();
    }
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::IsAtFormation, bAtFormation);
}

// ==================== 访问器实现 ====================

AXBSoldierActor* AXBSoldierAIController::GetSoldierActor() const
{
    return CachedSoldier.Get();
}
