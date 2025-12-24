/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Soldier/Component/XBSoldierFollowComponent.cpp

/**
 * @file XBSoldierFollowComponent.cpp
 * @brief 士兵跟随组件实现 - 实时锁定槽位
 *
 * @note 🔧 修改记录:
 *       1. 🔧 修复 GhostRotationInterpSpeed 过低导致的抖动：
 *          - 槽位位置计算默认使用主将即时Yaw（无旋转延迟）
 *          - 幽灵Yaw采用角度安全的指数平滑插值（避免 0/360 抖动）
 *       2. 🔧 锁定模式统一使用 AddMovementInput，减少输入向量差异导致的摆动
 */

#include "Soldier/Component/XBSoldierFollowComponent.h"
#include "Utils/XBLogCategories.h"
#include "Character/XBCharacterBase.h"
#include "Character/Components/XBFormationComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

UXBSoldierFollowComponent::UXBSoldierFollowComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UXBSoldierFollowComponent::BeginPlay()
{
    Super::BeginPlay();

    GetCachedMovementComponent();
    GetCachedCapsuleComponent();

    if (AActor* Owner = GetOwner())
    {
        LastFrameLocation = Owner->GetActorLocation();
        LastPositionForStuckCheck = LastFrameLocation;
    }

    SetMovementMode(true);

    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件初始化完成：追赶补偿倍率=%.2f，槽位即时Yaw=%s"),
        CatchUpSpeedMultiplier,
        bUseInstantLeaderYawForSlot ? TEXT("启用") : TEXT("禁用"));
}

void UXBSoldierFollowComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        CurrentMoveSpeed = 0.0f;
        return;
    }

    // 检查士兵是否已死亡，死亡则不更新
    if (AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(Owner))
    {
        if (Soldier->IsDead())
        {
            CurrentMoveSpeed = 0.0f;
            return;
        }
    }

    // 自由模式：战斗中，不控制位置
    if (CurrentMode == EXBFollowMode::Free)
    {
        if (UCharacterMovementComponent* MoveComp = GetCachedMovementComponent())
        {
            FVector Velocity = MoveComp->Velocity;
            Velocity.Z = 0.0f;
            CurrentMoveSpeed = Velocity.Size();
        }
        LastFrameLocation = Owner->GetActorLocation();
        return;
    }

    // 没有跟随目标
    if (!FollowTargetRef.IsValid())
    {
        CurrentMoveSpeed = 0.0f;
        LastFrameLocation = Owner->GetActorLocation();
        return;
    }

    // 更新幽灵目标（位置与Yaw），用于平滑编队
    UpdateGhostTarget(DeltaTime);

    // 每帧更新将领速度缓存（用于招募过渡模式）
    if (bSyncLeaderSprint && CurrentMode == EXBFollowMode::RecruitTransition)
    {
        CachedLeaderSpeed = GetLeaderCurrentSpeed();

        if (CachedLeaderCharacter.IsValid())
        {
            bLeaderIsSprinting = CachedLeaderCharacter->IsSprinting();
        }
    }

    FVector PreUpdateLocation = Owner->GetActorLocation();

    switch (CurrentMode)
    {
    case EXBFollowMode::Locked:
        UpdateLockedMode(DeltaTime);
        break;

    case EXBFollowMode::RecruitTransition:
        UpdateRecruitTransitionMode(DeltaTime);
        break;

    default:
        break;
    }

    // 计算实际移动速度（用于动画等）
    if (DeltaTime > KINDA_SMALL_NUMBER)
    {
        FVector CurrentLocation = Owner->GetActorLocation();
        FVector Delta = CurrentLocation - PreUpdateLocation;
        Delta.Z = 0.0f;
        CurrentMoveSpeed = Delta.Size() / DeltaTime;
    }

    LastFrameLocation = Owner->GetActorLocation();
}

void UXBSoldierFollowComponent::SyncLeaderSprintState(bool bLeaderSprinting, float LeaderCurrentSpeed)
{
    bLeaderIsSprinting = bLeaderSprinting;
    CachedLeaderSpeed = LeaderCurrentSpeed;

    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件：同步将领状态，冲刺=%s，速度=%.1f"),
        bLeaderSprinting ? TEXT("是") : TEXT("否"),
        LeaderCurrentSpeed);
}

float UXBSoldierFollowComponent::GetLeaderCurrentSpeed() const
{
    if (!CachedLeaderCharacter.IsValid())
    {
        return 0.0f;
    }

    AXBCharacterBase* Leader = CachedLeaderCharacter.Get();
    if (!Leader || !IsValid(Leader))
    {
        return 0.0f;
    }

    return Leader->GetCurrentMoveSpeed();
}

