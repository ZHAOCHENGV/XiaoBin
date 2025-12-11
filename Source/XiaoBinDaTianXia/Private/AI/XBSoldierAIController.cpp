/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/XBSoldierAIController.cpp

/**
 * @file XBSoldierAIController.cpp
 * @brief 士兵AI控制器实现
 * 
 * @note 🔧 修改记录:
 *       1. 修复 OnPossess 中访问未初始化组件导致的崩溃
 *       2. 将所有行为树初始化延迟到 OnPossess 完成后
 *       3. 添加安全的黑板更新方法
 */

#include "AI/XBSoldierAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Character/XBCharacterBase.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"

AXBSoldierAIController::AXBSoldierAIController()
{
    // 创建行为树组件
    // 说明: 行为树组件用于运行和管理行为树逻辑
    BehaviorTreeComp = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
    
    // 创建黑板组件
    // 说明: 黑板组件存储行为树所需的共享数据
    BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
    
    // 🔧 修改 - 禁用初始Tick，等待初始化完成后再启用
    // 说明: 避免在组件未就绪时执行Tick逻辑
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
}

void AXBSoldierAIController::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Log, TEXT("士兵AI控制器 %s BeginPlay"), *GetName());
}

/**
 * @brief AI控制器接管Pawn时的回调
 * @param InPawn 被接管的Pawn
 * @note 🔧 修改 - 只做最基本操作，所有初始化延迟到下一帧
 *       避免在组件未完全初始化时触发移动系统
 */
void AXBSoldierAIController::OnPossess(APawn* InPawn)
{
    // 空指针检查
    if (!InPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("AI控制器 %s OnPossess: InPawn 为空"), *GetName());
        return;
    }
    
    // 🔧 修改 - 先缓存士兵引用，再调用父类
    // 说明: 在调用 Super::OnPossess 之前缓存，确保后续能访问
    AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(InPawn);
    if (Soldier)
    {
        CachedSoldier = Soldier;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AI控制器 %s: 被控制的Pawn不是士兵类型: %s"), 
            *GetName(), *InPawn->GetName());
    }
    
    // 调用父类 OnPossess
    // 注意: 父类实现会设置 Pawn 引用等基本操作
    Super::OnPossess(InPawn);
    
    // 🔧 修改 - 将所有行为树初始化延迟到下一帧
    // 说明: 确保 Possess 完全完成、物理世界同步后再进行初始化
    if (Soldier)
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AXBSoldierAIController::DelayedOnPossess);
        
        UE_LOG(LogTemp, Log, TEXT("AI控制器 %s: Possess 成功，已安排延迟初始化"), *GetName());
    }
}

/**
 * @brief 延迟的 OnPossess 初始化
 * @note ✨ 新增 - 在 Possess 完成后的下一帧执行
 *       此时所有组件应该已完全初始化
 */
void AXBSoldierAIController::DelayedOnPossess()
{
    // 安全检查: 控制器是否有效
    if (!IsValid(this))
    {
        UE_LOG(LogTemp, Warning, TEXT("DelayedOnPossess: 控制器已无效"));
        return;
    }
    
    // 安全检查: 是否有Pawn
    APawn* MyPawn = GetPawn();
    if (!MyPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("DelayedOnPossess: Pawn 为空"));
        return;
    }
    
    // 安全检查: 缓存的士兵是否有效
    if (!CachedSoldier.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("DelayedOnPossess: 缓存的士兵引用无效"));
        return;
    }
    
    AXBSoldierCharacter* Soldier = CachedSoldier.Get();
    if (!Soldier || !IsValid(Soldier))
    {
        UE_LOG(LogTemp, Warning, TEXT("DelayedOnPossess: 士兵Actor无效"));
        return;
    }
    
    // 安全检查: 士兵是否正在销毁
    if (Soldier->IsPendingKillPending())
    {
        UE_LOG(LogTemp, Warning, TEXT("DelayedOnPossess: 士兵正在销毁中"));
        return;
    }
    
    // 获取要使用的行为树
    // 说明: 优先使用士兵配置的行为树，否则使用控制器默认行为树
    UBehaviorTree* BTToUse = nullptr;
    
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
        if (StartBehaviorTree(BTToUse))
        {
            // 标记初始化完成
            bIsInitialized = true;
            
            // 启用Tick
            SetActorTickEnabled(true);
            
            UE_LOG(LogTemp, Log, TEXT("AI控制器 %s: 行为树启动成功，Tick已启用"), *GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("AI控制器 %s: 行为树启动失败"), *GetName());
        }
    }
    else
    {
        // 没有行为树，也启用Tick用于状态更新
        bIsInitialized = true;
        SetActorTickEnabled(true);
        
        UE_LOG(LogTemp, Log, TEXT("士兵 %s 没有配置行为树，使用简单状态机"), *Soldier->GetName());
    }
}

