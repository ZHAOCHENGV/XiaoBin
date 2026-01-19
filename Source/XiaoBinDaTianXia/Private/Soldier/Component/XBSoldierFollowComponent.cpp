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
#include "XBCollisionChannels.h"

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


/**
 * @brief 跟随组件Tick
 * @param DeltaTime 帧间隔
 * @param TickType Tick类型
 * @param ThisTickFunction Tick函数
 * @note  🔧 修改 - 在 Locked/RecruitTransition 下都更新“主将速度感知”
 *        以便 Locked 模式出现加速传播波（前后排交错）
 */
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

    // 死亡不更新
    if (AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(Owner))
    {
        if (Soldier->IsDead())
        {
            CurrentMoveSpeed = 0.0f;
            return;
        }
    }

    // 自由模式：不控制位置
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

    // 无目标
    if (!FollowTargetRef.IsValid())
    {
        CurrentMoveSpeed = 0.0f;
        LastFrameLocation = Owner->GetActorLocation();
        return;
    }

    // 更新幽灵目标（位置/槽位Yaw等）
    UpdateGhostTarget(DeltaTime);

    // 🔧 修改 - 无论 Locked 还是 RecruitTransition，都刷新将领状态缓存（用于速度感知）
    if (CachedLeaderCharacter.IsValid())
    {
        CachedLeaderSpeed = GetLeaderCurrentSpeed();
        bLeaderIsSprinting = CachedLeaderCharacter->IsSprinting();
    }

    // ✨ 新增 - 更新“速度传播波”的感知速度
    UpdateLeaderSpeedPerception(DeltaTime);

    const FVector PreUpdateLocation = Owner->GetActorLocation();

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

    // 计算实际移动速度（给动画用）
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
 * @brief 更新主将速度感知（传播波 - 加速上升沿触发）
 * @param DeltaTime 帧间隔
 * @note  🔧 修改 - 修复波纹可能“不触发”的情况：
 *        1) 保留“加速度上升沿触发”主逻辑（开始加速瞬间触发）
 *        2) ✨ 新增：速度变化阈值触发兜底（LeaderSpeedWaveTriggerThreshold）
 *           避免某些插值/网络/状态切换导致 Accel 不够大时，波纹永远不进入 Pending
 */
