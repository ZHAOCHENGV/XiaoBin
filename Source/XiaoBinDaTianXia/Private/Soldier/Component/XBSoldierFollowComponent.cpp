/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Soldier/Component/XBSoldierFollowComponent.cpp

/**
 * @file XBSoldierFollowComponent.cpp
 * @brief 士兵跟随组件实现
 * 
 * @note 🔧 修改记录:
 *       1. 新增战斗状态控制（bIsInCombat）
 *       2. 战斗中启用移动组件和RVO避障
 *       3. 非战斗时禁用移动组件，直接设置位置
 *       4. 招募过渡使用插值实时追踪目标位置
 *       5. ✨ 新增 - 地面追踪功能，确保士兵贴地移动
 */

#include "Soldier/Component/XBSoldierFollowComponent.h"
#include "Utils/XBLogCategories.h"
#include "Character/XBCharacterBase.h"
#include "Character/Components/XBFormationComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetSystemLibrary.h"

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
    }
    
    SetMovementComponentEnabled(false);
    SetRVOAvoidanceEnabled(false);
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
            UE_LOG(LogXBSoldier, Verbose, TEXT("跟随组件: 恢复Pawn碰撞"));
        }
    }
    else
    {
        if (!bCollisionModified)
        {
            OriginalPawnResponse = Capsule->GetCollisionResponseToChannel(ECC_Pawn);
            Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
            bCollisionModified = true;
            UE_LOG(LogXBSoldier, Verbose, TEXT("跟随组件: 禁用Pawn碰撞"));
        }
    }
}

// ==================== 移动组件控制 ====================

void UXBSoldierFollowComponent::SetMovementComponentEnabled(bool bEnable)
{
    UCharacterMovementComponent* MoveComp = GetCachedMovementComponent();
    if (!MoveComp)
    {
        return;
    }
    
    if (bEnable)
    {
        if (bMovementStateModified)
        {
            MoveComp->SetComponentTickEnabled(true);
            MoveComp->SetMovementMode(MOVE_Walking);
            bMovementStateModified = false;
            UE_LOG(LogXBSoldier, Verbose, TEXT("跟随组件: 启用移动组件"));
        }
    }
    else
    {
        if (!bMovementStateModified)
        {
            bOriginalMovementEnabled = MoveComp->IsComponentTickEnabled();
            MoveComp->StopMovementImmediately();
            MoveComp->SetComponentTickEnabled(false);
            bMovementStateModified = true;
            UE_LOG(LogXBSoldier, Verbose, TEXT("跟随组件: 禁用移动组件"));
        }
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
    
    UE_LOG(LogXBSoldier, Verbose, TEXT("跟随组件: RVO避障 %s"), bEnable ? TEXT("启用") : TEXT("禁用"));
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
        SetMovementComponentEnabled(true);
        SetRVOAvoidanceEnabled(true);
        SetSoldierCollisionEnabled(true);
        
        UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 进入战斗状态，启用移动组件和RVO"));
    }
    else
    {
        SetMovementComponentEnabled(false);
        SetRVOAvoidanceEnabled(false);
        
        UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 退出战斗状态，禁用移动组件和RVO"));
    }
    
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
    
    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 模式切换 %d -> %d"), 
        static_cast<int32>(OldMode), static_cast<int32>(NewMode));
}

void UXBSoldierFollowComponent::EnterCombatMode()
{
    SetCombatState(true);
    SetFollowMode(EXBFollowMode::Free);
    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 进入战斗模式"));
}

void UXBSoldierFollowComponent::ExitCombatMode()
{
    SetCombatState(false);
    SetFollowMode(EXBFollowMode::Interpolating);
    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 退出战斗模式"));
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
    
    // ✨ 新增 - 应用地面追踪
    if (bEnableGroundTracking)
    {
        AdjustToGround(TargetPos);
    }
    
    Owner->SetActorLocation(TargetPos);
    
    if (bFollowRotation)
    {
        Owner->SetActorRotation(TargetRot);
    }
    
    UE_LOG(LogXBSoldier, Verbose, TEXT("跟随组件: 传送到编队位置 %s"), *TargetPos.ToString());
}

