/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Soldier/XBSoldierActor.cpp

/**
 * @file XBSoldierActor.cpp
 * @brief 士兵Actor实现
 * 
 * @note 🔧 修改记录:
 *       1. 修复组件初始化问题导致的崩溃
 *       2. 添加组件有效性检查
 *       3. 延迟启用移动组件Tick
 */

#include "Soldier/XBSoldierActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Soldier/Component/XBSoldierFollowComponent.h"
#include "Character/XBCharacterBase.h"
#include "Character/Components/XBFormationComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "AI/XBSoldierAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/DataTable.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"

AXBSoldierActor::AXBSoldierActor()
{
    PrimaryActorTick.bCanEverTick = true;
    // 🔧 修改 - 延迟启用Tick，等待组件初始化
    PrimaryActorTick.bStartWithTickEnabled = false;

    // 配置胶囊体
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->InitCapsuleSize(34.0f, 88.0f);
        Capsule->SetCollisionProfileName(TEXT("Pawn"));
    }

    // 配置网格体
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
    }

    // 创建跟随组件
    FollowComponent = CreateDefaultSubobject<UXBSoldierFollowComponent>(TEXT("FollowComponent"));

    // 🔧 修改 - 配置移动组件，延迟启用
    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->bOrientRotationToMovement = true;
        MovementComp->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
        MovementComp->MaxWalkSpeed = 400.0f;
        MovementComp->BrakingDecelerationWalking = 2000.0f;
        
        // 🔧 修改 - 初始时禁用移动组件的某些功能
        MovementComp->SetComponentTickEnabled(false);
    }

    // 完全禁用自动AI控制
    AutoPossessAI = EAutoPossessAI::Disabled;
    AIControllerClass = nullptr;
}

/**
 * @brief 组件初始化完成后的回调
 * @note ✨ 新增 - 验证所有组件正确初始化
 */
void AXBSoldierActor::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    
    bComponentsInitialized = true;
    
    // 验证胶囊体
    UCapsuleComponent* Capsule = GetCapsuleComponent();
    if (Capsule)
    {
        // 确保 Transform 有效
        FTransform CapsuleTransform = Capsule->GetComponentTransform();
        FVector Scale = CapsuleTransform.GetScale3D();
        
        if (Scale.IsNearlyZero() || Scale.ContainsNaN())
        {
            UE_LOG(LogTemp, Warning, TEXT("士兵 %s: Capsule Scale 无效 (%s)，修正为 (1,1,1)"), 
                *GetName(), *Scale.ToString());
            Capsule->SetWorldScale3D(FVector::OneVector);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("士兵 %s: CapsuleComponent 为空!"), *GetName());
    }
    
    // 验证移动组件
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    if (MoveComp)
    {
        if (!MoveComp->UpdatedComponent)
        {
            UE_LOG(LogTemp, Warning, TEXT("士兵 %s: MovementComponent 的 UpdatedComponent 为空"), *GetName());
            // 尝试设置
            MoveComp->SetUpdatedComponent(Capsule);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("士兵 %s: CharacterMovementComponent 为空!"), *GetName());
    }
    
    UE_LOG(LogTemp, Log, TEXT("士兵 %s: PostInitializeComponents 完成"), *GetName());
}

void AXBSoldierActor::BeginPlay()
{
    Super::BeginPlay();

    // 初始化血量
    if (bInitializedFromDataTable)
    {
        CurrentHealth = CachedTableRow.MaxHealth;
    }
    else
    {
        CurrentHealth = SoldierConfig.MaxHealth;
    }
    
    // 🔧 修改 - 延迟启用移动组件和Tick
    // 说明: 确保物理世界完全同步后再启用
    GetWorldTimerManager().SetTimerForNextTick([this]()
    {
        EnableMovementAndTick();
    });

    UE_LOG(LogTemp, Log, TEXT("士兵 %s BeginPlay - 阵营: %d, 状态: %d"), 
        *GetName(), static_cast<int32>(Faction), static_cast<int32>(CurrentState));
}

