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
#include "EngineUtils.h"  // ✨ 新增 - 世界遍历支持
#include "Components/CapsuleComponent.h"

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



// ✨ 新增 - 统一阵营解析入口，避免跨主将误伤
/**
 * @brief 解析目标阵营信息（优先使用士兵所属主将阵营）
 * @param Target 目标Actor
 * @param OutFaction 输出阵营
 * @param OutLeaderOwner 输出所属主将
 * @return 是否为可识别的战斗单位
 * @note   详细流程分析: 判定目标类型 -> 若为士兵则优先取主将阵营 -> 输出阵营/主将
 *         性能/架构注意事项: 该方法仅做轻量级类型判断，避免重复逻辑散落
 */
bool UXBSoldierBehaviorInterface::ResolveTargetFaction(AActor* Target, EXBFaction& OutFaction, AXBCharacterBase*& OutLeaderOwner) const
{
    // 初始化输出，避免上层使用脏数据
    OutFaction = EXBFaction::Neutral;
    OutLeaderOwner = nullptr;

    // 目标为空直接返回
    if (!Target)
    {
        return false;
    }

    // 若为士兵，优先使用其所属主将阵营
    if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(Target))
    {
        OutLeaderOwner = TargetSoldier->GetLeaderCharacter();
        OutFaction = OutLeaderOwner ? OutLeaderOwner->GetFaction() : TargetSoldier->GetFaction();
        return true;
    }

    // 若为主将，直接读取主将阵营
    if (AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Target))
    {
        OutLeaderOwner = TargetLeader;
        OutFaction = TargetLeader->GetFaction();
        return true;
    }

    return false;
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
 * @brief  检查已分配目标是否有效
 * @param  OutEnemy [输出] 当前有效目标（未找到置空）
 * @return bool 是否成功获取有效目标
 * @note   仅使用已分配目标，不主动扫描
 */
bool UXBSoldierBehaviorInterface::SearchForEnemy(AActor*& OutEnemy)
{
    // 初始化输出参数
    OutEnemy = nullptr;

    // 获取所属士兵
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return false;
    }

    // 获取已分配目标
    AActor* AssignedTarget = Soldier->CurrentAttackTarget.Get();
    if (!IsTargetValid(AssignedTarget))
    {
        return false;
    }

    // 输出有效目标
    OutEnemy = AssignedTarget;
    return true;
}