void UXBSoldierFollowComponent::StartInterpolateToFormation()
{
    SetFollowMode(EXBFollowMode::Interpolating);
    UE_LOG(LogXBSoldier, Verbose, TEXT("跟随组件: 开始插值回编队位置"));
}

void UXBSoldierFollowComponent::StartRecruitTransition()
{
    SetCombatState(false);
    SetFollowMode(EXBFollowMode::RecruitTransition);
    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 开始招募过渡，插值速度: %.1f"), RecruitTransitionSpeed);
}

void UXBSoldierFollowComponent::SetRecruitTransitionSpeed(float NewSpeed)
{
    RecruitTransitionSpeed = FMath::Clamp(NewSpeed, 1.0f, 50.0f);
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
    return FVector::Dist2D(Owner->GetActorLocation(), TargetPos);
}

// ==================== ✨ 新增：地面追踪实现 ====================

/**
 * @brief 获取指定位置的地面高度
 * @param InLocation 输入位置
 * @param OutGroundZ 输出的地面Z坐标
 * @return 是否成功找到地面
 * @note 使用射线检测从上往下查找地面
 */
bool UXBSoldierFollowComponent::GetGroundHeight(const FVector& InLocation, float& OutGroundZ) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    // 射线起点：从位置上方开始
    FVector TraceStart = InLocation + FVector(0.0f, 0.0f, GroundTraceStartOffset);
    // 射线终点：向下检测
    FVector TraceEnd = InLocation - FVector(0.0f, 0.0f, GroundTraceDistance);

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetOwner());
    QueryParams.bTraceComplex = false;

    // 使用 Visibility 通道检测地面
    bool bHit = World->LineTraceSingleByChannel(
        HitResult,
        TraceStart,
        TraceEnd,
        ECC_Visibility,
        QueryParams
    );

    if (bHit)
    {
        OutGroundZ = HitResult.ImpactPoint.Z;
        return true;
    }

    return false;
}

/**
 * @brief 将位置调整到地面上
 * @param InOutLocation 输入输出的位置
 * @note 确保角色始终贴地，考虑胶囊体半高偏移
 */
