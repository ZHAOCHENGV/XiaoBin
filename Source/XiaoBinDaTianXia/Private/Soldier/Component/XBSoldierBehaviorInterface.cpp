/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Soldier/Component/XBSoldierBehaviorInterface.cpp

/**
 * @file XBSoldierBehaviorInterface.cpp
 * @brief 士兵行为接口组件实现
 * 
 * @note ✨ 新增文件
 */

#include "Soldier/Component/XBSoldierBehaviorInterface.h"
#include "Utils/XBLogCategories.h"
#include "AI/XBSoldierPerceptionSubsystem.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Soldier/Component/XBSoldierFollowComponent.h"
#include "Data/XBSoldierDataAccessor.h"
#include "Character/XBCharacterBase.h"
#include "Utils/XBBlueprintFunctionLibrary.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"  // ✨ 新增 - 包含枚举定义
#include "NavigationSystem.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"

UXBSoldierBehaviorInterface::UXBSoldierBehaviorInterface()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UXBSoldierBehaviorInterface::BeginPlay()
{
    Super::BeginPlay();

    // 缓存士兵引用
    CachedSoldier = Cast<AXBSoldierCharacter>(GetOwner());

    // 缓存感知子系统
    if (UWorld* World = GetWorld())
    {
        CachedPerceptionSubsystem = World->GetSubsystem<UXBSoldierPerceptionSubsystem>();

        // 注册到感知子系统
        if (CachedPerceptionSubsystem.IsValid() && CachedSoldier.IsValid())
        {
            CachedPerceptionSubsystem->RegisterActor(
                CachedSoldier.Get(),
                CachedSoldier->GetFaction()
            );
        }
    }

    UE_LOG(LogXBAI, Log, TEXT("士兵行为接口组件初始化: %s"), 
        CachedSoldier.IsValid() ? *CachedSoldier->GetName() : TEXT("无效"));
}

void UXBSoldierBehaviorInterface::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 更新攻击冷却
    UpdateAttackCooldown(DeltaTime);
}

// ==================== 内部辅助方法 ====================

AXBSoldierCharacter* UXBSoldierBehaviorInterface::GetOwnerSoldier() const
{
    return CachedSoldier.Get();
}

UXBSoldierPerceptionSubsystem* UXBSoldierBehaviorInterface::GetPerceptionSubsystem() const
{
    return CachedPerceptionSubsystem.Get();
}

void UXBSoldierBehaviorInterface::UpdateAttackCooldown(float DeltaTime)
{
    if (AttackCooldownTimer > 0.0f)
    {
        AttackCooldownTimer -= DeltaTime;
        if (AttackCooldownTimer < 0.0f)
        {
            AttackCooldownTimer = 0.0f;
        }
    }
}

// ==================== 感知行为实现 ====================

/**
 * @brief 搜索最近的敌人
 * @note ✨ 核心优化 - 通过感知子系统执行，支持缓存和批量处理
 */
