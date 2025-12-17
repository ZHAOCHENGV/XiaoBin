/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Soldier/Component/XBSoldierFollowComponent.cpp

/**
 * @file XBSoldierFollowComponent.cpp
 * @brief 士兵跟随组件实现 - XY程序控制，Z轴物理控制
 * 
 * @note 🔧 修改记录:
 *       1. ❌ 删除 所有地面追踪代码
 *       2. 🔧 修改 MoveTowardsTargetXY() 只控制XY坐标
 *       3. 🔧 修改 移动组件保持启用以处理Z轴物理
 *       4. ✨ 新增 SetMovementMode() 控制行走/飞行模式
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
    
    // 🔧 修改 - 保持移动组件启用，让Z轴物理正常工作
    SetMovementMode(true);
    SetRVOAvoidanceEnabled(false);

    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件初始化 - Z轴由移动组件物理控制"));
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

    if (!FollowTargetRef.IsValid())
    {
        CurrentMoveSpeed = 0.0f;
        LastFrameLocation = Owner->GetActorLocation();
        return;
    }

    FVector PreUpdateLocation = Owner->GetActorLocation();

    switch (CurrentMode)
    {
    case EXBFollowMode::Locked:
        UpdateLockedMode(DeltaTime);
        break;
        
    case EXBFollowMode::Interpolating:
        UpdateInterpolatingMode(DeltaTime);
        break;
        
    case EXBFollowMode::RecruitTransition:
        UpdateRecruitTransitionMode(DeltaTime);
        break;
        
    default:
        break;
    }

    // 计算实际移动速度（只看XY）
    if (DeltaTime > KINDA_SMALL_NUMBER)
    {
        FVector CurrentLocation = Owner->GetActorLocation();
        FVector Delta = CurrentLocation - PreUpdateLocation;
        Delta.Z = 0.0f;
        float FrameDistance = Delta.Size();
        float CalculatedSpeed = FrameDistance / DeltaTime;
        
        CurrentMoveSpeed = FMath::FInterpTo(CurrentMoveSpeed, CalculatedSpeed, DeltaTime, 10.0f);
        
        if (CurrentMoveSpeed < 5.0f)
        {
            CurrentMoveSpeed = 0.0f;
        }
    }

    LastFrameLocation = Owner->GetActorLocation();
}

// ==================== 目标设置实现 ====================

void UXBSoldierFollowComponent::SetFollowTarget(AActor* NewTarget)
{
    FollowTargetRef = NewTarget;
    
    if (NewTarget)
    {
        if (AXBCharacterBase* CharTarget = Cast<AXBCharacterBase>(NewTarget))
        {
            CachedFormationComponent = CharTarget->GetFormationComponent();
        }
        
        UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 设置跟随目标 %s"), *NewTarget->GetName());
    }
    else
    {
        CachedFormationComponent = nullptr;
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

// ==================== 🔧 修改：移动模式控制 ====================

/**
 * @brief 设置移动组件的移动模式
 * @param bEnableWalking 是否启用行走模式
 * @note 🔧 核心修改 - 不再禁用移动组件，而是切换模式
 *       行走模式：启用重力和地面检测，Z轴由物理控制
 *       这样XY由我们程序控制，Z轴由引擎物理控制
 */
