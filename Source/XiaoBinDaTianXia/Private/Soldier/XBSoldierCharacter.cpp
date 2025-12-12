/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Soldier/XBSoldierCharacter.cpp

/**
 * @file XBSoldierCharacter.cpp
 * @brief 士兵Actor实现
 * 
 * @note 🔧 修改记录:
 *       1. 使用球形检测替代全量Actor搜索
 *       2. 从数据表读取所有配置值（消除硬编码）
 *       3. 使用项目专用日志类别
 *       4. 使用通用函数库进行阵营判断
 */

#include "Soldier/XBSoldierCharacter.h"
#include "Utils/XBLogCategories.h"
#include "Utils/XBBlueprintFunctionLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Soldier/Component/XBSoldierFollowComponent.h"
#include "Character/XBCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "AI/XBSoldierAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/DataTable.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"
#include "Soldier/Component/XBSoldierDebugComponent.h"

AXBSoldierCharacter::AXBSoldierCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->InitCapsuleSize(34.0f, 88.0f);
        Capsule->SetCollisionProfileName(TEXT("Pawn"));
    }

    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
    }

    FollowComponent = CreateDefaultSubobject<UXBSoldierFollowComponent>(TEXT("FollowComponent"));
    // ✨ 新增 - 创建调试组件
    DebugComponent = CreateDefaultSubobject<UXBSoldierDebugComponent>(TEXT("DebugComponent"));
    
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
    else
    {
        UE_LOG(LogXBSoldier, Error, TEXT("士兵 %s: CapsuleComponent 为空!"), *GetName());
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
    else
    {
        UE_LOG(LogXBSoldier, Error, TEXT("士兵 %s: CharacterMovementComponent 为空!"), *GetName());
    }
    
    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: PostInitializeComponents 完成"), *GetName());
}

void AXBSoldierCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (bInitializedFromDataTable)
    {
        CurrentHealth = CachedTableRow.MaxHealth;
    }
    else
    {
        CurrentHealth = SoldierConfig.MaxHealth;
    }
    
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

void AXBSoldierCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (AttackCooldownTimer > 0.0f)
    {
        AttackCooldownTimer -= DeltaTime;
    }

    if (!bIsRecruited)
    {
        return;
    }

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

// ==================== 配置访问方法（✨ 新增） ====================

float AXBSoldierCharacter::GetVisionRange() const
{
    if (bInitializedFromDataTable)
    {
        return CachedTableRow.GetVisionRange();
    }
    return 800.0f; // 默认值
}

float AXBSoldierCharacter::GetDisengageDistance() const
{
    if (bInitializedFromDataTable)
    {
        return CachedTableRow.AIConfig.DisengageDistance;
    }
    return 1000.0f; // 默认值
}

float AXBSoldierCharacter::GetReturnDelay() const
{
    if (bInitializedFromDataTable)
    {
        return CachedTableRow.AIConfig.ReturnDelay;
    }
    return 2.0f; // 默认值
}

float AXBSoldierCharacter::GetArrivalThreshold() const
{
    if (bInitializedFromDataTable)
    {
        return CachedTableRow.AIConfig.ArrivalThreshold;
    }
    return 50.0f; // 默认值
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

/**
 * @brief 被招募回调
 * @note 🔧 修改 - 招募后立即传送到编队位置
 */
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
        
        // 🔧 修改 - 使用招募过渡模式（移动组件驱动的平滑移动）
        FollowComponent->StartRecruitTransition();
    }
    
    // 更新阵营为将领阵营
    if (AXBCharacterBase* LeaderChar = Cast<AXBCharacterBase>(NewLeader))
    {
        Faction = LeaderChar->GetFaction();
    }
    
    // 设置为跟随状态
    SetSoldierState(EXBSoldierState::Following);
    
    // 延迟启动AI控制器
    GetWorldTimerManager().SetTimer(
        DelayedAIStartTimerHandle,
        this,
        &AXBSoldierCharacter::SpawnAndPossessAIController,
        0.3f,
        false
    );
    
    // 广播招募事件
    OnSoldierRecruited.Broadcast(this, NewLeader);
}

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
            // 🔧 修改 - 使用 Int 类型
            BBComp->SetValueAsInt(TEXT("SoldierState"), static_cast<int32>(CurrentState));
            BBComp->SetValueAsInt(TEXT("FormationSlot"), FormationSlotIndex);
            
            float AttackRange = bInitializedFromDataTable ? CachedTableRow.AttackRange : SoldierConfig.AttackRange;
            BBComp->SetValueAsFloat(TEXT("AttackRange"), AttackRange);
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