float UXBSoldierFollowComponent::CalculateRecruitTransitionSpeed(float DistanceToTarget) const
{
    float LeaderSpeed = CachedLeaderSpeed;
    if (LeaderSpeed < KINDA_SMALL_NUMBER && CachedLeaderCharacter.IsValid())
    {
        LeaderSpeed = GetLeaderCurrentSpeed();
    }

    if (CloseSlowdownDistance > 0.0f && DistanceToTarget <= CloseSlowdownDistance)
    {
        float CloseAlpha = 1.0f - FMath::Clamp(DistanceToTarget / CloseSlowdownDistance, 0.0f, 1.0f);

        float TargetSpeed = FMath::Max(LeaderSpeed, MinTransitionSpeed);

        float BaseApproachSpeed = RecruitTransitionSpeed;
        float FinalSpeed = FMath::Lerp(BaseApproachSpeed, TargetSpeed, CloseAlpha);

        FinalSpeed = FMath::Max(FinalSpeed, MinTransitionSpeed);

        if (DistanceToTarget < 50.0f && LeaderSpeed > KINDA_SMALL_NUMBER)
        {
            FinalSpeed = FMath::Min(FinalSpeed, LeaderSpeed * 1.1f);
        }

        return FinalSpeed;
    }

    const float NormalizedDistance = FMath::Clamp(DistanceToTarget / FMath::Max(ArrivalThreshold, 1.0f), 0.0f, 10.0f);
    float DistanceMultiplier = 1.0f + NormalizedDistance * DistanceSpeedMultiplier;
    DistanceMultiplier = FMath::Max(DistanceMultiplier, 1.0f);
    float DistanceBasedSpeed = RecruitTransitionSpeed * DistanceMultiplier;

    float LeaderBasedSpeed = 0.0f;
    if (bSyncLeaderSprint && LeaderSpeed > KINDA_SMALL_NUMBER)
    {
        LeaderBasedSpeed = LeaderSpeed * CatchUpSpeedMultiplier;

        if (bLeaderIsSprinting)
        {
            LeaderBasedSpeed *= 1.2f;
        }
    }

    float FinalSpeed = FMath::Max(DistanceBasedSpeed, LeaderBasedSpeed);
    FinalSpeed = FMath::Clamp(FinalSpeed, MinTransitionSpeed, MaxTransitionSpeed);

    return FinalSpeed;
}

bool UXBSoldierFollowComponent::IsRotationAligned(const FRotator& TargetRotation, float ToleranceDegrees) const
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return true;
    }

    FRotator CurrentRotation = Owner->GetActorRotation();
    float YawDiff = FMath::Abs(FRotator::NormalizeAxis(CurrentRotation.Yaw - TargetRotation.Yaw));
    return YawDiff <= ToleranceDegrees;
}

/**
 * @brief 更新锁定模式
 * @param DeltaTime 帧间隔
 * @note  🔧 修复点：
 *        1) 不再使用 SetActorLocation 步进（那会让移动组件速度为0，看起来像粘住主将）
 *        2) 改用 AddMovementInput 驱动 CharacterMovement，保证有真实速度/加速度
 *        3) 使用“死区 + 输入按误差缩放 + MaxWalkSpeed 插值”避免微抖与挤压
 */
void UXBSoldierFollowComponent::UpdateLockedMode(float DeltaTime)
{
   AActor* Owner = GetOwner();
    AActor* Leader = FollowTargetRef.Get();

    if (!Owner || !Leader || !IsValid(Leader))
    {
        return;
    }

    ACharacter* CharOwner = Cast<ACharacter>(Owner);
    UCharacterMovementComponent* MoveComp = GetCachedMovementComponent();
    if (!CharOwner || !MoveComp)
    {
        return;
    }

    const FVector TargetPosition = GetSmoothedFormationTarget();
    const FVector CurrentPosition = Owner->GetActorLocation();
    const float DistanceToSlot = FVector::Dist2D(CurrentPosition, TargetPosition);

    // ========== 速度策略 ==========
    float LeaderSpeed = 0.0f;
    if (CachedLeaderCharacter.IsValid())
    {
        LeaderSpeed = CachedLeaderCharacter->GetCurrentMoveSpeed();
    }

    // 🔧 修改 - 目标速度：至少不低于 LockedFollowMoveSpeed，并允许一定追赶余量（保持跟随感但不“贴死”）
    const float DesiredSpeed = FMath::Max(LockedFollowMoveSpeed, LeaderSpeed + LockedCatchUpExtraSpeed);

    // 速度插值：避免速度突跳产生顿挫
    const float NewMaxSpeed = (LockedSpeedInterpRate > 0.0f)
        ? FMath::FInterpTo(MoveComp->MaxWalkSpeed, DesiredSpeed, DeltaTime, LockedSpeedInterpRate)
        : DesiredSpeed;

    MoveComp->MaxWalkSpeed = NewMaxSpeed;

    // ========== 误差驱动移动：死区 + 输入缩放 ==========
    if (DistanceToSlot > LockedDeadzoneDistance)
    {
        const FVector MoveDir = (TargetPosition - CurrentPosition).GetSafeNormal2D();
        if (!MoveDir.IsNearlyZero())
        {
            // 为什么按误差缩放输入：误差越小越不抢位，减少“挤成团”和抖腿
            const float InputAlpha = FMath::Clamp(DistanceToSlot / FMath::Max(LockedFullInputDistance, 1.0f), 0.0f, 1.0f);
            CharOwner->AddMovementInput(MoveDir, InputAlpha);
        }
    }

    // ========== 朝向：锁定模式始终面向队伍前方 ==========
    if (bFollowRotation)
    {
        const FRotator TargetRotation = CalculateFormationWorldRotation();
        const FRotator NewRotation = FMath::RInterpTo(
            Owner->GetActorRotation(),
            TargetRotation,
            DeltaTime,
            LockedRotationInterpSpeed
        );
        Owner->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
    }
}
/**
 * @brief 更新招募过渡模式
 * @param DeltaTime 帧间隔
 * @note  🔧 修复点：
 *        1) 取消“接近槽位立即切状态”的硬切，改为到达确认时间（滞回），消除顿挫
 *        2) 旋转采用“移动方向 -> 队伍前方”的距离渐变混合，避免临界点突然转向
 *        3) 位移由 CharacterMovement 驱动，速度用插值平滑，视觉更丝滑且有速度感
 */