/**
 * @brief 启用移动组件和Tick
 * @note ✨ 新增 - 延迟启用，确保组件就绪
 */
void AXBSoldierActor::EnableMovementAndTick()
{
    if (!IsValid(this) || IsPendingKillPending())
    {
        return;
    }
    
    // 再次验证组件
    UCapsuleComponent* Capsule = GetCapsuleComponent();
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    
    if (!Capsule || !MoveComp)
    {
        UE_LOG(LogTemp, Error, TEXT("士兵 %s: 组件无效，无法启用移动"), *GetName());
        return;
    }
    
    // 验证 Transform
    FTransform CapsuleTransform = Capsule->GetComponentTransform();
    if (!CapsuleTransform.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("士兵 %s: Capsule Transform 无效"), *GetName());
        return;
    }
    
    // 启用移动组件
    MoveComp->SetComponentTickEnabled(true);
    
    // 启用Actor Tick
    SetActorTickEnabled(true);
    
    UE_LOG(LogTemp, Log, TEXT("士兵 %s: 移动组件和Tick已启用"), *GetName());
}

void AXBSoldierActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 更新攻击冷却
    if (AttackCooldownTimer > 0.0f)
    {
        AttackCooldownTimer -= DeltaTime;
    }

    // 未招募的士兵跳过状态更新
    if (!bIsRecruited)
    {
        return;
    }

    // 如果没有行为树或AI控制器，使用简单状态机
    AAIController* AICtrl = Cast<AAIController>(GetController());
    if (!BehaviorTreeAsset || !AICtrl)
    {
        switch (CurrentState)
        {
        case EXBSoldierState::Following:
            UpdateFollowing(DeltaTime);
            break;
        case EXBSoldierState::Combat:
            UpdateCombat(DeltaTime);
            break;
        case EXBSoldierState::Returning:
            UpdateReturning(DeltaTime);
            break;
        default:
            break;
        }
    }
}

// ==================== 招募系统实现 ====================

bool AXBSoldierActor::CanBeRecruited() const
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
    
    // ✨ 新增 - 检查组件是否就绪
    if (!bComponentsInitialized)
    {
        return false;
    }
    
    return true;
}

void AXBSoldierActor::OnRecruited(AActor* NewLeader, int32 SlotIndex)
{
    if (!NewLeader)
    {
        UE_LOG(LogTemp, Warning, TEXT("士兵 %s: 招募失败 - 将领为空"), *GetName());
        return;
    }
    
    if (bIsRecruited)
    {
        UE_LOG(LogTemp, Warning, TEXT("士兵 %s: 已被招募，忽略重复招募"), *GetName());
        return;
    }
    
    // ✨ 新增 - 检查组件是否就绪
    if (!bComponentsInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("士兵 %s: 组件未初始化，延迟招募"), *GetName());
        // 延迟再试
        FTimerHandle TempHandle;
        GetWorldTimerManager().SetTimer(TempHandle, [this, NewLeader, SlotIndex]()
        {
            OnRecruited(NewLeader, SlotIndex);
        }, 0.1f, false);
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("士兵 %s: 被将领 %s 招募，槽位: %d"), 
        *GetName(), *NewLeader->GetName(), SlotIndex);
    
    // 标记为已招募
    bIsRecruited = true;
    
    // 设置跟随目标
    FollowTarget = NewLeader;
    FormationSlotIndex = SlotIndex;
    
    // 更新跟随组件
    if (FollowComponent)
    {
        FollowComponent->SetFollowTarget(NewLeader);
        FollowComponent->SetFormationSlotIndex(SlotIndex);
    }
    
    // 更新阵营为将领阵营
    if (AXBCharacterBase* LeaderChar = Cast<AXBCharacterBase>(NewLeader))
    {
        Faction = LeaderChar->GetFaction();
    }
    
    // 设置为跟随状态
    SetSoldierState(EXBSoldierState::Following);
    
    // 🔧 修改 - 延迟启动AI控制器
    GetWorldTimerManager().SetTimer(
        DelayedAIStartTimerHandle,
        this,
        &AXBSoldierActor::SpawnAndPossessAIController,
        0.3f,  // 延迟 0.3 秒
        false
    );
    
    // 广播招募事件
    OnSoldierRecruited.Broadcast(this, NewLeader);
}