bool UXBSoldierBehaviorInterface::SearchForEnemy(AActor*& OutEnemy)
{
    OutEnemy = nullptr;

    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    UXBSoldierPerceptionSubsystem* Perception = GetPerceptionSubsystem();

    if (!Soldier || !Perception)
    {
        return false;
    }

    // 🔧 修改 - 目标选择优先级：先敌方士兵，再敌方主将
    EXBFaction PreferredFaction = EXBFaction::Neutral;
    bool bHasPreferredFaction = false;
    if (AXBCharacterBase* Leader = Soldier->GetLeaderCharacter())
    {
        if (AXBCharacterBase* EnemyLeader = Leader->GetLastAttackedEnemyLeader())
        {
            if (!EnemyLeader->IsDead())
            {
                PreferredFaction = EnemyLeader->GetFaction();
                bHasPreferredFaction = true;
            }
        }
    }

    auto SelectPriorityTarget = [Soldier, bHasPreferredFaction, PreferredFaction](const FXBPerceptionResult& Result) -> AActor*
    {
        if (!Soldier)
        {
            return nullptr;
        }

        AActor* NearestPreferredSoldier = nullptr;
        float NearestPreferredSoldierDistSq = MAX_FLT;

        AActor* NearestPreferredLeader = nullptr;
        float NearestPreferredLeaderDistSq = MAX_FLT;

        AActor* NearestSoldier = nullptr;
        float NearestSoldierDistSq = MAX_FLT;

        AActor* NearestLeader = nullptr;
        float NearestLeaderDistSq = MAX_FLT;

        const FVector SoldierLocation = Soldier->GetActorLocation();

        for (AActor* Candidate : Result.DetectedEnemies)
        {
            if (!Candidate || !IsValid(Candidate))
            {
                continue;
            }

            EXBFaction CandidateFaction = EXBFaction::Neutral;

            if (AXBSoldierCharacter* EnemySoldier = Cast<AXBSoldierCharacter>(Candidate))
            {
                if (EnemySoldier->GetSoldierState() == EXBSoldierState::Dead)
                {
                    continue;
                }

                CandidateFaction = EnemySoldier->GetFaction();

                const float DistSq = FVector::DistSquared(SoldierLocation, EnemySoldier->GetActorLocation());
                const bool bPreferred = bHasPreferredFaction && CandidateFaction == PreferredFaction;
                if (bPreferred)
                {
                    if (DistSq < NearestPreferredSoldierDistSq)
                    {
                        NearestPreferredSoldierDistSq = DistSq;
                        NearestPreferredSoldier = EnemySoldier;
                    }
                }
                else
                {
                    if (DistSq < NearestSoldierDistSq)
                    {
                        NearestSoldierDistSq = DistSq;
                        NearestSoldier = EnemySoldier;
                    }
                }
                continue;
            }

            if (AXBCharacterBase* EnemyLeader = Cast<AXBCharacterBase>(Candidate))
            {
                if (EnemyLeader->IsDead())
                {
                    continue;
                }

                CandidateFaction = EnemyLeader->GetFaction();

                const float DistSq = FVector::DistSquared(SoldierLocation, EnemyLeader->GetActorLocation());
                const bool bPreferred = bHasPreferredFaction && CandidateFaction == PreferredFaction;
                if (bPreferred)
                {
                    if (DistSq < NearestPreferredLeaderDistSq)
                    {
                        NearestPreferredLeaderDistSq = DistSq;
                        NearestPreferredLeader = EnemyLeader;
                    }
                }
                else
                {
                    if (DistSq < NearestLeaderDistSq)
                    {
                        NearestLeaderDistSq = DistSq;
                        NearestLeader = EnemyLeader;
                    }
                }
            }
        }

        if (NearestPreferredSoldier)
        {
            return NearestPreferredSoldier;
        }
        if (NearestPreferredLeader)
        {
            return NearestPreferredLeader;
        }
        return NearestSoldier ? NearestSoldier : NearestLeader;
    };

    // 检查本地缓存是否有效
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - PerceptionCacheTime < PerceptionCacheValidity)
    {
        AActor* CachedTarget = SelectPriorityTarget(CachedPerceptionResult);
        if (CachedTarget)
        {
            OutEnemy = CachedTarget;
            RecordEnemySeen();
            return true;
        }
        return false;
    }

    // ✨ 新增 - 根据战斗状态决定查询优先级
    EXBQueryPriority Priority = EXBQueryPriority::Normal;
    if (Soldier->GetSoldierState() == EXBSoldierState::Combat)
    {
        Priority = EXBQueryPriority::High;
    }
    else if (Soldier->GetSoldierState() == EXBSoldierState::Idle)
    {
        Priority = EXBQueryPriority::Low;
    }

    float VisionRange = Soldier->GetVisionRange();
    FVector Location = Soldier->GetActorLocation();
    EXBFaction Faction = Soldier->GetFaction();

    // 🔧 修改 - 使用带优先级的查询接口
    bool bFound = Perception->QueryNearestEnemyWithPriority(
        Soldier,
        Location,
        VisionRange,
        Faction,
        Priority,
        CachedPerceptionResult
    );

    PerceptionCacheTime = CurrentTime;

    if (bFound)
    {
        AActor* PriorityTarget = SelectPriorityTarget(CachedPerceptionResult);
        if (PriorityTarget)
        {
            OutEnemy = PriorityTarget;
            RecordEnemySeen();
            return true;
        }
    }

    return false;
}