// ==================== 初始化实现 ====================

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

    FXBSoldierTableRow* Row = DataTable->FindRow<FXBSoldierTableRow>(RowName, TEXT("AXBSoldierCharacter::InitializeFromDataTable"));
    if (!Row)
    {
        UE_LOG(LogXBSoldier, Error, TEXT("士兵初始化失败: 找不到行 '%s'"), *RowName.ToString());
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

    UE_LOG(LogXBSoldier, Log, TEXT("士兵从数据表初始化: %s, 类型=%d, 血量=%.1f, 视野=%.0f"), 
        *RowName.ToString(), static_cast<int32>(SoldierType), CurrentHealth, GetVisionRange());
}

void AXBSoldierCharacter::InitializeSoldier(const FXBSoldierConfig& InConfig, EXBFaction InFaction)
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

    UE_LOG(LogXBSoldier, Log, TEXT("士兵初始化: Type=%d, Health=%.1f"), 
        static_cast<int32>(SoldierType), CurrentHealth);
}

void AXBSoldierCharacter::ApplyVisualConfig()
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

// ==================== AI系统实现（🔧 修改 - 使用球形检测） ====================

/**
 * @brief 寻找最近的敌人
 * @return 最近的敌人Actor
 * @note 🔧 修改 - 使用通用函数库的球形检测替代全量Actor搜索
 */
AActor* AXBSoldierCharacter::FindNearestEnemy() const
{
    if (!bIsRecruited)
    {
        return nullptr;
    }

    // 🔧 修改 - 从数据表读取视野范围
    float VisionRange = GetVisionRange();

    // 🔧 修改 - 使用通用函数库的球形检测
    return UXBBlueprintFunctionLibrary::FindNearestEnemy(
        this,                       // WorldContext
        GetActorLocation(),         // Origin
        VisionRange,                // Radius
        Faction,                    // SourceFaction
        true                        // bIgnoreDead
    );
}

/**
 * @brief 检查周边是否有敌人
 * @param Radius 检测半径
 * @return 是否有敌人
 * @note 🔧 修改 - 使用通用函数库的球形检测
 */
bool AXBSoldierCharacter::HasEnemiesInRadius(float Radius) const
{
    FXBDetectionResult Result;
    return UXBBlueprintFunctionLibrary::DetectEnemiesInRadius(
        this,               // WorldContext
        GetActorLocation(), // Origin
        Radius,             // Radius
        Faction,            // SourceFaction
        true,               // bIgnoreDead
        Result              // OutResult
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

    float AttackRange = bInitializedFromDataTable ? CachedTableRow.AttackRange : SoldierConfig.AttackRange;
    return GetDistanceToTarget(Target) <= AttackRange;
}

/**
 * @brief 检查是否应该脱离战斗
 * @return true表示应该返回队列
 * @note 🔧 修改 - 从数据表读取配置
 */
bool AXBSoldierCharacter::ShouldDisengage() const
{
    // 🔧 修改 - 从数据表读取脱离距离
    float DisengageDistance = GetDisengageDistance();

    // 条件1：距离将领过远
    if (FollowTarget.IsValid())
    {
        AActor* Leader = FollowTarget.Get();
        if (Leader && IsValid(Leader))
        {
            float DistToLeader = FVector::Dist(GetActorLocation(), Leader->GetActorLocation());
            if (DistToLeader > DisengageDistance)
            {
                UE_LOG(LogXBSoldier, Verbose, TEXT("士兵 %s 距离将领过远: %.0f > %.0f"), 
                    *GetName(), DistToLeader, DisengageDistance);
                return true;
            }
        }
    }

    // 🔧 修改 - 从数据表读取视野范围和返回延迟
    float VisionRange = GetVisionRange();
    float ReturnDelay = GetReturnDelay();

    // 条件2：周边无敌人且超过返回延迟
    if (!HasEnemiesInRadius(VisionRange))
    {
        float TimeSinceLastEnemy = GetWorld()->GetTimeSeconds() - LastEnemySeenTime;
        if (TimeSinceLastEnemy > ReturnDelay)
        {
            UE_LOG(LogXBSoldier, Verbose, TEXT("士兵 %s 周边无敌人，返回队列"), *GetName());
            return true;
        }
    }

    return false;
}


/**
 * @brief 返回队列
 * @note 🔧 修改 - 使用跟随组件传送回编队位置
 */
void AXBSoldierCharacter::ReturnToFormation()
{
    CurrentAttackTarget = nullptr;
    
    // 🔧 修改 - 直接传送回编队位置
    if (FollowComponent)
    {
        FollowComponent->ExitCombatMode();
    }
    
    // 停止AI移动
    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        AICtrl->StopMovement();
    }
    
    SetSoldierState(EXBSoldierState::Following);

    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 传送回队列"), *GetName());
}

