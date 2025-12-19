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
    SetRVOAvoidanceEnabled(false);

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

    // ✨ 新增 - 每帧更新将领速度缓存（用于招募过渡模式）
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

    // 🔧 修改 - 优先使用 GetCurrentMoveSpeed()，这会返回实际的 MaxWalkSpeed
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
 * 
 * 这样确保：
 * - 将领静止时：士兵以距离加速的速度移动
 * - 将领移动时：士兵至少比将领快 CatchUpSpeedMultiplier 倍
 * - 将领冲刺时：士兵能够追上冲刺中的将领
 */
float UXBSoldierFollowComponent::CalculateRecruitTransitionSpeed(float DistanceToTarget) const
{
    // Step 1: 基础速度 + 距离加速
    float DistanceMultiplier = 1.0f + (DistanceToTarget / 100.0f) * (DistanceSpeedMultiplier - 1.0f);
    float DistanceBasedSpeed = RecruitTransitionSpeed * DistanceMultiplier;

    // Step 2: 将领速度补偿
    float LeaderBasedSpeed = 0.0f;
    if (bSyncLeaderSprint && CachedLeaderSpeed > KINDA_SMALL_NUMBER)
    {
        // 士兵需要比将领更快才能追上
        // 使用缓存的将领速度 × 补偿倍率
        LeaderBasedSpeed = CachedLeaderSpeed * CatchUpSpeedMultiplier;
        
        // 如果将领正在冲刺，额外增加速度确保能追上
        if (bLeaderIsSprinting)
        {
            // 冲刺时额外增加 20% 速度
            LeaderBasedSpeed *= 1.2f;
        }
    }

    // Step 3: 取两者的最大值
    float FinalSpeed = FMath::Max(DistanceBasedSpeed, LeaderBasedSpeed);

    // Step 4: 限制最大速度
    FinalSpeed = FMath::Min(FinalSpeed, MaxTransitionSpeed);

    return FinalSpeed;
}

// ==================== 🔧 修改：锁定模式 ====================

/**
 * @brief 更新锁定模式
 * @note 🔧 核心逻辑：每帧直接将士兵位置设置到槽位位置
 *       无论将领移动多快、旋转多快，士兵都实时跟随
 */
void UXBSoldierFollowComponent::UpdateLockedMode(float DeltaTime)
{
    AActor* Owner = GetOwner();
    AActor* Leader = FollowTargetRef.Get();
    
    if (!Owner || !Leader || !IsValid(Leader))
    {
        return;
    }
    
    FVector TargetPosition = CalculateFormationWorldPosition();
    FVector CurrentPosition = Owner->GetActorLocation();
    
    // 🔧 修改 - 保持士兵自身的Z坐标（由物理引擎控制贴地）
    Owner->SetActorLocation(FVector(TargetPosition.X, TargetPosition.Y, CurrentPosition.Z));
    
    if (bFollowRotation)
    {
        FRotator TargetRotation = CalculateFormationWorldRotation();
        Owner->SetActorRotation(TargetRotation);
    }
}

/**
 * @brief 更新招募过渡模式
 * @note 🔧 修改 - 使用 AI 移动而非手动设置位置，确保物理正确
 */
