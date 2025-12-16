/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Soldier/XBSoldierCharacter.cpp

/**
 * @file XBSoldierCharacter.cpp
 * @brief 士兵Actor实现 - 重构为数据驱动架构
 * 
 * @note 🔧 重构记录:
 *       1. 所有配置数据访问通过 DataAccessor
 *       2. 移除 FXBSoldierConfig 和 ToSoldierConfig()
 *       3. 简化初始化流程
 *       4. 保持运行时状态管理不变
 */

#include "Soldier/XBSoldierCharacter.h"
#include "Utils/XBLogCategories.h"
#include "Utils/XBBlueprintFunctionLibrary.h"
#include "Data/XBSoldierDataAccessor.h"              // ✨ 新增
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Soldier/Component/XBSoldierFollowComponent.h"
#include "Soldier/Component/XBSoldierDebugComponent.h"
#include "Character/XBCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "AI/XBSoldierAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/DataTable.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"
#include "XBCollisionChannels.h"
#include "Soldier/Component/XBSoldierBehaviorInterface.h"  // ✨ 新增

AXBSoldierCharacter::AXBSoldierCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    // ==================== 碰撞配置 ====================
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->InitCapsuleSize(34.0f, 88.0f);
        Capsule->SetCollisionObjectType(XBCollision::Soldier);
        Capsule->SetCollisionResponseToChannel(XBCollision::Leader, ECR_Overlap);
        Capsule->SetCollisionResponseToChannel(XBCollision::Soldier, ECR_Overlap);
        
        UE_LOG(LogXBSoldier, Warning, TEXT("士兵碰撞配置: ObjectType=%d, 对Leader(%d)响应=%d, 对Soldier(%d)响应=%d"),
            (int32)Capsule->GetCollisionObjectType(),
            (int32)XBCollision::Leader,
            (int32)Capsule->GetCollisionResponseToChannel(XBCollision::Leader),
            (int32)XBCollision::Soldier,
            (int32)Capsule->GetCollisionResponseToChannel(XBCollision::Soldier));
    }

    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
        MeshComp->SetCollisionResponseToChannel(XBCollision::Soldier, ECR_Ignore);
        MeshComp->SetCollisionResponseToChannel(XBCollision::Leader, ECR_Ignore);
    }

    // ==================== ✨ 新增：创建数据访问器组件 ====================
    DataAccessor = CreateDefaultSubobject<UXBSoldierDataAccessor>(TEXT("DataAccessor"));

    // ==================== 创建其他组件 ====================
    FollowComponent = CreateDefaultSubobject<UXBSoldierFollowComponent>(TEXT("FollowComponent"));
    DebugComponent = CreateDefaultSubobject<UXBSoldierDebugComponent>(TEXT("DebugComponent"));
    // ✨ 新增 - 创建行为接口组件
    BehaviorInterface = CreateDefaultSubobject<UXBSoldierBehaviorInterface>(TEXT("BehaviorInterface"));
    // ==================== 移动组件配置 ====================
    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->bOrientRotationToMovement = true;
        MovementComp->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
        MovementComp->MaxWalkSpeed = 400.0f;
        MovementComp->BrakingDecelerationWalking = 2000.0f;
        MovementComp->SetComponentTickEnabled(false);
    }

    AutoPossessAI = EAutoPossessAI::Disabled;
    AIControllerClass = nullptr;
}

void AXBSoldierCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    
    bComponentsInitialized = true;
    
    // 组件校验
    UCapsuleComponent* Capsule = GetCapsuleComponent();
    if (Capsule)
    {
        FTransform CapsuleTransform = Capsule->GetComponentTransform();
        FVector Scale = CapsuleTransform.GetScale3D();
        
        if (Scale.IsNearlyZero() || Scale.ContainsNaN())
        {
            UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: Capsule Scale 无效 (%s)，修正为 (1,1,1)"), 
                *GetName(), *Scale.ToString());
            Capsule->SetWorldScale3D(FVector::OneVector);
        }
    }
    
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    if (MoveComp)
    {
        if (!MoveComp->UpdatedComponent)
        {
            UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: MovementComponent 的 UpdatedComponent 为空"), *GetName());
            MoveComp->SetUpdatedComponent(Capsule);
        }
    }
    
    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: PostInitializeComponents 完成"), *GetName());
}

