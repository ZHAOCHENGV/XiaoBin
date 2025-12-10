/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Soldier/XBSoldierActor.cpp

/**
 * @file XBSoldierActor.cpp
 * @brief 士兵Actor实现 - 数据驱动 + 行为树AI
 * 
 * @note 🔧 修改记录:
 *       1. 重构为Character基类支持AI移动
 *       2. 实现数据表驱动配置
 *       3. 完善战斗系统（寻敌/攻击/撤退）
 *       4. 弓手特殊逻辑实现
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

AXBSoldierActor::AXBSoldierActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // 配置胶囊体
    GetCapsuleComponent()->InitCapsuleSize(34.0f, 88.0f);
    GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));

    // 配置网格体
    GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));

    // 创建跟随组件
    FollowComponent = CreateDefaultSubobject<UXBSoldierFollowComponent>(TEXT("FollowComponent"));

    // 配置移动组件
    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->bOrientRotationToMovement = true;
        MovementComp->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
        MovementComp->MaxWalkSpeed = 400.0f;
        MovementComp->BrakingDecelerationWalking = 2000.0f;
    }

    // 使用士兵专用AI控制器
    // 说明: 自动使用专门设计的士兵AI控制器，支持行为树
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AXBSoldierAIController::StaticClass();
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

    // 初始化AI
    InitializeAI();
}

void AXBSoldierActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 更新攻击冷却
    if (AttackCooldownTimer > 0.0f)
    {
        AttackCooldownTimer -= DeltaTime;
    }

    // 根据状态更新（如果没有使用行为树，使用简单状态机）
    if (!BehaviorTreeAsset)
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

// ==================== 初始化实现 ====================

/**
 * @brief 从数据表初始化士兵
 */
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

    // 缓存数据
    CachedTableRow = *Row;
    bInitializedFromDataTable = true;

    // 设置基本属性
    SoldierType = Row->SoldierType;
    Faction = InFaction;
    CurrentHealth = Row->MaxHealth;

    // 配置移动速度
    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->MaxWalkSpeed = Row->MoveSpeed;
        MovementComp->RotationRate = FRotator(0.0f, Row->RotationSpeed, 0.0f);
    }

    // 配置跟随组件
    if (FollowComponent)
    {
        FollowComponent->SetFollowSpeed(Row->MoveSpeed);
        FollowComponent->SetFollowInterpSpeed(Row->FollowInterpSpeed);
    }

    // 加载行为树
    if (!Row->AIConfig.BehaviorTree.IsNull())
    {
        BehaviorTreeAsset = Row->AIConfig.BehaviorTree.LoadSynchronous();
    }

    // 应用视觉配置
    ApplyVisualConfig();

    // 同步到旧配置（兼容）
    SoldierConfig.SoldierType = Row->SoldierType;
    SoldierConfig.MaxHealth = Row->MaxHealth;
    SoldierConfig.BaseDamage = Row->BaseDamage;
    SoldierConfig.AttackRange = Row->AttackRange;
    SoldierConfig.AttackInterval = Row->AttackInterval;
    SoldierConfig.MoveSpeed = Row->MoveSpeed;
    SoldierConfig.FollowInterpSpeed = Row->FollowInterpSpeed;
    SoldierConfig.HealthBonusToLeader = Row->HealthBonusToLeader;
    SoldierConfig.DamageBonusToLeader = Row->DamageBonusToLeader;

    UE_LOG(LogTemp, Log, TEXT("士兵从数据表初始化: %s, 类型=%d, 血量=%.1f, 伤害=%.1f"), 
        *RowName.ToString(), static_cast<int32>(SoldierType), CurrentHealth, Row->BaseDamage);
}

/**
 * @brief 初始化士兵（旧接口）
 */
void AXBSoldierActor::InitializeSoldier(const FXBSoldierConfig& InConfig, EXBFaction InFaction)
{
    SoldierConfig = InConfig;
    SoldierType = InConfig.SoldierType;
    Faction = InFaction;
    CurrentHealth = InConfig.MaxHealth;

    // 配置移动速度
    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->MaxWalkSpeed = InConfig.MoveSpeed;
    }

    // 配置跟随组件
    if (FollowComponent)
    {
        FollowComponent->SetFollowSpeed(InConfig.MoveSpeed);
        FollowComponent->SetFollowInterpSpeed(InConfig.FollowInterpSpeed);
    }

    // 设置网格
    if (InConfig.SoldierMesh)
    {
        GetMesh()->SetSkeletalMesh(InConfig.SoldierMesh);
    }

    UE_LOG(LogTemp, Log, TEXT("士兵初始化: Type=%d, Health=%.1f"), 
        static_cast<int32>(SoldierType), CurrentHealth);
}

/**
 * @brief 应用视觉配置
 */