void UXBSoldierFollowComponent::SetMovementMode(bool bEnableWalking)
{
    UCharacterMovementComponent* MoveComp = GetCachedMovementComponent();
    if (!MoveComp)
    {
        return;
    }
    
    // 🔧 关键：始终保持移动组件启用
    MoveComp->SetComponentTickEnabled(true);
    
    if (bEnableWalking)
    {
        // 行走模式：启用重力，让角色贴地
        MoveComp->SetMovementMode(MOVE_Walking);
        MoveComp->GravityScale = 1.0f;
    }
    else
    {
        // 飞行模式：无重力（用于特殊情况）
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
    
    // 🔧 修改 - 始终保持行走模式，让Z轴物理工作
    SetMovementMode(true);
    
    OnCombatStateChanged.Broadcast(bInCombat);
}

// ==================== 模式控制实现 ====================

void UXBSoldierFollowComponent::SetFollowMode(EXBFollowMode NewMode)
{
    if (CurrentMode == NewMode)
    {
        return;
    }
    
    EXBFollowMode OldMode = CurrentMode;
    CurrentMode = NewMode;
    
    if (bDisableCollisionDuringTransition)
    {
        if (NewMode == EXBFollowMode::RecruitTransition || NewMode == EXBFollowMode::Interpolating)
        {
            SetSoldierCollisionEnabled(false);
        }
        else if (NewMode == EXBFollowMode::Locked && !bIsInCombat)
        {
            SetSoldierCollisionEnabled(true);
        }
    }
    
    // 🔧 修改 - 所有模式都保持行走模式，让Z轴物理正常
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
    SetFollowMode(EXBFollowMode::Interpolating);
}

/**
 * @brief 传送到编队位置
 * @note 🔧 修改 - 只设置XY，Z使用当前位置让物理自然处理
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
    
    // 🔧 核心修改 - 只改变XY，保持当前Z（让物理系统处理下落/贴地）
    FVector NewPosition = FVector(TargetPos.X, TargetPos.Y, CurrentPos.Z);
    
    Owner->SetActorLocation(NewPosition);
    
    if (bFollowRotation)
    {
        Owner->SetActorRotation(TargetRot);
    }
    
    LastPositionForStuckCheck = NewPosition;
    AccumulatedStuckTime = 0.0f;
    
    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 传送到编队位置 XY(%0.f, %.0f), Z保持%.0f"), 
        NewPosition.X, NewPosition.Y, NewPosition.Z);
}

void UXBSoldierFollowComponent::StartInterpolateToFormation()
{
    SetFollowMode(EXBFollowMode::Interpolating);
}

void UXBSoldierFollowComponent::StartRecruitTransition()
{
    SetCombatState(false);
    SetFollowMode(EXBFollowMode::RecruitTransition);
    
    if (UWorld* World = GetWorld())
    {
        RecruitTransitionStartTime = World->GetTimeSeconds();
        LastValidMoveTime = RecruitTransitionStartTime;
    }
    
    if (AActor* Owner = GetOwner())
    {
        LastPositionForStuckCheck = Owner->GetActorLocation();
    }
    AccumulatedStuckTime = 0.0f;
    
    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 开始招募过渡（Z轴由物理控制）"));
}

void UXBSoldierFollowComponent::SetRecruitTransitionSpeed(float NewSpeed)
{
    RecruitTransitionSpeed = FMath::Max(100.0f, NewSpeed);
}

// ==================== 状态查询实现 ====================

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
    // 只比较XY距离
    return FVector::Dist2D(Owner->GetActorLocation(), TargetPos);
}

// ==================== 追赶速度计算 ====================

float UXBSoldierFollowComponent::CalculateCatchUpSpeed(float Distance, float LeaderSpeed) const
{
    float BaseSpeed = FMath::Max(RecruitTransitionSpeed, LeaderSpeed + CatchUpExtraSpeed);
    
    if (Distance <= CatchUpAccelerationDistance)
    {
        return BaseSpeed;
    }
    
    float DistanceRatio = (Distance - CatchUpAccelerationDistance) / CatchUpAccelerationDistance;
    float SpeedMultiplier = 1.0f + FMath::Clamp(DistanceRatio, 0.0f, MaxCatchUpSpeedMultiplier - 1.0f);
    
    float FinalSpeed = BaseSpeed * SpeedMultiplier;
    float MaxSpeed = RecruitTransitionSpeed * MaxCatchUpSpeedMultiplier;
    
    return FMath::Min(FinalSpeed, MaxSpeed);
}

// ==================== 传送检测 ====================

bool UXBSoldierFollowComponent::ShouldForceTeleport() const
{
    AActor* Owner = GetOwner();
    UWorld* World = GetWorld();
    
    if (!Owner || !World)
    {
        return false;
    }
    
    float CurrentTime = World->GetTimeSeconds();
    
    float Distance = GetDistanceToFormation();
    if (Distance > ForceTeleportDistance)
    {
        return true;
    }
    
    float ElapsedTime = CurrentTime - RecruitTransitionStartTime;
    if (ElapsedTime > RecruitTransitionTimeout)
    {
        return true;
    }
    
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
    
    if (bLockAfterRecruitTransition)
    {
        SetFollowMode(EXBFollowMode::Locked);
    }
    else
    {
        SetFollowMode(EXBFollowMode::Interpolating);
    }
    
    OnRecruitTransitionCompleted.Broadcast();
}

// ==================== 辅助方法 ====================

float UXBSoldierFollowComponent::GetLeaderMoveSpeed() const
{
    AActor* Leader = FollowTargetRef.Get();
    if (!Leader)
    {
        return 0.0f;
    }
    
    if (AXBCharacterBase* LeaderChar = Cast<AXBCharacterBase>(Leader))
    {
        return LeaderChar->GetCurrentMoveSpeed();
    }
    
    if (ACharacter* LeaderCharacter = Cast<ACharacter>(Leader))
    {
        if (UCharacterMovementComponent* MoveComp = LeaderCharacter->GetCharacterMovement())
        {
            FVector Velocity = MoveComp->Velocity;
            Velocity.Z = 0.0f;
            return Velocity.Size();
        }
    }
    
    return 0.0f;
}

// ==================== 🔧 核心修改：只控制XY的移动 ====================

/**
 * @brief 移动到目标位置（只控制XY）
 * @param TargetPosition 目标位置
 * @param DeltaTime 帧时间
 * @param MoveSpeed 移动速度
 * @return 是否已到达（XY距离小于阈值）
 * @note 🔧 核心设计：
 *       - 只修改 Actor 的 XY 坐标
 *       - Z 坐标完全由 CharacterMovementComponent 处理
 *       - 这样角色会自然地受重力影响并贴合地面
 */
bool UXBSoldierFollowComponent::MoveTowardsTargetXY(const FVector& TargetPosition, float DeltaTime, float MoveSpeed)
{
    AActor* Owner = GetOwner();
    if (!Owner || DeltaTime <= KINDA_SMALL_NUMBER)
    {
        return false;
    }
    
    FVector CurrentPosition = Owner->GetActorLocation();
    
    // 只在XY平面计算方向和距离
    FVector2D CurrentXY(CurrentPosition.X, CurrentPosition.Y);
    FVector2D TargetXY(TargetPosition.X, TargetPosition.Y);
    
    FVector2D Direction = TargetXY - CurrentXY;
    float Distance = Direction.Size();
    
    // 已到达
    if (Distance <= ArrivalThreshold)
    {
        return true;
    }
    
    // 计算移动
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
    
    // 🔧 关键：只设置XY，Z保持不变（由移动组件物理控制）
    FVector NewPosition = FVector(NewXY.X, NewXY.Y, CurrentPosition.Z);
    Owner->SetActorLocation(NewPosition);
    
    return MoveDistance >= Distance;
}

// ==================== 更新逻辑实现 ====================

/**
 * @brief 更新锁定模式
 * @note 🔧 修改 - 只控制XY坐标
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
    FRotator TargetRotation = CalculateFormationWorldRotation();
    FVector CurrentPosition = Owner->GetActorLocation();
    
    // 🔧 核心修改 - 只设置XY，Z保持当前值
    FVector NewPosition = FVector(TargetPosition.X, TargetPosition.Y, CurrentPosition.Z);
    Owner->SetActorLocation(NewPosition);
    
    if (bFollowRotation)
    {
        Owner->SetActorRotation(FRotator(0.0f, TargetRotation.Yaw, 0.0f));
    }
    
    // 检测是否被阻挡
    FVector ActualPosition = Owner->GetActorLocation();
    float ActualDistance = FVector::Dist2D(ActualPosition, TargetPosition);
    
    if (ActualDistance > BlockedThreshold)
    {
        SetFollowMode(EXBFollowMode::Interpolating);
    }
}

void UXBSoldierFollowComponent::UpdateInterpolatingMode(float DeltaTime)
{
    AActor* Owner = GetOwner();
    AActor* Leader = FollowTargetRef.Get();
    
    if (!Owner || !Leader || !IsValid(Leader))
    {
        return;
    }
    
    FVector TargetPosition = CalculateFormationWorldPosition();
    FRotator TargetRotation = CalculateFormationWorldRotation();
    
    bool bArrived = MoveTowardsTargetXY(TargetPosition, DeltaTime, MovementSpeed);
    
    if (bFollowRotation)
    {
        FRotator CurrentRotation = Owner->GetActorRotation();
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotationInterpolateSpeed);
        Owner->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
    }
    
    if (bArrived)
    {
        SetFollowMode(EXBFollowMode::Locked);
    }
}

void UXBSoldierFollowComponent::UpdateRecruitTransitionMode(float DeltaTime)
{
    AActor* Owner = GetOwner();
    AActor* Leader = FollowTargetRef.Get();
    
    if (!Owner || !Leader || !IsValid(Leader))
    {
        return;
    }
    
    if (ShouldForceTeleport())
    {
        PerformForceTeleport();
        return;
    }
    
    FVector TargetPosition = CalculateFormationWorldPosition();
    FRotator TargetRotation = CalculateFormationWorldRotation();
    FVector CurrentPosition = Owner->GetActorLocation();
    
    float Distance = FVector::Dist2D(CurrentPosition, TargetPosition);
    float LeaderSpeed = GetLeaderMoveSpeed();
    
    float CatchUpSpeed = CalculateCatchUpSpeed(Distance, LeaderSpeed);
    
    // 🔧 使用只控制XY的移动方法
    bool bArrived = MoveTowardsTargetXY(TargetPosition, DeltaTime, CatchUpSpeed);
    
    if (bFollowRotation)
    {
        FRotator CurrentRotation = Owner->GetActorRotation();
        float FastRotationSpeed = RotationInterpolateSpeed * 1.5f;
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, FastRotationSpeed);
        Owner->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
    }
    
    // 卡住检测
    FVector NewPosition = Owner->GetActorLocation();
    float MovedDistance = FVector::Dist2D(NewPosition, LastPositionForStuckCheck);
    float CurrentSpeed = (DeltaTime > KINDA_SMALL_NUMBER) ? (MovedDistance / DeltaTime) : 0.0f;
    
    if (CurrentSpeed < StuckSpeedThreshold && Distance > ArrivalThreshold * 2.0f)
    {
        AccumulatedStuckTime += DeltaTime;
    }
    else
    {
        AccumulatedStuckTime = 0.0f;
        LastPositionForStuckCheck = NewPosition;
    }
    
    if (bArrived)
    {
        UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 招募过渡完成"));
        
        if (bLockAfterRecruitTransition)
        {
            SetFollowMode(EXBFollowMode::Locked);
        }
        else
        {
            SetFollowMode(EXBFollowMode::Interpolating);
        }
        
        OnRecruitTransitionCompleted.Broadcast();
    }
}

// ==================== 计算方法实现 ====================

/**
 * @brief 计算编队世界位置
 * @return 编队位置（XY有效，Z仅供参考）
 */
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
    
    // 返回完整位置，但调用者只使用XY
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