void AXBSoldierCharacter::BeginPlay()
{
    Super::BeginPlay();

    // ==================== 🔧 从 DataAccessor 初始化血量 ====================
    if (IsDataAccessorValid())
    {
        CurrentHealth = DataAccessor->GetMaxHealth();
    }
    else
    {
        CurrentHealth = 100.0f; // 默认值
    }
    
    // 延迟启用移动和Tick
    GetWorldTimerManager().SetTimerForNextTick([this]()
    {
        EnableMovementAndTick();
    });

    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s BeginPlay - 阵营: %d, 状态: %d"), 
        *GetName(), static_cast<int32>(Faction), static_cast<int32>(CurrentState));
}

void AXBSoldierCharacter::EnableMovementAndTick()
{
    if (!IsValid(this) || IsPendingKillPending())
    {
        return;
    }
    
    UCapsuleComponent* Capsule = GetCapsuleComponent();
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    
    if (!Capsule || !MoveComp)
    {
        UE_LOG(LogXBSoldier, Error, TEXT("士兵 %s: 组件无效，无法启用移动"), *GetName());
        return;
    }
    
    FTransform CapsuleTransform = Capsule->GetComponentTransform();
    if (!CapsuleTransform.IsValid())
    {
        UE_LOG(LogXBSoldier, Error, TEXT("士兵 %s: Capsule Transform 无效"), *GetName());
        return;
    }
    
    MoveComp->SetComponentTickEnabled(true);
    SetActorTickEnabled(true);
    
    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: 移动组件和Tick已启用"), *GetName());
}


/**
 * @brief Tick 函数
 * @note 🔧 核心重构 - 移除所有 AI 决策逻辑
 *       AI 逻辑现在由行为树和 BehaviorInterface 组件处理
 */
void AXBSoldierCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
}

// ==================== ✨ 新增：数据访问器有效性检查 ====================

/**
 * @brief 检查数据访问器是否有效
 * @return 是否有效且已初始化
 */
bool AXBSoldierCharacter::IsDataAccessorValid() const
{
    return DataAccessor && DataAccessor->IsInitialized();
}

// ==================== 🔧 重构：初始化方法 ====================

/**
 * @brief 从数据表初始化
 * @param DataTable 数据表
 * @param RowName 行名
 * @param InFaction 阵营
 * @note 🔧 核心重构 - 简化为直接初始化 DataAccessor
 */
void AXBSoldierCharacter::InitializeFromDataTable(UDataTable* DataTable, FName RowName, EXBFaction InFaction)
{
    if (!DataTable)
    {
        UE_LOG(LogXBSoldier, Error, TEXT("士兵初始化失败: 数据表为空"));
        return;
    }

    if (RowName.IsNone())
    {
        UE_LOG(LogXBSoldier, Error, TEXT("士兵初始化失败: 行名为空"));
        return;
    }

    if (!DataAccessor)
    {
        UE_LOG(LogXBSoldier, Error, TEXT("士兵初始化失败: DataAccessor 组件未创建"));
        return;
    }

    // ==================== ✨ 核心：使用 DataAccessor 初始化 ====================
    bool bInitSuccess = DataAccessor->Initialize(
        DataTable, 
        RowName, 
        EXBResourceLoadStrategy::Synchronous  // 同步加载模式
    );

    if (!bInitSuccess)
    {
        UE_LOG(LogXBSoldier, Error, TEXT("士兵初始化失败: DataAccessor 初始化失败"));
        return;
    }

    // ==================== 🔧 从 DataAccessor 读取并缓存基础属性 ====================
    SoldierType = DataAccessor->GetSoldierType();
    Faction = InFaction;
    CurrentHealth = DataAccessor->GetMaxHealth();

    // ==================== 🔧 应用移动组件配置 ====================
    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->MaxWalkSpeed = DataAccessor->GetMoveSpeed();
        MovementComp->RotationRate = FRotator(0.0f, DataAccessor->GetRotationSpeed(), 0.0f);
    }

    // ==================== 🔧 应用跟随组件配置 ====================
    if (FollowComponent)
    {
        FollowComponent->SetFollowSpeed(DataAccessor->GetMoveSpeed());
        FollowComponent->SetFollowInterpSpeed(DataAccessor->GetFollowInterpSpeed());
    }

    // ==================== 🔧 加载行为树 ====================
    BehaviorTreeAsset = DataAccessor->GetBehaviorTree();

    // ==================== 🔧 应用视觉配置 ====================
    ApplyVisualConfig();

    UE_LOG(LogXBSoldier, Log, TEXT("士兵初始化成功: %s (类型=%s, 血量=%.1f, 视野=%.0f, 攻击范围=%.0f)"), 
        *RowName.ToString(),
        *UEnum::GetValueAsString(SoldierType),
        CurrentHealth,
        DataAccessor->GetVisionRange(),
        DataAccessor->GetAttackRange());
}