void UXBSoldierFollowComponent::UpdateRecruitTransitionMode(float DeltaTime)
{
    AActor* Owner = GetOwner();
    AActor* Leader = FollowTargetRef.Get();

    if (!Owner || !Leader || !IsValid(Leader))
    {
        return;
    }

    if (!bRecruitMovementActive)
    {
        return;
    }

    ACharacter* CharOwner = Cast<ACharacter>(Owner);
    UCharacterMovementComponent* MoveComp = GetCachedMovementComponent();
    if (!CharOwner || !MoveComp)
    {
        return;
    }

    const FVector TargetPosition = GetSmoothedFormationTarget();
    const FVector CurrentPosition = Owner->GetActorLocation();
    const float Distance = FVector::Dist2D(CurrentPosition, TargetPosition);

    // ========= 到达阈值动态调整（主将移动时放宽）=========
    float EffectiveArrivalThreshold = ArrivalThreshold;

    float LeaderSpeed = 0.0f;
    if (CachedLeaderCharacter.IsValid())
    {
        LeaderSpeed = CachedLeaderCharacter->GetCurrentMoveSpeed();
    }

    if (LeaderSpeed > 50.0f)
    {
        EffectiveArrivalThreshold = ArrivalThreshold * 1.5f;
    }

    // ========= 追赶速度计算 + 平滑 =========
    float DesiredSpeed = CalculateRecruitTransitionSpeed(Distance);
    DesiredSpeed = FMath::Clamp(DesiredSpeed, MinTransitionSpeed, MaxTransitionSpeed);

    if (SmoothedSpeedCache <= KINDA_SMALL_NUMBER)
    {
        SmoothedSpeedCache = DesiredSpeed;
    }

    if (bUseSpeedSmoothing && SpeedSmoothingRate > 0.0f)
    {
        SmoothedSpeedCache = FMath::FInterpTo(SmoothedSpeedCache, DesiredSpeed, DeltaTime, SpeedSmoothingRate);
        DesiredSpeed = SmoothedSpeedCache;
    }

    // 🔧 修改 - 再对 MaxWalkSpeed 做一次插值，减少“即将到位时速度变化”的顿挫
    const float NewMaxSpeed = FMath::FInterpTo(MoveComp->MaxWalkSpeed, DesiredSpeed, DeltaTime, 12.0f);
    MoveComp->MaxWalkSpeed = NewMaxSpeed;

    // ========= 位移与旋转 =========
    if (Distance > EffectiveArrivalThreshold)
    {
        ArrivedTimeAccumulator = 0.0f;

        const FVector MoveDirection = (TargetPosition - CurrentPosition).GetSafeNormal2D();
        if (!MoveDirection.IsNearlyZero())
        {
            CharOwner->AddMovementInput(MoveDirection, 1.0f);

            // ========= 旋转：移动方向 -> 队伍前方 渐变混合 =========
            if (bFollowRotation)
            {
                const float BlendDist = FMath::Max(RecruitRotationBlendDistance, 1.0f);

                // BlendAlpha：越接近槽位越趋向队伍前方
                const float BlendAlpha = 1.0f - FMath::Clamp(Distance / BlendDist, 0.0f, 1.0f);

                const float MoveYaw = MoveDirection.Rotation().Yaw;
                const float FormationYaw = CalculateFormationWorldRotation().Yaw;

                // 角度安全混合：避免 179->-179 抖动
                const float YawDelta = FMath::FindDeltaAngleDegrees(MoveYaw, FormationYaw);
                const float BlendedYaw = FRotator::NormalizeAxis(MoveYaw + YawDelta * BlendAlpha);

                const FRotator TargetRot(0.0f, BlendedYaw, 0.0f);

                // 转向速度也做渐变：远处更快朝移动方向，近处更柔和对齐队伍前方
                const float RotSpeed = FMath::Lerp(MoveDirectionRotationSpeed, AlignmentRotationSpeed, BlendAlpha);

                const FRotator NewRot = FMath::RInterpTo(Owner->GetActorRotation(), TargetRot, DeltaTime, RotSpeed);
                Owner->SetActorRotation(FRotator(0.0f, NewRot.Yaw, 0.0f));
            }
        }
    }
    else
    {
        // 在到达阈值内：累计确认时间（滞回），避免边界抖动导致“顿挫/掉帧感”
        ArrivedTimeAccumulator += DeltaTime;

        // 小幅补位（主将仍在移动/旋转时避免慢慢偏离）
        const FVector MicroDir = (TargetPosition - CurrentPosition).GetSafeNormal2D();
        if (!MicroDir.IsNearlyZero())
        {
            CharOwner->AddMovementInput(MicroDir, 0.25f);
        }

        // 持续对齐队伍前方（平滑）
        if (bFollowRotation)
        {
            const FRotator FormationRot = CalculateFormationWorldRotation();
            const FRotator NewRot = FMath::RInterpTo(Owner->GetActorRotation(), FormationRot, DeltaTime, AlignmentRotationSpeed);
            Owner->SetActorRotation(FRotator(0.0f, NewRot.Yaw, 0.0f));
        }

        // 达到确认时间 + 朝向对齐后，切到 Locked（无硬切顿挫）
        if (ArrivedTimeAccumulator >= ArriveConfirmTime &&
            IsRotationAligned(CalculateFormationWorldRotation(), AlignmentToleranceDegrees))
        {
            UE_LOG(LogXBSoldier, Log, TEXT("跟随组件：招募过渡完成（确认%.2fs），切换到锁定模式"), ArriveConfirmTime);

            bRecruitMovementActive = false;
            ArrivedTimeAccumulator = 0.0f;

            SetFollowMode(EXBFollowMode::Locked);
            OnRecruitTransitionCompleted.Broadcast();

            bLeaderIsSprinting = false;
            CachedLeaderSpeed = 0.0f;
        }
    }

    // ========= 卡住检测（保持你原逻辑）=========
    const FVector NewPosition = Owner->GetActorLocation();
    const float MovedDistance = FVector::Dist2D(NewPosition, LastPositionForStuckCheck);

    if (DeltaTime > KINDA_SMALL_NUMBER)
    {
        const float InstantSpeed = MovedDistance / DeltaTime;

        if (InstantSpeed < StuckSpeedThreshold && Distance > ArrivalThreshold * 2.0f)
        {
            AccumulatedStuckTime += DeltaTime;
        }
        else
        {
            AccumulatedStuckTime = 0.0f;
            LastPositionForStuckCheck = NewPosition;
        }
    }
    // 这里原本有 ShouldForceTeleport/PerformForceTeleport，但你当前 bAllowTeleportDuringRecruit 默认关闭
    // 若你后续希望启用“卡住/超时传送”，可在这里加上判断调用（不影响本次抖动修复核心）。
}