void AXBSoldierActor::ApplyVisualConfig()
{
    if (!bInitializedFromDataTable)
    {
        return;
    }

    // 加载并设置骨骼网格
    if (!CachedTableRow.VisualConfig.SkeletalMesh.IsNull())
    {
        USkeletalMesh* LoadedMesh = CachedTableRow.VisualConfig.SkeletalMesh.LoadSynchronous();
        if (LoadedMesh)
        {
            GetMesh()->SetSkeletalMesh(LoadedMesh);
        }
    }

    // 设置动画蓝图
    if (CachedTableRow.VisualConfig.AnimClass)
    {
        GetMesh()->SetAnimInstanceClass(CachedTableRow.VisualConfig.AnimClass);
    }

    // 设置缩放
    SetActorScale3D(FVector(CachedTableRow.VisualConfig.MeshScale));
}

/**
 * @brief 初始化AI
 */
void AXBSoldierActor::InitializeAI()
{
    if (!BehaviorTreeAsset)
    {
        return;
    }

    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        // 运行行为树
        AICtrl->RunBehaviorTree(BehaviorTreeAsset);

        // 设置黑板值
        if (UBlackboardComponent* BBComp = AICtrl->GetBlackboardComponent())
        {
            BBComp->SetValueAsObject(TEXT("Leader"), FollowTarget.Get());
            BBComp->SetValueAsEnum(TEXT("SoldierState"), static_cast<uint8>(CurrentState));
            BBComp->SetValueAsFloat(TEXT("AttackRange"), 
                bInitializedFromDataTable ? CachedTableRow.AttackRange : SoldierConfig.AttackRange);
        }

        UE_LOG(LogTemp, Log, TEXT("士兵AI初始化完成: %s"), *GetName());
    }
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

    // 更新黑板
    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BBComp = AICtrl->GetBlackboardComponent())
        {
            BBComp->SetValueAsObject(TEXT("Leader"), NewLeader);
            BBComp->SetValueAsInt(TEXT("FormationSlot"), SlotIndex);
        }
    }

    // 设置为跟随状态
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

    // 更新黑板
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

    // 更新黑板
    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BBComp = AICtrl->GetBlackboardComponent())
        {
            BBComp->SetValueAsEnum(TEXT("SoldierState"), static_cast<uint8>(NewState));
        }
    }

    // 广播状态变化
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

    SetSoldierState(EXBSoldierState::Combat);
    
    // 立即寻找目标
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

/**
 * @brief 执行攻击
 */
bool AXBSoldierActor::PerformAttack(AActor* Target)
{
    if (!Target || !CanAttack())
    {
        return false;
    }

    // 设置攻击冷却
    float AttackInterval = bInitializedFromDataTable ? CachedTableRow.AttackInterval : SoldierConfig.AttackInterval;
    AttackCooldownTimer = AttackInterval;

    // 播放攻击动画
    PlayAttackMontage();

    // 计算伤害
    float Damage = bInitializedFromDataTable ? CachedTableRow.BaseDamage : SoldierConfig.BaseDamage;

    // 对目标造成伤害
    if (AXBSoldierActor* TargetSoldier = Cast<AXBSoldierActor>(Target))
    {
        TargetSoldier->TakeSoldierDamage(Damage, this);
    }
    else if (AXBCharacterBase* TargetCharacter = Cast<AXBCharacterBase>(Target))
    {
        // 通过GAS造成伤害（需要GE）
        // 简化实现：直接log
        UE_LOG(LogTemp, Log, TEXT("士兵 %s 攻击将领 %s，伤害: %.1f"), 
            *GetName(), *Target->GetName(), Damage);
    }

    UE_LOG(LogTemp, Log, TEXT("士兵 %s 攻击 %s，伤害: %.1f"), 
        *GetName(), *Target->GetName(), Damage);

    return true;
}

/**
 * @brief 播放攻击蒙太奇
 */