void AXBSoldierActor::SpawnAndPossessAIController()
{
    // 安全检查
    if (!IsValid(this) || IsPendingKillPending())
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnAndPossessAIController: 士兵已无效"));
        return;
    }
    
    // ✨ 新增 - 验证组件状态
    UCapsuleComponent* Capsule = GetCapsuleComponent();
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    
    if (!Capsule || !MoveComp)
    {
        UE_LOG(LogTemp, Error, TEXT("士兵 %s: 组件无效，无法启动AI"), *GetName());
        return;
    }
    
    // 验证 Transform
    FTransform CapsuleTransform = Capsule->GetComponentTransform();
    if (!CapsuleTransform.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("士兵 %s: Transform 无效，再次延迟"), *GetName());
        GetWorldTimerManager().SetTimer(
            DelayedAIStartTimerHandle,
            this,
            &AXBSoldierActor::SpawnAndPossessAIController,
            0.1f,
            false
        );
        return;
    }
    
    // 检查是否已有控制器
    if (GetController())
    {
        UE_LOG(LogTemp, Log, TEXT("士兵 %s: 已有控制器，直接初始化AI"), *GetName());
        InitializeAI();
        return;
    }
    
    // 获取World
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("士兵 %s: 无法获取World"), *GetName());
        return;
    }
    
    // 确定要使用的AI控制器类
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
        UE_LOG(LogTemp, Error, TEXT("士兵 %s: AI控制器类无效"), *GetName());
        return;
    }
    
    // 生成AI控制器
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
        // Possess
        NewController->Possess(this);
        
        UE_LOG(LogTemp, Log, TEXT("士兵 %s: AI控制器创建成功 - %s"), 
            *GetName(), *NewController->GetName());
        
        // 初始化AI
        InitializeAI();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("士兵 %s: 无法创建AI控制器"), *GetName());
    }
}

void AXBSoldierActor::InitializeAI()
{
    AAIController* AICtrl = Cast<AAIController>(GetController());
    if (!AICtrl)
    {
        UE_LOG(LogTemp, Warning, TEXT("士兵 %s: InitializeAI - 无AI控制器"), *GetName());
        return;
    }
    
    if (BehaviorTreeAsset)
    {
        AICtrl->RunBehaviorTree(BehaviorTreeAsset);
        
        if (UBlackboardComponent* BBComp = AICtrl->GetBlackboardComponent())
        {
            BBComp->SetValueAsObject(TEXT("Self"), this);
            BBComp->SetValueAsObject(TEXT("Leader"), FollowTarget.Get());
            BBComp->SetValueAsEnum(TEXT("SoldierState"), static_cast<uint8>(CurrentState));
            BBComp->SetValueAsInt(TEXT("FormationSlot"), FormationSlotIndex);
            
            float AttackRange = bInitializedFromDataTable ? CachedTableRow.AttackRange : SoldierConfig.AttackRange;
            BBComp->SetValueAsFloat(TEXT("AttackRange"), AttackRange);
            BBComp->SetValueAsFloat(TEXT("DetectionRange"), 800.0f);
            BBComp->SetValueAsBool(TEXT("IsAtFormation"), true);
            BBComp->SetValueAsBool(TEXT("CanAttack"), true);
        }
        
        UE_LOG(LogTemp, Log, TEXT("士兵 %s: 行为树启动成功"), *GetName());
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("士兵 %s: 无行为树，使用状态机"), *GetName());
    }
}

// ==================== 初始化实现 ====================

