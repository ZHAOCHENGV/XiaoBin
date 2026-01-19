/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/XBSoldierAIController.cpp

/**
 * @file XBSoldierAIController.cpp
 * @brief 士兵AI控制器实现
 * 
 * @note 🔧 修改记录:
 *       1. SoldierState 使用 Int 类型替代 Enum
 *       2. 添加黑板键类型校验
 *       3. 使用项目专用日志类别
 *       4. 增强空指针检查
 */

#include "AI/XBSoldierAIController.h"
#include "Utils/XBLogCategories.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "Soldier/XBSoldierCharacter.h"
#include "TimerManager.h"


AXBSoldierAIController::AXBSoldierAIController()
{
    BehaviorTreeComp = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
    BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
    
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
}

void AXBSoldierAIController::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogXBAI, Log, TEXT("士兵AI控制器 %s BeginPlay"), *GetName());
}

void AXBSoldierAIController::OnPossess(APawn* InPawn)
{
    if (!InPawn)
    {
        UE_LOG(LogXBAI, Warning, TEXT("AI控制器 %s OnPossess: InPawn 为空"), *GetName());
        return;
    }
    
    AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(InPawn);
    if (Soldier)
    {
        CachedSoldier = Soldier;
    }
    else
    {
        UE_LOG(LogXBAI, Warning, TEXT("AI控制器 %s: 被控制的Pawn不是士兵类型: %s"), 
            *GetName(), *InPawn->GetName());
    }
    
    Super::OnPossess(InPawn);
    
    if (Soldier)
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AXBSoldierAIController::DelayedOnPossess);
        UE_LOG(LogXBAI, Log, TEXT("AI控制器 %s: Possess 成功，已安排延迟初始化"), *GetName());
    }
}

void AXBSoldierAIController::DelayedOnPossess()
{
    if (!IsValid(this))
    {
        UE_LOG(LogXBAI, Warning, TEXT("DelayedOnPossess: 控制器已无效"));
        return;
    }
    
    APawn* MyPawn = GetPawn();
    if (!MyPawn)
    {
        UE_LOG(LogXBAI, Warning, TEXT("DelayedOnPossess: Pawn 为空"));
        return;
    }
    
    if (!CachedSoldier.IsValid())
    {
        UE_LOG(LogXBAI, Warning, TEXT("DelayedOnPossess: 缓存的士兵引用无效"));
        return;
    }
    
    AXBSoldierCharacter* Soldier = CachedSoldier.Get();
    if (!Soldier || !IsValid(Soldier))
    {
        UE_LOG(LogXBAI, Warning, TEXT("DelayedOnPossess: 士兵Actor无效"));
        return;
    }
    
    if (Soldier->IsPendingKillPending())
    {
        UE_LOG(LogXBAI, Warning, TEXT("DelayedOnPossess: 士兵正在销毁中"));
        return;
    }
    
    UBehaviorTree* BTToUse = nullptr;
    
    if (Soldier->BehaviorTreeAsset != nullptr)
    {
        BTToUse = Soldier->BehaviorTreeAsset;
        UE_LOG(LogXBAI, Log, TEXT("使用士兵 %s 配置的行为树: %s"), 
            *Soldier->GetName(), *BTToUse->GetName());
    }
    else if (DefaultBehaviorTree != nullptr)
    {
        BTToUse = DefaultBehaviorTree;
        UE_LOG(LogXBAI, Log, TEXT("使用默认行为树控制士兵 %s"), *Soldier->GetName());
    }
    
    if (BTToUse)
    {
        if (StartBehaviorTree(BTToUse))
        {
            bIsInitialized = true;
            SetActorTickEnabled(true);
            UE_LOG(LogXBAI, Log, TEXT("AI控制器 %s: 行为树启动成功，Tick已启用"), *GetName());
        }
        else
        {
            UE_LOG(LogXBAI, Error, TEXT("AI控制器 %s: 行为树启动失败"), *GetName());
        }
    }
    else
    {
        bIsInitialized = true;
        SetActorTickEnabled(true);
        UE_LOG(LogXBAI, Log, TEXT("士兵 %s 没有配置行为树，使用简单状态机"), *Soldier->GetName());
    }
}

void AXBSoldierAIController::OnUnPossess()
{
    GetWorldTimerManager().ClearAllTimersForObject(this);
    StopBehaviorTreeLogic();
    SetActorTickEnabled(false);
    bIsInitialized = false;
    CachedSoldier.Reset();
    
    Super::OnUnPossess();
    
    UE_LOG(LogXBAI, Log, TEXT("AI控制器 %s: UnPossess 完成"), *GetName());
}

void AXBSoldierAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (!bIsInitialized)
    {
        return;
    }
    
    if (!CachedSoldier.IsValid())
    {
        return;
    }
    
    BlackboardUpdateTimer += DeltaTime;
    if (BlackboardUpdateTimer >= BlackboardUpdateInterval)
    {
        BlackboardUpdateTimer = 0.0f;
        UpdateDistanceValuesSafe();
    }
}

// ==================== 行为树控制实现 ====================

bool AXBSoldierAIController::StartBehaviorTree(UBehaviorTree* BehaviorTreeAsset)
{
    if (!BehaviorTreeAsset)
    {
        UE_LOG(LogXBAI, Error, TEXT("AI控制器 %s 启动行为树失败: 行为树资产为空"), *GetName());
        return false;
    }
    
    if (!SetupSoldierBlackboard(BehaviorTreeAsset))
    {
        UE_LOG(LogXBAI, Error, TEXT("AI控制器 %s 初始化黑板失败"), *GetName());
        return false;
    }
    
    // ✨ 新增 - 校验黑板键类型
    if (bValidateBlackboardKeys)
    {
        if (!ValidateAllBlackboardKeys())
        {
            UE_LOG(LogXBAI, Warning, TEXT("AI控制器 %s: 黑板键校验失败，部分功能可能异常"), *GetName());
            // 不阻止启动，只是警告
        }
    }
    
    RefreshBlackboardValuesSafe();

    // 🔧 修改 - 使用 RunBehaviorTree 统一启动逻辑，确保 BrainComponent 正确初始化
    bool bSuccess = RunBehaviorTree(BehaviorTreeAsset);

    if (bSuccess && !BehaviorTreeComp)
    {
        BehaviorTreeComp = Cast<UBehaviorTreeComponent>(GetBrainComponent());
    }
    
    if (bSuccess)
    {
        UE_LOG(LogXBAI, Log, TEXT("AI控制器 %s 成功启动行为树 %s"), 
            *GetName(), *BehaviorTreeAsset->GetName());
    }
    else
    {
        UE_LOG(LogXBAI, Error, TEXT("AI控制器 %s 启动行为树 %s 失败"), 
            *GetName(), *BehaviorTreeAsset->GetName());
    }
    
    return bSuccess;
}

void AXBSoldierAIController::StopBehaviorTreeLogic()
{
    if (BehaviorTreeComp && BehaviorTreeComp->IsRunning())
    {
        BehaviorTreeComp->StopTree(EBTStopMode::Safe);
        UE_LOG(LogXBAI, Log, TEXT("AI控制器 %s 停止行为树"), *GetName());
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
        UE_LOG(LogXBAI, Warning, TEXT("行为树或黑板资产为空"));
        return false;
    }
    
    UBlackboardComponent* BBCompRaw = BlackboardComp.Get();
    if (UseBlackboard(BT->BlackboardAsset, BBCompRaw))
    {
        if (BBCompRaw != BlackboardComp.Get())
        {
            BlackboardComp = BBCompRaw;
        }
        return true;
    }
    
    UE_LOG(LogXBAI, Error, TEXT("UseBlackboard 失败"));
    return false;
}

// ==================== 黑板键校验实现 ====================

/**
 * @brief 获取黑板键的类型
 * @param KeyName 键名
 * @return 键类型
 * @note ✨ 新增
 */
EXBBlackboardKeyType AXBSoldierAIController::GetBlackboardKeyType(FName KeyName) const
{
    if (!BlackboardComp)
    {
        return EXBBlackboardKeyType::Unknown;
    }
    
    const UBlackboardData* BBAsset = BlackboardComp->GetBlackboardAsset();
    if (!BBAsset)
    {
        return EXBBlackboardKeyType::Unknown;
    }
    
    FBlackboard::FKey KeyID = BlackboardComp->GetKeyID(KeyName);
    if (KeyID == FBlackboard::InvalidKey)
    {
        return EXBBlackboardKeyType::Unknown;
    }
    
    TSubclassOf<UBlackboardKeyType> KeyType = BlackboardComp->GetKeyType(KeyID);
    if (!KeyType)
    {
        return EXBBlackboardKeyType::Unknown;
    }
    
    if (KeyType->IsChildOf(UBlackboardKeyType_Object::StaticClass()))
    {
        return EXBBlackboardKeyType::Object;
    }
    if (KeyType->IsChildOf(UBlackboardKeyType_Vector::StaticClass()))
    {
        return EXBBlackboardKeyType::Vector;
    }
    if (KeyType->IsChildOf(UBlackboardKeyType_Int::StaticClass()))
    {
        return EXBBlackboardKeyType::Int;
    }
    if (KeyType->IsChildOf(UBlackboardKeyType_Float::StaticClass()))
    {
        return EXBBlackboardKeyType::Float;
    }
    if (KeyType->IsChildOf(UBlackboardKeyType_Bool::StaticClass()))
    {
        return EXBBlackboardKeyType::Bool;
    }
    // 🔧 修改 - Enum 类型也接受（向后兼容）
    if (KeyType->IsChildOf(UBlackboardKeyType_Enum::StaticClass()))
    {
        return EXBBlackboardKeyType::Int; // Enum 可以用 Int 方式设置
    }
    
    return EXBBlackboardKeyType::Unknown;
}