void UXBSoldierFollowComponent::UpdateRecruitTransitionMode(float DeltaTime)
{
    AActor* Owner = GetOwner();
    AActor* Leader = FollowTargetRef.Get();
    
    if (!Owner || !Leader || !IsValid(Leader))
    {
        return;
    }
    
    // 检查是否需要强制传送
    if (ShouldForceTeleport())
    {
        PerformForceTeleport();
        return;
    }
    
    FVector TargetPosition = CalculateFormationWorldPosition();
    FVector CurrentPosition = Owner->GetActorLocation();
    
    float Distance = FVector::Dist2D(CurrentPosition, TargetPosition);
    
    // 计算动态速度
    float ActualSpeed = CalculateRecruitTransitionSpeed(Distance);
    
    // 🔧 修改 - 使用移动组件进行移动
    UCharacterMovementComponent* MoveComp = GetCachedMovementComponent();
    ACharacter* CharOwner = Cast<ACharacter>(Owner);
    
    if (MoveComp && CharOwner && Distance > ArrivalThreshold)
    {
        // 设置移动速度
        MoveComp->MaxWalkSpeed = ActualSpeed;
        
        // 计算移动方向
        FVector MoveDirection = (TargetPosition - CurrentPosition).GetSafeNormal2D();
        
        if (!MoveDirection.IsNearlyZero())
        {
            // ✨ 核心 - 使用 AddMovementInput 驱动移动
            CharOwner->AddMovementInput(MoveDirection, 1.0f);
            
            // 平滑旋转
            if (bFollowRotation)
            {
                FRotator CurrentRotation = Owner->GetActorRotation();
                FRotator TargetRotation = MoveDirection.Rotation();
                FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RecruitRotationInterpSpeed);
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
    
    // 到达检测
    float CurrentDistance = FVector::Dist2D(Owner->GetActorLocation(), TargetPosition);
    if (CurrentDistance <= ArrivalThreshold)
    {
        UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 招募过渡完成，切换到锁定模式"));
        SetFollowMode(EXBFollowMode::Locked);
        
        bLeaderIsSprinting = false;
        CachedLeaderSpeed = 0.0f;
        
        if (bFollowRotation)
        {
            FRotator LeaderRotation = CalculateFormationWorldRotation();
            Owner->SetActorRotation(LeaderRotation);
        }
        
        OnRecruitTransitionCompleted.Broadcast();
    }
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
            
            // ✨ 新增 - 立即缓存将领状态
            bLeaderIsSprinting = CharTarget->IsSprinting();
            CachedLeaderSpeed = CharTarget->GetCurrentMoveSpeed();
        }
        
        UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 设置跟随目标 %s, 将领冲刺: %s, 速度: %.1f"), 
            *NewTarget->GetName(),
            bLeaderIsSprinting ? TEXT("是") : TEXT("否"),
            CachedLeaderSpeed);
    }
    else
    {
        CachedFormationComponent = nullptr;
        CachedLeaderCharacter = nullptr;
        bLeaderIsSprinting = false;
        CachedLeaderSpeed = 0.0f;
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
    UCharacterMovementComponent* MoveComp = GetCachedMovementComponent();
    if (!MoveComp)
    {
        return;
    }
    
    MoveComp->SetAvoidanceEnabled(bEnable);
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
        SetRVOAvoidanceEnabled(true);
        SetSoldierCollisionEnabled(true);
    }
    else
    {
        SetRVOAvoidanceEnabled(false);
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
    
    // 退出战斗后直接传送到槽位，然后锁定
    TeleportToFormationPosition();
    SetFollowMode(EXBFollowMode::Locked);
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
 * @note 🔧 兼容旧接口：由于现在使用实时锁定，直接传送并锁定
 */
void UXBSoldierFollowComponent::StartInterpolateToFormation()
{
    // 直接传送到槽位并锁定
    TeleportToFormationPosition();
    SetFollowMode(EXBFollowMode::Locked);
    
    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: StartInterpolateToFormation -> 直接锁定"));
}
/**
 * @brief 开始招募过渡
 * @note 🔧 修改 - 确保移动组件正确配置
 */
void UXBSoldierFollowComponent::StartRecruitTransition()
{
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
    
    // 确保移动组件正确配置
    UCharacterMovementComponent* MoveComp = GetCachedMovementComponent();
    if (MoveComp)
    {
        MoveComp->GravityScale = 1.0f;
        MoveComp->SetComponentTickEnabled(true);
        MoveComp->SetMovementMode(MOVE_Walking);
    }
    
    // 获取将领状态
    if (CachedLeaderCharacter.IsValid())
    {
        bLeaderIsSprinting = CachedLeaderCharacter->IsSprinting();
        CachedLeaderSpeed = CachedLeaderCharacter->GetCurrentMoveSpeed();
    }
    
    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 开始招募过渡"));
}

// ==================== 状态查询 ====================

FVector UXBSoldierFollowComponent::GetTargetPosition() const
{
    return CalculateFormationWorldPosition();
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
    
    if (!Owner || !World)
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
    
    // ✨ 新增 - 清理将领速度缓存
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
    
    FVector LeaderLocation = Leader->GetActorLocation();
    FRotator LeaderRotation = Leader->GetActorRotation();
    
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
    
    FRotator LeaderRotation = Leader->GetActorRotation();
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