void UXBSoldierFollowComponent::UpdateAlignmentPhase(float DeltaTime)
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    FRotator FormationRotation = CalculateFormationWorldRotation();

    if (IsRotationAligned(FormationRotation, AlignmentToleranceDegrees))
    {
        UE_LOG(LogXBSoldier, Log, TEXT("跟随组件：对齐完成，切换到锁定模式"));

        Owner->SetActorRotation(FRotator(0.0f, FormationRotation.Yaw, 0.0f));
        SetFollowMode(EXBFollowMode::Locked);
        OnRecruitTransitionCompleted.Broadcast();
        return;
    }

    FRotator CurrentRotation = Owner->GetActorRotation();
    FRotator NewRotation = FMath::RInterpTo(
        CurrentRotation,
        FormationRotation,
        DeltaTime,
        AlignmentRotationSpeed
    );
    Owner->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));

    FVector TargetPosition = GetSmoothedFormationTarget();
    FVector CurrentPosition = Owner->GetActorLocation();
    float Distance = FVector::Dist2D(CurrentPosition, TargetPosition);

    if (Distance > ArrivalThreshold * 2.0f)
    {
        UCharacterMovementComponent* MoveComp = GetCachedMovementComponent();
        ACharacter* CharOwner = Cast<ACharacter>(Owner);

        if (MoveComp && CharOwner)
        {
            float LeaderSpeed = 0.0f;
            if (CachedLeaderCharacter.IsValid())
            {
                LeaderSpeed = CachedLeaderCharacter->GetCurrentMoveSpeed();
            }

            MoveComp->MaxWalkSpeed = FMath::Max(LeaderSpeed, LockedFollowMoveSpeed);

            FVector MoveDirection = (TargetPosition - CurrentPosition).GetSafeNormal2D();
            if (!MoveDirection.IsNearlyZero())
            {
                CharOwner->AddMovementInput(MoveDirection, 1.0f);
            }
        }
    }
}

/**
 * @brief 更新幽灵目标（位置与Yaw插值）
 * @param DeltaTime 帧间隔
 * @note  🔧 关键修复点：
 *        1) 幽灵Yaw采用角度安全的指数平滑（避免 0/360 抖动）
 *        2) 槽位Yaw使用“最小插值速度”，保证 GhostRotationInterpSpeed 很低也不抖
 *        3) 槽位Yaw可限制最大角速度，避免大旋转时目标点甩动导致士兵穿插堆叠
 *        4) 槽位中心默认用主将即时位置，减少绕滞后中心旋转造成的交叉路径
 */
