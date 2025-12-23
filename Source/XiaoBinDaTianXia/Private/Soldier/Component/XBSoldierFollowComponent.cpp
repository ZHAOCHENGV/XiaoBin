/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Soldier/Component/XBSoldierFollowComponent.cpp

/**
 * @file XBSoldierFollowComponent.cpp
 * @brief 士兵跟随组件实现 - 实时锁定槽位
 * 
 * @note 🔧 修改记录:
 *       1. 🔧 修改 UpdateLockedMode() 每帧直接设置位置
 *       2. ❌ 删除 插值模式（合并到锁定模式）
 *       3. ❌ 删除 速度计算逻辑
 *       4. 🔧 简化 整体代码结构
 *       5. ✨ 新增 将领速度感知，招募过渡时同步将领移动速度
 *       6. ✨ 新增 CalculateRecruitTransitionSpeed() 动态速度计算
 *       7. ✨ 新增 招募过渡分为移动阶段和对齐阶段
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

    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件初始化 - 实时锁定槽位模式，追赶补偿倍率: %.2f"), CatchUpSpeedMultiplier);
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

    // 更新幽灵目标，平滑跟随将领旋转与位置
    UpdateGhostTarget(DeltaTime);

    // 每帧更新将领速度缓存（用于招募过渡模式）
    if (bSyncLeaderSprint && CurrentMode == EXBFollowMode::RecruitTransition)
    {
        CachedLeaderSpeed = GetLeaderCurrentSpeed();
        
        // 检测将领冲刺状态
        if (CachedLeaderCharacter.IsValid())
        {
            bLeaderIsSprinting = CachedLeaderCharacter->IsSprinting();
        }
    }

    FVector PreUpdateLocation = Owner->GetActorLocation();

    // 根据模式更新
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

    // 计算实际移动速度
    if (DeltaTime > KINDA_SMALL_NUMBER)
    {
        FVector CurrentLocation = Owner->GetActorLocation();
        FVector Delta = CurrentLocation - PreUpdateLocation;
        Delta.Z = 0.0f;
        CurrentMoveSpeed = Delta.Size() / DeltaTime;
    }

    LastFrameLocation = Owner->GetActorLocation();
}

// ==================== ✨ 新增：将领速度感知方法 ====================

/**
 * @brief 同步将领冲刺状态
 * @param bLeaderSprinting 将领是否正在冲刺
 * @param LeaderCurrentSpeed 将领当前移动速度
 * @note ✨ 新增 - 由士兵招募时调用，确保过渡期间速度同步
 */
void UXBSoldierFollowComponent::SyncLeaderSprintState(bool bLeaderSprinting, float LeaderCurrentSpeed)
{
    bLeaderIsSprinting = bLeaderSprinting;
    CachedLeaderSpeed = LeaderCurrentSpeed;

    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 同步将领状态 - 冲刺: %s, 速度: %.1f"), 
        bLeaderSprinting ? TEXT("是") : TEXT("否"), LeaderCurrentSpeed);
}

/**
 * @brief 获取将领当前移动速度
 * @return 将领速度，如果无法获取则返回0
 * @note ✨ 新增 - 实时获取将领速度用于追赶计算
 */
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

    // 优先使用 GetCurrentMoveSpeed()，这会返回实际的 MaxWalkSpeed
    return Leader->GetCurrentMoveSpeed();
}

/**
 * @brief 计算招募过渡时的实际移动速度
 * @param DistanceToTarget 到目标的距离
 * @return 计算后的移动速度
 * @note ✨ 新增 - 核心速度计算逻辑
 * 
 * 速度计算公式：
 * 1. 基础速度 = RecruitTransitionSpeed
 * 2. 距离加速 = 基础速度 × (1 + 距离/100 × (DistanceSpeedMultiplier - 1))
 * 3. 将领速度补偿 = 将领当前速度 × CatchUpSpeedMultiplier
 * 4. 最终速度 = max(距离加速, 将领速度补偿) ，但不超过 MaxTransitionSpeed
 */