bool UXBSoldierBehaviorInterface::HasEnemyInSight() const
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - PerceptionCacheTime < PerceptionCacheValidity)
    {
        // 🔧 修改 - 清理后检查数量
        // 注意：这里需要 const_cast 或者将方法改为非 const
        return CachedPerceptionResult.DetectedEnemies.Num() > 0;
    }

    // 缓存过期，执行新查询
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    UXBSoldierPerceptionSubsystem* Perception = GetPerceptionSubsystem();

    if (!Soldier || !Perception)
    {
        return false;
    }

    FXBPerceptionResult Result;
    Perception->QueryNearestEnemy(
        Soldier,
        Soldier->GetActorLocation(),
        Soldier->GetVisionRange(),
        Soldier->GetFaction(),
        Result
    );

    return Result.DetectedEnemies.Num() > 0;
}

bool UXBSoldierBehaviorInterface::IsTargetValid(AActor* Target) const
{
    if (!Target || !IsValid(Target))
    {
        return false;
    }

    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return false;
    }

    // 检查是否是士兵且已死亡
    if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(Target))
    {
        if (TargetSoldier->GetSoldierState() == EXBSoldierState::Dead)
        {
            return false;
        }
        return UXBBlueprintFunctionLibrary::AreFactionsHostile(Soldier->GetFaction(), TargetSoldier->GetFaction());
    }

    // 检查是否是将领且已死亡
    if (AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Target))
    {
        if (TargetLeader->IsDead())
        {
            return false;
        }
        return UXBBlueprintFunctionLibrary::AreFactionsHostile(Soldier->GetFaction(), TargetLeader->GetFaction());
    }

    return true;
}

// ==================== 战斗行为实现 ====================

/**
 * @brief 执行攻击
 * @note 包含完整的攻击逻辑：冷却检查、距离检查、动画播放、伤害应用
 */
EXBBehaviorResult UXBSoldierBehaviorInterface::ExecuteAttack(AActor* Target)
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return EXBBehaviorResult::Failed;
    }

    // 检查是否可以攻击
    if (!CanAttack(Target))
    {
        // 如果只是冷却中，返回进行中
        if (AttackCooldownTimer > 0.0f && IsInAttackRange(Target))
        {
            // 🔧 修改 - 冷却中也保持朝向目标并停止移动，避免在攻击范围内乱跑
            FaceTarget(Target, GetWorld()->GetDeltaSeconds());
            if (AAIController* AIController = Cast<AAIController>(Soldier->GetController()))
            {
                AIController->StopMovement();
            }
            return EXBBehaviorResult::InProgress;
        }
        return EXBBehaviorResult::Failed;
    }

    // 🔧 修改 - 进入攻击时停止移动并面向目标，保证攻击稳定触发
    if (AAIController* AIController = Cast<AAIController>(Soldier->GetController()))
    {
        AIController->StopMovement();
    }
    FaceTarget(Target, GetWorld()->GetDeltaSeconds());

    // 播放攻击蒙太奇（必须成功）
    if (!PlayAttackMontage())
    {
        UE_LOG(LogXBCombat, Warning, TEXT("士兵 %s 攻击失败：未能播放攻击蒙太奇"), *Soldier->GetName());
        return EXBBehaviorResult::Failed;
    }

    // 设置攻击冷却
    float AttackInterval = Soldier->GetAttackInterval();
    AttackCooldownTimer = AttackInterval;

    // 应用伤害
    float Damage = Soldier->GetBaseDamage();
    ApplyDamageToTarget(Target, Damage);

    // 🔧 修改 - 记录看见敌人，避免战斗状态被过早清理
    RecordEnemySeen();

    UE_LOG(LogXBCombat, Verbose, TEXT("士兵 %s 攻击 %s，伤害: %.1f"),
        *Soldier->GetName(), *Target->GetName(), Damage);

    // 广播行为完成
    OnBehaviorCompleted.Broadcast(FName("Attack"), EXBBehaviorResult::Success);

    return EXBBehaviorResult::Success;
}

bool UXBSoldierBehaviorInterface::CanAttack(AActor* Target) const
{
    if (!Target || !IsValid(Target))
    {
        return false;
    }

    // 冷却检查
    if (AttackCooldownTimer > 0.0f)
    {
        return false;
    }

    // 目标有效性检查
    if (!IsTargetValid(Target))
    {
        return false;
    }

    // 距离检查
    if (!IsInAttackRange(Target))
    {
        return false;
    }

    return true;
}