void UXBSoldierFollowComponent::UpdateGhostTarget(float DeltaTime)
{
     AActor* Leader = FollowTargetRef.Get();
    if (!Leader || !IsValid(Leader))
    {
        bGhostInitialized = false;
        GhostSlotTargetLocation = FVector::ZeroVector;
        return;
    }

    const FVector LeaderLocation = Leader->GetActorLocation();
    const float LeaderYaw = FRotator::NormalizeAxis(Leader->GetActorRotation().Yaw);

    // ✨ 新增 - 角度安全指数平滑（帧率无关）
    auto InterpAngleExp = [](float CurrentDeg, float TargetDeg, float Dt, float Speed) -> float
    {
        if (Speed <= KINDA_SMALL_NUMBER || Dt <= KINDA_SMALL_NUMBER)
        {
            return FRotator::NormalizeAxis(TargetDeg);
        }

        // 为什么使用 FindDeltaAngleDegrees：它天然处理 359->0 的最短角路径
        const float Delta = FMath::FindDeltaAngleDegrees(CurrentDeg, TargetDeg);
        const float Alpha = 1.0f - FMath::Exp(-Speed * Dt);
        return FRotator::NormalizeAxis(CurrentDeg + Delta * Alpha);
    };

    // 初始化
    if (!bGhostInitialized)
    {
        GhostTargetLocation = LeaderLocation;

        GhostYawDegrees = LeaderYaw;
        GhostSlotYawDegrees = LeaderYaw;

        GhostTargetRotation = FRotator(0.0f, GhostYawDegrees, 0.0f);

        bGhostInitialized = true;

        // 初始化槽位目标点
        const FVector2D SlotOffset = GetSlotLocalOffset();
        const FVector LocalOffset3D(SlotOffset.X, SlotOffset.Y, 0.0f);

        const FVector SlotCenter = bUseInstantLeaderLocationForSlotCenter ? LeaderLocation : GhostTargetLocation;
        GhostSlotTargetLocation = SlotCenter + FRotator(0.0f, GhostSlotYawDegrees, 0.0f).RotateVector(LocalOffset3D);
        return;
    }

    // 幽灵位置插值：用于整体平滑
    GhostTargetLocation = FMath::VInterpTo(
        GhostTargetLocation,
        LeaderLocation,
        DeltaTime,
        GhostLocationInterpSpeed
    );

    // 幽灵Yaw插值：用于士兵朝向/队伍朝向平滑
    GhostYawDegrees = InterpAngleExp(GhostYawDegrees, LeaderYaw, DeltaTime, GhostRotationInterpSpeed);
    GhostTargetRotation = FRotator(0.0f, GhostYawDegrees, 0.0f);

    // 🔧 修改 - 槽位Yaw：强制最小插值速度，避免你把 GhostRotationInterpSpeed 调低后出现抖动
    const float SlotYawInterpSpeed = FMath::Max(GhostRotationInterpSpeed, MinGhostSlotYawInterpSpeed);

    // 先插值到“期望槽位Yaw”
    const float PrevSlotYaw = GhostSlotYawDegrees;
    float NewSlotYaw = InterpAngleExp(GhostSlotYawDegrees, LeaderYaw, DeltaTime, SlotYawInterpSpeed);

    // ✨ 新增 - 限制槽位Yaw最大角速度：防止主将大幅快速转身时槽位目标点瞬间甩动导致穿插堆叠
    if (bClampSlotYawRate && DeltaTime > KINDA_SMALL_NUMBER)
    {
        const float MaxDelta = MaxSlotYawRateDegPerSec * DeltaTime;
        const float RawDelta = FMath::FindDeltaAngleDegrees(PrevSlotYaw, NewSlotYaw);

        const float ClampedDelta = FMath::Clamp(RawDelta, -MaxDelta, MaxDelta);
        NewSlotYaw = FRotator::NormalizeAxis(PrevSlotYaw + ClampedDelta);
    }

    GhostSlotYawDegrees = NewSlotYaw;

    // 计算槽位目标点（注意：中心点默认使用主将即时位置）
    const FVector2D SlotOffset = GetSlotLocalOffset();
    const FVector LocalOffset3D(SlotOffset.X, SlotOffset.Y, 0.0f);

    const FVector SlotCenter = bUseInstantLeaderLocationForSlotCenter ? LeaderLocation : GhostTargetLocation;
    GhostSlotTargetLocation = SlotCenter + FRotator(0.0f, GhostSlotYawDegrees, 0.0f).RotateVector(LocalOffset3D);
}

FVector UXBSoldierFollowComponent::GetSmoothedFormationTarget() const
{
    if (bGhostInitialized && !GhostSlotTargetLocation.IsZero())
    {
        return GhostSlotTargetLocation;
    }

    return CalculateFormationWorldPosition();
}

/**
 * @brief 计算编队世界位置
 * @return 槽位世界坐标
 * @note  🔧 修改 - 位置优先使用“槽位Yaw”而不是“幽灵Yaw”，避免旋转滞后引发抖动
 */
FVector UXBSoldierFollowComponent::CalculateFormationWorldPosition() const
{
    AActor* Leader = FollowTargetRef.Get();
    if (!Leader || !IsValid(Leader))
    {
        AActor* Owner = GetOwner();
        return Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
    }

    const FVector LeaderLocation = bGhostInitialized ? GhostTargetLocation : Leader->GetActorLocation();

    const float YawForSlot = bGhostInitialized
        ? GhostSlotYawDegrees
        : FRotator::NormalizeAxis(Leader->GetActorRotation().Yaw);

    const FVector2D SlotOffset = GetSlotLocalOffset();
    const FVector LocalOffset3D(SlotOffset.X, SlotOffset.Y, 0.0f);

    const FVector WorldOffset = FRotator(0.0f, YawForSlot, 0.0f).RotateVector(LocalOffset3D);
    return LeaderLocation + WorldOffset;
}

/**
 * @brief 计算编队世界旋转
 * @return 队伍朝向（Yaw）
 * @note  使用幽灵Yaw做平滑，让队伍朝向更柔和；不影响槽位位置稳定性
 */
FRotator UXBSoldierFollowComponent::CalculateFormationWorldRotation() const
{
    AActor* Leader = FollowTargetRef.Get();
    if (!Leader || !IsValid(Leader))
    {
        AActor* Owner = GetOwner();
        return Owner ? Owner->GetActorRotation() : FRotator::ZeroRotator;
    }

    const float Yaw = bGhostInitialized
        ? GhostYawDegrees
        : FRotator::NormalizeAxis(Leader->GetActorRotation().Yaw);

    return FRotator(0.0f, Yaw, 0.0f);
}

// ==================== 目标设置 ====================

