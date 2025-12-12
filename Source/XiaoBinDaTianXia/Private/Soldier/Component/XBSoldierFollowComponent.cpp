/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Soldier/Component/XBSoldierFollowComponent.cpp

/**
 * @file XBSoldierFollowComponent.cpp
 * @brief 士兵跟随组件实现（紧密编队模式）
 * 
 * @note 🔧 完全重写:
 *       1. 锁定模式：每帧同步位置，完全跟随将领
 *       2. 插值模式：被阻挡后平滑回位
 *       3. 自由模式：战斗中不干预
 */

#include "Soldier/Component/XBSoldierFollowComponent.h"
#include "Utils/XBLogCategories.h"
#include "Character/XBCharacterBase.h"
#include "Character/Components/XBFormationComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UXBSoldierFollowComponent::UXBSoldierFollowComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UXBSoldierFollowComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UXBSoldierFollowComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 自由模式不处理
    if (CurrentMode == EXBFollowMode::Free)
    {
        return;
    }

    // 没有跟随目标不处理
    if (!FollowTargetRef.IsValid())
    {
        return;
    }

    UpdateFollowing(DeltaTime);
}

// ==================== 目标设置实现 ====================

void UXBSoldierFollowComponent::SetFollowTarget(AActor* NewTarget)
{
    FollowTargetRef = NewTarget;
    
    // 缓存编队组件
    if (NewTarget)
    {
        if (AXBCharacterBase* CharTarget = Cast<AXBCharacterBase>(NewTarget))
        {
            CachedFormationComponent = CharTarget->GetFormationComponent();
        }
        
        // 记录初始位置
        LastLeaderLocation = NewTarget->GetActorLocation();
        LastLeaderRotation = NewTarget->GetActorRotation();
        
        // 标记需要刷新槽位偏移
        bNeedRefreshSlotOffset = true;
        
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
    bNeedRefreshSlotOffset = true;
    
    UE_LOG(LogXBSoldier, Verbose, TEXT("跟随组件: 设置槽位索引 %d"), SlotIndex);
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
    
    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 模式切换 %d -> %d"), 
        static_cast<int32>(OldMode), static_cast<int32>(NewMode));
}

void UXBSoldierFollowComponent::EnterCombatMode()
{
    SetFollowMode(EXBFollowMode::Free);
    
    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 进入战斗模式（自由移动）"));
}

void UXBSoldierFollowComponent::ExitCombatMode()
{
    // 立即传送回编队位置
    TeleportToFormationPosition();
    
    // 切换到锁定模式
    SetFollowMode(EXBFollowMode::Locked);
    
    UE_LOG(LogXBSoldier, Log, TEXT("跟随组件: 退出战斗模式（传送回编队）"));
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
    
    // 直接设置位置和旋转
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

// ==================== 更新逻辑实现 ====================

void UXBSoldierFollowComponent::UpdateFollowing(float DeltaTime)
{
    switch (CurrentMode)
    {
    case EXBFollowMode::Locked:
        UpdateLockedMode(DeltaTime);
        break;
        
    case EXBFollowMode::Interpolating:
        UpdateInterpolatingMode(DeltaTime);
        break;
        
    case EXBFollowMode::Free:
        // 自由模式不处理
        break;
    }
}

/**
 * @brief 更新锁定模式
 * @param DeltaTime 帧时间
 * @note 核心逻辑:
 *       1. 计算编队世界位置（将领位置 + 旋转后的偏移）
 *       2. 插值或直接设置士兵位置
 *       3. 同步将领旋转
 */
void UXBSoldierFollowComponent::UpdateLockedMode(float DeltaTime)
{
    AActor* Owner = GetOwner();
    AActor* Leader = FollowTargetRef.Get();
    
    if (!Owner || !Leader || !IsValid(Leader))
    {
        return;
    }
    
    // 计算目标位置和旋转
    FVector TargetPosition = CalculateFormationWorldPosition();
    FRotator TargetRotation = CalculateFormationWorldRotation();
    
    // 获取当前位置
    FVector CurrentPosition = Owner->GetActorLocation();
    FRotator CurrentRotation = Owner->GetActorRotation();
    
    // ==================== 位置更新 ====================
    
    if (bInterpolateInLockedMode)
    {
        // 使用插值平滑移动
        FVector NewPosition = FMath::VInterpTo(CurrentPosition, TargetPosition, DeltaTime, LockedModeInterpolateSpeed);
        Owner->SetActorLocation(NewPosition);
    }
    else
    {
        // 直接设置位置（完全同步）
        Owner->SetActorLocation(TargetPosition);
    }
    
    // ==================== 旋转更新 ====================
    
    if (bFollowRotation)
    {
        // 插值旋转
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotationInterpolateSpeed);
        Owner->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
    }
    
    // ==================== 阻挡检测 ====================
    
    // 检查是否被阻挡（实际位置与目标位置差距过大）
    // 注意：由于我们直接设置位置，正常情况下不会被阻挡
    // 但如果有物理碰撞阻止移动，可能会出现偏差
    float ActualDistance = FVector::Dist2D(Owner->GetActorLocation(), TargetPosition);
    if (ActualDistance > BlockedThreshold)
    {
        // 被阻挡了，切换到插值模式
        UE_LOG(LogXBSoldier, Warning, TEXT("跟随组件: 检测到阻挡，距离: %.1f，切换到插值模式"), ActualDistance);
        SetFollowMode(EXBFollowMode::Interpolating);
    }
    
    // 更新上一帧数据
    LastLeaderLocation = Leader->GetActorLocation();
    LastLeaderRotation = Leader->GetActorRotation();
}

