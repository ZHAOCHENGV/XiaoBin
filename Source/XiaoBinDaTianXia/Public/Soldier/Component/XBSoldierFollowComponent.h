/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Soldier/Component/XBSoldierFollowComponent.h

/**
 * @file XBSoldierFollowComponent.h
 * @brief 士兵跟随组件 - 实时锁定槽位
 * 
 * @note 🔧 修改记录:
 *       1. 🔧 修改 锁定模式完全实时同步位置和旋转
 *       2. ❌ 删除 不必要的速度计算
 *       3. 🔧 简化 只保留必要的配置
 *       4. ✨ 新增 将领速度感知，招募过渡时同步将领移动速度
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
    Locked              UMETA(DisplayName = "锁定跟随"),
    Free                UMETA(DisplayName = "自由移动"),
    RecruitTransition   UMETA(DisplayName = "招募过渡")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatStateChangedDelegate, bool, bInCombat);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRecruitTransitionCompleted);

/**
 * @brief 士兵跟随组件
 * @note 核心设计：
 *       - 锁定模式：每帧直接设置位置到槽位，完全实时同步
 *       - 自由模式：战斗时脱离编队
 *       - 招募过渡：快速追赶到槽位，同步将领移动速度
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

    UFUNCTION(BlueprintCallable, Category = "XB|Follow", meta = (DisplayName = "开始招募过渡", ToolTip = "开始以招募模式跟随：可选延迟、使用冲刺/加速配置，避免瞬移。"))
    void StartRecruitTransition();

    /**
     * @brief 内部启动招募过渡
     * @note 内部使用，处理延迟后真正开始移动
     */
    void StartRecruitTransition_Internal();

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

    UFUNCTION(BlueprintCallable, Category = "XB|Follow")
    void SetFollowSpeed(float NewSpeed) { RecruitTransitionSpeed = NewSpeed; }

    // ✨ 新增 - 同步将领冲刺状态
    /**
     * @brief 通知将领冲刺状态变化
     * @param bLeaderSprinting 将领是否正在冲刺
     * @param LeaderCurrentSpeed 将领当前移动速度
     * @note 招募过渡时，士兵需要同步将领的移动速度才能追上
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Follow", meta = (DisplayName = "同步将领冲刺状态"))
    void SyncLeaderSprintState(bool bLeaderSprinting, float LeaderCurrentSpeed);

    // ==================== 委托事件 ====================

    UPROPERTY(BlueprintAssignable, Category = "XB|Follow|Combat", meta = (DisplayName = "战斗状态变化"))
    FOnCombatStateChangedDelegate OnCombatStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "XB|Follow", meta = (DisplayName = "招募过渡完成"))
    FOnRecruitTransitionCompleted OnRecruitTransitionCompleted;

protected:
    // ==================== 内部方法 ====================

    /**
     * @brief 更新锁定模式
     * @note 🔧 核心：直接设置位置和旋转，完全实时同步
     */
    void UpdateLockedMode(float DeltaTime);

    /**
     * @brief 更新招募过渡模式
     */
    void UpdateRecruitTransitionMode(float DeltaTime);

    /**
     * @brief 更新幽灵目标（位置与旋转插值）
     * @param DeltaTime 帧间隔
     * @note 🔧 使用插值后的幽灵位置/朝向计算槽位，避免瞬间转向导致摆尾过猛
     */
    void UpdateGhostTarget(float DeltaTime);

    /**
     * @brief 获取当前平滑后的编队目标位置
     * @note ✨ 优先使用幽灵目标对应的槽位位置，避免直接依赖将领位置导致堆叠
     */
    FVector GetSmoothedFormationTarget() const;

    /**
     * @brief 计算编队世界位置
     */
    FVector CalculateFormationWorldPosition() const;

    /**
     * @brief 计算编队世界旋转
     */
    FRotator CalculateFormationWorldRotation() const;

    /**
     * @brief 获取槽位本地偏移
     */
    FVector2D GetSlotLocalOffset() const;

    /**
     * @brief 获取指定XY位置的地面Z坐标
     * @param XYLocation XY位置
     * @param FallbackZ 检测失败时的回退Z值
     * @return 地面Z坐标
     */
    float GetGroundHeightAtLocation(const FVector2D& XYLocation, float FallbackZ) const;

    /**
     * @brief 移动到目标位置（只控制XY）
     */
    bool MoveTowardsTargetXY(const FVector& TargetPosition, float DeltaTime, float MoveSpeed);

    UCharacterMovementComponent* GetCachedMovementComponent();
    UCapsuleComponent* GetCachedCapsuleComponent();

    void SetSoldierCollisionEnabled(bool bEnableCollision);
    void SetMovementMode(bool bEnableWalking);
    void SetRVOAvoidanceEnabled(bool bEnable);

    bool ShouldForceTeleport() const;
    void PerformForceTeleport();

    // ✨ 新增 - 计算招募过渡时的实际移动速度
    /**
     * @brief 计算招募过渡时的实际移动速度
     * @param DistanceToTarget 到目标的距离
     * @return 计算后的移动速度
     * @note 综合考虑：基础速度 + 将领速度 + 距离加速 + 追赶补偿
     */
    float CalculateRecruitTransitionSpeed(float DistanceToTarget) const;

    // ✨ 新增 - 获取将领当前速度
    /**
     * @brief 获取将领当前移动速度
     * @return 将领速度，如果无法获取则返回0
     */
    float GetLeaderCurrentSpeed() const;