void UXBSoldierFollowComponent::UpdateLeaderSpeedPerception(float DeltaTime)
{
    if (!CachedLeaderCharacter.IsValid())
    {
        bLeaderSpeedWaveInitialized = false;
        bLeaderSpeedEventPending = false;

        InstantLeaderSpeed = 0.0f;
        PerceivedLeaderSpeed = 0.0f;

        PrevInstantLeaderSpeedForAccel = 0.0f;
        bWasLeaderAccelerating = false;

        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const float Now = World->GetTimeSeconds();

    // 主将瞬时速度（当前“指令速度”，你这边返回的是 MaxWalkSpeed）
    InstantLeaderSpeed = GetLeaderCurrentSpeed();

    // 初始化
    if (!bLeaderSpeedWaveInitialized)
    {
        PerceivedLeaderSpeed = InstantLeaderSpeed;
        PendingLeaderSpeed = InstantLeaderSpeed;

        PrevInstantLeaderSpeedForAccel = InstantLeaderSpeed;
        bWasLeaderAccelerating = false;
        LastAccelEventTime = Now;

        bLeaderSpeedWaveInitialized = true;
        bLeaderSpeedEventPending = false;

        bPrevLeaderSprintingForWave = bLeaderIsSprinting;

        CachedEstimatedColumns = GetEstimatedFormationColumns();
        CachedSlotsNumForColumns = CachedFormationComponent.IsValid()
            ? CachedFormationComponent->GetFormationSlots().Num()
            : 0;

        return;
    }

    // 如果禁用波纹：感知速度平滑贴近瞬时速度
    if (!bEnableLeaderSpeedWave)
    {
        PerceivedLeaderSpeed = (LeaderSpeedWaveApplyInterpRate > 0.0f)
            ? FMath::FInterpTo(PerceivedLeaderSpeed, InstantLeaderSpeed, DeltaTime, LeaderSpeedWaveApplyInterpRate)
            : InstantLeaderSpeed;

        PrevInstantLeaderSpeedForAccel = InstantLeaderSpeed;
        bWasLeaderAccelerating = false;
        return;
    }

    // ===================== ✨ 加速度上升沿检测 =====================

    float Accel = 0.0f;
    if (DeltaTime > KINDA_SMALL_NUMBER)
    {
        // 为什么用瞬时速度差：你主将加速是通过 MaxWalkSpeed 插值，dV/dt 在开始加速时会很大
        Accel = (InstantLeaderSpeed - PrevInstantLeaderSpeedForAccel) / DeltaTime;
    }
    PrevInstantLeaderSpeedForAccel = InstantLeaderSpeed;

    // Sprint 变化也视作“开始加速事件”（确保按键触发立刻有波纹）
    const bool bSprintChanged = (bLeaderIsSprinting != bPrevLeaderSprintingForWave);

    // 处于“加速中”判定（上升沿需要这个状态机）
    const bool bIsLeaderAccelerating = (Accel >= AccelStartThreshold);
    const bool bStopAccelerating = (Accel <= AccelStopThreshold);

    // 上升沿：从非加速 -> 加速
    const bool bAccelRisingEdge = (bTriggerWaveOnAccelStart && bIsLeaderAccelerating && !bWasLeaderAccelerating);

    // 触发冷却：避免插值抖动造成短时间重复触发
    const bool bCooldownOk = ((Now - LastAccelEventTime) >= AccelEventCooldown);

    // ✨ 新增 - 速度变化阈值触发兜底（你在 .h 暴露了参数，但原逻辑没用上）
    // 为什么：某些情况下加速度可能不够大，但速度差已经足够“肉眼可见”，此时也应该触发一次波纹
    const bool bSpeedDeltaTrigger =
        (LeaderSpeedWaveTriggerThreshold > 0.0f) &&
        (FMath::Abs(InstantLeaderSpeed - PerceivedLeaderSpeed) >= LeaderSpeedWaveTriggerThreshold);

    if (((bAccelRisingEdge && bCooldownOk) || (bSpeedDeltaTrigger && bCooldownOk)) || bSprintChanged)
    {
        // 🔧 修改 - 只在“开始加速瞬间”记录事件起点
        LeaderSpeedEventStartTime = Now;
        bLeaderSpeedEventPending = true;

        LastAccelEventTime = Now;

        // PendingLeaderSpeed 初始取当前（后续会持续更新）
        PendingLeaderSpeed = InstantLeaderSpeed;

        UE_LOG(LogXBSoldier, Log, TEXT("跟随组件：触发速度传播波；Accel=%.1f，速度差=%.1f，槽位=%d，延迟=%.3fs"),
            Accel,
            FMath::Abs(InstantLeaderSpeed - PerceivedLeaderSpeed),
            FormationSlotIndex,
            ComputeLeaderSpeedWaveDelay());
    }

    // 记录加速状态（用于下一帧上升沿检测）
    if (bStopAccelerating)
    {
        bWasLeaderAccelerating = false;
    }
    else if (bIsLeaderAccelerating)
    {
        bWasLeaderAccelerating = true;
    }

    bPrevLeaderSprintingForWave = bLeaderIsSprinting;

    // ===================== 🔧 波纹事件处理 =====================

    if (!bLeaderSpeedEventPending)
    {
        // 没有事件时，慢慢贴近主将速度（避免长期漂移）
        PerceivedLeaderSpeed = (LeaderSpeedWaveApplyInterpRate > 0.0f)
            ? FMath::FInterpTo(PerceivedLeaderSpeed, InstantLeaderSpeed, DeltaTime, LeaderSpeedWaveApplyInterpRate)
            : InstantLeaderSpeed;

        return;
    }

    // 🔧 修改 - 事件期间：PendingLeaderSpeed 持续跟随主将瞬时速度
    // 为什么：你要的是“开始加速触发波”，而不是“每个增量都重新触发”，因此不重置 StartTime，只更新目标值
    PendingLeaderSpeed = InstantLeaderSpeed;

    const float Delay = ComputeLeaderSpeedWaveDelay();
    const float Elapsed = Now - LeaderSpeedEventStartTime;

    if (Elapsed >= Delay)
    {
        // 到点后快速贴近（插值），前排先开始，后排后开始 → 交错感
        PerceivedLeaderSpeed = (LeaderSpeedWaveApplyInterpRate > 0.0f)
            ? FMath::FInterpTo(PerceivedLeaderSpeed, PendingLeaderSpeed, DeltaTime, LeaderSpeedWaveApplyInterpRate)
            : PendingLeaderSpeed;

        // 事件结束条件：主将不再加速 & 已基本跟上
        if (!bWasLeaderAccelerating && FMath::Abs(PerceivedLeaderSpeed - PendingLeaderSpeed) <= 15.0f)
        {
            bLeaderSpeedEventPending = false;
        }
    }
    // 未到点：保持当前感知速度不变 → “后排晚看到”的核心效果
}

/**
 * @brief 计算主将速度传播波的延迟（按槽位/行号推导）
 * @return 本士兵的触发延迟（秒）
 * @note   🔧 修改 - 修复“波纹无效果/延迟全部相同”的问题：
 *        原因：当 LeaderSpeedWaveMaxDelay 设置为 0.5 等较小值时，
 *              原逻辑直接 Min(Delay, MaxDelay) 会把大量槽位的延迟硬截断成同一个值，
 *              导致传播波失去层级差异（看起来像没有波纹）。
 *        修复：当设置了 MaxDelay 时，不做硬截断，而是把“原始延迟增量”根据编队规模归一化，
 *              映射到 [0, MaxDelay]，既能限制最大延迟，又能保留波纹梯度。
 */
float UXBSoldierFollowComponent::ComputeLeaderSpeedWaveDelay() const
{
   const int32 SlotIndex = FMath::Max(FormationSlotIndex, 0);

    // 1) 估算列数与行号
    const int32 Columns = FMath::Max(CachedEstimatedColumns, 1);
    const int32 RowIndex = SlotIndex / Columns;

    // 2) 原始“延迟增量”计算：按行或按槽位（取更大的那个，确保后排更晚）
    const float RawRowDelay = LeaderSpeedWaveDelayPerRow * static_cast<float>(RowIndex);
    const float RawSlotDelay = LeaderSpeedWaveDelayPerSlot * static_cast<float>(SlotIndex);
    float RawDelayAdd = FMath::Max(RawRowDelay, RawSlotDelay);

    // 3) 确定性抖动：每个士兵固定，避免每帧波动导致视觉噪声
    if (LeaderSpeedWaveRandomJitter > 0.0f)
    {
        RawDelayAdd += GetDeterministicRandom01() * LeaderSpeedWaveRandomJitter;
    }

    // 4) 🔧 修改 - MaxDelay 不再硬截断，而是归一化映射
    // 为什么：硬截断会让大量槽位全部变成同一个延迟（例如全是 0.5s）
    if (LeaderSpeedWaveMaxDelay > 0.0f)
    {
        // 编队规模推导：用槽位总数估算最大行号与最大槽位号
        int32 SlotCount = 0;
        if (CachedFormationComponent.IsValid())
        {
            const UXBFormationComponent* FormationComp = CachedFormationComponent.Get();
            if (FormationComp)
            {
                SlotCount = FormationComp->GetFormationSlots().Num();
            }
        }

        // 如果拿不到槽位数，退化为“保留原逻辑但做安全限制”
        if (SlotCount <= 0)
        {
            RawDelayAdd = FMath::Min(RawDelayAdd, LeaderSpeedWaveMaxDelay);
        }
        else
        {
            const int32 MaxSlotIndex = FMath::Max(SlotCount - 1, 1);
            const int32 MaxRowIndex = FMath::Max(MaxSlotIndex / Columns, 1);

            // 估算“理论最大延迟增量”（用于归一化）
            const float MaxByRow = LeaderSpeedWaveDelayPerRow * static_cast<float>(MaxRowIndex);
            const float MaxBySlot = LeaderSpeedWaveDelayPerSlot * static_cast<float>(MaxSlotIndex);
            const float MaxExpectedAdd = FMath::Max(FMath::Max(MaxByRow, MaxBySlot), KINDA_SMALL_NUMBER);

            // 归一化并映射到 [0, MaxDelay]
            const float Normalized = FMath::Clamp(RawDelayAdd / MaxExpectedAdd, 0.0f, 1.0f);
            RawDelayAdd = Normalized * LeaderSpeedWaveMaxDelay;
        }
    }

    const float FinalDelay = LeaderSpeedWaveBaseDelay + RawDelayAdd;
    return FMath::Max(FinalDelay, 0.0f);
}

/**
 * @brief 估算当前编队列数（用于由槽位序号推导行号）
 * @return 列数（>=1）
 * @note  为什么要估算：
 *        - FormationComponent 的“实际Columns”不是公开字段
 *        - 但 FormationSlots 中第一行槽位的 LocalOffset.X 相同，可用来推断列数
 *        - 只在槽位数变化时重新估算，避免每帧开销
 */
int32 UXBSoldierFollowComponent::GetEstimatedFormationColumns() const
{
    if (!CachedFormationComponent.IsValid())
    {
        return 4;
    }

    UXBFormationComponent* FormationComp = CachedFormationComponent.Get();
    if (!FormationComp)
    {
        return 4;
    }

    const TArray<FXBFormationSlot>& Slots = FormationComp->GetFormationSlots();
    if (Slots.Num() <= 0)
    {
        return 4;
    }

    // 第一行的 X 偏移是相同的，统计连续相同X的数量即列数
    const float FirstRowX = Slots[0].LocalOffset.X;
    const float Tolerance = 0.1f;

    int32 Columns = 0;
    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        if (FMath::Abs(Slots[i].LocalOffset.X - FirstRowX) <= Tolerance)
        {
            ++Columns;
        }
        else
        {
            break;
        }
    }

    return FMath::Max(Columns, 1);
}

