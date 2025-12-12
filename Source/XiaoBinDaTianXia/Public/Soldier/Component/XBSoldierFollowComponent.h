/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Soldier/Component/XBSoldierFollowComponent.h

/**
 * @file XBSoldierFollowComponent.h
 * @brief 士兵跟随组件 - 紧密编队跟随模式
 * 
 * @note 🔧 完全重写:
 *       1. 使用位置设置而非导航移动
 *       2. 完全跟随将领位移和旋转
 *       3. 被阻挡后插值回原位置
 *       4. 只有战斗时才脱离编队
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "XBSoldierFollowComponent.generated.h"

class AXBCharacterBase;
class UXBFormationComponent;

/**
 * @brief 跟随模式枚举
 */
UENUM(BlueprintType)
enum class EXBFollowMode : uint8
{
    /** @brief 锁定模式 - 完全跟随将领，位置同步 */
    Locked      UMETA(DisplayName = "锁定跟随"),
    
    /** @brief 插值模式 - 正在插值回编队位置 */
    Interpolating   UMETA(DisplayName = "插值中"),
    
    /** @brief 自由模式 - 战斗中，自由移动 */
    Free        UMETA(DisplayName = "自由移动")
};

/**
 * @brief 士兵跟随组件（紧密编队模式）
 * 
 * @note 核心逻辑:
 *       - 锁定模式：每帧同步将领位置+编队偏移，跟随将领旋转
 *       - 插值模式：被阻挡后，插值回编队位置
 *       - 自由模式：战斗中，不干预士兵位置
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName = "XB Soldier Follow (Locked)"))
class XIAOBINDATIANXIA_API UXBSoldierFollowComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UXBSoldierFollowComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, 
        FActorComponentTickFunction* ThisTickFunction) override;

    // ==================== 目标设置 ====================

    /**
     * @brief 设置跟随目标
     * @param NewTarget 目标Actor（将领）
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Follow", meta = (DisplayName = "设置跟随目标"))
    void SetFollowTarget(AActor* NewTarget);

    /**
     * @brief 获取跟随目标
     */
    UFUNCTION(BlueprintPure, Category = "XB|Follow", meta = (DisplayName = "获取跟随目标"))
    AActor* GetFollowTarget() const { return FollowTargetRef.Get(); }

    // ==================== 编队设置 ====================

    /**
     * @brief 设置编队槽位索引
     * @param SlotIndex 槽位索引
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Follow", meta = (DisplayName = "设置槽位索引"))
    void SetFormationSlotIndex(int32 SlotIndex);

    /**
     * @brief 获取编队槽位索引
     */
    UFUNCTION(BlueprintPure, Category = "XB|Follow", meta = (DisplayName = "获取槽位索引"))
    int32 GetFormationSlotIndex() const { return FormationSlotIndex; }

    // ==================== 模式控制 ====================

    /**
     * @brief 设置跟随模式
     * @param NewMode 新模式
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Follow", meta = (DisplayName = "设置跟随模式"))
    void SetFollowMode(EXBFollowMode NewMode);

    /**
     * @brief 获取当前跟随模式
     */
    UFUNCTION(BlueprintPure, Category = "XB|Follow", meta = (DisplayName = "获取跟随模式"))
    EXBFollowMode GetFollowMode() const { return CurrentMode; }

    /**
     * @brief 进入战斗（切换到自由模式）
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Follow", meta = (DisplayName = "进入战斗"))
    void EnterCombatMode();

    /**
     * @brief 退出战斗（立即传送回编队位置）
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Follow", meta = (DisplayName = "退出战斗"))
    void ExitCombatMode();

    /**
     * @brief 立即传送到编队位置
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Follow", meta = (DisplayName = "传送到编队位置"))
    void TeleportToFormationPosition();

    /**
     * @brief 开始插值到编队位置
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Follow", meta = (DisplayName = "插值到编队位置"))
    void StartInterpolateToFormation();

    // ==================== 状态查询 ====================

    /**
     * @brief 获取目标编队位置（世界坐标）
     */
    UFUNCTION(BlueprintPure, Category = "XB|Follow", meta = (DisplayName = "获取编队位置"))
    FVector GetTargetPosition() const;

    /**
     * @brief 检查是否在编队位置
     */
    UFUNCTION(BlueprintPure, Category = "XB|Follow", meta = (DisplayName = "是否在编队位置"))
    bool IsAtFormationPosition() const;

    /**
     * @brief 获取到编队位置的距离
     */
    UFUNCTION(BlueprintPure, Category = "XB|Follow", meta = (DisplayName = "到编队位置距离"))
    float GetDistanceToFormation() const;

    // ==================== 外部调用 ====================

    /**
     * @brief 更新跟随（供 Tick 或外部调用）
     * @param DeltaTime 帧时间
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Follow", meta = (DisplayName = "更新跟随"))
    void UpdateFollowing(float DeltaTime);

    // ==================== 速度设置（兼容旧接口） ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Follow")
    void SetFollowSpeed(float NewSpeed) { InterpolateSpeed = NewSpeed / 100.0f; }

    UFUNCTION(BlueprintCallable, Category = "XB|Follow")
    void SetFollowInterpSpeed(float NewSpeed) { InterpolateSpeed = NewSpeed; }

    UFUNCTION(BlueprintCallable, Category = "XB|Follow")
    void SetInterpSpeed(float NewSpeed) { InterpolateSpeed = NewSpeed; }

protected:
    // ==================== 内部方法 ====================

    /**
     * @brief 更新锁定模式（完全跟随将领）
     */
    void UpdateLockedMode(float DeltaTime);

    /**
     * @brief 更新插值模式（插值回编队位置）
     */
    void UpdateInterpolatingMode(float DeltaTime);

    /**
     * @brief 计算编队世界位置
     */
    FVector CalculateFormationWorldPosition() const;

    /**
     * @brief 计算编队世界旋转
     */
    FRotator CalculateFormationWorldRotation() const;

    /**
     * @brief 检测是否被阻挡（偏离编队位置）
     */
    bool IsBlockedFromFormation() const;

    /**
     * @brief 从编队组件获取槽位偏移
     */
    FVector2D GetSlotLocalOffset() const;