bool UXBSoldierBehaviorInterface::HasEnemyInSight() const
{
    const AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return false;
    }

    return IsTargetValid(Soldier->CurrentAttackTarget.Get());
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

    // 🔧 安全增强 [建议添加]：攻击目标绝不能是自己
    if (Target == Soldier)
    {
        return false;
    }

    AXBCharacterBase* OwnerLeader = Soldier->GetLeaderCharacter();
    if (!OwnerLeader)
    {
        return false;
    }

    AXBCharacterBase* TargetLeaderOwner = OwnerLeader->GetLastAttackedEnemyLeader();
    if (!TargetLeaderOwner || TargetLeaderOwner->IsDead())
    {
        return false;
    }

    // 检查是否是士兵且已死亡
    if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(Target))
    {
        if (TargetSoldier->GetLeaderCharacter() != TargetLeaderOwner)
        {
            return false;
        }
        if (TargetSoldier->GetSoldierState() == EXBSoldierState::Dead)
        {
            return false;
        }
        // 🔧 修改 - 使用所属主将阵营作为有效阵营，避免跨主将误伤
        EXBFaction TargetFaction = TargetSoldier->GetFaction();
        if (AXBCharacterBase* TargetLeader = TargetSoldier->GetLeaderCharacter())
        {
            TargetFaction = TargetLeader->GetFaction();
        }
        return UXBBlueprintFunctionLibrary::AreFactionsHostile(Soldier->GetFaction(), TargetFaction);
    }

    // 检查是否是将领且已死亡
    if (AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Target))
    {
        if (TargetLeader != TargetLeaderOwner)
        {
            return false;
        }
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

    // 🔧 修改 - 主将在草丛中时禁止攻击，保持跟随
    if (AXBCharacterBase* Leader = Soldier->GetLeaderCharacter())
    {
        if (Leader->IsHiddenInBush())
        {
            Soldier->ReturnToFormation();
            return EXBBehaviorResult::Failed;
        }
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

    // 🔧 修改 - 近战伤害由蒙太奇Tag触发GA处理，避免提前结算
    // 弓手不使用该Tag，伤害应由投射物命中时处理
    if (Soldier->GetSoldierType() == EXBSoldierType::Archer)
    {
        UE_LOG(LogXBCombat, Verbose, TEXT("弓手攻击不走近战Tag: %s"), *Soldier->GetName());
    }

    // 🔧 修改 - 记录看见敌人，避免战斗状态被过早清理
    RecordEnemySeen();

    UE_LOG(LogXBCombat, Verbose, TEXT("士兵 %s 攻击 %s，等待近战Tag结算"),
        *Soldier->GetName(), *Target->GetName());

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
    const float CenterDistance = FVector::Dist2D(Soldier->GetActorLocation(), Target->GetActorLocation());
    const float EdgeDistance = CenterDistance - SelfRadius - TargetRadius;

    return EdgeDistance <= AttackRange;
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
        // 🔧 修改 - 使用所属主将阵营作为有效阵营，避免跨主将误伤
        EXBFaction TargetFaction = TargetSoldier->GetFaction();
        if (AXBCharacterBase* TargetLeader = TargetSoldier->GetLeaderCharacter())
        {
            TargetFaction = TargetLeader->GetFaction();
        }
        if (!UXBBlueprintFunctionLibrary::AreFactionsHostile(Soldier->GetFaction(), TargetFaction))
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

    // ✨ 修改 - 计算正确的停止距离
    // 停在攻击范围内而非贴脸：AcceptanceRadius = AttackRange - CapsuleRadius
    if (AcceptanceRadius < 0.0f)
    {
        const float AttackRange = Soldier->GetAttackRange();
        const float CapsuleRadius = Soldier->GetCapsuleComponent() 
            ? Soldier->GetCapsuleComponent()->GetScaledCapsuleRadius() 
            : 40.0f;
        AcceptanceRadius = FMath::Max(50.0f, AttackRange - CapsuleRadius);
    }

    AAIController* AIController = Cast<AAIController>(Soldier->GetController());
    if (!AIController)
    {
        return EXBBehaviorResult::Failed;
    }

    float Distance = FVector::Dist(Soldier->GetActorLocation(), Target->GetActorLocation());
    
    // ✨ 优化 - 调整避让权重，战斗时保持一定避让能力，避免扎堆
    if (UCharacterMovementComponent* MoveComp = Soldier->GetCharacterMovement())
    {
        if (Distance <= Soldier->GetAttackRange())
        {
            // 攻击阶段：降低但不完全关闭避让权重，避免挤成一团
            MoveComp->AvoidanceWeight = 0.3f;
        }
        else
        {
            // 移动阶段：提高避让权重，更好地绕开障碍
            MoveComp->AvoidanceWeight = FMath::Max(0.5f, Soldier->GetAvoidanceWeight());
        }
    }
    
    if (Distance <= AcceptanceRadius)
    {
        return EXBBehaviorResult::Success;
    }

    // 移动缓冲：防止在攻击范围边缘反复触发移动/停止
    const float MoveHysteresis = Soldier->GetAttackRange() * 0.15f;
    const float EffectiveAcceptance = AcceptanceRadius + MoveHysteresis;
    if (Distance <= EffectiveAcceptance)
    {
        return EXBBehaviorResult::Success;
    }

    // ✨ 新增 - 添加随机偏移，让士兵从不同角度接近目标，避免扎堆
    FVector TargetLocation = Target->GetActorLocation();
    FVector SoldierLocation = Soldier->GetActorLocation();
    FVector ToTarget = (TargetLocation - SoldierLocation).GetSafeNormal2D();
    
    // 计算垂直于目标方向的向量
    FVector RightVector = FVector::CrossProduct(ToTarget, FVector::UpVector);
    
    // 使用士兵ID作为随机种子，保证每个士兵的偏移一致且可预测
    FRandomStream RandomStream(Soldier->GetUniqueID());
    float RandomAngle = RandomStream.FRandRange(-60.0f, 60.0f);  // ±60度的角度偏移
    float RandomDistance = RandomStream.FRandRange(50.0f, 150.0f);  // 50-150单位的距离偏移
    
    // 计算偏移后的位置
    FVector Offset = RightVector.RotateAngleAxis(RandomAngle, FVector::UpVector) * RandomDistance;
    FVector DispersedTarget = TargetLocation + Offset;
    
    // 使用分散后的位置作为移动目标，让士兵围绕目标形成包围圈
    EPathFollowingRequestResult::Type Result = AIController->MoveToLocation(
        DispersedTarget,
        AcceptanceRadius,
        true,  // bStopOnOverlap
        true,  // bUsePathfinding
        true,  // bProjectDestinationToNavigation
        true   // bCanStrafe
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

/**
 * @brief  判断是否需要脱离战斗
 * @param  无
 * @return 是否脱离战斗
 * @note   详细流程分析: 先处理“目标为跟随态”超距脱战 -> 距离超限强制脱战 -> 目标非战斗时仅保留距离限制 -> 否则按无敌人时间判定
 *         性能/架构注意事项: 通过快速距离判断避免频繁感知查询，降低每帧开销
 */
bool UXBSoldierBehaviorInterface::ShouldDisengage() const
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return false;
    }

    // ✨ 新增 - 预先缓存追击距离与主将距离，减少重复计算
    float DisengageDistance = Soldier->GetDisengageDistance();
    float DistToLeader = GetDistanceToLeader();

    if (AXBCharacterBase* Leader = Soldier->GetLeaderCharacter())
    {
        AXBCharacterBase* TargetLeader = Leader->GetLastAttackedEnemyLeader();
        if (TargetLeader && !TargetLeader->IsDead())
        {
            return false;
        }
    }

    // ✨ 新增 - 目标状态判定：用于处理目标脱离战斗后的追击逻辑
    // 说明：当目标不处于战斗时，士兵允许追击，但必须受“追击距离”上限约束
    bool bIsTargetInCombat = true;
    bool bIsTargetFollowing = false;
    if (AActor* CurrentTarget = Soldier->CurrentAttackTarget.Get())
    {
        // 说明：目标类型不同，对应的战斗状态来源不同，必须区分读取以避免误判
        // 目标是士兵：检查其战斗状态
        if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(CurrentTarget))
        {
            bIsTargetInCombat = (TargetSoldier->GetSoldierState() == EXBSoldierState::Combat);
            bIsTargetFollowing = (TargetSoldier->GetSoldierState() == EXBSoldierState::Following);
        }
        // 目标是将领：检查其战斗状态
        else if (AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(CurrentTarget))
        {
            bIsTargetInCombat = TargetLeader->IsInCombat();
        }
    }

    // 🔧 修改 - 目标为跟随态时允许脱战：追击过远会导致队列散开
    if (bIsTargetFollowing && DistToLeader >= DisengageDistance)
    {
        UE_LOG(LogXBAI, Log, TEXT("士兵 %s 追击跟随目标超距: %.0f >= %.0f"),
            *Soldier->GetName(), DistToLeader, DisengageDistance);
        return true;
    }

    // 🔧 修改 - 距离超限时允许强制脱战，即使主将仍处于战斗
    if (DistToLeader >= DisengageDistance)
    {
        UE_LOG(LogXBAI, Verbose, TEXT("士兵 %s 距离将领过远: %.0f >= %.0f"),
            *Soldier->GetName(), DistToLeader, DisengageDistance);
        return true;
    }

    // 🔧 修改 - 目标脱离战斗时，优先进入追击模式，仅按追击距离判定是否脱战
    // 说明：此处直接返回 false 是为了维持追击，直到超过追击距离由上方条件触发脱战
    if (!bIsTargetInCombat)
    {
        return false;
    }

    // ✨ 核心修复 - 士兵有有效攻击目标时，绝对不脱战
    // 说明：即使感知系统判定"无敌人"，只要 CurrentAttackTarget 有效且存活，士兵就应继续战斗
    // 这避免了战斗中因感知缓存刷新不及时而误触发脱战
    if (AActor* Target = Soldier->CurrentAttackTarget.Get())
    {
        bool bTargetAlive = false;
        if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(Target))
        {
            bTargetAlive = (TargetSoldier->GetSoldierState() != EXBSoldierState::Dead);
        }
        else if (AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Target))
        {
            bTargetAlive = !TargetLeader->IsDead();
        }
        
        if (bTargetAlive)
        {
            // 有活着的攻击目标，不允许脱战
            return false;
        }
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
