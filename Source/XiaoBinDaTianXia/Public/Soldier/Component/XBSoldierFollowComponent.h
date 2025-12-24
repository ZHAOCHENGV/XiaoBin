/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Soldier/Component/XBSoldierFollowComponent.h

/**
 * @file XBSoldierFollowComponent.h
 * @brief 士兵跟随组件 - 实时锁定槽位
 * @note  🔧 修改记录:
 *        1. 🔧 修复 GhostRotationInterpSpeed 过低导致的抖动：槽位位置计算与幽灵旋转解耦
 *        2. ✨ 新增 槽位使用即时Yaw/最小插值速度配置
 *        3. ✨ 新增 幽灵Yaw缓存（Yaw-only）用于角度安全插值
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "XBSoldierFollowComponent.generated.h"

class AXBCharacterBase;
class UXBFormationComponent;
class UCharacterMovementComponent;
class UCapsuleComponent;

UENUM(BlueprintType)
enum class EXBFollowMode : uint8
{
    Locked              UMETA(DisplayName = "锁定跟随"),
    Free                UMETA(DisplayName = "自由移动"),
    RecruitTransition   UMETA(DisplayName = "招募过渡")
};

UENUM(BlueprintType)
enum class EXBRecruitTransitionPhase : uint8
{
    Moving      UMETA(DisplayName = "移动中"),
    Aligning    UMETA(DisplayName = "对齐中")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatStateChangedDelegate, bool, bInCombat);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRecruitTransitionCompleted);

/**
 * @brief 士兵跟随组件
 * @note  核心设计：
 *       - 锁定模式：持续贴合槽位（走过去而非瞬移）
 *       - 自由模式：战斗中脱离编队
 *       - 招募过渡：追赶到槽位，随后对齐队伍朝向
 *       - 🔧 修复：槽位位置计算默认使用主将即时Yaw，避免低 GhostRotationInterpSpeed 引发抖动
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

    UFUNCTION(BlueprintPure, Category = "XB|Follow", meta = (DisplayName = "获取招募过渡阶段"))
    EXBRecruitTransitionPhase GetRecruitTransitionPhase() const { return CurrentRecruitPhase; }

    // ==================== 速度设置 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Follow")
    void SetFollowSpeed(float NewSpeed) { RecruitTransitionSpeed = NewSpeed; }

    UFUNCTION(BlueprintCallable, Category = "XB|Follow", meta = (DisplayName = "同步将领冲刺状态"))
    void SyncLeaderSprintState(bool bLeaderSprinting, float LeaderCurrentSpeed);

    // ==================== 委托事件 ====================

    UPROPERTY(BlueprintAssignable, Category = "XB|Follow|Combat", meta = (DisplayName = "战斗状态变化"))
    FOnCombatStateChangedDelegate OnCombatStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "XB|Follow", meta = (DisplayName = "招募过渡完成"))
    FOnRecruitTransitionCompleted OnRecruitTransitionCompleted;

protected:
    // ==================== 内部方法 ====================

    void UpdateLockedMode(float DeltaTime);
    void UpdateRecruitTransitionMode(float DeltaTime);
    void UpdateAlignmentPhase(float DeltaTime);

    void UpdateGhostTarget(float DeltaTime);
    FVector GetSmoothedFormationTarget() const;

    FVector CalculateFormationWorldPosition() const;
    FRotator CalculateFormationWorldRotation() const;

    FVector2D GetSlotLocalOffset() const;
    float GetGroundHeightAtLocation(const FVector2D& XYLocation, float FallbackZ) const;

    bool MoveTowardsTargetXY(const FVector& TargetPosition, float DeltaTime, float MoveSpeed);

    UCharacterMovementComponent* GetCachedMovementComponent();
    UCapsuleComponent* GetCachedCapsuleComponent();

    void SetSoldierCollisionEnabled(bool bEnableCollision);
    void SetMovementMode(bool bEnableWalking);
    void SetRVOAvoidanceEnabled(bool bEnable);

    bool ShouldForceTeleport() const;
    void PerformForceTeleport();

    float CalculateRecruitTransitionSpeed(float DistanceToTarget) const;
    float GetLeaderCurrentSpeed() const;

    bool IsRotationAligned(const FRotator& TargetRotation, float ToleranceDegrees = 5.0f) const;

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "招募允许传送", ToolTip = "关闭后招募/补位过程绝不传送，始终走路过去。"))
    bool bAllowTeleportDuringRecruit = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "移动时转向速度", ClampMin = "0.1", ToolTip = "追赶过程中朝向移动方向的旋转速度，越大越快朝向目标槽位。"))
    float MoveDirectionRotationSpeed = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "对齐阶段转向速度", ClampMin = "0.1", ToolTip = "到达槽位后，转向队伍前方的旋转速度。"))
    float AlignmentRotationSpeed = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "对齐容差角度", ClampMin = "1.0", ClampMax = "30.0", ToolTip = "朝向与队伍前方的角度差小于此值时，视为对齐完成。"))
    float AlignmentToleranceDegrees = 5.0f;

    // ==================== 锁定模式配置 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Locked", meta = (DisplayName = "锁定移动速度", ClampMin = "0.0", ToolTip = "锁定模式下的平移速度，过大可能导致抖动。"))
    float LockedFollowMoveSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Locked", meta = (DisplayName = "锁定转向速度", ClampMin = "0.1", ToolTip = "锁定模式朝向队伍前方的旋转速度，越大越快。"))
    float LockedRotationInterpSpeed = 8.0f;

    // ==================== 幽灵目标插值配置 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Ghost", meta = (DisplayName = "幽灵位置插值速度", ClampMin = "0.1"))
    float GhostLocationInterpSpeed = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Ghost", meta = (DisplayName = "幽灵旋转插值速度", ClampMin = "0.1"))
    float GhostRotationInterpSpeed = 8.0f;

    // 🔧 修改 - 抖动修复核心开关：槽位位置计算与幽灵旋转解耦
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Ghost",
        meta = (DisplayName = "槽位使用即时Yaw",
            ToolTip = "开启后：槽位位置计算使用将领即时Yaw（无旋转延迟），仅士兵朝向使用幽灵Yaw平滑。可彻底消除 GhostRotationInterpSpeed 过低导致的追逐抖动。"))
    bool bUseInstantLeaderYawForSlot = true;

    // ✨ 新增 - 当不使用即时Yaw时，给槽位Yaw一个最小插值速度防抖
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Ghost",
        meta = (DisplayName = "槽位Yaw最小插值速度", ClampMin = "0.0",
            ToolTip = "仅在关闭“槽位使用即时Yaw”时生效。防止槽位Yaw插值过慢导致槽位位置抖动。"))
    float MinGhostSlotYawInterpSpeed = 12.0f;

    // ✨ 新增 - 槽位中心点是否使用主将即时位置（推荐启用）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Ghost",
        meta = (DisplayName = "槽位中心使用主将即时位置",
            ToolTip = "启用后：槽位目标点围绕主将即时位置旋转/平移，减少大旋转时的交叉穿插与堆叠。"))
    bool bUseInstantLeaderLocationForSlotCenter = true;

    // ✨ 新增 - 限制槽位Yaw角速度，避免大角度旋转时目标点甩动过猛导致士兵挤成一团
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Ghost",
        meta = (DisplayName = "限制槽位Yaw角速度",
            ToolTip = "启用后：每秒槽位Yaw最大变化受限，主将快速转身时编队旋转更可控，减少穿插堆叠。"))
    bool bClampSlotYawRate = true;

    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Ghost",
        meta = (DisplayName = "槽位Yaw最大角速度(度/秒)", ClampMin = "10.0", ClampMax = "1080.0",
            ToolTip = "槽位目标点旋转的最大角速度。过小会显得编队转身很慢，过大会增加穿插概率。建议 180~360。"))
    float MaxSlotYawRateDegPerSec = 360.0f;

    // ==================== 追赶补偿配置 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit",
        meta = (DisplayName = "追赶主将速度倍率", ClampMin = "1.0", ClampMax = "5.0",
            ToolTip = "士兵追赶时会叠加主将当前速度×该倍率，倍率越大越容易追上冲刺中的主将。"))
    float CatchUpSpeedMultiplier = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit",
        meta = (DisplayName = "同步主将冲刺", ToolTip = "开启后，士兵追赶时会读取主将的冲刺状态与速度，自动提速。"))
    bool bSyncLeaderSprint = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit",
        meta = (DisplayName = "追赶时禁用碰撞", ToolTip = "开启可减少追赶过程卡住，但可能穿模；关闭更物理真实。"))
    bool bDisableCollisionDuringTransition = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit",
        meta = (DisplayName = "强制传送距离", ClampMin = "500.0", ToolTip = "距离超过此值会直接传送回队列，过小可能产生瞬移感。"))
    float ForceTeleportDistance = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit",
        meta = (DisplayName = "追赶超时时间", ClampMin = "0.0", ToolTip = "超过该时间仍未到位会触发传送，设为0可关闭超时传送。"))
    float RecruitTransitionTimeout = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit",
        meta = (DisplayName = "卡住检测时间", ClampMin = "0.0", ToolTip = "连续低速超过该时间视为卡住，会触发传送或重新定位。"))
    float StuckDetectionTime = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit",
        meta = (DisplayName = "卡住速度阈值", ClampMin = "0.0", ToolTip = "低于该速度会累计卡住时间，设为0关闭卡住检测。"))
    float StuckSpeedThreshold = 50.0f;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Locked",
    meta = (DisplayName = "锁定死区距离", ClampMin = "0.0",
        ToolTip = "距离槽位小于该值时不再推动移动输入，保留轻微滞后感，避免像粘在主将身后。"))
    float LockedDeadzoneDistance = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Locked",
        meta = (DisplayName = "锁定输入满量距离", ClampMin = "1.0",
            ToolTip = "误差距离达到该值时移动输入强度为1，误差越小输入越小，减少微抖与挤压。"))
    float LockedFullInputDistance = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Locked",
        meta = (DisplayName = "锁定速度插值率", ClampMin = "0.0",
            ToolTip = "锁定模式下 MaxWalkSpeed 变化的平滑强度，避免速度突变造成顿挫。"))
    float LockedSpeedInterpRate = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Locked",
        meta = (DisplayName = "锁定追赶额外速度", ClampMin = "0.0",
            ToolTip = "锁定模式下，当偏离槽位较远时允许比主将更快，用于追赶但不会瞬间贴死。"))
    float LockedCatchUpExtraSpeed = 600.0f;


    // ✨ 新增 - 招募过渡的“旋转混合/到达确认”配置（放到 XB|Follow|Recruit 分类附近）

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit",
        meta = (DisplayName = "招募旋转混合距离", ClampMin = "1.0",
            ToolTip = "距离槽位小于该值时，士兵朝向会从“移动方向”逐渐混合到“队伍前方”，消除接近槽位的顿挫。"))
    float RecruitRotationBlendDistance = 250.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit",
        meta = (DisplayName = "到达确认时间(秒)", ClampMin = "0.0", ClampMax = "1.0",
            ToolTip = "需要在到达阈值内持续这么久才认为到位，避免边界抖动造成状态切换顿挫。"))
    float ArriveConfirmTime = 0.08f;


    // ✨ 新增 - 运行时状态（放到你的状态变量区域）

    // 🔧 修改 - 用“到达累积时间”替代硬切阶段，避免接近槽位时的顿挫
    float ArrivedTimeAccumulator = 0.0f;

    // ==================== 战斗状态 ====================

    UPROPERTY(BlueprintReadOnly, Category = "XB|Follow|Combat", meta = (DisplayName = "是否战斗中"))
    bool bIsInCombat = false;

    // ==================== 将领状态缓存 ====================

    UPROPERTY(BlueprintReadOnly, Category = "XB|Follow", meta = (DisplayName = "将领正在冲刺"))
    bool bLeaderIsSprinting = false;

    UPROPERTY(BlueprintReadOnly, Category = "XB|Follow", meta = (DisplayName = "将领当前速度"))
    float CachedLeaderSpeed = 0.0f;

    // ==================== 状态 ====================

    FVector LastFrameLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "XB|Follow", meta = (DisplayName = "当前移动速度"))
    float CurrentMoveSpeed = 0.0f;

    float SmoothedSpeedCache = 0.0f;

    ECollisionResponse OriginalPawnResponse = ECR_Block;
    bool bCollisionModified = false;

    float RecruitTransitionStartTime = 0.0f;
    FVector LastPositionForStuckCheck = FVector::ZeroVector;
    float AccumulatedStuckTime = 0.0f;
    bool bRecruitMovementActive = false;

    UPROPERTY(BlueprintReadOnly, Category = "XB|Follow", meta = (DisplayName = "招募过渡阶段"))
    EXBRecruitTransitionPhase CurrentRecruitPhase = EXBRecruitTransitionPhase::Moving;

    // ==================== 幽灵目标状态 ====================

    FVector GhostTargetLocation = FVector::ZeroVector;

    // 🔧 修改 - 不再依赖完整Rotator插值来驱动槽位位置；使用Yaw-only插值避免角度跳变
    FRotator GhostTargetRotation = FRotator::ZeroRotator;
    bool bGhostInitialized = false;

    FVector GhostSlotTargetLocation = FVector::ZeroVector;

    // ✨ 新增 - 幽灵Yaw缓存（角度安全插值）
    float GhostYawDegrees = 0.0f;

    // ✨ 新增 - 槽位Yaw缓存（可选择即时或最小插值）
    float GhostSlotYawDegrees = 0.0f;

    FTimerHandle DelayedRecruitStartHandle;
};