void AXBSoldierActor::InitializeFromDataTable(UDataTable* DataTable, FName RowName, EXBFaction InFaction)
{
    if (!DataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("士兵初始化失败: 数据表为空"));
        return;
    }

    if (RowName.IsNone())
    {
        UE_LOG(LogTemp, Error, TEXT("士兵初始化失败: 行名为空"));
        return;
    }

    FXBSoldierTableRow* Row = DataTable->FindRow<FXBSoldierTableRow>(RowName, TEXT("AXBSoldierActor::InitializeFromDataTable"));
    if (!Row)
    {
        UE_LOG(LogTemp, Error, TEXT("士兵初始化失败: 找不到行 '%s'"), *RowName.ToString());
        return;
    }

    CachedTableRow = *Row;
    bInitializedFromDataTable = true;

    SoldierType = Row->SoldierType;
    Faction = InFaction;
    CurrentHealth = Row->MaxHealth;

    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->MaxWalkSpeed = Row->MoveSpeed;
        MovementComp->RotationRate = FRotator(0.0f, Row->RotationSpeed, 0.0f);
    }

    if (FollowComponent)
    {
        FollowComponent->SetFollowSpeed(Row->MoveSpeed);
        FollowComponent->SetFollowInterpSpeed(Row->FollowInterpSpeed);
    }

    if (!Row->AIConfig.BehaviorTree.IsNull())
    {
        BehaviorTreeAsset = Row->AIConfig.BehaviorTree.LoadSynchronous();
    }

    ApplyVisualConfig();

    SoldierConfig.SoldierType = Row->SoldierType;
    SoldierConfig.MaxHealth = Row->MaxHealth;
    SoldierConfig.BaseDamage = Row->BaseDamage;
    SoldierConfig.AttackRange = Row->AttackRange;
    SoldierConfig.AttackInterval = Row->AttackInterval;
    SoldierConfig.MoveSpeed = Row->MoveSpeed;
    SoldierConfig.FollowInterpSpeed = Row->FollowInterpSpeed;
    SoldierConfig.HealthBonusToLeader = Row->HealthBonusToLeader;
    SoldierConfig.DamageBonusToLeader = Row->DamageBonusToLeader;

    UE_LOG(LogTemp, Log, TEXT("士兵从数据表初始化: %s, 类型=%d, 血量=%.1f"), 
        *RowName.ToString(), static_cast<int32>(SoldierType), CurrentHealth);
}

void AXBSoldierActor::InitializeSoldier(const FXBSoldierConfig& InConfig, EXBFaction InFaction)
{
    SoldierConfig = InConfig;
    SoldierType = InConfig.SoldierType;
    Faction = InFaction;
    CurrentHealth = InConfig.MaxHealth;

    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->MaxWalkSpeed = InConfig.MoveSpeed;
    }

    if (FollowComponent)
    {
        FollowComponent->SetFollowSpeed(InConfig.MoveSpeed);
        FollowComponent->SetFollowInterpSpeed(InConfig.FollowInterpSpeed);
    }

    if (InConfig.SoldierMesh)
    {
        GetMesh()->SetSkeletalMesh(InConfig.SoldierMesh);
    }

    UE_LOG(LogTemp, Log, TEXT("士兵初始化: Type=%d, Health=%.1f"), 
        static_cast<int32>(SoldierType), CurrentHealth);
}

void AXBSoldierActor::ApplyVisualConfig()
{
    if (!bInitializedFromDataTable)
    {
        return;
    }

    if (!CachedTableRow.VisualConfig.SkeletalMesh.IsNull())
    {
        USkeletalMesh* LoadedMesh = CachedTableRow.VisualConfig.SkeletalMesh.LoadSynchronous();
        if (LoadedMesh)
        {
            GetMesh()->SetSkeletalMesh(LoadedMesh);
        }
    }

    if (CachedTableRow.VisualConfig.AnimClass)
    {
        GetMesh()->SetAnimInstanceClass(CachedTableRow.VisualConfig.AnimClass);
    }

    SetActorScale3D(FVector(CachedTableRow.VisualConfig.MeshScale));
}

// ==================== 跟随系统实现 ====================

void AXBSoldierActor::SetFollowTarget(AActor* NewLeader, int32 SlotIndex)
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