protected:
    // ==================== 引用 ====================

    /** @brief 跟随目标引用 */
    UPROPERTY()
    TWeakObjectPtr<AActor> FollowTargetRef;

    /** @brief 缓存的编队组件 */
    UPROPERTY()
    TWeakObjectPtr<UXBFormationComponent> CachedFormationComponent;

    // ==================== 配置 ====================

    /** @brief 编队槽位索引 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow", meta = (DisplayName = "槽位索引"))
    int32 FormationSlotIndex = INDEX_NONE;

    /** @brief 当前跟随模式 */
    UPROPERTY(BlueprintReadOnly, Category = "XB|Follow", meta = (DisplayName = "当前模式"))
    EXBFollowMode CurrentMode = EXBFollowMode::Locked;

    /** @brief 插值速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow", meta = (DisplayName = "插值速度", ClampMin = "1.0"))
    float InterpolateSpeed = 10.0f;

    /** @brief 旋转插值速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow", meta = (DisplayName = "旋转插值速度", ClampMin = "1.0"))
    float RotationInterpolateSpeed = 15.0f;

    /** @brief 到达编队位置的阈值 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow", meta = (DisplayName = "到达阈值", ClampMin = "1.0"))
    float ArrivalThreshold = 10.0f;

    /** @brief 阻挡检测阈值（超过此距离视为被阻挡） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow", meta = (DisplayName = "阻挡阈值", ClampMin = "10.0"))
    float BlockedThreshold = 100.0f;

    /** @brief 是否跟随将领旋转 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow", meta = (DisplayName = "跟随旋转"))
    bool bFollowRotation = true;

    /** @brief 锁定模式下是否使用插值（否则直接设置位置） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow", meta = (DisplayName = "锁定模式插值"))
    bool bInterpolateInLockedMode = true;

    /** @brief 锁定模式的插值速度（比普通插值更快） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Follow", meta = (DisplayName = "锁定插值速度", ClampMin = "1.0"))
    float LockedModeInterpolateSpeed = 25.0f;

    // ==================== 状态 ====================

    /** @brief 上一帧的将领位置（用于检测移动） */
    FVector LastLeaderLocation = FVector::ZeroVector;

    /** @brief 上一帧的将领旋转 */
    FRotator LastLeaderRotation = FRotator::ZeroRotator;

    /** @brief 缓存的编队槽位偏移 */
    FVector2D CachedSlotOffset = FVector2D::ZeroVector;

    /** @brief 是否需要刷新槽位偏移 */
    bool bNeedRefreshSlotOffset = true;
};