float UXBSoldierFollowComponent::CalculateRecruitTransitionSpeed(float DistanceToTarget) const
{
    // 获取将领当前速度（作为速度上限参考）
    float LeaderSpeed = CachedLeaderSpeed;
    if (LeaderSpeed < KINDA_SMALL_NUMBER && CachedLeaderCharacter.IsValid())
    {
        LeaderSpeed = GetLeaderCurrentSpeed();
    }
    
    // 在接近范围内，直接使用将领速度（避免震荡）
    if (CloseSlowdownDistance > 0.0f && DistanceToTarget <= CloseSlowdownDistance)
    {
        // 计算接近程度（0=刚进入范围，1=到达槽位）
        float CloseAlpha = 1.0f - FMath::Clamp(DistanceToTarget / CloseSlowdownDistance, 0.0f, 1.0f);
        
        // 在接近范围内，速度逐渐趋近将领速度
        float TargetSpeed = FMath::Max(LeaderSpeed, MinTransitionSpeed);
        
        // 从追赶速度平滑过渡到将领速度
        float BaseApproachSpeed = RecruitTransitionSpeed;
        float FinalSpeed = FMath::Lerp(BaseApproachSpeed, TargetSpeed, CloseAlpha);
        
        // 确保不低于最小速度
        FinalSpeed = FMath::Max(FinalSpeed, MinTransitionSpeed);
        
        // 在非常接近时（距离<50），严格限制速度不超过将领速度
        if (DistanceToTarget < 50.0f && LeaderSpeed > KINDA_SMALL_NUMBER)
        {
            FinalSpeed = FMath::Min(FinalSpeed, LeaderSpeed * 1.1f);
        }
        
        return FinalSpeed;
    }

    // === 远距离追赶模式 ===
    
    // Step 1: 基础速度 + 距离加速
    const float NormalizedDistance = FMath::Clamp(DistanceToTarget / FMath::Max(ArrivalThreshold, 1.0f), 0.0f, 10.0f);
    float DistanceMultiplier = 1.0f + NormalizedDistance * DistanceSpeedMultiplier;
    DistanceMultiplier = FMath::Max(DistanceMultiplier, 1.0f);
    float DistanceBasedSpeed = RecruitTransitionSpeed * DistanceMultiplier;

    // Step 2: 将领速度补偿
    float LeaderBasedSpeed = 0.0f;
    if (bSyncLeaderSprint && LeaderSpeed > KINDA_SMALL_NUMBER)
    {
        LeaderBasedSpeed = LeaderSpeed * CatchUpSpeedMultiplier;
        
        if (bLeaderIsSprinting)
        {
            LeaderBasedSpeed *= 1.2f;
        }
    }

    // Step 3: 取两者的最大值
    float FinalSpeed = FMath::Max(DistanceBasedSpeed, LeaderBasedSpeed);

    // Step 4: 限制最大速度
    FinalSpeed = FMath::Clamp(FinalSpeed, MinTransitionSpeed, MaxTransitionSpeed);

    return FinalSpeed;
}

// ==================== ✨ 新增：旋转对齐检查 ====================

/**
 * @brief 检查当前朝向是否已对齐目标朝向
 * @param TargetRotation 目标朝向
 * @param ToleranceDegrees 容差角度
 * @return 是否已对齐
 */
bool UXBSoldierFollowComponent::IsRotationAligned(const FRotator& TargetRotation, float ToleranceDegrees) const
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return true;
    }

    FRotator CurrentRotation = Owner->GetActorRotation();
    
    // 只比较 Yaw 角度
    float YawDiff = FMath::Abs(FRotator::NormalizeAxis(CurrentRotation.Yaw - TargetRotation.Yaw));
    
    return YawDiff <= ToleranceDegrees;
}

// ==================== 🔧 修改：锁定模式 ====================

/**
 * @brief 更新锁定模式
 * @note 🔧 核心逻辑：使用可调速度与转向插值平滑贴合槽位
 */