AXBCharacterBase* AXBSoldierActor::GetLeaderCharacter() const
{
    return Cast<AXBCharacterBase>(FollowTarget.Get());
}

void AXBSoldierActor::SetFormationSlotIndex(int32 NewIndex)
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

void AXBSoldierActor::SetSoldierState(EXBSoldierState NewState)
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
            BBComp->SetValueAsEnum(TEXT("SoldierState"), static_cast<uint8>(NewState));
        }
    }

    OnSoldierStateChanged.Broadcast(OldState, NewState);

    UE_LOG(LogTemp, Log, TEXT("士兵 %s 状态变化: %d -> %d"), 
        *GetName(), static_cast<int32>(OldState), static_cast<int32>(NewState));
}

// ==================== 战斗系统实现 ====================

void AXBSoldierActor::EnterCombat()
{
    if (CurrentState == EXBSoldierState::Dead)
    {
        return;
    }

    if (!bIsRecruited)
    {
        return;
    }

    SetSoldierState(EXBSoldierState::Combat);
    CurrentAttackTarget = FindNearestEnemy();

    UE_LOG(LogTemp, Log, TEXT("士兵 %s 进入战斗, 目标: %s"), 
        *GetName(), CurrentAttackTarget.IsValid() ? *CurrentAttackTarget->GetName() : TEXT("无"));
}

void AXBSoldierActor::ExitCombat()
{
    if (CurrentState == EXBSoldierState::Dead)
    {
        return;
    }

    CurrentAttackTarget = nullptr;
    SetSoldierState(EXBSoldierState::Returning);

    UE_LOG(LogTemp, Log, TEXT("士兵 %s 退出战斗，返回队列"), *GetName());
}

float AXBSoldierActor::TakeSoldierDamage(float DamageAmount, AActor* DamageSource)
{
    if (CurrentState == EXBSoldierState::Dead)
    {
        return 0.0f;
    }

    float ActualDamage = FMath::Min(DamageAmount, CurrentHealth);
    CurrentHealth -= ActualDamage;

    UE_LOG(LogTemp, Log, TEXT("士兵 %s 受到 %.1f 伤害, 剩余血量: %.1f"), 
        *GetName(), ActualDamage, CurrentHealth);

    if (CurrentHealth <= 0.0f)
    {
        HandleDeath();
    }

    return ActualDamage;
}

bool AXBSoldierActor::PerformAttack(AActor* Target)
{
    if (!Target || !CanAttack())
    {
        return false;
    }

    float AttackInterval = bInitializedFromDataTable ? CachedTableRow.AttackInterval : SoldierConfig.AttackInterval;
    AttackCooldownTimer = AttackInterval;

    PlayAttackMontage();

    float Damage = bInitializedFromDataTable ? CachedTableRow.BaseDamage : SoldierConfig.BaseDamage;

    if (AXBSoldierActor* TargetSoldier = Cast<AXBSoldierActor>(Target))
    {
        TargetSoldier->TakeSoldierDamage(Damage, this);
    }

    UE_LOG(LogTemp, Log, TEXT("士兵 %s 攻击 %s，伤害: %.1f"), 
        *GetName(), *Target->GetName(), Damage);

    return true;
}