void UXBSoldierFollowComponent::SetFollowTarget(AActor* NewTarget)
{
    FollowTargetRef = NewTarget;
    CachedLeaderCharacter = Cast<AXBCharacterBase>(NewTarget);

    if (NewTarget)
    {
        if (AXBCharacterBase* CharTarget = Cast<AXBCharacterBase>(NewTarget))
        {
            CachedFormationComponent = CharTarget->GetFormationComponent();

            bLeaderIsSprinting = CharTarget->IsSprinting();
            CachedLeaderSpeed = CharTarget->GetCurrentMoveSpeed();
        }

        // 🔧 修改 - 初始化Yaw缓存（Yaw-only），避免第一帧角度突跳
        const float InitYaw = FRotator::NormalizeAxis(NewTarget->GetActorRotation().Yaw);
        GhostYawDegrees = InitYaw;
        GhostSlotYawDegrees = InitYaw;

        GhostTargetLocation = NewTarget->GetActorLocation();
        GhostTargetRotation = FRotator(0.0f, GhostYawDegrees, 0.0f);

        bGhostInitialized = true;

        const FVector2D SlotOffset = GetSlotLocalOffset();
        GhostSlotTargetLocation = GhostTargetLocation + FRotator(0.0f, GhostSlotYawDegrees, 0.0f).RotateVector(FVector(SlotOffset.X, SlotOffset.Y, 0.0f));

        UE_LOG(LogXBSoldier, Log, TEXT("跟随组件：设置跟随目标=%s，槽位即时Yaw=%s"),
            *NewTarget->GetName(),
            bUseInstantLeaderYawForSlot ? TEXT("启用") : TEXT("禁用"));
    }
    else
    {
        CachedFormationComponent = nullptr;
        CachedLeaderCharacter = nullptr;
        bLeaderIsSprinting = false;
        CachedLeaderSpeed = 0.0f;

        bGhostInitialized = false;
        GhostSlotTargetLocation = FVector::ZeroVector;
    }
}

void UXBSoldierFollowComponent::SetFormationSlotIndex(int32 SlotIndex)
{
    FormationSlotIndex = SlotIndex;
    UE_LOG(LogXBSoldier, Verbose, TEXT("跟随组件：设置槽位索引=%d"), SlotIndex);
}

// ==================== 缓存/碰撞/模式/其他函数保持不变（你的原实现） ====================

UCharacterMovementComponent* UXBSoldierFollowComponent::GetCachedMovementComponent()
{
    if (CachedMovementComponent.IsValid())
    {
        return CachedMovementComponent.Get();
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return nullptr;
    }

    if (ACharacter* CharOwner = Cast<ACharacter>(Owner))
    {
        CachedMovementComponent = CharOwner->GetCharacterMovement();
    }

    return CachedMovementComponent.Get();
}

UCapsuleComponent* UXBSoldierFollowComponent::GetCachedCapsuleComponent()
{
    if (CachedCapsuleComponent.IsValid())
    {
        return CachedCapsuleComponent.Get();
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return nullptr;
    }

    if (ACharacter* CharOwner = Cast<ACharacter>(Owner))
    {
        CachedCapsuleComponent = CharOwner->GetCapsuleComponent();
    }

    return CachedCapsuleComponent.Get();
}

void UXBSoldierFollowComponent::SetSoldierCollisionEnabled(bool bEnableCollision)
{
    UCapsuleComponent* Capsule = GetCachedCapsuleComponent();
    if (!Capsule)
    {
        return;
    }

    if (bEnableCollision)
    {
        if (bCollisionModified)
        {
            Capsule->SetCollisionResponseToChannel(ECC_Pawn, OriginalPawnResponse);
            bCollisionModified = false;
        }
    }
    else
    {
        if (!bCollisionModified)
        {
            OriginalPawnResponse = Capsule->GetCollisionResponseToChannel(ECC_Pawn);
            Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
            bCollisionModified = true;
        }
    }
}

void UXBSoldierFollowComponent::SetMovementMode(bool bEnableWalking)
{
    UCharacterMovementComponent* MoveComp = GetCachedMovementComponent();
    if (!MoveComp)
    {
        return;
    }

    MoveComp->SetComponentTickEnabled(true);

    if (bEnableWalking)
    {
        MoveComp->SetMovementMode(MOVE_Walking);
        MoveComp->GravityScale = 1.0f;
    }
    else
    {
        MoveComp->SetMovementMode(MOVE_Flying);
        MoveComp->GravityScale = 0.0f;
    }
}

void UXBSoldierFollowComponent::SetRVOAvoidanceEnabled(bool bEnable)
{
    UCharacterMovementComponent* MoveComp = GetCachedMovementComponent();
    if (MoveComp)
    {
        MoveComp->SetAvoidanceEnabled(false);
    }
}

void UXBSoldierFollowComponent::SetCombatState(bool bInCombat)
{
    if (bIsInCombat == bInCombat)
    {
        return;
    }

    bIsInCombat = bInCombat;

    if (bInCombat)
    {
        SetSoldierCollisionEnabled(true);
    }

    SetMovementMode(true);

    OnCombatStateChanged.Broadcast(bInCombat);
}

void UXBSoldierFollowComponent::SetFollowMode(EXBFollowMode NewMode)
{
    if (CurrentMode == NewMode)
    {
        return;
    }

    EXBFollowMode OldMode = CurrentMode;
    CurrentMode = NewMode;

    if (NewMode != EXBFollowMode::RecruitTransition)
    {
        CurrentRecruitPhase = EXBRecruitTransitionPhase::Moving;
    }

    if (bDisableCollisionDuringTransition)
    {
        if (NewMode == EXBFollowMode::RecruitTransition)
        {
            SetSoldierCollisionEnabled(false);
        }
        else if (NewMode == EXBFollowMode::Locked && !bIsInCombat)
        {
            SetSoldierCollisionEnabled(true);
        }
    }

    SetMovementMode(true);

    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件：模式切换 %d -> %d"),
        static_cast<int32>(OldMode), static_cast<int32>(NewMode));
}

void UXBSoldierFollowComponent::EnterCombatMode()
{
    SetCombatState(true);
    SetFollowMode(EXBFollowMode::Free);
}

void UXBSoldierFollowComponent::ExitCombatMode()
{
    SetCombatState(false);
    SetFollowMode(EXBFollowMode::RecruitTransition);
    StartRecruitTransition();
}

