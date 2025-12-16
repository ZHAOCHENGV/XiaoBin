/* --- 完整文件代码 --- */
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
 *       5. ✨ 新增 - 地面追踪功能，确保士兵贴地移动
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

/**
 * @brief 士兵跟随组件
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

    UFUNCTION(BlueprintCallable, Category = "XB|Follow")
    void SetFollowInterpSpeed(float NewSpeed) { MovementSpeed = NewSpeed; }

    UFUNCTION(BlueprintCallable, Category = "XB|Follow")
    void SetInterpSpeed(float NewSpeed) { MovementSpeed = NewSpeed; }

    // ==================== 委托事件 ====================

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

    bool MoveTowardsTargetDirect(const FVector& TargetPosition, float DeltaTime, float MoveSpeed);
    bool MoveTowardsTargetInterp(const FVector& TargetPosition, float DeltaTime, float InterpSpeed);

    float GetLeaderMoveSpeed() const;

    UCharacterMovementComponent* GetCachedMovementComponent();
    UCapsuleComponent* GetCachedCapsuleComponent();

    void SetSoldierCollisionEnabled(bool bEnableCollision);
    void SetMovementComponentEnabled(bool bEnable);
    void SetRVOAvoidanceEnabled(bool bEnable);

    // ✨ 新增 - 地面追踪
    /**
     * @brief 获取指定位置的地面高度
     * @param InLocation 输入位置
     * @param OutGroundZ 输出的地面Z坐标
     * @return 是否成功找到地面
     * @note 使用射线检测从上往下查找地面
     */
    bool GetGroundHeight(const FVector& InLocation, float& OutGroundZ) const;

    /**
     * @brief 将位置调整到地面上
     * @param InOutLocation 输入输出的位置
     * @note 确保角色始终贴地
     */
    void AdjustToGround(FVector& InOutLocation) const;

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "招募过渡速度", ClampMin = "1.0", ClampMax = "50.0"))
    float RecruitTransitionSpeed = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "过渡完成后锁定"))
    bool bLockAfterRecruitTransition = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Recruit", meta = (DisplayName = "过渡时禁用碰撞"))
    bool bDisableCollisionDuringTransition = true;

    // ✨ 新增 - 地面追踪配置
    /** @brief 是否启用地面追踪（确保士兵贴地） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Ground", meta = (DisplayName = "启用地面追踪"))
    bool bEnableGroundTracking = true;

    /** @brief 地面检测的起始高度偏移（从角色位置向上） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Ground", meta = (DisplayName = "检测起始高度偏移", ClampMin = "0.0"))
    float GroundTraceStartOffset = 200.0f;

    /** @brief 地面检测的距离（从起始点向下） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow|Ground", meta = (DisplayName = "检测距离", ClampMin = "100.0"))
    float GroundTraceDistance = 500.0f;

    // ==================== 战斗状态 ====================

    UPROPERTY(BlueprintReadOnly, Category = "XB|Follow|Combat", meta = (DisplayName = "是否战斗中"))
    bool bIsInCombat = false;

    // ==================== 状态 ====================

    FVector LastFrameLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "XB|Follow", meta = (DisplayName = "当前移动速度"))
    float CurrentMoveSpeed = 0.0f;

    ECollisionResponse OriginalPawnResponse = ECR_Block;
    bool bCollisionModified = false;

    bool bOriginalMovementEnabled = true;
    bool bMovementStateModified = false;
};