bool AXBSoldierCharacter::ShouldRetreat() const
{
    if (SoldierType != EXBSoldierType::Archer)
    {
        return false;
    }

    if (!CurrentAttackTarget.IsValid())
    {
        return false;
    }

    if (!bInitializedFromDataTable)
    {
        return false;
    }

    float DistToTarget = GetDistanceToTarget(CurrentAttackTarget.Get());
    return DistToTarget < CachedTableRow.ArcherConfig.MinAttackDistance;
}

void AXBSoldierCharacter::RetreatFromTarget(AActor* Target)
{
    if (!Target || !IsValid(Target) || !bInitializedFromDataTable)
    {
        return;
    }

    FVector RetreatDirection = (GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
    float RetreatDistance = CachedTableRow.ArcherConfig.RetreatDistance;
    FVector RetreatTarget = GetActorLocation() + RetreatDirection * RetreatDistance;

    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        AICtrl->MoveToLocation(RetreatTarget, 10.0f, true, true, true, true);
    }

    UE_LOG(LogXBSoldier, Verbose, TEXT("弓手 %s 后撤，目标距离: %.0f"), 
        *GetName(), GetDistanceToTarget(Target));
}

FVector AXBSoldierCharacter::CalculateAvoidanceDirection(const FVector& DesiredDirection)
{
    // 🔧 修改 - 从数据表读取避障配置
    float AvoidanceRadius = bInitializedFromDataTable ? 
        CachedTableRow.AIConfig.AvoidanceRadius : 100.0f;
    float AvoidanceWeight = bInitializedFromDataTable ?
        CachedTableRow.AIConfig.AvoidanceWeight : 0.3f;

    if (AvoidanceRadius <= 0.0f)
    {
        return DesiredDirection;
    }

    FVector AvoidanceForce = FVector::ZeroVector;
    FVector MyLocation = GetActorLocation();

    // 🔧 修改 - 使用球形检测获取附近的友军
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

void AXBSoldierCharacter::MoveToTarget(AActor* Target)
{
    if (!Target || !IsValid(Target))
    {
        return;
    }

    AAIController* AICtrl = Cast<AAIController>(GetController());
    if (!AICtrl)
    {
        return;
    }

    float AcceptanceRadius = bInitializedFromDataTable ? CachedTableRow.AttackRange * 0.9f : SoldierConfig.AttackRange * 0.9f;

    AICtrl->MoveToActor(
        Target,
        AcceptanceRadius,
        true,
        true,
        true,
        nullptr,
        true
    );

    UE_LOG(LogXBSoldier, VeryVerbose, TEXT("士兵 %s 追踪目标 %s，距离: %.0f"), 
        *GetName(), *Target->GetName(), GetDistanceToTarget(Target));
}

void AXBSoldierCharacter::MoveToFormationPosition()
{
    // 🔧 修改 - 使用跟随组件的插值模式
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


/**
 * @brief 检查是否到达编队位置
 * @note 🔧 修改 - 使用跟随组件的判断
 */
bool AXBSoldierCharacter::IsAtFormationPosition() const
{
    if (FollowComponent)
    {
        return FollowComponent->IsAtFormationPosition();
    }
    
    // 降级：使用旧逻辑
    FVector TargetPos = GetFormationWorldPosition();
    float ArrivalThreshold = GetArrivalThreshold();
    return FVector::Dist2D(GetActorLocation(), TargetPos) <= ArrivalThreshold;
}

/**
 * @brief 安全检查是否到达编队位置
 */
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
            // 🔧 修改 - 使用 Int 类型
            BBComp->SetValueAsInt(TEXT("SoldierState"), static_cast<int32>(NewState));
        }
    }

    OnSoldierStateChanged.Broadcast(OldState, NewState);

    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 状态变化: %d -> %d"), 
        *GetName(), static_cast<int32>(OldState), static_cast<int32>(NewState));
}