bool AXBSoldierAIController::ValidateBlackboardKey(FName KeyName, EXBBlackboardKeyType ExpectedType) const
{
    if (!BlackboardComp)
    {
        UE_LOG(LogXBAI, Warning, TEXT("校验黑板键失败: 黑板组件为空"));
        return false;
    }
    
    EXBBlackboardKeyType ActualType = GetBlackboardKeyType(KeyName);
    
    if (ActualType == EXBBlackboardKeyType::Unknown)
    {
        UE_LOG(LogXBAI, Warning, TEXT("黑板键 '%s' 不存在或类型未知"), *KeyName.ToString());
        return false;
    }
    
    if (ActualType != ExpectedType)
    {
        UE_LOG(LogXBAI, Warning, TEXT("黑板键 '%s' 类型不匹配: 期望 %d, 实际 %d"), 
            *KeyName.ToString(), static_cast<int32>(ExpectedType), static_cast<int32>(ActualType));
        return false;
    }
    
    return true;
}

/**
 * @brief 校验所有必需的黑板键
 * @return 是否所有键都校验通过
 * @note ✨ 新增 - 在初始化时调用，输出所有不匹配的键
 */
bool AXBSoldierAIController::ValidateAllBlackboardKeys() const
{
    if (!BlackboardComp)
    {
        UE_LOG(LogXBAI, Error, TEXT("校验黑板键失败: 黑板组件为空"));
        return false;
    }
    
    bool bAllValid = true;
    
    // 定义需要校验的键及其期望类型
    struct FKeyValidation
    {
        FName KeyName;
        EXBBlackboardKeyType ExpectedType;
        bool bRequired; // 是否必需
    };
    
    TArray<FKeyValidation> KeysToValidate = {
        // 对象类型
        { XBSoldierBBKeys::Leader,          EXBBlackboardKeyType::Object, true },
        { XBSoldierBBKeys::CurrentTarget,   EXBBlackboardKeyType::Object, true },
        { XBSoldierBBKeys::Self,            EXBBlackboardKeyType::Object, true },
        
        // 位置类型
        { XBSoldierBBKeys::TargetLocation,      EXBBlackboardKeyType::Vector, true },
        
        // 🔧 修改 - 整数类型（SoldierState 使用 Int）
        { XBSoldierBBKeys::SoldierState,    EXBBlackboardKeyType::Int, true },
        { XBSoldierBBKeys::FormationSlot,   EXBBlackboardKeyType::Int, true },
        
        // 浮点类型
        { XBSoldierBBKeys::AttackRange,         EXBBlackboardKeyType::Float, true },
        { XBSoldierBBKeys::DetectionRange,      EXBBlackboardKeyType::Float, false },
        { XBSoldierBBKeys::VisionRange,         EXBBlackboardKeyType::Float, false },
        { XBSoldierBBKeys::DistanceToTarget,    EXBBlackboardKeyType::Float, true },
        { XBSoldierBBKeys::DistanceToLeader,    EXBBlackboardKeyType::Float, true },
        
        // 布尔类型
        { XBSoldierBBKeys::HasTarget,       EXBBlackboardKeyType::Bool, true },
        { XBSoldierBBKeys::IsInCombat,      EXBBlackboardKeyType::Bool, true },
        { XBSoldierBBKeys::ShouldRetreat,   EXBBlackboardKeyType::Bool, false },
        { XBSoldierBBKeys::CanAttack,       EXBBlackboardKeyType::Bool, true },
    };
    
    int32 FailCount = 0;
    
    for (const FKeyValidation& Validation : KeysToValidate)
    {
        if (!ValidateBlackboardKey(Validation.KeyName, Validation.ExpectedType))
        {
            if (Validation.bRequired)
            {
                FailCount++;
                bAllValid = false;
            }
        }
    }
    
    if (FailCount > 0)
    {
        UE_LOG(LogXBAI, Warning, TEXT("黑板键校验完成: %d 个必需键校验失败"), FailCount);
    }
    else
    {
        UE_LOG(LogXBAI, Log, TEXT("黑板键校验完成: 所有必需键校验通过"));
    }
    
    return bAllValid;
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

/**
 * @brief 设置士兵状态
 * @param NewState 新状态
 * @note 🔧 修改 - 使用 SetValueAsInt 替代 SetValueAsEnum
 *       蓝图中将黑板键配置为 Int 类型
 */
void AXBSoldierAIController::SetSoldierState(uint8 NewState)
{
    if (!BlackboardComp)
    {
        return;
    }
    
    // 🔧 修改 - 使用 Int 类型
    BlackboardComp->SetValueAsInt(XBSoldierBBKeys::SoldierState, static_cast<int32>(NewState));
    
    bool bInCombat = (NewState == static_cast<uint8>(EXBSoldierState::Combat));
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::IsInCombat, bInCombat);
}

