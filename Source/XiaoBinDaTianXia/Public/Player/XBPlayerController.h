// Copyright XiaoBing Project. All Rights Reserved.

/**
 * @file XBPlayerController.h
 * @brief 玩家控制器头文件
 * 
 * 功能说明：
 * - 处理玩家输入
 * - 控制镜头旋转和缩放
 * - 镜头旋转独立于角色朝向
 * 
 * 注意事项：
 * - 镜头旋转只影响移动方向计算，不影响角色模型朝向
 * - 滚轮中键同时重置距离和旋转
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "XBPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UXBInputConfig;
struct FInputActionValue;
class AXBPlayerCharacter;
class AXBConfigCameraPawn;

UCLASS()
class XIAOBINDATIANXIA_API AXBPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AXBPlayerController();

    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual void OnPossess(APawn* InPawn) override;

    /**
     * @brief 生成主将后重置镜头旋转
     * @return 无
     * @note   详细流程分析: 重置Yaw/Pitch缓存 -> 立即应用到主将镜头
     *         性能/架构注意事项: 仅在配置阶段切换到主将时调用，避免频繁刷新
     */
    void ResetCameraAfterSpawnLeader();

    // ==================== 镜头控制接口 ====================

    /**
     * @brief 设置镜头距离
     * @param NewDistance 新的镜头距离值
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Camera", meta = (DisplayName = "设置镜头距离"))
    void SetCameraDistance(float NewDistance);

    /**
     * @brief 获取当前镜头距离
     * @return 当前镜头距离
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Camera", meta = (DisplayName = "获取镜头距离"))
    float GetCameraDistance() const { return CurrentCameraDistance; }

    /**
     * @brief 获取当前镜头Yaw偏移
     * @return 当前Yaw偏移值
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Camera", meta = (DisplayName = "获取镜头Yaw偏移"))
    float GetCameraYawOffset() const { return CurrentCameraYawOffset; }

    // 🔧 修改 - 重置镜头函数增加说明
    /**
     * @brief 重置镜头到默认状态
     * 
     * 功能说明：
     * - 同时重置镜头距离和旋转
     * - 使用平滑插值过渡
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Camera", meta = (DisplayName = "重置镜头"))
    void ResetCamera();

    /**
     * @brief 获取输入配置
     * @return 输入配置资产指针
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Input", meta = (DisplayName = "获取输入配置"))
    UXBInputConfig* GetInputConfig() const { return InputConfig; }

    // ✨ 新增 - 脱离战斗输入
    /**
     * @brief 脱离战斗（逃跑）
     * @note 对应按键：通常绑定到 R 键
     */
    void HandleDisengageCombat();