void UXBSoldierFollowComponent::UpdateLockedMode(float DeltaTime)
{
    AActor* Owner = GetOwner();
    AActor* Leader = FollowTargetRef.Get();
    
    if (!Owner || !Leader || !IsValid(Leader))
    {
        return;
    }
    
    FRotator FormationRotation = CalculateFormationWorldRotation();
    FVector TargetPosition = GetSmoothedFormationTarget();
    FVector CurrentPosition = Owner->GetActorLocation();
    
    // 计算到槽位的距离
    float DistanceToSlot = FVector::Dist2D(CurrentPosition, TargetPosition);
    
    UCharacterMovementComponent* MoveComp = GetCachedMovementComponent();
    if (MoveComp)
    {
        // 根据距离动态调整速度
        float ActualMoveSpeed = LockedFollowMoveSpeed;
        
        // 获取将领速度
        float LeaderSpeed = 0.0f;
        if (CachedLeaderCharacter.IsValid())
        {
            LeaderSpeed = CachedLeaderCharacter->GetCurrentMoveSpeed();
        }
        
        // 在到达阈值内，严格匹配将领速度
        if (DistanceToSlot <= ArrivalThreshold * 2.0f)
        {
            ActualMoveSpeed = FMath::Max(LeaderSpeed, 100.0f);
        }
        else if (DistanceToSlot <= CloseSlowdownDistance)
        {
            float Alpha = DistanceToSlot / CloseSlowdownDistance;
            ActualMoveSpeed = FMath::Lerp(FMath::Max(LeaderSpeed, 100.0f), LockedFollowMoveSpeed, Alpha);
        }
        
        MoveComp->MaxWalkSpeed = ActualMoveSpeed;
        
        // 只有距离超过阈值才发起移动
        if (DistanceToSlot > ArrivalThreshold)
        {
            FVector MoveDir = (TargetPosition - CurrentPosition).GetSafeNormal2D();
            if (!MoveDir.IsNearlyZero())
            {
                MoveComp->AddInputVector(MoveDir);
            }
        }
    }
    
    // 旋转更新 - 锁定模式始终朝向队伍前方
    if (bFollowRotation)
    {
        FRotator TargetRotation = FormationRotation;
        FRotator NewRotation = FMath::RInterpTo(
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
 * @note 🔧 修改 - 分为移动阶段和对齐阶段
 *       移动阶段：朝向移动方向（槽位方向）
 *       对齐阶段：到达槽位后，转向队伍前方（将领朝向）
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

    // ✨ 新增 - 根据当前阶段分发处理
    if (CurrentRecruitPhase == EXBRecruitTransitionPhase::Aligning)
    {
        // 对齐阶段：只处理旋转
        UpdateAlignmentPhase(DeltaTime);
        return;
    }

    // === 移动阶段 ===
    
    FVector TargetPosition = GetSmoothedFormationTarget();
    FVector CurrentPosition = Owner->GetActorLocation();
    
    float Distance = FVector::Dist2D(CurrentPosition, TargetPosition);
    
    // 到达检测阈值
    float EffectiveArrivalThreshold = ArrivalThreshold;
    
    // 将领移动时，扩大到达阈值
    float LeaderSpeed = 0.0f;
    if (CachedLeaderCharacter.IsValid())
    {
        LeaderSpeed = CachedLeaderCharacter->GetCurrentMoveSpeed();
    }
    
    if (LeaderSpeed > 50.0f)
    {
        EffectiveArrivalThreshold = ArrivalThreshold * 1.5f;
    }
    
    // 🔧 修改 - 到达槽位后，进入对齐阶段而非直接切换到锁定模式
    if (Distance <= EffectiveArrivalThreshold)
    {
        UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 到达槽位，进入对齐阶段"));
        
        // ✨ 新增 - 切换到对齐阶段
        CurrentRecruitPhase = EXBRecruitTransitionPhase::Aligning;
        
        // 清理速度缓存
        bLeaderIsSprinting = false;
        CachedLeaderSpeed = 0.0f;
        
        return;
    }
    
    // 计算动态速度
    float ActualSpeed = CalculateRecruitTransitionSpeed(Distance);
    ActualSpeed = FMath::Clamp(ActualSpeed, MinTransitionSpeed, MaxTransitionSpeed);
    
    if (SmoothedSpeedCache <= KINDA_SMALL_NUMBER)
    {
        SmoothedSpeedCache = ActualSpeed;
    }
    if (bUseSpeedSmoothing && SpeedSmoothingRate > 0.0f)
    {
        SmoothedSpeedCache = FMath::FInterpTo(SmoothedSpeedCache, ActualSpeed, DeltaTime, SpeedSmoothingRate);
        ActualSpeed = SmoothedSpeedCache;
    }
    
    // 使用移动组件进行移动
    UCharacterMovementComponent* MoveComp = GetCachedMovementComponent();
    ACharacter* CharOwner = Cast<ACharacter>(Owner);
    
    if (MoveComp && CharOwner && Distance > ArrivalThreshold)
    {
        MoveComp->MaxWalkSpeed = ActualSpeed;
        
        FVector MoveDirection = (TargetPosition - CurrentPosition).GetSafeNormal2D();
        
        if (!MoveDirection.IsNearlyZero())
        {
            CharOwner->AddMovementInput(MoveDirection, 1.0f);
            
            // 🔧 修改 - 移动阶段朝向移动方向（槽位方向），而不是队伍前方
            if (bFollowRotation)
            {
                FRotator CurrentRotation = Owner->GetActorRotation();
                // ✨ 核心修改 - 使用移动方向作为目标朝向
                FRotator TargetRotation = MoveDirection.Rotation();
                FRotator NewRotation = FMath::RInterpTo(
                    CurrentRotation, 
                    TargetRotation, 
                    DeltaTime, 
                    MoveDirectionRotationSpeed  // 使用移动时的转向速度
                );
                Owner->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
            }
        }
    }
    
    // 卡住检测
    FVector NewPosition = Owner->GetActorLocation();
    float MovedDistance = FVector::Dist2D(NewPosition, LastPositionForStuckCheck);
    
    if (DeltaTime > KINDA_SMALL_NUMBER)
    {
        float InstantSpeed = MovedDistance / DeltaTime;
        
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
}

/**
 * @brief 更新对齐阶段（到达槽位后转向队伍前方）
 * @param DeltaTime 帧间隔
 * @note ✨ 新增 - 核心逻辑：
 *       1. 到达槽位后停止位移
 *       2. 平滑转向队伍前方（将领朝向）
 *       3. 转向完成后切换到锁定模式
 */
void UXBSoldierFollowComponent::UpdateAlignmentPhase(float DeltaTime)
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    // 获取队伍前方朝向（将领朝向）
    FRotator FormationRotation = CalculateFormationWorldRotation();
    
    // 检查是否已对齐
    if (IsRotationAligned(FormationRotation, AlignmentToleranceDegrees))
    {
        // ✨ 对齐完成，切换到锁定模式
        UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 对齐完成，切换到锁定模式"));
        
        // 确保最终朝向精确
        Owner->SetActorRotation(FRotator(0.0f, FormationRotation.Yaw, 0.0f));
        
        // 切换模式
        SetFollowMode(EXBFollowMode::Locked);
        
        // 广播过渡完成事件
        OnRecruitTransitionCompleted.Broadcast();
        
        return;
    }
    
    // 持续转向队伍前方
    FRotator CurrentRotation = Owner->GetActorRotation();
    FRotator NewRotation = FMath::RInterpTo(
        CurrentRotation,
        FormationRotation,
        DeltaTime,
        AlignmentRotationSpeed
    );
    Owner->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
    
    // 对齐阶段也需要保持在槽位（将领可能在移动）
    FVector TargetPosition = GetSmoothedFormationTarget();
    FVector CurrentPosition = Owner->GetActorLocation();
    float Distance = FVector::Dist2D(CurrentPosition, TargetPosition);
    
    // 如果偏离槽位太远（将领移动导致），需要跟随
    if (Distance > ArrivalThreshold * 2.0f)
    {
        UCharacterMovementComponent* MoveComp = GetCachedMovementComponent();
        ACharacter* CharOwner = Cast<ACharacter>(Owner);
        
        if (MoveComp && CharOwner)
        {
            // 获取将领速度作为跟随速度
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

// ==================== ✨ 新增：幽灵目标插值 ====================

/**
 * @brief 更新幽灵目标（位置与旋转插值）
 * @param DeltaTime 帧间隔
 * @note 🔧 使用插值后的幽灵位置/朝向计算槽位，避免瞬转导致队伍扭曲
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

    FVector LeaderLocation = Leader->GetActorLocation();
    FRotator LeaderRotation = Leader->GetActorRotation();

    if (!bGhostInitialized)
    {
        GhostTargetLocation = LeaderLocation;
        GhostTargetRotation = LeaderRotation;
        bGhostInitialized = true;
        return;
    }

    // 使用插值让跟随更平滑
    GhostTargetLocation = FMath::VInterpTo(
        GhostTargetLocation,
        LeaderLocation,
        DeltaTime,
        GhostLocationInterpSpeed
    );

    GhostTargetRotation = FMath::RInterpTo(
        GhostTargetRotation,
        LeaderRotation,
        DeltaTime,
        GhostRotationInterpSpeed
    );

    // 直接缓存幽灵槽位世界坐标，供插值使用
    FVector2D SlotOffset = GetSlotLocalOffset();
    FVector LocalOffset3D(SlotOffset.X, SlotOffset.Y, 0.0f);
    FVector WorldOffset = GhostTargetRotation.RotateVector(LocalOffset3D);
    GhostSlotTargetLocation = GhostTargetLocation + WorldOffset;
}

/**
 * @brief 获取当前平滑后的编队目标位置
 * @note 优先返回幽灵槽位位置，失败时回退到即时计算
 */
FVector UXBSoldierFollowComponent::GetSmoothedFormationTarget() const
{
    if (bGhostInitialized && !GhostSlotTargetLocation.IsZero())
    {
        return GhostSlotTargetLocation;
    }

    return CalculateFormationWorldPosition();
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
            
            // 立即缓存将领状态
            bLeaderIsSprinting = CharTarget->IsSprinting();
            CachedLeaderSpeed = CharTarget->GetCurrentMoveSpeed();
        }
        
        UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 设置跟随目标 %s, 将领冲刺: %s, 速度: %.1f"), 
            *NewTarget->GetName(),
            bLeaderIsSprinting ? TEXT("是") : TEXT("否"),
            CachedLeaderSpeed);

        // 初始化幽灵目标
        GhostTargetLocation = NewTarget->GetActorLocation();
        GhostTargetRotation = NewTarget->GetActorRotation();
        bGhostInitialized = true;
        FVector2D SlotOffset = GetSlotLocalOffset();
        GhostSlotTargetLocation = GhostTargetLocation + GhostTargetRotation.RotateVector(FVector(SlotOffset.X, SlotOffset.Y, 0.0f));
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
    UE_LOG(LogXBSoldier, Verbose, TEXT("跟随组件: 设置槽位索引 %d"), SlotIndex);
}

// ==================== 组件缓存 ====================

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

// ==================== 碰撞控制 ====================

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
    // 移除RVO控制，保持默认关闭，避免跟随期间被避让干扰
    UCharacterMovementComponent* MoveComp = GetCachedMovementComponent();
    if (MoveComp)
    {
        MoveComp->SetAvoidanceEnabled(false);
    }
}

// ==================== 战斗状态控制 ====================

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

// ==================== 模式控制 ====================

void UXBSoldierFollowComponent::SetFollowMode(EXBFollowMode NewMode)
{
    if (CurrentMode == NewMode)
    {
        return;
    }
    
    EXBFollowMode OldMode = CurrentMode;
    CurrentMode = NewMode;
    
    // ✨ 新增 - 切换模式时重置招募过渡阶段
    if (NewMode != EXBFollowMode::RecruitTransition)
    {
        CurrentRecruitPhase = EXBRecruitTransitionPhase::Moving;
    }
    
    // 碰撞控制
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
    
    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 模式切换 %d -> %d"), 
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

    // 改为招募过渡移动回槽位，避免瞬移闪现
    SetFollowMode(EXBFollowMode::RecruitTransition);
    StartRecruitTransition();
}

/**
 * @brief 传送到编队位置
 * @note 🔧 修改 - 传送时进行地面检测，确保士兵贴地
 */
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
    
    // 检测目标XY位置的地面高度
    float GroundZ = GetGroundHeightAtLocation(
        FVector2D(TargetPos.X, TargetPos.Y),
        CurrentPos.Z
    );
    
    // 加上角色的半高（确保脚踩地面而不是陷入地面）
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
    
    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 传送到编队位置，地面Z=%.1f"), GroundZ);
}

/**
 * @brief 开始插值到编队位置
 * @note 🔧 修改 - 使用招募过渡实现平滑插值，不再瞬移
 */
void UXBSoldierFollowComponent::StartInterpolateToFormation()
{
    // 复用招募过渡逻辑，保证物理与碰撞正确
    SetCombatState(false);
    SetFollowMode(EXBFollowMode::RecruitTransition);

    if (UWorld* World = GetWorld())
    {
        RecruitTransitionStartTime = World->GetTimeSeconds();
    }

    if (AActor* Owner = GetOwner())
    {
        LastPositionForStuckCheck = Owner->GetActorLocation();
    }
    AccumulatedStuckTime = 0.0f;

    // 确保移动组件配置正确
    UCharacterMovementComponent* MoveComp = GetCachedMovementComponent();
    if (MoveComp)
    {
        MoveComp->GravityScale = 1.0f;
        MoveComp->SetComponentTickEnabled(true);
        MoveComp->SetMovementMode(MOVE_Walking);
        SmoothedSpeedCache = MoveComp->MaxWalkSpeed;
    }

    // 缓存将领状态，便于追赶
    if (CachedLeaderCharacter.IsValid())
    {
        bLeaderIsSprinting = CachedLeaderCharacter->IsSprinting();
        CachedLeaderSpeed = CachedLeaderCharacter->GetCurrentMoveSpeed();
    }

    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: StartInterpolateToFormation -> 进入平滑招募过渡"));
}

/**
 * @brief 开始招募过渡
 * @note 🔧 修改 - 确保移动组件正确配置，重置阶段状态
 */
void UXBSoldierFollowComponent::StartRecruitTransition()
{
    SetCombatState(false);
    SetFollowMode(EXBFollowMode::RecruitTransition);
    
    // ✨ 新增 - 重置招募过渡阶段为移动阶段
    CurrentRecruitPhase = EXBRecruitTransitionPhase::Moving;

    // 支持可配置启动延迟
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

    UCharacterMovementComponent* MoveComp = GetCachedMovementComponent();
    if (MoveComp)
    {
        MoveComp->GravityScale = 1.0f;
        MoveComp->SetComponentTickEnabled(true);
        MoveComp->SetMovementMode(MOVE_Walking);
        SmoothedSpeedCache = MoveComp->MaxWalkSpeed;
    }   

    if (CachedLeaderCharacter.IsValid())
    {
        bLeaderIsSprinting = CachedLeaderCharacter->IsSprinting();
        CachedLeaderSpeed = CachedLeaderCharacter->GetCurrentMoveSpeed();
    }

    // 开始真实移动
    bRecruitMovementActive = true;

    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 开始招募过渡 (延迟 %.2fs 已处理)"), RecruitStartDelay);
}

// ==================== 状态查询 ====================

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

// ==================== 移动实现 ====================

/**
 * @brief 移动到目标位置（只控制XY，Z保持当前值）
 * @note 🔧 修改 - 明确只修改XY，Z由物理引擎通过重力控制
 */
bool UXBSoldierFollowComponent::MoveTowardsTargetXY(const FVector& TargetPosition, float DeltaTime, float MoveSpeed)
{
    AActor* Owner = GetOwner();
    if (!Owner || DeltaTime <= KINDA_SMALL_NUMBER)
    {
        return false;
    }
    
    FVector CurrentPosition = Owner->GetActorLocation();
    
    FVector2D CurrentXY(CurrentPosition.X, CurrentPosition.Y);
    FVector2D TargetXY(TargetPosition.X, TargetPosition.Y);
    
    FVector2D Direction = TargetXY - CurrentXY;
    float Distance = Direction.Size();
    
    if (Distance <= ArrivalThreshold)
    {
        // 到达时也保持当前Z
        Owner->SetActorLocation(FVector(TargetXY.X, TargetXY.Y, CurrentPosition.Z));
        return true;
    }
    
    Direction.Normalize();
    float MoveDistance = MoveSpeed * DeltaTime;
    
    FVector2D NewXY;
    if (MoveDistance >= Distance)
    {
        NewXY = TargetXY;
    }
    else
    {
        NewXY = CurrentXY + Direction * MoveDistance;
    }
    
    // 只设置XY，Z保持不变（由移动组件的重力控制贴地）
    Owner->SetActorLocation(FVector(NewXY.X, NewXY.Y, CurrentPosition.Z));
    
    return MoveDistance >= Distance;
}

// ==================== 辅助方法 ====================

bool UXBSoldierFollowComponent::ShouldForceTeleport() const
{
    AActor* Owner = GetOwner();
    UWorld* World = GetWorld();
    
    if (!Owner || !World || !bAllowTeleportDuringRecruit)
    {
        return false;
    }
    
    float CurrentTime = World->GetTimeSeconds();
    
    // 距离太远
    float Distance = GetDistanceToFormation();
    if (Distance > ForceTeleportDistance)
    {
        return true;
    }
    
    // 超时
    float ElapsedTime = CurrentTime - RecruitTransitionStartTime;
    if (ElapsedTime > RecruitTransitionTimeout)
    {
        return true;
    }
    
    // 卡住
    if (AccumulatedStuckTime > StuckDetectionTime)
    {
        return true;
    }
    
    return false;
}

void UXBSoldierFollowComponent::PerformForceTeleport()
{
    UE_LOG(LogXBSoldier, Warning, TEXT("跟随组件: 执行强制传送"));
    
    TeleportToFormationPosition();
    SetFollowMode(EXBFollowMode::Locked);
    
    // 清理将领速度缓存
    bLeaderIsSprinting = false;
    CachedLeaderSpeed = 0.0f;
    
    OnRecruitTransitionCompleted.Broadcast();
}

// ==================== 计算方法 ====================

FVector UXBSoldierFollowComponent::CalculateFormationWorldPosition() const
{
    AActor* Leader = FollowTargetRef.Get();
    if (!Leader || !IsValid(Leader))
    {
        AActor* Owner = GetOwner();
        return Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
    }

    // 使用幽灵目标位置/旋转计算槽位
    FVector LeaderLocation = bGhostInitialized ? GhostTargetLocation : Leader->GetActorLocation();
    FRotator LeaderRotation = bGhostInitialized ? GhostTargetRotation : Leader->GetActorRotation();
    
    FVector2D SlotOffset = GetSlotLocalOffset();
    FVector LocalOffset3D(SlotOffset.X, SlotOffset.Y, 0.0f);
    FVector WorldOffset = LeaderRotation.RotateVector(LocalOffset3D);
    
    return LeaderLocation + WorldOffset;
}

FRotator UXBSoldierFollowComponent::CalculateFormationWorldRotation() const
{
    AActor* Leader = FollowTargetRef.Get();
    if (!Leader || !IsValid(Leader))
    {
        AActor* Owner = GetOwner();
        return Owner ? Owner->GetActorRotation() : FRotator::ZeroRotator;
    }
    
    // 使用幽灵目标旋转，避免瞬间转向
    FRotator LeaderRotation = bGhostInitialized ? GhostTargetRotation : Leader->GetActorRotation();
    return FRotator(0.0f, LeaderRotation.Yaw, 0.0f);
}

FVector2D UXBSoldierFollowComponent::GetSlotLocalOffset() const
{
    // 优先从编队组件获取
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
    
    // 回退：手动计算
    if (FormationSlotIndex >= 0)
    {
        int32 Columns = 4;
        int32 Row = FormationSlotIndex / Columns;
        int32 Col = FormationSlotIndex % Columns;
        
        float HorizontalSpacing = 100.0f;
        float VerticalSpacing = 100.0f;
        float MinDistanceToLeader = 150.0f;
        
        float OffsetX = -(MinDistanceToLeader + Row * VerticalSpacing);
        float OffsetY = (Col - (Columns - 1) * 0.5f) * HorizontalSpacing;
        
        return FVector2D(OffsetX, OffsetY);
    }
    
    return FVector2D::ZeroVector;
}

/**
 * @brief 获取指定XY位置的地面Z坐标
 * @param XYLocation XY位置
 * @param FallbackZ 检测失败时的回退Z值
 * @return 地面Z坐标
 * @note 使用射线检测从上往下找地面
 */
float UXBSoldierFollowComponent::GetGroundHeightAtLocation(const FVector2D& XYLocation, float FallbackZ) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return FallbackZ;
    }
    
    // 从高处向下发射射线
    FVector TraceStart = FVector(XYLocation.X, XYLocation.Y, FallbackZ + 500.0f);
    FVector TraceEnd = FVector(XYLocation.X, XYLocation.Y, FallbackZ - 1000.0f);
    
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.bTraceComplex = false;
    
    // 忽略自己和将领
    if (AActor* Owner = GetOwner())
    {
        QueryParams.AddIgnoredActor(Owner);
    }
    if (FollowTargetRef.IsValid())
    {
        QueryParams.AddIgnoredActor(FollowTargetRef.Get());
    }
    
    // 只检测静态世界几何体
    bool bHit = World->LineTraceSingleByChannel(
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