/**
 * @brief 应用视觉配置
 * @note 🔧 从 DataAccessor 获取资源
 */
void AXBSoldierCharacter::ApplyVisualConfig()
{
    if (!IsDataAccessorValid())
    {
        return;
    }

    // 🔧 修复 - 重命名变量避免与 ACharacter::Mesh 冲突
    USkeletalMesh* SoldierMesh = DataAccessor->GetSkeletalMesh();
    if (SoldierMesh)
    {
        GetMesh()->SetSkeletalMesh(SoldierMesh);
    }

    TSubclassOf<UAnimInstance> AnimClass = DataAccessor->GetAnimClass();
    if (AnimClass)
    {
        GetMesh()->SetAnimInstanceClass(AnimClass);
    }

    float MeshScale = DataAccessor->GetRawData().VisualConfig.MeshScale;
    if (!FMath::IsNearlyEqual(MeshScale, 1.0f))
    {
        SetActorScale3D(FVector(MeshScale));
    }
}

// ==================== 🔧 重构：配置属性访问方法 ====================
// 所有配置数据访问都委托给 DataAccessor，带默认值保护

FText AXBSoldierCharacter::GetDisplayName() const
{
    return IsDataAccessorValid() ? DataAccessor->GetDisplayName() : FText::FromString(TEXT("未命名士兵"));
}

FGameplayTagContainer AXBSoldierCharacter::GetSoldierTags() const
{
    return IsDataAccessorValid() ? DataAccessor->GetSoldierTags() : FGameplayTagContainer();
}

float AXBSoldierCharacter::GetMaxHealth() const
{
    return IsDataAccessorValid() ? DataAccessor->GetMaxHealth() : 100.0f;
}

float AXBSoldierCharacter::GetBaseDamage() const
{
    return IsDataAccessorValid() ? DataAccessor->GetBaseDamage() : 10.0f;
}

float AXBSoldierCharacter::GetAttackRange() const
{
    return IsDataAccessorValid() ? DataAccessor->GetAttackRange() : 150.0f;
}

float AXBSoldierCharacter::GetAttackInterval() const
{
    return IsDataAccessorValid() ? DataAccessor->GetAttackInterval() : 1.0f;
}

float AXBSoldierCharacter::GetMoveSpeed() const
{
    return IsDataAccessorValid() ? DataAccessor->GetMoveSpeed() : 400.0f;
}

float AXBSoldierCharacter::GetSprintSpeedMultiplier() const
{
    return IsDataAccessorValid() ? DataAccessor->GetSprintSpeedMultiplier() : 2.0f;
}

float AXBSoldierCharacter::GetFollowInterpSpeed() const
{
    return IsDataAccessorValid() ? DataAccessor->GetFollowInterpSpeed() : 5.0f;
}

float AXBSoldierCharacter::GetRotationSpeed() const
{
    return IsDataAccessorValid() ? DataAccessor->GetRotationSpeed() : 360.0f;
}