void AXBSoldierAIController::OnUnPossess()
{
    // 清除所有定时器
    // 说明: 避免在 UnPossess 后定时器回调访问无效数据
    GetWorldTimerManager().ClearAllTimersForObject(this);
    
    // 停止行为树
    StopBehaviorTreeLogic();
    
    // 禁用Tick
    SetActorTickEnabled(false);
    
    // 重置状态
    bIsInitialized = false;
    CachedSoldier.Reset();
    
    Super::OnUnPossess();
    
    UE_LOG(LogTemp, Log, TEXT("AI控制器 %s: UnPossess 完成"), *GetName());
}

void AXBSoldierAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 安全检查: 是否已初始化
    if (!bIsInitialized)
    {
        return;
    }
    
    // 安全检查: 士兵是否有效
    if (!CachedSoldier.IsValid())
    {
        return;
    }
    
    // 定期更新黑板值
    // 说明: 避免每帧更新，使用间隔更新提高性能
    BlackboardUpdateTimer += DeltaTime;
    if (BlackboardUpdateTimer >= BlackboardUpdateInterval)
    {
        BlackboardUpdateTimer = 0.0f;
        UpdateDistanceValuesSafe();
    }
}

// ==================== 行为树控制实现 ====================

/**
 * @brief 启动行为树
 * @param BehaviorTreeAsset 行为树资产
 * @return 是否成功启动
 */
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
    
    // 安全地刷新黑板初始值
    // 说明: 使用安全版本，不触发移动组件
    RefreshBlackboardValuesSafe();
    
    // 启动行为树
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
    if (BehaviorTreeComp && BehaviorTreeComp->IsRunning())
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

/**
 * @brief 初始化士兵黑板
 * @param BT 行为树资产
 * @return 是否成功初始化
 */
bool AXBSoldierAIController::SetupSoldierBlackboard(UBehaviorTree* BT)
{
    if (!BT || !BT->BlackboardAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("行为树或黑板资产为空"));
        return false;
    }
    
    // 使用行为树的黑板资产初始化黑板组件
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
    if (Target && IsValid(Target))
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

/**
 * @brief 安全地刷新黑板值
 * @note 不访问任何可能触发移动组件的函数
 *       只使用简单的 Get 方法获取数据
 */
void AXBSoldierAIController::RefreshBlackboardValuesSafe()
{
    if (!BlackboardComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("RefreshBlackboardValuesSafe: BlackboardComp 为空"));
        return;
    }
    
    if (!CachedSoldier.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("RefreshBlackboardValuesSafe: CachedSoldier 无效"));
        return;
    }
    
    AXBSoldierCharacter* Soldier = CachedSoldier.Get();
    if (!Soldier || !IsValid(Soldier))
    {
        return;
    }
    
    // 安全检查: 士兵是否正在销毁
    if (Soldier->IsPendingKillPending())
    {
        return;
    }
    
    // 设置自身引用
    BlackboardComp->SetValueAsObject(XBSoldierBBKeys::Self, Soldier);
    
    // 设置将领（只获取引用，不计算位置）
    AActor* Leader = Soldier->GetFollowTarget();
    SetLeader(Leader);
    
    // 设置状态
    SetSoldierState(static_cast<uint8>(Soldier->GetSoldierState()));
    
    // 设置编队槽位
    int32 SlotIndex = Soldier->GetFormationSlotIndex();
    BlackboardComp->SetValueAsInt(XBSoldierBBKeys::FormationSlot, SlotIndex);
    
    // 🔧 修改 - 使用士兵当前位置作为初始编队位置
    // 说明: 避免调用可能触发移动组件的 GetFormationWorldPosition
    FVector CurrentPosition = Soldier->GetActorLocation();
    SetFormationPosition(CurrentPosition);
    
    // 设置攻击范围
    float AttackRange = Soldier->GetSoldierConfig().AttackRange;
    SetAttackRange(AttackRange);
    
    // 设置检测范围
    BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DetectionRange, 800.0f);
    
    // 设置默认值
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::CanAttack, true);
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::IsAtFormation, true);
    BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToTarget, MAX_FLT);
    BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToLeader, 0.0f);
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::ShouldRetreat, false);
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
    
    UE_LOG(LogTemp, Verbose, TEXT("AI控制器 %s: 黑板值安全刷新完成"), *GetName());
}