protected:
    // ==================== 输入配置 ====================

    /**
     * @brief 默认输入映射上下文
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input", meta = (DisplayName = "默认输入映射上下文"))
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    /**
     * @brief 输入映射上下文优先级
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input", meta = (DisplayName = "映射上下文优先级"))
    int32 MappingContextPriority = 0;

    /**
     * @brief 输入配置资产
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input", meta = (DisplayName = "输入配置资产"))
    TObjectPtr<UXBInputConfig> InputConfig;

    // ==================== 镜头配置 ====================

    /**
     * @brief 默认镜头距离
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XB|Camera", meta = (DisplayName = "默认镜头距离", ClampMin = "100.0"))
    float DefaultCameraDistance = 1200.0f;

    /**
     * @brief 最小镜头距离
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XB|Camera", meta = (DisplayName = "最小镜头距离", ClampMin = "100.0"))
    float MinCameraDistance = 400.0f;

    /**
     * @brief 最大镜头距离
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XB|Camera", meta = (DisplayName = "最大镜头距离", ClampMin = "500.0"))
    float MaxCameraDistance = 2500.0f;

    /**
     * @brief 镜头缩放步进（每次滚轮）
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XB|Camera", meta = (DisplayName = "镜头缩放步进", ClampMin = "10.0"))
    float CameraZoomStep = 150.0f;

    /**
     * @brief 镜头旋转速度（度/秒）
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XB|Camera", meta = (DisplayName = "镜头旋转速度", ClampMin = "10.0"))
    float CameraRotationSpeed = 120.0f;

    /**
     * @brief 镜头距离插值速度
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XB|Camera", meta = (DisplayName = "镜头缩放平滑度", ClampMin = "1.0"))
    float CameraZoomInterpSpeed = 10.0f;

    // 🔧 修改 - 增加旋转插值速度配置
    /**
     * @brief 镜头旋转插值速度
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XB|Camera", meta = (DisplayName = "镜头旋转平滑度", ClampMin = "1.0"))
    float CameraRotationInterpSpeed = 8.0f;

    /**
     * @brief 镜头重置插值速度
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XB|Camera", meta = (DisplayName = "镜头重置平滑度", ClampMin = "1.0"))
    float CameraResetInterpSpeed = 5.0f;

    /**
     * @brief 是否使用镜头朝向作为移动方向
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XB|Camera", meta = (DisplayName = "移动方向跟随镜头"))
    bool bUseCameraForwardForMovement = true;


    UPROPERTY()
    TWeakObjectPtr<AXBPlayerCharacter> CachedPlayerCharacter;

    UPROPERTY()
    TWeakObjectPtr<AXBConfigCameraPawn> CachedConfigPawn;

    // ==================== 输入回调 ====================

    void HandleMoveInput(const FInputActionValue& InputValue);
    void HandleDashInputStarted();
    void HandleDashInputCompleted();
    void HandleLookInput(const FInputActionValue& InputValue);
    void HandleCameraZoomInput(const FInputActionValue& InputValue);
    void HandleCameraRotateLeftStarted();
    void HandleCameraRotateLeftCompleted();
    void HandleCameraRotateRightStarted();
    void HandleCameraRotateRightCompleted();
    void HandleCameraResetInput();
    void HandleAttackInput();
    void HandleSkillInput();
    void HandleRecallInput();
    void HandleSpawnLeaderInput();

    // ✨ 新增 - 配置阶段放置输入回调
    void HandlePlacementClickInput();
    void HandlePlacementCancelInput();
    void HandlePlacementDeleteInput();
    void HandlePlacementRotateInput(const FInputActionValue& InputValue);

private:
    // ==================== 镜头状态 ====================

    /**
     * @brief 当前镜头距离
     */
    float CurrentCameraDistance;

    /**
     * @brief 目标镜头距离（用于平滑插值）
     */
    float TargetCameraDistance;

    /**
     * @brief 当前镜头Yaw偏移
     */
    float CurrentCameraYawOffset = 0.0f;

    // ✨ 新增 - 目标Yaw偏移（用于平滑重置）
    /**
     * @brief 目标镜头Yaw偏移（用于平滑插值）
     */
    float TargetCameraYawOffset = 0.0f;

    /**
     * @brief 是否正在左旋
     */
    bool bIsRotatingLeft = false;

    /**
     * @brief 是否正在右旋
     */
    bool bIsRotatingRight = false;

    // 🔧 修改 - 分离距离和旋转的重置状态
    /**
     * @brief 是否正在重置镜头距离
     */
    bool bIsResettingDistance = false;

    // ✨ 新增 - 旋转重置状态
    /**
     * @brief 是否正在重置镜头旋转
     */
    bool bIsResettingRotation = false;

    // ==================== 内部方法 ====================

    void BindInputActions();
    void UpdateCamera(float DeltaTime);
    void UpdateCameraDistance(float DeltaTime);
    void UpdateCameraRotation(float DeltaTime);
    void ApplyCameraSettings();
    FVector CalculateMoveDirection(const FVector2D& InputVector) const;

    // ✨ 新增 - 镜头Pitch状态
    float CurrentCameraPitch = -45.0f;
    float TargetCameraPitch = -45.0f;
};