float AXBSoldierCharacter::GetVisionRange() const
{
    return IsDataAccessorValid() ? DataAccessor->GetVisionRange() : 800.0f;
}

float AXBSoldierCharacter::GetDisengageDistance() const
{
    return IsDataAccessorValid() ? DataAccessor->GetDisengageDistance() : 1000.0f;
}

float AXBSoldierCharacter::GetReturnDelay() const
{
    return IsDataAccessorValid() ? DataAccessor->GetReturnDelay() : 2.0f;
}

float AXBSoldierCharacter::GetArrivalThreshold() const
{
    return IsDataAccessorValid() ? DataAccessor->GetArrivalThreshold() : 50.0f;
}

float AXBSoldierCharacter::GetAvoidanceRadius() const
{
    return IsDataAccessorValid() ? DataAccessor->GetAvoidanceRadius() : 50.0f;
}

float AXBSoldierCharacter::GetAvoidanceWeight() const
{
    return IsDataAccessorValid() ? DataAccessor->GetAvoidanceWeight() : 0.3f;
}

float AXBSoldierCharacter::GetHealthBonusToLeader() const
{
    return IsDataAccessorValid() ? DataAccessor->GetHealthBonusToLeader() : 20.0f;
}

float AXBSoldierCharacter::GetDamageBonusToLeader() const
{
    return IsDataAccessorValid() ? DataAccessor->GetDamageBonusToLeader() : 2.0f;
}

float AXBSoldierCharacter::GetScaleBonusToLeader() const
{
    return IsDataAccessorValid() ? DataAccessor->GetScaleBonusToLeader() : 0.01f;
}

// ==================== 招募系统实现 ====================

bool AXBSoldierCharacter::CanBeRecruited() const
{
    if (bIsRecruited)
    {
        return false;
    }
    
    if (Faction != EXBFaction::Neutral)
    {
        return false;
    }
    
    if (CurrentState != EXBSoldierState::Idle)
    {
        return false;
    }
    
    if (CurrentHealth <= 0.0f)
    {
        return false;
    }
    
    if (!bComponentsInitialized)
    {
        return false;
    }
    
    return true;
}

void AXBSoldierCharacter::OnRecruited(AActor* NewLeader, int32 SlotIndex)
{
    if (!NewLeader)
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: 招募失败 - 将领为空"), *GetName());
        return;
    }
    
    if (bIsRecruited)
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: 已被招募，忽略重复招募"), *GetName());
        return;
    }
    
    if (!bComponentsInitialized)
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: 组件未初始化，延迟招募"), *GetName());
        FTimerHandle TempHandle;
        GetWorldTimerManager().SetTimer(TempHandle, [this, NewLeader, SlotIndex]()
        {
            OnRecruited(NewLeader, SlotIndex);
        }, 0.1f, false);
        return;
    }
    
    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: 被将领 %s 招募，槽位: %d"), 
        *GetName(), *NewLeader->GetName(), SlotIndex);
    
    bIsRecruited = true;
    FollowTarget = NewLeader;
    FormationSlotIndex = SlotIndex;
    
    if (FollowComponent)
    {
        FollowComponent->SetFollowTarget(NewLeader);
        FollowComponent->SetFormationSlotIndex(SlotIndex);
        FollowComponent->StartRecruitTransition();
    }
    
    if (AXBCharacterBase* LeaderChar = Cast<AXBCharacterBase>(NewLeader))
    {
        Faction = LeaderChar->GetFaction();
    }
    
    SetSoldierState(EXBSoldierState::Following);
    
    GetWorldTimerManager().SetTimer(
        DelayedAIStartTimerHandle,
        this,
        &AXBSoldierCharacter::SpawnAndPossessAIController,
        0.3f,
        false
    );
    
    OnSoldierRecruited.Broadcast(this, NewLeader);
}

// ==================== AI控制器初始化 ====================