bool UXBSoldierBehaviorInterface::IsInAttackRange(AActor* Target) const
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier || !Target)
    {
        return false;
    }

    const float AttackRange = Soldier->GetAttackRange();
    const float SelfRadius = Soldier->GetSimpleCollisionRadius();
    const float TargetRadius = Target->GetSimpleCollisionRadius();
    const float Distance = FVector::Dist2D(Soldier->GetActorLocation(), Target->GetActorLocation());

    return Distance <= (AttackRange + SelfRadius + TargetRadius);
}

bool UXBSoldierBehaviorInterface::PlayAttackMontage()
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return false;
    }

    UXBSoldierDataAccessor* DataAccessor = Soldier->GetDataAccessor();
    if (!DataAccessor || !DataAccessor->IsInitialized())
    {
        return false;
    }

    UAnimMontage* AttackMontage = DataAccessor->GetBasicAttackMontage();
    if (!AttackMontage)
    {
        return false;
    }

    USkeletalMeshComponent* Mesh = Soldier->GetMesh();
    if (!Mesh)
    {
        return false;
    }

    UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
    if (!AnimInstance)
    {
        return false;
    }

    return AnimInstance->Montage_Play(AttackMontage) > 0.0f;
}

void UXBSoldierBehaviorInterface::ApplyDamageToTarget(AActor* Target, float Damage)
{
    if (!Target)
    {
        return;
    }

    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return;
    }

    // 对士兵应用伤害
    if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(Target))
    {
        if (!UXBBlueprintFunctionLibrary::AreFactionsHostile(Soldier->GetFaction(), TargetSoldier->GetFaction()))
        {
            return;
        }
        TargetSoldier->TakeSoldierDamage(Damage, GetOwner());
    }
    // 对将领应用伤害（通过 GAS）
    else if (AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Target))
    {
        if (!UXBBlueprintFunctionLibrary::AreFactionsHostile(Soldier->GetFaction(), TargetLeader->GetFaction()))
        {
            return;
        }
        // TODO: 通过 GAS 应用伤害
        UE_LOG(LogXBCombat, Verbose, TEXT("士兵攻击将领，伤害待 GAS 处理"));
    }
}

// ==================== 移动行为实现 ====================

EXBBehaviorResult UXBSoldierBehaviorInterface::MoveToLocation(const FVector& TargetLocation, float AcceptanceRadius)
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return EXBBehaviorResult::Failed;
    }

    AAIController* AIController = Cast<AAIController>(Soldier->GetController());
    if (!AIController)
    {
        return EXBBehaviorResult::Failed;
    }

    // 检查是否已到达
    float Distance = FVector::Dist(Soldier->GetActorLocation(), TargetLocation);
    if (Distance <= AcceptanceRadius)
    {
        return EXBBehaviorResult::Success;
    }

    // 发起移动请求
    EPathFollowingRequestResult::Type Result = AIController->MoveToLocation(
        TargetLocation,
        AcceptanceRadius,
        true,
        true,
        true,
        true
    );

    // 🔧 修改 - 使用 if-else 替代 switch（避免枚举不完整问题）
    if (Result == EPathFollowingRequestResult::Type::RequestSuccessful)
    {
        return EXBBehaviorResult::InProgress;
    }
    else if (Result == EPathFollowingRequestResult::Type::AlreadyAtGoal)
    {
        return EXBBehaviorResult::Success;
    }
    else
    {
        return EXBBehaviorResult::Failed;
    }
}

EXBBehaviorResult UXBSoldierBehaviorInterface::MoveToActor(AActor* Target, float AcceptanceRadius)
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier || !Target)
    {
        return EXBBehaviorResult::Failed;
    }

    if (AcceptanceRadius < 0.0f)
    {
        AcceptanceRadius = Soldier->GetAttackRange() * 0.9f;
    }

    AAIController* AIController = Cast<AAIController>(Soldier->GetController());
    if (!AIController)
    {
        return EXBBehaviorResult::Failed;
    }

    float Distance = FVector::Dist(Soldier->GetActorLocation(), Target->GetActorLocation());
    if (Distance <= AcceptanceRadius)
    {
        return EXBBehaviorResult::Success;
    }

    EPathFollowingRequestResult::Type Result = AIController->MoveToActor(
        Target,
        AcceptanceRadius,
        true,
        true
    );

    // 🔧 修改 - 使用 if-else 替代 switch
    if (Result == EPathFollowingRequestResult::Type::RequestSuccessful)
    {
        return EXBBehaviorResult::InProgress;
    }
    else if (Result == EPathFollowingRequestResult::Type::AlreadyAtGoal)
    {
        return EXBBehaviorResult::Success;
    }
    else
    {
        return EXBBehaviorResult::Failed;
    }
}