void AXBSoldierAIController::RefreshBlackboardValues()
{
    // 直接调用安全版本
    RefreshBlackboardValuesSafe();
}

/**
 * @brief 安全地更新距离值
 * @note 只使用简单的位置计算，不触发移动组件
 */
void AXBSoldierAIController::UpdateDistanceValuesSafe()
{
    if (!BlackboardComp)
    {
        return;
    }
    
    if (!CachedSoldier.IsValid())
    {
        return;
    }
    
    AXBSoldierCharacter* Soldier = CachedSoldier.Get();
    if (!Soldier || !IsValid(Soldier) || Soldier->IsPendingKillPending())
    {
        return;
    }
    
    // 获取士兵当前位置（简单操作，不触发移动组件）
    FVector SoldierLocation = Soldier->GetActorLocation();
    
    // ========== 更新到目标的距离 ==========
    UObject* TargetObj = BlackboardComp->GetValueAsObject(XBSoldierBBKeys::CurrentTarget);
    if (AActor* Target = Cast<AActor>(TargetObj))
    {
        if (IsValid(Target) && !Target->IsPendingKillPending())
        {
            float DistToTarget = FVector::Dist(SoldierLocation, Target->GetActorLocation());
            BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToTarget, DistToTarget);
            
            // 更新目标位置
            BlackboardComp->SetValueAsVector(XBSoldierBBKeys::TargetLocation, Target->GetActorLocation());
        }
        else
        {
            // 目标无效，清除
            BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToTarget, MAX_FLT);
            BlackboardComp->SetValueAsObject(XBSoldierBBKeys::CurrentTarget, nullptr);
            BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
        }
    }
    else
    {
        BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToTarget, MAX_FLT);
    }
    
    // ========== 更新到将领的距离 ==========
    UObject* LeaderObj = BlackboardComp->GetValueAsObject(XBSoldierBBKeys::Leader);
    if (AActor* Leader = Cast<AActor>(LeaderObj))
    {
        if (IsValid(Leader) && !Leader->IsPendingKillPending())
        {
            float DistToLeader = FVector::Dist(SoldierLocation, Leader->GetActorLocation());
            BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToLeader, DistToLeader);
            
            // 更新是否应该撤退（超过1000距离）
            BlackboardComp->SetValueAsBool(XBSoldierBBKeys::ShouldRetreat, DistToLeader > 1000.0f);
        }
    }
    
    // ========== 更新是否可以攻击 ==========
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::CanAttack, Soldier->CanAttack());
    
    // ========== 更新编队位置（使用安全方法）==========
    FVector FormationPos = Soldier->GetFormationWorldPositionSafe();
    if (!FormationPos.IsZero() && !FormationPos.ContainsNaN())
    {
        SetFormationPosition(FormationPos);
        
        // 计算到编队位置的距离
        float DistToFormation = FVector::Dist2D(SoldierLocation, FormationPos);
        BlackboardComp->SetValueAsBool(XBSoldierBBKeys::IsAtFormation, DistToFormation <= 50.0f);
    }
    
    // ========== 更新状态 ==========
    SetSoldierState(static_cast<uint8>(Soldier->GetSoldierState()));
}

// ==================== 访问器实现 ====================

AXBSoldierCharacter* AXBSoldierAIController::GetSoldierActor() const
{
    return CachedSoldier.Get();
}