// ==================== 战斗系统实现 ====================

/**
 * @brief 进入战斗
 * @note 🔧 修改 - 通知跟随组件切换到自由模式
 */
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

    // 🔧 修改 - 通知跟随组件进入战斗模式
    if (FollowComponent)
    {
        FollowComponent->EnterCombatMode();
    }

    SetSoldierState(EXBSoldierState::Combat);
    CurrentAttackTarget = FindNearestEnemy();

    UE_LOG(LogXBCombat, Log, TEXT("士兵 %s 进入战斗, 目标: %s"), 
        *GetName(), CurrentAttackTarget.IsValid() ? *CurrentAttackTarget->GetName() : TEXT("无"));
}

/**
 * @brief 退出战斗
 * @note 🔧 修改 - 通知跟随组件传送回编队位置
 */
void AXBSoldierCharacter::ExitCombat()
{
    if (CurrentState == EXBSoldierState::Dead)
    {
        return;
    }

    CurrentAttackTarget = nullptr;
    
    // 🔧 修改 - 通知跟随组件退出战斗模式
    if (FollowComponent)
    {
        FollowComponent->ExitCombatMode();
    }
    
    // 停止AI移动
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
    if (!Target || !IsValid(Target) || !CanAttack())
    {
        return false;
    }

    float AttackInterval = bInitializedFromDataTable ? CachedTableRow.AttackInterval : SoldierConfig.AttackInterval;
    AttackCooldownTimer = AttackInterval;

    PlayAttackMontage();

    float Damage = bInitializedFromDataTable ? CachedTableRow.BaseDamage : SoldierConfig.BaseDamage;

    if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(Target))
    {
        TargetSoldier->TakeSoldierDamage(Damage, this);
    }

    UE_LOG(LogXBCombat, Log, TEXT("士兵 %s 攻击 %s，伤害: %.1f"), 
        *GetName(), *Target->GetName(), Damage);

    return true;
}

bool AXBSoldierCharacter::PlayAttackMontage()
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

/**
 * @brief 更新战斗逻辑
 * @param DeltaTime 帧时间
 * @note 🔧 修改 - 使用数据表配置和球形检测
 */