/**
 * @brief 更新插值模式
 * @param DeltaTime 帧时间
 * @note 被阻挡后，平滑插值回编队位置
 */
void UXBSoldierFollowComponent::UpdateInterpolatingMode(float DeltaTime)
{
    AActor* Owner = GetOwner();
    AActor* Leader = FollowTargetRef.Get();
    
    if (!Owner || !Leader || !IsValid(Leader))
    {
        return;
    }
    
    // 计算目标位置和旋转
    FVector TargetPosition = CalculateFormationWorldPosition();
    FRotator TargetRotation = CalculateFormationWorldRotation();
    
    // 获取当前位置
    FVector CurrentPosition = Owner->GetActorLocation();
    FRotator CurrentRotation = Owner->GetActorRotation();
    
    // ==================== 位置插值 ====================
    
    FVector NewPosition = FMath::VInterpTo(CurrentPosition, TargetPosition, DeltaTime, InterpolateSpeed);
    Owner->SetActorLocation(NewPosition);
    
    // ==================== 旋转插值 ====================
    
    if (bFollowRotation)
    {
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotationInterpolateSpeed);
        Owner->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
    }
    
    // ==================== 到达检测 ====================
    
    float Distance = FVector::Dist2D(NewPosition, TargetPosition);
    if (Distance <= ArrivalThreshold)
    {
        // 到达编队位置，切换回锁定模式
        UE_LOG(LogXBSoldier, Verbose, TEXT("跟随组件: 到达编队位置，切换回锁定模式"));
        SetFollowMode(EXBFollowMode::Locked);
    }
    
    // 更新上一帧数据
    LastLeaderLocation = Leader->GetActorLocation();
    LastLeaderRotation = Leader->GetActorRotation();
}

// ==================== 计算方法实现 ====================

/**
 * @brief 计算编队世界位置
 * @return 世界坐标位置
 * @note 公式: 将领位置 + 将领旋转.RotateVector(槽位本地偏移)
 */
FVector UXBSoldierFollowComponent::CalculateFormationWorldPosition() const
{
    AActor* Leader = FollowTargetRef.Get();
    if (!Leader || !IsValid(Leader))
    {
        AActor* Owner = GetOwner();
        return Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
    }
    
    // 获取将领位置和旋转
    FVector LeaderLocation = Leader->GetActorLocation();
    FRotator LeaderRotation = Leader->GetActorRotation();
    
    // 获取槽位本地偏移
    FVector2D SlotOffset = GetSlotLocalOffset();
    
    // 转换为3D偏移（X=前后，Y=左右，Z=0）
    FVector LocalOffset3D(SlotOffset.X, SlotOffset.Y, 0.0f);
    
    // 应用将领旋转
    FVector WorldOffset = LeaderRotation.RotateVector(LocalOffset3D);
    
    // 计算最终位置（保持与将领相同的Z高度）
    FVector FinalPosition = LeaderLocation + WorldOffset;
    FinalPosition.Z = LeaderLocation.Z;
    
    return FinalPosition;
}

/**
 * @brief 计算编队世界旋转
 * @return 世界旋转
 * @note 士兵朝向与将领一致
 */
FRotator UXBSoldierFollowComponent::CalculateFormationWorldRotation() const
{
    AActor* Leader = FollowTargetRef.Get();
    if (!Leader || !IsValid(Leader))
    {
        AActor* Owner = GetOwner();
        return Owner ? Owner->GetActorRotation() : FRotator::ZeroRotator;
    }
    
    // 跟随将领的Yaw旋转
    FRotator LeaderRotation = Leader->GetActorRotation();
    return FRotator(0.0f, LeaderRotation.Yaw, 0.0f);
}

/**
 * @brief 检测是否被阻挡
 * @return 是否被阻挡
 */
bool UXBSoldierFollowComponent::IsBlockedFromFormation() const
{
    return GetDistanceToFormation() > BlockedThreshold;
}

/**
 * @brief 获取槽位本地偏移
 * @return 2D偏移（X=前后，Y=左右）
 */
FVector2D UXBSoldierFollowComponent::GetSlotLocalOffset() const
{
    // 如果有编队组件，从中获取
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
    
    // 默认偏移（如果没有编队组件）
    // 使用简单的行列计算
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