bool AXBSoldierActor::PlayAttackMontage()
{
    UAnimMontage* AttackMontage = nullptr;

    if (bInitializedFromDataTable && !CachedTableRow.BasicAttack.AbilityMontage.IsNull())
    {
        AttackMontage = CachedTableRow.BasicAttack.AbilityMontage.LoadSynchronous();
    }
    else if (!SoldierConfig.SoldierTags.IsEmpty())
    {
        // 旧配置方式
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

/**
 * @brief 寻找最近的敌人
 */
AActor* AXBSoldierActor::FindNearestEnemy() const
{
    float DetectionRange = bInitializedFromDataTable ? 
        CachedTableRow.AIConfig.DetectionRange : 800.0f;

    TArray<AActor*> PotentialTargets;
    
    // 获取所有角色
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AXBCharacterBase::StaticClass(), PotentialTargets);
    
    // 获取所有士兵
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

        // 检查阵营
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

    // 尝试从将领的编队组件获取位置
    if (AXBCharacterBase* Leader = Cast<AXBCharacterBase>(FollowTarget.Get()))
    {
        // 简化实现：使用跟随组件计算
        if (FollowComponent)
        {
            return FollowComponent->GetTargetPosition();
        }
    }

    return FollowTarget->GetActorLocation();
}

bool AXBSoldierActor::IsAtFormationPosition() const
{
    FVector TargetPos = GetFormationWorldPosition();
    float ArrivalThreshold = 50.0f;
    return FVector::Dist2D(GetActorLocation(), TargetPos) <= ArrivalThreshold;
}

/**
 * @brief 获取编队世界位置（安全版本）
 * @note 🔧 新增 - 在组件未初始化时返回ZeroVector而非崩溃
 */
FVector AXBSoldierActor::GetFormationWorldPositionSafe() const
{
    // 安全检查: 确保跟随目标有效
    if (!FollowTarget.IsValid())
    {
        return FVector::ZeroVector;
    }
    
    AActor* Target = FollowTarget.Get();
    if (!Target || !IsValid(Target))
    {
        return FVector::ZeroVector;
    }
    
    // 安全检查: 确保跟随组件有效
    if (!FollowComponent)
    {
        return Target->GetActorLocation();
    }
    
    // 尝试从将领的编队组件获取位置
    if (AXBCharacterBase* Leader = Cast<AXBCharacterBase>(Target))
    {
        // 使用跟随组件计算（内部有安全检查）
        FVector TargetPos = FollowComponent->GetTargetPosition();
        
        // 额外检查: 确保返回的位置有效
        if (!TargetPos.IsZero() && TargetPos.ContainsNaN() == false)
        {
            return TargetPos;
        }
    }
    
    // 回退: 返回将领位置
    return Target->GetActorLocation();
}

/**
 * @brief 是否到达编队位置（安全版本）
 * @note 🔧 新增 - 在组件未初始化时返回true而非崩溃
 */
bool AXBSoldierActor::IsAtFormationPositionSafe() const
{
    // 安全检查: 没有跟随目标时认为已在位置
    if (!FollowTarget.IsValid())
    {
        return true;
    }
    
    AActor* Target = FollowTarget.Get();
    if (!Target || !IsValid(Target))
    {
        return true;
    }
    
    // 安全检查: 没有有效槽位时认为已在位置
    if (FormationSlotIndex == INDEX_NONE)
    {
        return true;
    }
    
    // 获取安全的编队位置
    FVector TargetPos = GetFormationWorldPositionSafe();
    
    // 如果获取的位置为零向量，说明组件未就绪，返回true避免不必要的移动
    if (TargetPos.IsZero())
    {
        return true;
    }
    
    float ArrivalThreshold = 50.0f;
    return FVector::Dist2D(GetActorLocation(), TargetPos) <= ArrivalThreshold;
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
    // 更新寻敌计时器
    float SearchInterval = bInitializedFromDataTable ? 
        CachedTableRow.AIConfig.TargetSearchInterval : 0.5f;
    
    TargetSearchTimer += DeltaTime;
    if (TargetSearchTimer >= SearchInterval || !CurrentAttackTarget.IsValid())
    {
        TargetSearchTimer = 0.0f;
        CurrentAttackTarget = FindNearestEnemy();
    }

    // 检查是否应该脱离战斗
    if (ShouldDisengage())
    {
        ExitCombat();
        return;
    }

    // 没有目标时返回
    if (!CurrentAttackTarget.IsValid())
    {
        // 周围没有敌人，返回队列
        ExitCombat();
        return;
    }

    AActor* Target = CurrentAttackTarget.Get();
    float DistanceToEnemy = GetDistanceToTarget(Target);
    float AttackRange = bInitializedFromDataTable ? CachedTableRow.AttackRange : SoldierConfig.AttackRange;

    // ✨ 新增 - 弓手特殊逻辑
    if (SoldierType == EXBSoldierType::Archer && bInitializedFromDataTable)
    {
        if (CachedTableRow.ArcherConfig.bStationaryAttack && DistanceToEnemy <= AttackRange)
        {
            // 弓手在攻击范围内，原地攻击不追踪
            FaceTarget(Target, DeltaTime);
            
            if (CanAttack())
            {
                PerformAttack(Target);
            }
            return;
        }
        
        // 弓手过近时后撤
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

    // 普通士兵逻辑：追踪到攻击范围内
    if (DistanceToEnemy > AttackRange)
    {
        MoveToTarget(Target);
    }
    else
    {
        // 在攻击范围内
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

    // 检查是否到达编队位置
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
    SetSoldierState(EXBSoldierState::Dead);

    // 广播死亡事件
    OnSoldierDied.Broadcast(this);

    // 通知将领士兵死亡
    if (AXBCharacterBase* LeaderCharacter = GetLeaderCharacter())
    {
        LeaderCharacter->OnSoldierDied(this);
    }

    // 禁用碰撞
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 停止AI
    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        AICtrl->StopMovement();
    }

    // 播放死亡动画
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

    // 延迟销毁
    SetLifeSpan(2.0f);

    UE_LOG(LogTemp, Log, TEXT("士兵 %s 死亡"), *GetName());
}