void AXBSoldierCharacter::UpdateCombat(float DeltaTime)
{
    // ==================== 1. 脱离战斗检测 ====================
    if (ShouldDisengage())
    {
        UE_LOG(LogXBCombat, Verbose, TEXT("士兵 %s 脱离战斗条件满足，返回队列"), *GetName());
        ReturnToFormation();
        return;
    }

    // ==================== 2. 目标搜索/更新 ====================
    // 🔧 修改 - 从数据表读取寻敌间隔
    float SearchInterval = bInitializedFromDataTable ? 
        CachedTableRow.AIConfig.TargetSearchInterval : 0.5f;

    TargetSearchTimer += DeltaTime;
    if (TargetSearchTimer >= SearchInterval || !CurrentAttackTarget.IsValid())
    {
        TargetSearchTimer = 0.0f;
        AActor* NewTarget = FindNearestEnemy();

        if (NewTarget)
        {
            CurrentAttackTarget = NewTarget;
            LastEnemySeenTime = GetWorld()->GetTimeSeconds();
        }
    }

    // ==================== 3. 无目标处理 ====================
    if (!CurrentAttackTarget.IsValid())
    {
        // 🔧 修改 - 从数据表读取返回延迟
        float ReturnDelayTime = GetReturnDelay();
        float TimeSinceLastEnemy = GetWorld()->GetTimeSeconds() - LastEnemySeenTime;
        if (TimeSinceLastEnemy > ReturnDelayTime)
        {
            UE_LOG(LogXBCombat, Verbose, TEXT("士兵 %s 长时间无目标，返回队列"), *GetName());
            ReturnToFormation();
        }
        return;
    }

    AActor* Target = CurrentAttackTarget.Get();
    
    // ✨ 新增 - 空指针检查
    if (!Target || !IsValid(Target))
    {
        CurrentAttackTarget = nullptr;
        return;
    }

    float DistanceToEnemy = GetDistanceToTarget(Target);
    float AttackRange = bInitializedFromDataTable ? CachedTableRow.AttackRange : SoldierConfig.AttackRange;

    // ==================== 4. 弓手特殊逻辑 ====================
    if (SoldierType == EXBSoldierType::Archer && bInitializedFromDataTable)
    {
        if (CachedTableRow.ArcherConfig.bStationaryAttack && DistanceToEnemy <= AttackRange)
        {
            if (AAIController* AICtrl = Cast<AAIController>(GetController()))
            {
                AICtrl->StopMovement();
            }

            FaceTarget(Target, DeltaTime);

            if (CanAttack())
            {
                PerformAttack(Target);
            }

            UE_LOG(LogXBCombat, VeryVerbose, TEXT("弓手 %s 原地攻击 %s"), *GetName(), *Target->GetName());
            return;
        }

        if (ShouldRetreat())
        {
            RetreatFromTarget(Target);
            return;
        }
    }

    // ==================== 5. 距离判定与行动 ====================
    if (DistanceToEnemy > AttackRange)
    {
        MoveToTarget(Target);
    }
    else
    {
        if (AAIController* AICtrl = Cast<AAIController>(GetController()))
        {
            AICtrl->StopMovement();
        }

        FaceTarget(Target, DeltaTime);

        if (CanAttack())
        {
            PerformAttack(Target);
        }
    }
}

/**
 * @brief 更新跟随状态
 * @param DeltaTime 帧时间
 * @note 🔧 修改 - 跟随组件会自动处理，这里只做状态检查
 */
void AXBSoldierCharacter::UpdateFollowing(float DeltaTime)
{
    // 跟随组件会在自己的 Tick 中处理位置更新
    // 这里只检查是否需要进入战斗
    
    // 如果启用了自动战斗检测，可以在这里添加逻辑
    // 目前由行为树或外部触发战斗
}

void AXBSoldierCharacter::UpdateReturning(float DeltaTime)
{
    // 跟随组件会处理位置更新
    // 这里检查是否到达编队位置
    
    if (FollowComponent && FollowComponent->IsAtFormationPosition())
    {
        SetSoldierState(EXBSoldierState::Following);
    }
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
        float RotationSpeed = bInitializedFromDataTable ? CachedTableRow.RotationSpeed : 360.0f;
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotationSpeed / 90.0f);
        SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
    }
}

// ==================== 逃跑系统实现 ====================
/**
 * @brief 设置逃跑状态
 * @param bEscaping 是否逃跑
 * @note 🔧 修改 - 逃跑时传送回编队位置
 */
void AXBSoldierCharacter::SetEscaping(bool bEscaping)
{
    bIsEscaping = bEscaping;

    if (bEscaping)
    {
        // 🔧 修改 - 逃跑时退出战斗状态
        if (FollowComponent)
        {
            // 设置战斗状态为false
            FollowComponent->SetCombatState(false);
            
            if (CurrentState == EXBSoldierState::Combat)
            {
                CurrentAttackTarget = nullptr;
                SetSoldierState(EXBSoldierState::Following);
            }
            
            // 使用插值模式回到编队位置
            FollowComponent->StartInterpolateToFormation();
        }
        
        if (AAIController* AICtrl = Cast<AAIController>(GetController()))
        {
            AICtrl->StopMovement();
        }
    }

    // 更新移动速度
    float BaseSpeed = bInitializedFromDataTable ? CachedTableRow.MoveSpeed : SoldierConfig.MoveSpeed;
    float SprintMultiplier = bInitializedFromDataTable ? CachedTableRow.SprintSpeedMultiplier : 2.0f;

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

    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 死亡"), *GetName());
}