bool AXBSoldierActor::PlayAttackMontage()
{
    UAnimMontage* AttackMontage = nullptr;

    if (bInitializedFromDataTable && !CachedTableRow.BasicAttack.AbilityMontage.IsNull())
    {
        AttackMontage = CachedTableRow.BasicAttack.AbilityMontage.LoadSynchronous();
    }

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

AActor* AXBSoldierActor::FindNearestEnemy() const
{
    if (!bIsRecruited)
    {
        return nullptr;
    }

    float DetectionRange = bInitializedFromDataTable ? 
        CachedTableRow.AIConfig.DetectionRange : 800.0f;

    TArray<AActor*> PotentialTargets;
    
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AXBCharacterBase::StaticClass(), PotentialTargets);
    
    TArray<AActor*> SoldierActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AXBSoldierActor::StaticClass(), SoldierActors);
    PotentialTargets.Append(SoldierActors);

    AActor* NearestEnemy = nullptr;
    float NearestDistance = DetectionRange;

    for (AActor* Target : PotentialTargets)
    {
        if (Target == this)
        {
            continue;
        }

        bool bIsEnemy = false;
        if (const AXBCharacterBase* CharTarget = Cast<AXBCharacterBase>(Target))
        {
            bIsEnemy = (Faction == EXBFaction::Player || Faction == EXBFaction::Ally) ? 
                (CharTarget->GetFaction() == EXBFaction::Enemy) :
                (CharTarget->GetFaction() == EXBFaction::Player || CharTarget->GetFaction() == EXBFaction::Ally);
        }
        else if (const AXBSoldierActor* SoldierTarget = Cast<AXBSoldierActor>(Target))
        {
            if (SoldierTarget->GetSoldierState() == EXBSoldierState::Dead)
            {
                continue;
            }
            if (!SoldierTarget->IsRecruited())
            {
                continue;
            }
            bIsEnemy = (Faction == EXBFaction::Player || Faction == EXBFaction::Ally) ? 
                (SoldierTarget->GetFaction() == EXBFaction::Enemy) :
                (SoldierTarget->GetFaction() == EXBFaction::Player || SoldierTarget->GetFaction() == EXBFaction::Ally);
        }

        if (!bIsEnemy)
        {
            continue;
        }

        float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
        if (Distance < NearestDistance)
        {
            NearestDistance = Distance;
            NearestEnemy = Target;
        }
    }

    return NearestEnemy;
}

float AXBSoldierActor::GetDistanceToTarget(AActor* Target) const
{
    if (!Target)
    {
        return MAX_FLT;
    }
    return FVector::Dist(GetActorLocation(), Target->GetActorLocation());
}

bool AXBSoldierActor::IsInAttackRange(AActor* Target) const
{
    if (!Target)
    {
        return false;
    }

    float AttackRange = bInitializedFromDataTable ? CachedTableRow.AttackRange : SoldierConfig.AttackRange;
    return GetDistanceToTarget(Target) <= AttackRange;
}

bool AXBSoldierActor::ShouldDisengage() const
{
    if (!FollowTarget.IsValid())
    {
        return false;
    }

    float DisengageDistance = bInitializedFromDataTable ? 
        CachedTableRow.AIConfig.DisengageDistance : 1000.0f;

    return FVector::Dist(GetActorLocation(), FollowTarget->GetActorLocation()) > DisengageDistance;
}

void AXBSoldierActor::MoveToTarget(AActor* Target)
{
    if (!Target)
    {
        return;
    }

    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        AICtrl->MoveToActor(Target);
    }
}

void AXBSoldierActor::MoveToFormationPosition()
{
    FVector TargetPos = GetFormationWorldPosition();
    
    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        AICtrl->MoveToLocation(TargetPos);
    }
}

FVector AXBSoldierActor::GetFormationWorldPosition() const
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

FVector AXBSoldierActor::GetFormationWorldPositionSafe() const
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

bool AXBSoldierActor::IsAtFormationPosition() const
{
    FVector TargetPos = GetFormationWorldPosition();
    float ArrivalThreshold = 50.0f;
    return FVector::Dist2D(GetActorLocation(), TargetPos) <= ArrivalThreshold;
}

bool AXBSoldierActor::IsAtFormationPositionSafe() const
{
    if (!FollowTarget.IsValid() || FormationSlotIndex == INDEX_NONE)
    {
        return true;
    }
    
    FVector TargetPos = GetFormationWorldPositionSafe();
    if (TargetPos.IsZero())
    {
        return true;
    }
    
    return FVector::Dist2D(GetActorLocation(), TargetPos) <= 50.0f;
}

// ==================== 逃跑系统实现 ====================