EXBBehaviorResult UXBSoldierBehaviorInterface::ReturnToFormation()
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return EXBBehaviorResult::Failed;
    }

    // 检查是否已在编队位置
    if (IsAtFormationPosition())
    {
        return EXBBehaviorResult::Success;
    }

    // 获取编队位置
    FVector FormationPosition = Soldier->GetFormationWorldPositionSafe();
    if (FormationPosition.IsZero())
    {
        return EXBBehaviorResult::Failed;
    }

    // 移动到编队位置
    return MoveToLocation(FormationPosition, Soldier->GetArrivalThreshold());
}

void UXBSoldierBehaviorInterface::StopMovement()
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return;
    }

    if (AAIController* AIController = Cast<AAIController>(Soldier->GetController()))
    {
        AIController->StopMovement();
    }
}

bool UXBSoldierBehaviorInterface::IsAtFormationPosition() const
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return true;
    }

    FVector FormationPosition = Soldier->GetFormationWorldPositionSafe();
    if (FormationPosition.IsZero())
    {
        return true;
    }

    float Distance = FVector::Dist2D(Soldier->GetActorLocation(), FormationPosition);
    return Distance <= Soldier->GetArrivalThreshold();
}

// ==================== 决策辅助实现 ====================

bool UXBSoldierBehaviorInterface::ShouldDisengage() const
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return false;
    }

    // 🔧 修改 - 有有效目标时不脱离战斗，避免攻击中被强制切回跟随
    if (AActor* CurrentTarget = Soldier->CurrentAttackTarget.Get())
    {
        if (IsTargetValid(CurrentTarget))
        {
            return false;
        }
    }

    // 条件1：距离将领过远
    float DisengageDistance = Soldier->GetDisengageDistance();
    float DistToLeader = GetDistanceToLeader();
    if (DistToLeader >= DisengageDistance)
    {
        UE_LOG(LogXBAI, Verbose, TEXT("士兵 %s 距离将领过远: %.0f >= %.0f"),
            *Soldier->GetName(), DistToLeader, DisengageDistance);
        return true;
    }

    // 条件2：长时间无敌人
    float ReturnDelay = Soldier->GetReturnDelay();
    float CurrentTime = GetWorld()->GetTimeSeconds();
    float TimeSinceLastEnemy = CurrentTime - LastEnemySeenTime;

    if (!HasEnemyInSight() && TimeSinceLastEnemy > ReturnDelay)
    {
        UE_LOG(LogXBAI, Verbose, TEXT("士兵 %s 长时间无敌人: %.1f > %.1f"),
            *Soldier->GetName(), TimeSinceLastEnemy, ReturnDelay);
        return true;
    }

    return false;
}

float UXBSoldierBehaviorInterface::GetDistanceToLeader() const
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return MAX_FLT;
    }

    AActor* Leader = Soldier->GetFollowTarget();
    if (!Leader || !IsValid(Leader))
    {
        return MAX_FLT;
    }

    return FVector::Dist(Soldier->GetActorLocation(), Leader->GetActorLocation());
}

void UXBSoldierBehaviorInterface::RecordEnemySeen()
{
    LastEnemySeenTime = GetWorld()->GetTimeSeconds();
}

void UXBSoldierBehaviorInterface::FaceTarget(AActor* Target, float DeltaTime)
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier || !Target)
    {
        return;
    }

    FVector Direction = (Target->GetActorLocation() - Soldier->GetActorLocation()).GetSafeNormal2D();
    if (!Direction.IsNearlyZero())
    {
        FRotator TargetRotation = Direction.Rotation();
        FRotator CurrentRotation = Soldier->GetActorRotation();

        float RotationSpeed = Soldier->GetRotationSpeed();
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotationSpeed / 90.0f);
        Soldier->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
    }
}