void UXBSoldierFollowComponent::AdjustToGround(FVector& InOutLocation) const
{
    float GroundZ = 0.0f;
    if (GetGroundHeight(InOutLocation, GroundZ))
    {
        float CapsuleHalfHeight = 88.0f; // 默认值
        
        // 直接获取，不使用缓存
        if (AActor* Owner = GetOwner())
        {
            if (ACharacter* CharOwner = Cast<ACharacter>(Owner))
            {
                if (UCapsuleComponent* Capsule = CharOwner->GetCapsuleComponent())
                {
                    CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
                }
            }
        }

        InOutLocation.Z = GroundZ + CapsuleHalfHeight;
    }
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

/**
 * @brief 直接设置位置移动到目标（非战斗时使用）
 * @param TargetPosition 目标位置
 * @param DeltaTime 帧时间
 * @param MoveSpeed 移动速度
 * @return 是否已到达
 * @note 🔧 修改 - 增加地面追踪
 */
bool UXBSoldierFollowComponent::MoveTowardsTargetDirect(const FVector& TargetPosition, float DeltaTime, float MoveSpeed)
{
    AActor* Owner = GetOwner();
    if (!Owner || DeltaTime <= KINDA_SMALL_NUMBER)
    {
        return false;
    }
    
    FVector CurrentPosition = Owner->GetActorLocation();
    FVector Direction = TargetPosition - CurrentPosition;
    
    Direction.Z = 0.0f;
    float Distance = Direction.Size();
    
    if (Distance <= ArrivalThreshold)
    {
        return true;
    }
    
    Direction.Normalize();
    float MoveDistance = MoveSpeed * DeltaTime;
    
    FVector NewPosition;
    if (MoveDistance >= Distance)
    {
        NewPosition = TargetPosition;
    }
    else
    {
        NewPosition = CurrentPosition + Direction * MoveDistance;
    }
    
    // ✨ 新增 - 应用地面追踪
    if (bEnableGroundTracking)
    {
        AdjustToGround(NewPosition);
    }
    else
    {
        // 保持原来的Z坐标
        NewPosition.Z = CurrentPosition.Z;
    }
    
    Owner->SetActorLocation(NewPosition);
    
    return MoveDistance >= Distance;
}

/**
 * @brief 使用插值移动到目标位置
 * @param TargetPosition 目标位置
 * @param DeltaTime 帧时间
 * @param InterpSpeed 插值速度
 * @return 是否已到达
 * @note 🔧 修改 - 增加地面追踪
 */
bool UXBSoldierFollowComponent::MoveTowardsTargetInterp(const FVector& TargetPosition, float DeltaTime, float InterpSpeed)
{
    AActor* Owner = GetOwner();
    if (!Owner || DeltaTime <= KINDA_SMALL_NUMBER)
    {
        return false;
    }
    
    FVector CurrentPosition = Owner->GetActorLocation();
    
    FVector CurrentXY = FVector(CurrentPosition.X, CurrentPosition.Y, 0.0f);
    FVector TargetXY = FVector(TargetPosition.X, TargetPosition.Y, 0.0f);
    
    float Distance = FVector::Dist(CurrentXY, TargetXY);
    
    if (Distance <= ArrivalThreshold)
    {
        return true;
    }
    
    FVector NewXY = FMath::VInterpTo(CurrentXY, TargetXY, DeltaTime, InterpSpeed);
    
    FVector NewPosition = FVector(NewXY.X, NewXY.Y, CurrentPosition.Z);
    
    // ✨ 新增 - 应用地面追踪
    if (bEnableGroundTracking)
    {
        AdjustToGround(NewPosition);
    }
    
    Owner->SetActorLocation(NewPosition);
    
    return false;
}

// ==================== 更新逻辑实现 ====================

/**
 * @brief 更新锁定模式
 * @param DeltaTime 帧时间
 * @note 🔧 修改 - 增加地面追踪
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
    
    // ✨ 新增 - 应用地面追踪
    if (bEnableGroundTracking)
    {
        AdjustToGround(TargetPosition);
    }
    else
    {
        FVector CurrentPosition = Owner->GetActorLocation();
        TargetPosition.Z = CurrentPosition.Z;
    }
    
    Owner->SetActorLocation(TargetPosition);
    
    if (bFollowRotation)
    {
        Owner->SetActorRotation(FRotator(0.0f, TargetRotation.Yaw, 0.0f));
    }
    
    FVector ActualPosition = Owner->GetActorLocation();
    float ActualDistance = FVector::Dist2D(ActualPosition, TargetPosition);
    
    if (ActualDistance > BlockedThreshold)
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("跟随组件: 锁定模式检测到阻挡，距离: %.1f"), ActualDistance);
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
    
    bool bArrived = MoveTowardsTargetDirect(TargetPosition, DeltaTime, MovementSpeed);
    
    if (bFollowRotation)
    {
        FRotator CurrentRotation = Owner->GetActorRotation();
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotationInterpolateSpeed);
        Owner->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
    }
    
    if (bArrived)
    {
        UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 插值完成，切换到锁定模式"));
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
    
    FVector TargetPosition = CalculateFormationWorldPosition();
    FRotator TargetRotation = CalculateFormationWorldRotation();
    
    bool bArrived = MoveTowardsTargetInterp(TargetPosition, DeltaTime, RecruitTransitionSpeed);
    
    if (bFollowRotation)
    {
        FRotator CurrentRotation = Owner->GetActorRotation();
        float FastRotationSpeed = RotationInterpolateSpeed * 1.5f;
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, FastRotationSpeed);
        Owner->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
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
    }
}

// ==================== 计算方法实现 ====================

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
    
    FVector FinalPosition = LeaderLocation + WorldOffset;
    
    return FinalPosition;
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