void AXBSoldierCharacter::SpawnAndPossessAIController()
{
    if (!IsValid(this) || IsPendingKillPending())
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("SpawnAndPossessAIController: 士兵已无效"));
        return;
    }
    
    UCapsuleComponent* Capsule = GetCapsuleComponent();
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    
    if (!Capsule || !MoveComp)
    {
        UE_LOG(LogXBSoldier, Error, TEXT("士兵 %s: 组件无效，无法启动AI"), *GetName());
        return;
    }
    
    FTransform CapsuleTransform = Capsule->GetComponentTransform();
    if (!CapsuleTransform.IsValid())
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: Transform 无效，再次延迟"), *GetName());
        GetWorldTimerManager().SetTimer(
            DelayedAIStartTimerHandle,
            this,
            &AXBSoldierCharacter::SpawnAndPossessAIController,
            0.1f,
            false
        );
        return;
    }
    
    if (GetController())
    {
        UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: 已有控制器，直接初始化AI"), *GetName());
        InitializeAI();
        return;
    }
    
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogXBSoldier, Error, TEXT("士兵 %s: 无法获取World"), *GetName());
        return;
    }
    
    UClass* ControllerClassToUse = nullptr;
    if (SoldierAIControllerClass)
    {
        ControllerClassToUse = SoldierAIControllerClass.Get();
    }
    else
    {
        ControllerClassToUse = AXBSoldierAIController::StaticClass();
    }
    
    if (!ControllerClassToUse)
    {
        UE_LOG(LogXBSoldier, Error, TEXT("士兵 %s: AI控制器类无效"), *GetName());
        return;
    }
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    AAIController* NewController = World->SpawnActor<AAIController>(
        ControllerClassToUse,
        GetActorLocation(),
        GetActorRotation(),
        SpawnParams
    );
    
    if (NewController)
    {
        NewController->Possess(this);
        
        UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: AI控制器创建成功 - %s"), 
            *GetName(), *NewController->GetName());
        
        InitializeAI();
    }
    else
    {
        UE_LOG(LogXBSoldier, Error, TEXT("士兵 %s: 无法创建AI控制器"), *GetName());
    }
}

void AXBSoldierCharacter::InitializeAI()
{
    AAIController* AICtrl = Cast<AAIController>(GetController());
    if (!AICtrl)
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: InitializeAI - 无AI控制器"), *GetName());
        return;
    }
    
    if (BehaviorTreeAsset)
    {
        AICtrl->RunBehaviorTree(BehaviorTreeAsset);
        
        if (UBlackboardComponent* BBComp = AICtrl->GetBlackboardComponent())
        {
            BBComp->SetValueAsObject(TEXT("Self"), this);
            BBComp->SetValueAsObject(TEXT("Leader"), FollowTarget.Get());
            BBComp->SetValueAsInt(TEXT("SoldierState"), static_cast<int32>(CurrentState));
            BBComp->SetValueAsInt(TEXT("FormationSlot"), FormationSlotIndex);
            
            // 🔧 从 DataAccessor 获取配置
            BBComp->SetValueAsFloat(TEXT("AttackRange"), GetAttackRange());
            BBComp->SetValueAsFloat(TEXT("VisionRange"), GetVisionRange());
            BBComp->SetValueAsFloat(TEXT("DetectionRange"), GetVisionRange());
            BBComp->SetValueAsBool(TEXT("IsAtFormation"), true);
            BBComp->SetValueAsBool(TEXT("CanAttack"), true);
        }
        
        UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: 行为树启动成功"), *GetName());
    }
    else
    {
        UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: 无行为树，使用状态机"), *GetName());
    }
}

// ==================== 跟随系统实现 ====================

void AXBSoldierCharacter::SetFollowTarget(AActor* NewLeader, int32 SlotIndex)
{
    FollowTarget = NewLeader;
    FormationSlotIndex = SlotIndex;

    if (FollowComponent)
    {
        FollowComponent->SetFollowTarget(NewLeader);
        FollowComponent->SetFormationSlotIndex(SlotIndex);
    }

    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BBComp = AICtrl->GetBlackboardComponent())
        {
            BBComp->SetValueAsObject(TEXT("Leader"), NewLeader);
            BBComp->SetValueAsInt(TEXT("FormationSlot"), SlotIndex);
        }
    }

    if (NewLeader)
    {
        SetSoldierState(EXBSoldierState::Following);
    }
    else
    {
        SetSoldierState(EXBSoldierState::Idle);
    }
}