void AXBSoldierActor::SetEscaping(bool bEscaping)
{
    bIsEscaping = bEscaping;

    float BaseSpeed = bInitializedFromDataTable ? CachedTableRow.MoveSpeed : SoldierConfig.MoveSpeed;
    float SprintMultiplier = bInitializedFromDataTable ? CachedTableRow.SprintSpeedMultiplier : 2.0f;

    float NewSpeed = bEscaping ? BaseSpeed * SprintMultiplier : BaseSpeed;

    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->MaxWalkSpeed = NewSpeed;
    }

    if (FollowComponent)
    {
        FollowComponent->SetFollowSpeed(NewSpeed);
    }
}

// ==================== 更新逻辑实现 ====================

void AXBSoldierActor::UpdateFollowing(float DeltaTime)
{
    if (FollowComponent)
    {
        FollowComponent->UpdateFollowing(DeltaTime);
    }
}

void AXBSoldierActor::UpdateCombat(float DeltaTime)
{
    float SearchInterval = bInitializedFromDataTable ? 
        CachedTableRow.AIConfig.TargetSearchInterval : 0.5f;
    
    TargetSearchTimer += DeltaTime;
    if (TargetSearchTimer >= SearchInterval || !CurrentAttackTarget.IsValid())
    {
        TargetSearchTimer = 0.0f;
        CurrentAttackTarget = FindNearestEnemy();
    }

    if (ShouldDisengage())
    {
        ExitCombat();
        return;
    }

    if (!CurrentAttackTarget.IsValid())
    {
        ExitCombat();
        return;
    }

    AActor* Target = CurrentAttackTarget.Get();
    float DistanceToEnemy = GetDistanceToTarget(Target);
    float AttackRange = bInitializedFromDataTable ? CachedTableRow.AttackRange : SoldierConfig.AttackRange;

    if (SoldierType == EXBSoldierType::Archer && bInitializedFromDataTable)
    {
        if (CachedTableRow.ArcherConfig.bStationaryAttack && DistanceToEnemy <= AttackRange)
        {
            FaceTarget(Target, DeltaTime);
            if (CanAttack())
            {
                PerformAttack(Target);
            }
            return;
        }
        
        if (DistanceToEnemy < CachedTableRow.ArcherConfig.MinAttackDistance)
        {
            FVector RetreatDirection = (GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();
            FVector RetreatTarget = GetActorLocation() + RetreatDirection * CachedTableRow.ArcherConfig.RetreatDistance;
            
            if (AAIController* AICtrl = Cast<AAIController>(GetController()))
            {
                AICtrl->MoveToLocation(RetreatTarget);
            }
            return;
        }
    }

    if (DistanceToEnemy > AttackRange)
    {
        MoveToTarget(Target);
    }
    else
    {
        FaceTarget(Target, DeltaTime);
        if (CanAttack())
        {
            PerformAttack(Target);
        }
    }
}

void AXBSoldierActor::UpdateReturning(float DeltaTime)
{
    MoveToFormationPosition();

    if (IsAtFormationPosition())
    {
        SetSoldierState(EXBSoldierState::Following);
    }
}

void AXBSoldierActor::FaceTarget(AActor* Target, float DeltaTime)
{
    if (!Target)
    {
        return;
    }

    FVector Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    if (!Direction.IsNearlyZero())
    {
        FRotator TargetRotation = Direction.Rotation();
        FRotator CurrentRotation = GetActorRotation();
        float RotationSpeed = bInitializedFromDataTable ? CachedTableRow.RotationSpeed : 360.0f;
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotationSpeed / 90.0f);
        SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
    }
}

void AXBSoldierActor::HandleDeath()
{
    // 清除定时器
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

    // 禁用移动组件
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->SetComponentTickEnabled(false);
    }

    if (bInitializedFromDataTable && !CachedTableRow.VisualConfig.DeathMontage.IsNull())
    {
        UAnimMontage* DeathMontage = CachedTableRow.VisualConfig.DeathMontage.LoadSynchronous();
        if (DeathMontage)
        {
            if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
            {
                AnimInstance->Montage_Play(DeathMontage);
            }
        }
    }

    SetLifeSpan(2.0f);

    UE_LOG(LogTemp, Log, TEXT("士兵 %s 死亡"), *GetName());
}