void AXBSoldierAIController::SetAttackRange(float Range)
{
    if (!BlackboardComp)
    {
        return;
    }
    
    BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::AttackRange, Range);
}

void AXBSoldierAIController::SetVisionRange(float Range)
{
    if (!BlackboardComp)
    {
        return;
    }
    
    BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::VisionRange, Range);
    // 同时设置 DetectionRange 保持向后兼容
    BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DetectionRange, Range);
}

void AXBSoldierAIController::UpdateCombatState(bool bInCombat)
{
    if (!BlackboardComp)
    {
        return;
    }
    
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::IsInCombat, bInCombat);
}

void AXBSoldierAIController::RefreshBlackboardValuesSafe()
{
    if (!BlackboardComp)
    {
        UE_LOG(LogXBAI, Warning, TEXT("RefreshBlackboardValuesSafe: BlackboardComp 为空"));
        return;
    }
    
    if (!CachedSoldier.IsValid())
    {
        UE_LOG(LogXBAI, Warning, TEXT("RefreshBlackboardValuesSafe: CachedSoldier 无效"));
        return;
    }
    
    AXBSoldierCharacter* Soldier = CachedSoldier.Get();
    if (!Soldier || !IsValid(Soldier))
    {
        return;
    }
    
    if (Soldier->IsPendingKillPending())
    {
        return;
    }
    
    BlackboardComp->SetValueAsObject(XBSoldierBBKeys::Self, Soldier);
    
    AActor* Leader = Soldier->GetFollowTarget();
    SetLeader(Leader);
    
    SetSoldierState(static_cast<uint8>(Soldier->GetSoldierState()));
    
    int32 SlotIndex = Soldier->GetFormationSlotIndex();
    BlackboardComp->SetValueAsInt(XBSoldierBBKeys::FormationSlot, SlotIndex);
    
    // 🔧 修复 - 直接调用 Getter 方法
    SetAttackRange(Soldier->GetAttackRange());
    SetVisionRange(Soldier->GetVisionRange());
    
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::CanAttack, true);
    BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToTarget, MAX_FLT);
    BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToLeader, 0.0f);
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::ShouldRetreat, false);
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
    
    UE_LOG(LogXBAI, Verbose, TEXT("AI控制器 %s: 黑板值安全刷新完成"), *GetName());
}

void AXBSoldierAIController::RefreshBlackboardValues()
{
    RefreshBlackboardValuesSafe();
}

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
    
    FVector SoldierLocation = Soldier->GetActorLocation();
    
    // 更新到目标的距离
    UObject* TargetObj = BlackboardComp->GetValueAsObject(XBSoldierBBKeys::CurrentTarget);
    if (AActor* Target = Cast<AActor>(TargetObj))
    {
        if (IsValid(Target) && !Target->IsPendingKillPending())
        {
            const float SelfRadius = Soldier->GetSimpleCollisionRadius();
            const float TargetRadius = Target->GetSimpleCollisionRadius();
            float DistToTarget = FVector::Dist2D(SoldierLocation, Target->GetActorLocation());
            DistToTarget = FMath::Max(0.0f, DistToTarget - (SelfRadius + TargetRadius));
            BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToTarget, DistToTarget);
            BlackboardComp->SetValueAsVector(XBSoldierBBKeys::TargetLocation, Target->GetActorLocation());
        }
        else
        {
            BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToTarget, MAX_FLT);
            BlackboardComp->SetValueAsObject(XBSoldierBBKeys::CurrentTarget, nullptr);
            BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
        }
    }
    else
    {
        BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToTarget, MAX_FLT);
    }
    
    // 更新到将领的距离
    UObject* LeaderObj = BlackboardComp->GetValueAsObject(XBSoldierBBKeys::Leader);
    if (AActor* Leader = Cast<AActor>(LeaderObj))
    {
        if (IsValid(Leader) && !Leader->IsPendingKillPending())
        {
            float DistToLeader = FVector::Dist(SoldierLocation, Leader->GetActorLocation());
            BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToLeader, DistToLeader);
            
            // 🔧 修改 - 由行为树 Service 统一写入 ShouldRetreat，避免多系统冲突
        }
    }
    
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::CanAttack, Soldier->CanAttack());
    
    // 🔧 修改 - 使用 Int 类型
    SetSoldierState(static_cast<uint8>(Soldier->GetSoldierState()));
}

AXBSoldierCharacter* AXBSoldierAIController::GetSoldierActor() const
{
    return CachedSoldier.Get();
}