protected:
    // ==================== 引用 ====================

    UPROPERTY()
    TWeakObjectPtr<AActor> FollowTargetRef;

    UPROPERTY()
    TWeakObjectPtr<UXBFormationComponent> CachedFormationComponent;

    UPROPERTY()
    TWeakObjectPtr<AXBCharacterBase> CachedLeaderCharacter;

    UPROPERTY()
    TWeakObjectPtr<UCharacterMovementComponent> CachedMovementComponent;

    UPROPERTY()
    TWeakObjectPtr<UCapsuleComponent> CachedCapsuleComponent;

    // ==================== 配置 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow", meta = (DisplayName = "槽位索引"))
    int32 FormationSlotIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "XB|Follow", meta = (DisplayName = "当前模式"))
    EXBFollowMode CurrentMode = EXBFollowMode::RecruitTransition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow", meta = (DisplayName = "到达阈值", ClampMin = "1.0"))
    float ArrivalThreshold = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow", meta = (DisplayName = "跟随旋转"))
    bool bFollowRotation = true;

    // ==================== 招募过渡配置 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "招募移动基础速度", ClampMin = "100.0", ToolTip = "士兵开始追赶时的基础速度，过低会导致跟不上主将。"))
    float RecruitTransitionSpeed = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "距离加速倍率", ClampMin = "1.0", ClampMax = "8.0", ToolTip = "与主将距离越远速度越快，倍率越大加速越明显。"))
    float DistanceSpeedMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "最大追赶速度", ClampMin = "500.0", ToolTip = "士兵追赶时的速度上限，避免过快穿透。"))
    float MaxTransitionSpeed = 8000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "最小追赶速度", ClampMin = "0.0", ToolTip = "保证追赶时不低于此速度，避免调小基础速度后走得过慢。"))
    float MinTransitionSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "靠近减速距离", ClampMin = "0.0", ToolTip = "距离槽位小于该值时逐步降速，避免冲过槽位。"))
    float CloseSlowdownDistance = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "启用速度平滑", ToolTip = "开启后追赶速度会用插值平滑，减少忽快忽慢。"))
    bool bUseSpeedSmoothing = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "速度平滑插值率", ClampMin = "0.0", ToolTip = "追赶速度变化的平滑强度，越大越快贴近目标速度，0表示完全不平滑。"))
    float SpeedSmoothingRate = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "招募启动延迟(秒)", ClampMin = "0.0", ToolTip = "士兵开始奔向槽位前的延迟，默认0立即移动。"))
    float RecruitStartDelay = 0.0f;

    // ✨ 新增 - 招募转向速度（可蓝图调节）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "转向槽位插值速度", ClampMin = "0.1", ToolTip = "追赶过程中旋转对齐槽位的速度，越大越快朝向队列方向。"))
    float RecruitRotationInterpSpeed = 10.0f;

    // ✨ 新增 - 锁定模式移动速度（可蓝图调节，防止瞬移）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Locked", meta = (DisplayName = "锁定移动速度", ClampMin = "0.0", ToolTip = "锁定模式下的平移速度，过大可能导致抖动。"))
    float LockedFollowMoveSpeed = 600.0f;

    // ✨ 新增 - 锁定模式转向速度
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Locked", meta = (DisplayName = "锁定转向速度", ClampMin = "0.1", ToolTip = "锁定模式朝向槽位的旋转速度，越大越快面对队列方向。"))
    float LockedRotationInterpSpeed = 8.0f;

    // ✨ 新增 - 幽灵目标插值配置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Ghost", meta = (DisplayName = "幽灵位置插值速度", ClampMin = "0.1"))
    float GhostLocationInterpSpeed = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Ghost", meta = (DisplayName = "幽灵旋转插值速度", ClampMin = "0.1"))
    float GhostRotationInterpSpeed = 8.0f;

    // ✨ 新增 - 追赶补偿配置
    /**
     * @brief 追赶速度补偿倍率
     * @note 当将领移动时，士兵需要额外的速度来追赶
     *       公式：实际速度 = 基础速度 + 将领速度 × 补偿倍率
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "追赶主将速度倍率", ClampMin = "1.0", ClampMax = "5.0", ToolTip = "士兵追赶时会叠加主将当前速度×该倍率，倍率越大越容易追上冲刺中的主将。"))
    float CatchUpSpeedMultiplier = 1.5f;

    // ✨ 新增 - 冲刺同步配置
    /**
     * @brief 是否同步将领冲刺状态
     * @note 启用后，招募过渡时会检测将领是否冲刺并同步速度
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "同步主将冲刺", ToolTip = "开启后，士兵追赶时会读取主将的冲刺状态与速度，自动提速。"))
    bool bSyncLeaderSprint = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "追赶时禁用碰撞", ToolTip = "开启可减少追赶过程卡住，但可能穿模；关闭更物理真实。"))
    bool bDisableCollisionDuringTransition = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "强制传送距离", ClampMin = "500.0", ToolTip = "距离超过此值会直接传送回队列，过小可能产生瞬移感。"))
    float ForceTeleportDistance = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "追赶超时时间", ClampMin = "0.0", ToolTip = "超过该时间仍未到位会触发传送，设为0可关闭超时传送。"))
    float RecruitTransitionTimeout = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "卡住检测时间", ClampMin = "0.0", ToolTip = "连续低速超过该时间视为卡住，会触发传送或重新定位。"))
    float StuckDetectionTime = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "卡住速度阈值", ClampMin = "0.0", ToolTip = "低于该速度会累计卡住时间，设为0关闭卡住检测。"))
    float StuckSpeedThreshold = 50.0f;

    // ==================== 战斗状态 ====================

    UPROPERTY(BlueprintReadOnly, Category = "XB|Follow|Combat", meta = (DisplayName = "是否战斗中"))
    bool bIsInCombat = false;

    // ==================== ✨ 新增：将领状态缓存 ====================

    /** @brief 将领是否正在冲刺 */
    UPROPERTY(BlueprintReadOnly, Category = "XB|Follow", meta = (DisplayName = "将领正在冲刺"))
    bool bLeaderIsSprinting = false;

    /** @brief 将领当前速度（缓存值，用于速度计算） */
    UPROPERTY(BlueprintReadOnly, Category = "XB|Follow", meta = (DisplayName = "将领当前速度"))
    float CachedLeaderSpeed = 0.0f;

    // ==================== 状态 ====================

    FVector LastFrameLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "XB|Follow", meta = (DisplayName = "当前移动速度"))
    float CurrentMoveSpeed = 0.0f;

    // 速度平滑缓存（不暴露蓝图）
    float SmoothedSpeedCache = 0.0f;

    ECollisionResponse OriginalPawnResponse = ECR_Block;
    bool bCollisionModified = false;

    // 招募过渡状态追踪
    float RecruitTransitionStartTime = 0.0f;
    FVector LastPositionForStuckCheck = FVector::ZeroVector;
    float AccumulatedStuckTime = 0.0f;

    // ✨ 新增 - 幽灵目标状态
    FVector GhostTargetLocation = FVector::ZeroVector;
    FRotator GhostTargetRotation = FRotator::ZeroRotator;
    bool bGhostInitialized = false;
    FVector GhostSlotTargetLocation = FVector::ZeroVector;

    FTimerHandle DelayedRecruitStartHandle;
};