void UXBSoldierFollowComponent::TeleportToFormationPosition()
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    FVector TargetPos = CalculateFormationWorldPosition();
    FRotator TargetRot = CalculateFormationWorldRotation();
    FVector CurrentPos = Owner->GetActorLocation();

    float GroundZ = GetGroundHeightAtLocation(
        FVector2D(TargetPos.X, TargetPos.Y),
        CurrentPos.Z
    );

    float CharacterHalfHeight = 88.0f;
    if (UCapsuleComponent* Capsule = GetCachedCapsuleComponent())
    {
        CharacterHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    }

    Owner->SetActorLocation(FVector(TargetPos.X, TargetPos.Y, GroundZ + CharacterHalfHeight));

    if (bFollowRotation)
    {
        Owner->SetActorRotation(TargetRot);
    }

    LastPositionForStuckCheck = Owner->GetActorLocation();
    AccumulatedStuckTime = 0.0f;

    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件：传送到编队位置，地面Z=%.1f"), GroundZ);
}
/**
 * @brief 开始插值到编队位置
 * @note  🔧 修改 - 直接复用招募过渡逻辑，并确保 bRecruitMovementActive 生效
 *        这是编队更新补位的核心入口，必须能真正推动移动组件产生速度
 */
void UXBSoldierFollowComponent::StartInterpolateToFormation()
{
    // 为什么统一走招募过渡：能保证移动组件驱动、速度平滑、旋转混合一致
    SetCombatState(false);
    SetFollowMode(EXBFollowMode::RecruitTransition);

    // 🔧 修改 - 重置到达确认计时，避免上一轮残留造成顿挫
    ArrivedTimeAccumulator = 0.0f;

    // 直接启动（遵从 RecruitStartDelay 配置）
    StartRecruitTransition();

    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件：StartInterpolateToFormation -> 进入招募过渡补位（组件驱动）"));
}

void UXBSoldierFollowComponent::StartRecruitTransition()
{
    SetCombatState(false);
    SetFollowMode(EXBFollowMode::RecruitTransition);

    CurrentRecruitPhase = EXBRecruitTransitionPhase::Moving;

    if (DelayedRecruitStartHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(DelayedRecruitStartHandle);
    }

    bRecruitMovementActive = false;

    if (RecruitStartDelay > 0.0f)
    {
        GetWorld()->GetTimerManager().SetTimer(
            DelayedRecruitStartHandle,
            this,
            &UXBSoldierFollowComponent::StartRecruitTransition_Internal,
            RecruitStartDelay,
            false
        );
        return;
    }

    StartRecruitTransition_Internal();
}
/**
 * @brief 内部启动招募过渡（延迟结束后真正开始移动）
 * @note  🔧 修改 - 重置到达累积计时，避免上一次残留导致“刚开始就判定到位”引发顿挫
 */
void UXBSoldierFollowComponent::StartRecruitTransition_Internal()
{
    if (UWorld* World = GetWorld())
    {
        RecruitTransitionStartTime = World->GetTimeSeconds();
    }

    if (AActor* Owner = GetOwner())
    {
        LastPositionForStuckCheck = Owner->GetActorLocation();
    }

    AccumulatedStuckTime = 0.0f;

    // 🔧 修改 - 到达确认计时清零（非常关键）
    ArrivedTimeAccumulator = 0.0f;

    UCharacterMovementComponent* MoveComp = GetCachedMovementComponent();
    if (MoveComp)
    {
        // 为什么要保证 Walking：避免外部状态残留导致速度/摩擦/贴地不一致
        MoveComp->GravityScale = 1.0f;
        MoveComp->SetComponentTickEnabled(true);
        MoveComp->SetMovementMode(MOVE_Walking);

        // 速度缓存用于后续平滑
        SmoothedSpeedCache = MoveComp->MaxWalkSpeed;
    }

    if (CachedLeaderCharacter.IsValid())
    {
        bLeaderIsSprinting = CachedLeaderCharacter->IsSprinting();
        CachedLeaderSpeed = CachedLeaderCharacter->GetCurrentMoveSpeed();
    }

    bRecruitMovementActive = true;

    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件：开始招募过渡（延迟%.2fs已处理），到达计时已重置"), RecruitStartDelay);
}

FVector UXBSoldierFollowComponent::GetTargetPosition() const
{
    return GetSmoothedFormationTarget();
}

bool UXBSoldierFollowComponent::IsAtFormationPosition() const
{
    return GetDistanceToFormation() <= ArrivalThreshold;
}

float UXBSoldierFollowComponent::GetDistanceToFormation() const
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return 0.0f;
    }

    FVector TargetPos = CalculateFormationWorldPosition();
    return FVector::Dist2D(Owner->GetActorLocation(), TargetPos);
}
/**
 * @brief 移动到目标位置（只控制XY，Z保持当前值）
 * @param TargetPosition 目标位置
 * @param DeltaTime 帧间隔
 * @param MoveSpeed 移动速度
 * @return 是否已到达
 * @note  Z 由 CharacterMovement 的重力/贴地负责，这里不手动改Z，避免上下抖动与穿插地面
 */
