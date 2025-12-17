/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Soldier/Component/XBSoldierFollowComponent.h

/**
 * @file XBSoldierFollowComponent.h
 * @brief 士兵跟随组件 - XY程序控制，Z轴物理控制
 * 
 * @note 🔧 修改记录:
 *       1. ❌ 删除 地面追踪相关代码
 *       2. 🔧 修改 移动逻辑只控制XY，Z轴由移动组件处理
 *       3. 🔧 修改 招募过渡时启用移动组件让物理生效
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "XBSoldierFollowComponent.generated.h"

class AXBCharacterBase;
class UXBFormationComponent;
class UCharacterMovementComponent;
class UCapsuleComponent;

/**
 * @brief 跟随模式枚举
 */
UENUM(BlueprintType)
enum class EXBFollowMode : uint8
{
    Locked      UMETA(DisplayName = "锁定跟随"),
    Interpolating   UMETA(DisplayName = "插值中"),
    Free        UMETA(DisplayName = "自由移动"),
    RecruitTransition   UMETA(DisplayName = "招募过渡")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatStateChangedDelegate, bool, bInCombat);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRecruitTransitionCompleted);

/**
 * @brief 士兵跟随组件
 * @note 核心设计：
 *       - XY轴：由本组件程序控制
 *       - Z轴：由CharacterMovementComponent物理控制
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName = "XB Soldier Follow"))
class XIAOBINDATIANXIA_API UXBSoldierFollowComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UXBSoldierFollowComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, 
        FActorComponentTickFunction* ThisTickFunction) override;

    // ==================== 目标设置 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Follow", meta = (DisplayName = "设置跟随目标"))
    void SetFollowTarget(AActor* NewTarget);

    UFUNCTION(BlueprintPure, Category = "XB|Follow", meta = (DisplayName = "获取跟随目标"))
    AActor* GetFollowTarget() const { return FollowTargetRef.Get(); }

    // ==================== 编队设置 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Follow", meta = (DisplayName = "设置槽位索引"))
    void SetFormationSlotIndex(int32 SlotIndex);

    UFUNCTION(BlueprintPure, Category = "XB|Follow", meta = (DisplayName = "获取槽位索引"))
    int32 GetFormationSlotIndex() const { return FormationSlotIndex; }

    // ==================== 模式控制 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Follow", meta = (DisplayName = "设置跟随模式"))
    void SetFollowMode(EXBFollowMode NewMode);

    UFUNCTION(BlueprintPure, Category = "XB|Follow", meta = (DisplayName = "获取跟随模式"))
    EXBFollowMode GetFollowMode() const { return CurrentMode; }

    UFUNCTION(BlueprintCallable, Category = "XB|Follow", meta = (DisplayName = "进入战斗"))
    void EnterCombatMode();

    UFUNCTION(BlueprintCallable, Category = "XB|Follow", meta = (DisplayName = "退出战斗"))
    void ExitCombatMode();

    UFUNCTION(BlueprintCallable, Category = "XB|Follow", meta = (DisplayName = "传送到编队位置"))
    void TeleportToFormationPosition();

    UFUNCTION(BlueprintCallable, Category = "XB|Follow", meta = (DisplayName = "插值到编队位置"))
    void StartInterpolateToFormation();

    UFUNCTION(BlueprintCallable, Category = "XB|Follow", meta = (DisplayName = "开始招募过渡"))
    void StartRecruitTransition();

    // ==================== 战斗状态控制 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Follow|Combat", meta = (DisplayName = "设置战斗状态"))
    void SetCombatState(bool bInCombat);

    UFUNCTION(BlueprintPure, Category = "XB|Follow|Combat", meta = (DisplayName = "是否战斗中"))
    bool IsInCombat() const { return bIsInCombat; }

    // ==================== 状态查询 ====================

    UFUNCTION(BlueprintPure, Category = "XB|Follow", meta = (DisplayName = "获取编队位置"))
    FVector GetTargetPosition() const;

    UFUNCTION(BlueprintPure, Category = "XB|Follow", meta = (DisplayName = "是否在编队位置"))
    bool IsAtFormationPosition() const;

    UFUNCTION(BlueprintPure, Category = "XB|Follow", meta = (DisplayName = "到编队位置距离"))
    float GetDistanceToFormation() const;

    UFUNCTION(BlueprintPure, Category = "XB|Follow", meta = (DisplayName = "是否招募过渡中"))
    bool IsInRecruitTransition() const { return CurrentMode == EXBFollowMode::RecruitTransition; }

    UFUNCTION(BlueprintPure, Category = "XB|Follow", meta = (DisplayName = "获取当前移动速度"))
    float GetCurrentMoveSpeed() const { return CurrentMoveSpeed; }

    // ==================== 速度设置 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Follow", meta = (DisplayName = "设置招募过渡速度"))
    void SetRecruitTransitionSpeed(float NewSpeed);

    UFUNCTION(BlueprintPure, Category = "XB|Follow", meta = (DisplayName = "获取招募过渡速度"))
    float GetRecruitTransitionSpeed() const { return RecruitTransitionSpeed; }

    UFUNCTION(BlueprintCallable, Category = "XB|Follow")
    void SetFollowSpeed(float NewSpeed) { MovementSpeed = NewSpeed; }

    // ==================== 委托事件 ====================

    UPROPERTY(BlueprintAssignable, Category = "XB|Follow|Combat", meta = (DisplayName = "战斗状态变化"))
    FOnCombatStateChangedDelegate OnCombatStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "XB|Follow", meta = (DisplayName = "招募过渡完成"))
    FOnRecruitTransitionCompleted OnRecruitTransitionCompleted;

protected:
    // ==================== 内部方法 ====================

    void UpdateLockedMode(float DeltaTime);
    void UpdateInterpolatingMode(float DeltaTime);
    void UpdateRecruitTransitionMode(float DeltaTime);

    FVector CalculateFormationWorldPosition() const;
    FRotator CalculateFormationWorldRotation() const;
    FVector2D GetSlotLocalOffset() const;

    /**
     * @brief 移动到目标位置（只控制XY）
     * @param TargetPosition 目标位置（只使用XY）
     * @param DeltaTime 帧时间
     * @param MoveSpeed 移动速度
     * @return 是否已到达
     * @note 🔧 核心修改 - 只设置XY，Z由移动组件处理
     */
    bool MoveTowardsTargetXY(const FVector& TargetPosition, float DeltaTime, float MoveSpeed);

    float GetLeaderMoveSpeed() const;

    UCharacterMovementComponent* GetCachedMovementComponent();
    UCapsuleComponent* GetCachedCapsuleComponent();

    void SetSoldierCollisionEnabled(bool bEnableCollision);
    
    /**
     * @brief 设置移动组件的移动模式
     * @param bEnableWalking 是否启用行走模式（启用重力和地面检测）
     * @note 🔧 修改 - 不再完全禁用组件，而是切换模式
     */
    void SetMovementMode(bool bEnableWalking);
    
    void SetRVOAvoidanceEnabled(bool bEnable);

    float CalculateCatchUpSpeed(float Distance, float LeaderSpeed) const;
    bool ShouldForceTeleport() const;
    void PerformForceTeleport();

