// Source/XiaoBinDaTianXia/Public/Soldier/Component/XBSoldierFollowComponent.h

/**
 * @file XBSoldierFollowComponent.h
 * @brief 士兵跟随组件 - 紧密编队跟随模式
 * 
 * @note 🔧 修改记录:
 *       1. 新增 bIsInCombat 战斗状态变量
 *       2. 战斗中启用移动组件和RVO避障
 *       3. 非战斗时禁用移动组件，直接设置位置
 *       4. 招募过渡使用插值实时追踪目标位置
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
    /** @brief 锁定模式 - 完全跟随将领，位置实时同步 */
    Locked      UMETA(DisplayName = "锁定跟随"),
    
    /** @brief 插值模式 - 被阻挡后，平滑回编队位置 */
    Interpolating   UMETA(DisplayName = "插值中"),
    
    /** @brief 自由模式 - 战斗中，使用AI和移动组件 */
    Free        UMETA(DisplayName = "自由移动"),
    
    /** @brief 招募过渡模式 - 招募后插值移动到编队位置 */
    RecruitTransition   UMETA(DisplayName = "招募过渡")
};

// ✨ 新增 - 战斗状态变化委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatStateChangedDelegate, bool, bInCombat);

/**
 * @brief 士兵跟随组件
 * 
 * @note 核心逻辑:
 *       - 战斗中(bIsInCombat=true)：启用移动组件和RVO避障，使用AI逻辑
 *       - 非战斗(bIsInCombat=false)：禁用移动组件，直接设置位置
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

    // ==================== 战斗状态控制（✨ 新增） ====================

    /**
     * @brief 设置战斗状态
     * @param bInCombat 是否处于战斗中
     * @note 战斗中启用移动组件和RVO避障，非战斗时禁用
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Follow|Combat", meta = (DisplayName = "设置战斗状态"))
    void SetCombatState(bool bInCombat);

    /**
     * @brief 获取是否处于战斗中
     */
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

    UFUNCTION(BlueprintCallable, Category = "XB|Follow")
    void SetFollowInterpSpeed(float NewSpeed) { MovementSpeed = NewSpeed; }

    UFUNCTION(BlueprintCallable, Category = "XB|Follow")
    void SetInterpSpeed(float NewSpeed) { MovementSpeed = NewSpeed; }

    // ==================== 委托事件 ====================

    /** @brief 战斗状态变化委托 */
    UPROPERTY(BlueprintAssignable, Category = "XB|Follow|Combat", meta = (DisplayName = "战斗状态变化"))
    FOnCombatStateChangedDelegate OnCombatStateChanged;

protected:
    // ==================== 内部方法 ====================

    void UpdateLockedMode(float DeltaTime);
    void UpdateInterpolatingMode(float DeltaTime);
    void UpdateRecruitTransitionMode(float DeltaTime);

    FVector CalculateFormationWorldPosition() const;
    FRotator CalculateFormationWorldRotation() const;
    FVector2D GetSlotLocalOffset() const;

    /**
     * @brief 直接设置位置移动到目标（非战斗时使用）
     * @param TargetPosition 目标位置
     * @param DeltaTime 帧时间
     * @param MoveSpeed 移动速度
     * @return 是否已到达
     */
    bool MoveTowardsTargetDirect(const FVector& TargetPosition, float DeltaTime, float MoveSpeed);

    /**
     * @brief 使用插值移动到目标位置
     * @param TargetPosition 目标位置
     * @param DeltaTime 帧时间
     * @param InterpSpeed 插值速度
     * @return 是否已到达
     */
    bool MoveTowardsTargetInterp(const FVector& TargetPosition, float DeltaTime, float InterpSpeed);

    float GetLeaderMoveSpeed() const;

    UCharacterMovementComponent* GetCachedMovementComponent();
    UCapsuleComponent* GetCachedCapsuleComponent();

    /**
     * @brief 设置与其他士兵的碰撞状态
     */
    void SetSoldierCollisionEnabled(bool bEnableCollision);

    // ✨ 新增 - 移动组件控制
    /**
     * @brief 启用或禁用移动组件
     * @param bEnable 是否启用
     * @note 战斗时启用，非战斗时禁用
     */
    void SetMovementComponentEnabled(bool bEnable);

    /**
     * @brief 启用或禁用RVO避障
     * @param bEnable 是否启用
     */
    void SetRVOAvoidanceEnabled(bool bEnable);

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

    /** @brief 旋转插值速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Speed", meta = (DisplayName = "旋转插值速度", ClampMin = "1.0"))
    float RotationInterpolateSpeed = 15.0f;

    /** @brief 到达编队位置的阈值 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow", meta = (DisplayName = "到达阈值", ClampMin = "1.0"))
    float ArrivalThreshold = 30.0f;

    /** @brief 阻挡检测阈值 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow", meta = (DisplayName = "阻挡阈值", ClampMin = "10.0"))
    float BlockedThreshold = 150.0f;

    /** @brief 是否跟随将领旋转 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow", meta = (DisplayName = "跟随旋转"))
    bool bFollowRotation = true;

    /** @brief 移动速度（用于插值模式） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Speed", meta = (DisplayName = "移动速度", ClampMin = "100.0"))
    float MovementSpeed = 600.0f;

    /** @brief 招募过渡插值速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "招募过渡速度", ClampMin = "1.0", ClampMax = "50.0"))
    float RecruitTransitionSpeed = 8.0f;

    /** @brief 招募完成后是否自动切换到锁定模式 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "过渡完成后锁定"))
    bool bLockAfterRecruitTransition = true;

    /** @brief 过渡时禁用与其他士兵的碰撞 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "过渡时禁用碰撞"))
    bool bDisableCollisionDuringTransition = true;

    // ==================== 战斗状态（✨ 新增） ====================

    /** 
     * @brief 是否处于战斗中
     * @note 战斗中启用移动组件和RVO避障，非战斗时禁用
     */
    UPROPERTY(BlueprintReadOnly, Category = "XB|Follow|Combat", meta = (DisplayName = "是否战斗中"))
    bool bIsInCombat = false;

    // ==================== 状态 ====================

    /** @brief 上一帧位置（用于计算移动速度） */
    FVector LastFrameLocation = FVector::ZeroVector;

    /** @brief 当前移动速度（供动画蓝图读取） */
    UPROPERTY(BlueprintReadOnly, Category = "XB|Follow", meta = (DisplayName = "当前移动速度"))
    float CurrentMoveSpeed = 0.0f;

    /** @brief 记录原始碰撞响应 */
    ECollisionResponse OriginalPawnResponse = ECR_Block;
    bool bCollisionModified = false;

    /** @brief 记录移动组件原始状态 */
    bool bOriginalMovementEnabled = true;
    bool bMovementStateModified = false;
};