bool UXBSoldierFollowComponent::MoveTowardsTargetXY(const FVector& TargetPosition, float DeltaTime, float MoveSpeed)
{
    AActor* Owner = GetOwner();
    if (!Owner || DeltaTime <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    // 为什么只处理XY：跟随/编队是平面队列逻辑，Z交给物理/导航贴地可避免台阶与坡面抖动
    const FVector CurrentPosition = Owner->GetActorLocation();

    const FVector2D CurrentXY(CurrentPosition.X, CurrentPosition.Y);
    const FVector2D TargetXY(TargetPosition.X, TargetPosition.Y);

    FVector2D Direction = TargetXY - CurrentXY;
    const float Distance = Direction.Size();

    // 到达阈值内直接对齐XY（不动Z）
    if (Distance <= ArrivalThreshold)
    {
        Owner->SetActorLocation(FVector(TargetXY.X, TargetXY.Y, CurrentPosition.Z));
        return true;
    }

    Direction.Normalize();

    const float MoveDistance = MoveSpeed * DeltaTime;

    FVector2D NewXY;
    if (MoveDistance >= Distance)
    {
        NewXY = TargetXY;
    }
    else
    {
        NewXY = CurrentXY + Direction * MoveDistance;
    }

    // 只改XY：Z保持不变，避免“人为写Z”与移动组件贴地修正打架造成抖动
    Owner->SetActorLocation(FVector(NewXY.X, NewXY.Y, CurrentPosition.Z));

    return MoveDistance >= Distance;
}

/**
 * @brief 是否需要强制传送
 * @return 是否应该传送
 * @note  当前默认 bAllowTeleportDuringRecruit=false，因此通常不会触发（保持你原项目策略）
 */
bool UXBSoldierFollowComponent::ShouldForceTeleport() const
{
    AActor* Owner = GetOwner();
    UWorld* World = GetWorld();

    if (!Owner || !World || !bAllowTeleportDuringRecruit)
    {
        return false;
    }

    const float CurrentTime = World->GetTimeSeconds();

    // 距离过远：说明追赶成本太高，传送回编队避免掉队
    const float Distance = GetDistanceToFormation();
    if (Distance > ForceTeleportDistance)
    {
        return true;
    }

    // 超时：长时间追不上，避免永远拖尾
    const float ElapsedTime = CurrentTime - RecruitTransitionStartTime;
    if (RecruitTransitionTimeout > 0.0f && ElapsedTime > RecruitTransitionTimeout)
    {
        return true;
    }

    // 卡住：低速持续过久说明可能被碰撞/卡位阻塞
    if (StuckDetectionTime > 0.0f && AccumulatedStuckTime > StuckDetectionTime)
    {
        return true;
    }

    return false;
}

/**
 * @brief 执行强制传送
 * @note  传送后直接切换到 Locked，确保立刻稳定贴合编队
 */
void UXBSoldierFollowComponent::PerformForceTeleport()
{
    UE_LOG(LogXBSoldier, Warning, TEXT("跟随组件：执行强制传送（距离/超时/卡住触发）"));

    TeleportToFormationPosition();
    SetFollowMode(EXBFollowMode::Locked);

    // 为什么要清理缓存：传送后不再是“追赶”语义，否则速度补偿可能造成瞬时加速
    bLeaderIsSprinting = false;
    CachedLeaderSpeed = 0.0f;

    OnRecruitTransitionCompleted.Broadcast();
}

/**
 * @brief 获取槽位本地偏移
 * @return 槽位LocalOffset（X向后、Y向左右）
 * @note  优先从 FormationComponent 读取（你的编队配置最权威）；没有则回退到简易网格
 */
FVector2D UXBSoldierFollowComponent::GetSlotLocalOffset() const
{
    // 优先从编队组件获取，确保与将领编队一致
    if (CachedFormationComponent.IsValid() && FormationSlotIndex != INDEX_NONE)
    {
        UXBFormationComponent* FormationComp = CachedFormationComponent.Get();
        if (FormationComp)
        {
            const TArray<FXBFormationSlot>& Slots = FormationComp->GetFormationSlots();
            if (Slots.IsValidIndex(FormationSlotIndex))
            {
                return Slots[FormationSlotIndex].LocalOffset;
            }
        }
    }

    // 回退：简易4列阵（避免没有编队组件时完全不可用）
    if (FormationSlotIndex >= 0)
    {
        const int32 Columns = 4;
        const int32 Row = FormationSlotIndex / Columns;
        const int32 Col = FormationSlotIndex % Columns;

        const float HorizontalSpacing = 100.0f;
        const float VerticalSpacing = 100.0f;
        const float MinDistanceToLeader = 150.0f;

        const float OffsetX = -(MinDistanceToLeader + Row * VerticalSpacing);
        const float OffsetY = (Col - (Columns - 1) * 0.5f) * HorizontalSpacing;

        return FVector2D(OffsetX, OffsetY);
    }

    return FVector2D::ZeroVector;
}

/**
 * @brief 获取指定XY位置的地面Z坐标
 * @param XYLocation XY位置
 * @param FallbackZ 检测失败时回退Z
 * @return 地面Z
 * @note  仅用于“传送”这类需要强制落地的场景；正常移动时不建议每帧打射线（成本高）
 */
float UXBSoldierFollowComponent::GetGroundHeightAtLocation(const FVector2D& XYLocation, float FallbackZ) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return FallbackZ;
    }

    const FVector TraceStart(XYLocation.X, XYLocation.Y, FallbackZ + 500.0f);
    const FVector TraceEnd(XYLocation.X, XYLocation.Y, FallbackZ - 1000.0f);

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.bTraceComplex = false;

    // 为什么要忽略自己/将领：避免射线打到角色胶囊或将领胶囊导致“地面高度”错误
    if (AActor* Owner = GetOwner())
    {
        QueryParams.AddIgnoredActor(Owner);
    }
    if (FollowTargetRef.IsValid())
    {
        QueryParams.AddIgnoredActor(FollowTargetRef.Get());
    }

    const bool bHit = World->LineTraceSingleByChannel(
        HitResult,
        TraceStart,
        TraceEnd,
        ECC_WorldStatic,
        QueryParams
    );

    if (bHit)
    {
        return HitResult.Location.Z;
    }

    return FallbackZ;
}