protected:
    // ==================== 引用 ====================

    UPROPERTY()
    TWeakObjectPtr<AActor> FollowTargetRef;

    UPROPERTY()
    TWeakObjectPtr<UXBFormationComponent> CachedFormationComponent;

    UPROPERTY()
    TWeakObjectPtr<UCharacterMovementComponent> CachedMovementComponent;

    UPROPERTY()
    TWeakObjectPtr<UCapsuleComponent> CachedCapsuleComponent;

    // ==================== 配置 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow", meta = (DisplayName = "槽位索引"))
    int32 FormationSlotIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "XB|Follow", meta = (DisplayName = "当前模式"))
    EXBFollowMode CurrentMode = EXBFollowMode::RecruitTransition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Speed", meta = (DisplayName = "旋转插值速度", ClampMin = "1.0"))
    float RotationInterpolateSpeed = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow", meta = (DisplayName = "到达阈值", ClampMin = "1.0"))
    float ArrivalThreshold = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow", meta = (DisplayName = "阻挡阈值", ClampMin = "10.0"))
    float BlockedThreshold = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow", meta = (DisplayName = "跟随旋转"))
    bool bFollowRotation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Speed", meta = (DisplayName = "移动速度", ClampMin = "100.0"))
    float MovementSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "招募过渡基础速度", ClampMin = "100.0"))
    float RecruitTransitionSpeed = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "过渡完成后锁定"))
    bool bLockAfterRecruitTransition = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "过渡时禁用碰撞"))
    bool bDisableCollisionDuringTransition = true;

    // 追赶加速配置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "追赶加速距离", ClampMin = "100.0"))
    float CatchUpAccelerationDistance = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "最大追赶速度倍率", ClampMin = "1.0", ClampMax = "5.0"))
    float MaxCatchUpSpeedMultiplier = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "追赶额外速度", ClampMin = "0.0"))
    float CatchUpExtraSpeed = 200.0f;

    // 传送保护配置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "强制传送距离", ClampMin = "500.0"))
    float ForceTeleportDistance = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "过渡超时时间", ClampMin = "1.0"))
    float RecruitTransitionTimeout = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "卡住检测时间", ClampMin = "0.5"))
    float StuckDetectionTime = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "卡住速度阈值", ClampMin = "1.0"))
    float StuckSpeedThreshold = 30.0f;

    // ==================== 战斗状态 ====================

    UPROPERTY(BlueprintReadOnly, Category = "XB|Follow|Combat", meta = (DisplayName = "是否战斗中"))
    bool bIsInCombat = false;

    // ==================== 状态 ====================

    FVector LastFrameLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "XB|Follow", meta = (DisplayName = "当前移动速度"))
    float CurrentMoveSpeed = 0.0f;

    ECollisionResponse OriginalPawnResponse = ECR_Block;
    bool bCollisionModified = false;

    // 招募过渡状态追踪
    float RecruitTransitionStartTime = 0.0f;
    float LastValidMoveTime = 0.0f;
    FVector LastPositionForStuckCheck = FVector::ZeroVector;
    float AccumulatedStuckTime = 0.0f;
};