/**
 * @brief 生成确定性随机抖动（每个士兵固定）
 * @return [0,1) 随机值
 * @note  为什么要确定性：
 *        - 同一士兵每次运行保持一致，便于调试与复现
 *        - 不依赖全局随机，避免多人联机/回放出现差异
 */
float UXBSoldierFollowComponent::GetDeterministicRandom01() const
{
    const AActor* Owner = GetOwner();
    const int32 Seed = Owner ? Owner->GetUniqueID() : 1;

    // 经典 hash->float：sin 哈希用于生成稳定伪随机
    const float S = FMath::Sin(static_cast<float>(Seed) * 12.9898f) * 43758.5453f;
    return FMath::Frac(S);
}

/**
 * @brief  锁定模式更新（严格跟随槽位）
 * @param  DeltaTime 帧间隔
 * @note   🔧 修改 - 朝向严格对齐主将朝向：
 *        原逻辑用 CalculateFormationWorldRotation()，在 GhostYaw 插值/限速时可能与主将Yaw不一致，
 *        导致士兵看起来“队形在走但脸没对齐主将”。
 *        修复：Locked 模式下，若启用 bFollowRotation，直接以 Leader->GetActorRotation().Yaw 作为目标Yaw。
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

    // 🔧 修改 - 速度来源使用“感知速度”（用于产生按行传播波纹）
    const float LeaderSpeedForThisSoldier = (bEnableLeaderSpeedWave ? PerceivedLeaderSpeed : InstantLeaderSpeed);

    // ==================== 误差归一化（用于输入与追赶速度缩放） ====================

    const float Deadzone = FMath::Max(LockedDeadzoneDistance, 0.0f);
    const float FullDist = FMath::Max(LockedFullInputDistance, Deadzone + 1.0f);
    const float ErrorAlpha = FMath::Clamp((DistanceToSlot - Deadzone) / (FullDist - Deadzone), 0.0f, 1.0f);

    // ==================== 波纹保护：延迟未到点时抑制追赶速度 ====================

    bool bHoldCatchUpForWave = false;
    if (bEnableLeaderSpeedWave && bLeaderSpeedEventPending)
    {
        if (UWorld* World = GetWorld())
        {
            const float Now = World->GetTimeSeconds();
            const float Delay = ComputeLeaderSpeedWaveDelay();
            const float Elapsed = Now - LeaderSpeedEventStartTime;

            if (Elapsed < Delay)
            {
                bHoldCatchUpForWave = true;
            }
        }
    }

    // ==================== 追赶额外速度（仅在偏离槽位时） ====================

    float CatchUpExtra = LockedCatchUpExtraSpeed * ErrorAlpha;

    if (bHoldCatchUpForWave)
    {
        CatchUpExtra = 0.0f;
    }

    const float DesiredSpeed = FMath::Max(LockedFollowMoveSpeed, LeaderSpeedForThisSoldier) + CatchUpExtra;

    const float NewMaxSpeed = (LockedSpeedInterpRate > 0.0f)
        ? FMath::FInterpTo(MoveComp->MaxWalkSpeed, DesiredSpeed, DeltaTime, LockedSpeedInterpRate)
        : DesiredSpeed;

    MoveComp->MaxWalkSpeed = NewMaxSpeed;

    // ==================== 位移：输入强度随误差缩放 ====================

    if (DistanceToSlot > Deadzone)
    {
        const FVector MoveDir = (TargetPosition - CurrentPosition).GetSafeNormal2D();
        const float InputScale = FMath::Clamp(ErrorAlpha, 0.15f, 1.0f);
        CharOwner->AddMovementInput(MoveDir, InputScale);
    }

    // ==================== 🔧 修改 - 朝向：严格对齐主将Yaw ====================

    if (bFollowRotation)
    {
        const float LeaderYaw = FRotator::NormalizeAxis(Leader->GetActorRotation().Yaw);

        const FRotator TargetRotation(0.0f, LeaderYaw, 0.0f);
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
/**
 * @brief 设置跟随目标
 * @param NewTarget 新目标
 * @note  🔧 修改 - 初始化加速度上升沿检测状态，避免切换目标后第一帧误触发波纹
 */
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

        // 初始化幽灵目标
        GhostTargetLocation = NewTarget->GetActorLocation();
        const float InitYaw = FRotator::NormalizeAxis(NewTarget->GetActorRotation().Yaw);
        GhostYawDegrees = InitYaw;
        GhostSlotYawDegrees = InitYaw;
        GhostTargetRotation = FRotator(0.0f, GhostYawDegrees, 0.0f);
        bGhostInitialized = true;

        const FVector2D SlotOffset = GetSlotLocalOffset();
        GhostSlotTargetLocation = GhostTargetLocation + FRotator(0.0f, GhostSlotYawDegrees, 0.0f).RotateVector(FVector(SlotOffset.X, SlotOffset.Y, 0.0f));

        // 初始化速度传播波
        bLeaderSpeedWaveInitialized = false;
        bLeaderSpeedEventPending = false;

        PendingLeaderSpeed = 0.0f;
        LeaderSpeedEventStartTime = 0.0f;
        bPrevLeaderSprintingForWave = bLeaderIsSprinting;

        // 🔧 修改 - 初始化加速度检测缓存
        InstantLeaderSpeed = GetLeaderCurrentSpeed();
        PerceivedLeaderSpeed = InstantLeaderSpeed;

        PrevInstantLeaderSpeedForAccel = InstantLeaderSpeed;
        bWasLeaderAccelerating = false;

        if (UWorld* World = GetWorld())
        {
            LastAccelEventTime = World->GetTimeSeconds();
        }
        else
        {
            LastAccelEventTime = -10000.0f;
        }

        CachedEstimatedColumns = GetEstimatedFormationColumns();
        CachedSlotsNumForColumns = CachedFormationComponent.IsValid()
            ? CachedFormationComponent->GetFormationSlots().Num()
            : 0;

        UE_LOG(LogXBSoldier, Log, TEXT("跟随组件：设置跟随目标=%s，已启用加速上升沿波纹；列数=%d"),
            *NewTarget->GetName(), CachedEstimatedColumns);
    }
    else
    {
        CachedFormationComponent = nullptr;
        CachedLeaderCharacter = nullptr;

        bLeaderIsSprinting = false;
        CachedLeaderSpeed = 0.0f;

        bGhostInitialized = false;
        GhostSlotTargetLocation = FVector::ZeroVector;

        bLeaderSpeedWaveInitialized = false;
        bLeaderSpeedEventPending = false;

        InstantLeaderSpeed = 0.0f;
        PerceivedLeaderSpeed = 0.0f;

        PrevInstantLeaderSpeedForAccel = 0.0f;
        bWasLeaderAccelerating = false;
        LastAccelEventTime = -10000.0f;
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
            Capsule->SetCollisionResponseToChannel(XBCollision::Soldier, OriginalSoldierResponse);
            bCollisionModified = false;
        }

        // 🔧 修改 - 战斗中开启士兵间阻挡，避免重叠
        if (bIsInCombat)
        {
            Capsule->SetCollisionResponseToChannel(XBCollision::Soldier, ECR_Block);
        }
    }
    else
    {
        if (!bCollisionModified)
        {
            OriginalPawnResponse = Capsule->GetCollisionResponseToChannel(ECC_Pawn);
            OriginalSoldierResponse = Capsule->GetCollisionResponseToChannel(XBCollision::Soldier);
            Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
            Capsule->SetCollisionResponseToChannel(XBCollision::Soldier, ECR_Overlap);
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



void UXBSoldierFollowComponent::SetCombatState(bool bInCombat)
{
    if (bIsInCombat == bInCombat)
    {
        return;
    }

    bIsInCombat = bInCombat;

    if (bInCombat)
    {
        // 🔧 修改 - 战斗时开启碰撞与避让，避免士兵重叠
        SetSoldierCollisionEnabled(true);


        // 🔧 修改 - 退出战斗后根据跟随模式恢复碰撞设置
        if (bDisableCollisionDuringTransition && CurrentMode == EXBFollowMode::RecruitTransition)
        {
            SetSoldierCollisionEnabled(false);
        }
        else
        {
            SetSoldierCollisionEnabled(true);
        }
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