AXBCharacterBase* AXBSoldierCharacter::GetLeaderCharacter() const
{
    return Cast<AXBCharacterBase>(FollowTarget.Get());
}

void AXBSoldierCharacter::SetFormationSlotIndex(int32 NewIndex)
{
    FormationSlotIndex = NewIndex;

    if (FollowComponent)
    {
        FollowComponent->SetFormationSlotIndex(NewIndex);
    }

    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BBComp = AICtrl->GetBlackboardComponent())
        {
            BBComp->SetValueAsInt(TEXT("FormationSlot"), NewIndex);
        }
    }
}

// ==================== 状态管理实现 ====================

void AXBSoldierCharacter::SetSoldierState(EXBSoldierState NewState)
{
    if (CurrentState == NewState)
    {
        return;
    }

    EXBSoldierState OldState = CurrentState;
    CurrentState = NewState;

    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BBComp = AICtrl->GetBlackboardComponent())
        {
            BBComp->SetValueAsInt(TEXT("SoldierState"), static_cast<int32>(NewState));
        }
    }

    OnSoldierStateChanged.Broadcast(OldState, NewState);

    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 状态变化: %d -> %d"), 
        *GetName(), static_cast<int32>(OldState), static_cast<int32>(NewState));
}

// ==================== 战斗系统实现 ====================

void AXBSoldierCharacter::EnterCombat()
{
    if (CurrentState == EXBSoldierState::Dead)
    {
        return;
    }

    if (!bIsRecruited)
    {
        return;
    }

    if (FollowComponent)
    {
        FollowComponent->EnterCombatMode();
    }

    SetSoldierState(EXBSoldierState::Combat);
    
    // 🔧 修改 - 通过 BehaviorInterface 查找敌人
    if (BehaviorInterface)
    {
        AActor* FoundEnemy = nullptr;
        if (BehaviorInterface->SearchForEnemy(FoundEnemy))
        {
            CurrentAttackTarget = FoundEnemy;
        }
    }

    UE_LOG(LogXBCombat, Log, TEXT("士兵 %s 进入战斗, 目标: %s"), 
        *GetName(), CurrentAttackTarget.IsValid() ? *CurrentAttackTarget->GetName() : TEXT("无"));
}

void AXBSoldierCharacter::ExitCombat()
{
    if (CurrentState == EXBSoldierState::Dead)
    {
        return;
    }

    CurrentAttackTarget = nullptr;
    
    if (FollowComponent)
    {
        FollowComponent->ExitCombatMode();
    }
    
    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        AICtrl->StopMovement();
    }

    SetSoldierState(EXBSoldierState::Following);

    UE_LOG(LogXBCombat, Log, TEXT("士兵 %s 退出战斗"), *GetName());
}

float AXBSoldierCharacter::TakeSoldierDamage(float DamageAmount, AActor* DamageSource)
{
    if (CurrentState == EXBSoldierState::Dead)
    {
        return 0.0f;
    }

    float ActualDamage = FMath::Min(DamageAmount, CurrentHealth);
    CurrentHealth -= ActualDamage;

    UE_LOG(LogXBCombat, Log, TEXT("士兵 %s 受到 %.1f 伤害, 剩余血量: %.1f"), 
        *GetName(), ActualDamage, CurrentHealth);

    if (CurrentHealth <= 0.0f)
    {
        HandleDeath();
    }

    return ActualDamage;
}

bool AXBSoldierCharacter::PerformAttack(AActor* Target)
{
    // 🔧 修改 - 委托给 BehaviorInterface
    if (BehaviorInterface)
    {
        EXBBehaviorResult Result = BehaviorInterface->ExecuteAttack(Target);
        return Result == EXBBehaviorResult::Success;
    }
    return false;
}

bool AXBSoldierCharacter::PlayAttackMontage()
{
    if (!IsDataAccessorValid())
    {
        return false;
    }

    UAnimMontage* AttackMontage = DataAccessor->GetBasicAttackMontage();

    if (!AttackMontage)
    {
        return false;
    }

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        return AnimInstance->Montage_Play(AttackMontage) > 0.0f;
    }

    return false;
}

// ==================== AI系统实现 ====================


bool AXBSoldierCharacter::CanAttack() const
{
    // 🔧 修改 - 委托给 BehaviorInterface
    if (BehaviorInterface)
    {
        return BehaviorInterface->GetAttackCooldownRemaining() <= 0.0f && 
               CurrentState != EXBSoldierState::Dead;
    }
    return false;
}

bool AXBSoldierCharacter::HasEnemiesInRadius(float Radius) const
{
    FXBDetectionResult Result;
    return UXBBlueprintFunctionLibrary::DetectEnemiesInRadius(
        this,
        GetActorLocation(),
        Radius,
        Faction,
        true,
        Result
    );
}

float AXBSoldierCharacter::GetDistanceToTarget(AActor* Target) const
{
    if (!Target || !IsValid(Target))
    {
        return MAX_FLT;
    }
    return FVector::Dist(GetActorLocation(), Target->GetActorLocation());
}

bool AXBSoldierCharacter::IsInAttackRange(AActor* Target) const
{
    if (!Target || !IsValid(Target))
    {
        return false;
    }

    // 🔧 从 DataAccessor 获取攻击范围
    float AttackRange = GetAttackRange();
    return GetDistanceToTarget(Target) <= AttackRange;
}



void AXBSoldierCharacter::ReturnToFormation()
{
    CurrentAttackTarget = nullptr;
    
    if (FollowComponent)
    {
        FollowComponent->ExitCombatMode();
    }
    
    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        AICtrl->StopMovement();
    }
    
    SetSoldierState(EXBSoldierState::Following);

    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 传送回队列"), *GetName());
}



FVector AXBSoldierCharacter::CalculateAvoidanceDirection(const FVector& DesiredDirection)
{
    // 🔧 从 DataAccessor 获取避障配置
    float AvoidanceRadius = GetAvoidanceRadius();
    float AvoidanceWeight = GetAvoidanceWeight();

    if (AvoidanceRadius <= 0.0f)
    {
        return DesiredDirection;
    }

    FVector AvoidanceForce = FVector::ZeroVector;
    FVector MyLocation = GetActorLocation();

    FXBDetectionResult AlliesResult;
    UXBBlueprintFunctionLibrary::DetectAlliesInRadius(
        this,
        MyLocation,
        AvoidanceRadius,
        Faction,
        true,
        AlliesResult
    );

    int32 AvoidanceCount = 0;

    for (AActor* OtherActor : AlliesResult.DetectedActors)
    {
        if (OtherActor == this)
        {
            continue;
        }

        float Distance = FVector::Dist2D(MyLocation, OtherActor->GetActorLocation());
        if (Distance > KINDA_SMALL_NUMBER)
        {
            FVector AwayDirection = (MyLocation - OtherActor->GetActorLocation()).GetSafeNormal2D();
            float Strength = 1.0f - (Distance / AvoidanceRadius);
            AvoidanceForce += AwayDirection * Strength;
            AvoidanceCount++;
        }
    }

    if (AvoidanceCount == 0)
    {
        return DesiredDirection;
    }

    AvoidanceForce.Normalize();

    FVector BlendedDirection = DesiredDirection * (1.0f - AvoidanceWeight) + 
                               AvoidanceForce * AvoidanceWeight;

    return BlendedDirection.GetSafeNormal();
}



void AXBSoldierCharacter::MoveToFormationPosition()
{
    if (FollowComponent)
    {
        FollowComponent->StartInterpolateToFormation();
    }
}

FVector AXBSoldierCharacter::GetFormationWorldPosition() const
{
    if (!FollowTarget.IsValid())
    {
        return GetActorLocation();
    }

    if (FollowComponent)
    {
        return FollowComponent->GetTargetPosition();
    }

    return FollowTarget->GetActorLocation();
}

FVector AXBSoldierCharacter::GetFormationWorldPositionSafe() const
{
    if (!FollowTarget.IsValid())
    {
        return FVector::ZeroVector;
    }
    
    AActor* Target = FollowTarget.Get();
    if (!Target || !IsValid(Target))
    {
        return FVector::ZeroVector;
    }
    
    if (!FollowComponent)
    {
        return Target->GetActorLocation();
    }
    
    FVector TargetPos = FollowComponent->GetTargetPosition();
    if (!TargetPos.IsZero() && !TargetPos.ContainsNaN())
    {
        return TargetPos;
    }
    
    return Target->GetActorLocation();
}

bool AXBSoldierCharacter::IsAtFormationPosition() const
{
    if (FollowComponent)
    {
        return FollowComponent->IsAtFormationPosition();
    }
    
    FVector TargetPos = GetFormationWorldPosition();
    // 🔧 从 DataAccessor 获取到达阈值
    float ArrivalThreshold = GetArrivalThreshold();
    return FVector::Dist2D(GetActorLocation(), TargetPos) <= ArrivalThreshold;
}

bool AXBSoldierCharacter::IsAtFormationPositionSafe() const
{
    if (!FollowTarget.IsValid() || FormationSlotIndex == INDEX_NONE)
    {
        return true;
    }
    
    if (FollowComponent)
    {
        return FollowComponent->IsAtFormationPosition();
    }
    
    return true;
}

// ==================== 逃跑系统实现 ====================

void AXBSoldierCharacter::SetEscaping(bool bEscaping)
{
    bIsEscaping = bEscaping;

    if (bEscaping)
    {
        if (FollowComponent)
        {
            FollowComponent->SetCombatState(false);
            
            if (CurrentState == EXBSoldierState::Combat)
            {
                CurrentAttackTarget = nullptr;
                SetSoldierState(EXBSoldierState::Following);
            }
            
            FollowComponent->StartInterpolateToFormation();
        }
        
        if (AAIController* AICtrl = Cast<AAIController>(GetController()))
        {
            AICtrl->StopMovement();
        }
    }

    // 🔧 从 DataAccessor 获取移动速度配置
    float BaseSpeed = GetMoveSpeed();
    float SprintMultiplier = GetSprintSpeedMultiplier();

    float NewSpeed = bEscaping ? BaseSpeed * SprintMultiplier : BaseSpeed;

    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->MaxWalkSpeed = NewSpeed;
    }
}

// ==================== 死亡系统实现 ====================

void AXBSoldierCharacter::HandleDeath()
{
    GetWorldTimerManager().ClearTimer(DelayedAIStartTimerHandle);
    
    SetSoldierState(EXBSoldierState::Dead);

    OnSoldierDied.Broadcast(this);

    if (AXBCharacterBase* LeaderCharacter = GetLeaderCharacter())
    {
        LeaderCharacter->OnSoldierDied(this);
    }

    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        AICtrl->StopMovement();
    }

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->SetComponentTickEnabled(false);
    }

    // 🔧 播放死亡蒙太奇（从 DataAccessor 获取）
    if (IsDataAccessorValid())
    {
        UAnimMontage* DeathMontage = DataAccessor->GetDeathMontage();
        if (DeathMontage)
        {
            if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
            {
                AnimInstance->Montage_Play(DeathMontage);
            }
        }
    }

    SetLifeSpan(2.0f);

    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 死亡"), *GetName());
}



void AXBSoldierCharacter::FaceTarget(AActor* Target, float DeltaTime)
{
    if (!Target || !IsValid(Target))
    {
        return;
    }

    FVector Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    if (!Direction.IsNearlyZero())
    {
        FRotator TargetRotation = Direction.Rotation();
        FRotator CurrentRotation = GetActorRotation();
        
        // 🔧 从 DataAccessor 获取旋转速度
        float RotationSpeed = GetRotationSpeed();
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotationSpeed / 90.0f);
        SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
    }
}
