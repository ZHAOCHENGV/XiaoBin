// Copyright XiaoBing Project. All Rights Reserved.

/**
 * @file XBPlayerCharacter.h
 * @brief 玩家角色类头文件
 * 
 * 功能说明：
 * - 玩家角色的核心类，包含镜头控制、移动、冲刺等功能
 * - 冲刺系统：长按加速，松开恢复原速度
 * - 磁场组件：用于招募士兵
 * - 编队组件：管理跟随的士兵队列
 * 
 * 注意事项：
 * - 冲刺不使用冷却时间，完全由玩家输入控制
 * - 所有配置参数都暴露给蓝图编辑
 */

#pragma once

#include "CoreMinimal.h"
#include "Character/XBCharacterBase.h"
#include "XBPlayerCharacter.generated.h"

// 前向声明
class USpringArmComponent;
class UCameraComponent;
class UXBMagnetFieldComponent;
class UXBFormationComponent;

// ✨ 新增 - 冲刺状态变化委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FXBOnDashStateChanged, bool, bIsDashing);

/**
 * @brief 玩家角色类
 * 
 * 详细流程：
 * 1. 构造函数创建所有组件
 * 2. BeginPlay 初始化移动参数和绑定事件
 * 3. Tick 持续更新冲刺状态
 * 4. 输入系统控制冲刺开始/结束
 */
UCLASS()
class XIAOBINDATIANXIA_API AXBPlayerCharacter : public AXBCharacterBase
{
    GENERATED_BODY()

public:
    /**
     * @brief 构造函数
     * 创建并初始化所有组件
     */
    AXBPlayerCharacter();

    /**
     * @brief 每帧更新
     * @param DeltaTime 帧间隔时间
     * 
     * 功能：更新冲刺状态和移动速度
     */
    virtual void Tick(float DeltaTime) override;

    /**
     * @brief 游戏开始时调用
     * 初始化移动参数，绑定磁场事件
     */
    virtual void BeginPlay() override;

    /**
     * @brief 设置玩家输入组件
     * @param PlayerInputComponent 输入组件指针
     */
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    // ==================== 镜头控制 ====================

    /**
     * @brief 设置镜头距离
     * @param NewDistance 新的镜头距离值
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Camera", meta = (DisplayName = "设置镜头距离"))
    void SetCameraDistance(float NewDistance);

    /**
     * @brief 获取当前镜头距离
     * @return 当前镜头臂长度
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Camera", meta = (DisplayName = "获取镜头距离"))
    float GetCameraDistance() const;

    /**
     * @brief 设置镜头Yaw偏移
     * @param YawOffset Yaw角度偏移值
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Camera", meta = (DisplayName = "设置镜头Yaw偏移"))
    void SetCameraYawOffset(float YawOffset);

    /**
     * @brief 获取镜头Yaw偏移
     * @return 当前Yaw偏移值
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Camera", meta = (DisplayName = "获取镜头Yaw偏移"))
    float GetCameraYawOffset() const { return CurrentCameraYawOffset; }

    // ==================== 冲刺系统 ====================

    // 🔧 修改 - 简化冲刺接口，改为长按控制
    /**
     * @brief 开始冲刺（长按时调用）
     * 
     * 功能说明：
     * - 将移动速度提升到冲刺速度
     * - 可以在冲刺中改变方向
     * - 持续按住则持续冲刺
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Movement", meta = (DisplayName = "开始冲刺"))
    void StartDash();

    /**
     * @brief 结束冲刺（松开时调用）
     * 
     * 功能说明：
     * - 立即恢复到正常移动速度
     * - 无冷却时间，可立即再次冲刺
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Movement", meta = (DisplayName = "结束冲刺"))
    void StopDash();

    /**
     * @brief 检查是否正在冲刺
     * @return true 正在冲刺，false 未冲刺
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "XB|Movement", meta = (DisplayName = "是否正在冲刺"))
    bool IsDashing() const { return bIsDashing; }

    /**
     * @brief 获取当前实际移动速度
     * @return 当前移动速度值
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "XB|Movement", meta = (DisplayName = "获取当前移动速度"))
    float GetCurrentMoveSpeed() const;

    // ==================== 组件访问 ====================

    /**
     * @brief 获取弹簧臂组件
     * @return 弹簧臂组件指针
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Components", meta = (DisplayName = "获取弹簧臂组件"))
    USpringArmComponent* GetSpringArmComponent() const { return SpringArmComponent; }

    /**
     * @brief 获取摄像机组件
     * @return 摄像机组件指针
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Components", meta = (DisplayName = "获取摄像机组件"))
    UCameraComponent* GetCameraComponent() const { return CameraComponent; }

    /**
     * @brief 获取磁场组件
     * @return 磁场组件指针
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Components", meta = (DisplayName = "获取磁场组件"))
    UXBMagnetFieldComponent* GetMagnetFieldComponent() const { return MagnetFieldComponent; }

    /**
     * @brief 获取编队组件
     * @return 编队组件指针
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Components", meta = (DisplayName = "获取编队组件"))
    UXBFormationComponent* GetFormationComponent() const { return FormationComponent; }

    // ==================== 委托事件 ====================

    // 🔧 修改 - 使用单一委托，传递冲刺状态
    /**
     * @brief 冲刺状态变化事件
     * @param bIsDashing 当前是否正在冲刺
     * 
     * 使用方法：在蓝图中绑定此事件以响应冲刺状态变化
     */
    UPROPERTY(BlueprintAssignable, Category = "XB|Movement", meta = (DisplayName = "冲刺状态变化事件"))
    FXBOnDashStateChanged OnDashStateChanged;

protected:
    // ==================== 组件 ====================

    /**
     * @brief 弹簧臂组件（镜头支撑）
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "XB|Camera", meta = (DisplayName = "弹簧臂组件"))
    TObjectPtr<USpringArmComponent> SpringArmComponent;

    /**
     * @brief 摄像机组件
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "XB|Camera", meta = (DisplayName = "摄像机组件"))
    TObjectPtr<UCameraComponent> CameraComponent;

    /**
     * @brief 磁场组件（用于士兵招募）
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "XB|Army", meta = (DisplayName = "磁场组件"))
    TObjectPtr<UXBMagnetFieldComponent> MagnetFieldComponent;

    /**
     * @brief 编队组件
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "XB|Army", meta = (DisplayName = "编队组件"))
    TObjectPtr<UXBFormationComponent> FormationComponent;

    // ==================== 镜头配置 ====================

    /**
     * @brief 默认镜头俯角
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XB|Camera", meta = (DisplayName = "默认镜头俯角"))
    float DefaultCameraPitch = -50.0f;

    /**
     * @brief 镜头跟随延迟速度
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XB|Camera", meta = (DisplayName = "镜头跟随延迟"))
    float CameraLagSpeed = 10.0f;

    /**
     * @brief 镜头旋转延迟速度
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XB|Camera", meta = (DisplayName = "镜头旋转延迟"))
    float CameraRotationLagSpeed = 10.0f;

    // ==================== 移动配置 ====================

    // 🔧 修改 - 将移动配置暴露给蓝图
    /**
     * @brief 基础移动速度（正常行走）
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Movement", meta = (DisplayName = "基础移动速度", ClampMin = "0.0"))
    float BaseMoveSpeed = 600.0f;

    /**
     * @brief 基础转向速率
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Movement", meta = (DisplayName = "基础转向速率", ClampMin = "0.0"))
    float BaseRotationRate = 540.0f;

    // ==================== 冲刺配置 ====================

    // 🔧 修改 - 简化冲刺配置，移除冷却相关
    /**
     * @brief 冲刺速度倍率
     * 冲刺时的速度 = 基础速度 × 此倍率
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Movement|Dash", meta = (DisplayName = "冲刺速度倍率", ClampMin = "1.0", ClampMax = "5.0"))
    float DashSpeedMultiplier = 2.0f;

    /**
     * @brief 冲刺时是否可以改变方向
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Movement|Dash", meta = (DisplayName = "冲刺时可转向"))
    bool bCanSteerWhileDashing = true;

    /**
     * @brief 冲刺时的转向速率倍率
     * 冲刺转向速率 = 基础转向速率 × 此倍率
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Movement|Dash", meta = (DisplayName = "冲刺转向速率倍率", ClampMin = "0.0", ClampMax = "2.0", EditCondition = "bCanSteerWhileDashing"))
    float DashSteerRateMultiplier = 0.7f;

    /**
     * @brief 速度变化插值速度
     * 控制从正常速度到冲刺速度的过渡平滑度
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Movement|Dash", meta = (DisplayName = "速度变化平滑度", ClampMin = "1.0", ClampMax = "50.0"))
    float SpeedInterpRate = 15.0f;

    // ✨ 新增 - 冲刺状态蓝图可读
    /**
     * @brief 当前是否正在冲刺（蓝图可读）
     */
    UPROPERTY(BlueprintReadOnly, Category = "XB|Movement|Dash", meta = (DisplayName = "正在冲刺"))
    bool bIsDashing = false;

    // ✨ 新增 - 目标速度和当前速度蓝图可读
    /**
     * @brief 目标移动速度
     */
    UPROPERTY(BlueprintReadOnly, Category = "XB|Movement|Dash", meta = (DisplayName = "目标移动速度"))
    float TargetMoveSpeed = 0.0f;

private:
    // ==================== 镜头状态 ====================

    /**
     * @brief 当前镜头Yaw偏移
     */
    float CurrentCameraYawOffset = 0.0f;

    // ==================== 内部方法 ====================

    // 🔧 修改 - 更新冲刺逻辑
    /**
     * @brief 更新冲刺状态（每帧调用）
     * @param DeltaTime 帧间隔时间
     * 
     * 功能：
     * - 根据冲刺状态平滑插值移动速度
     * - 更新角色移动组件的速度参数
     */
    void UpdateDash(float DeltaTime);

    /**
     * @brief 配置移动组件
     * 设置初始移动速度和转向速率
     */
    void SetupMovementComponent();

    /**
     * @brief 应用移动速度到角色
     * @param NewSpeed 新的移动速度
     */
    void ApplyMoveSpeed(float NewSpeed);

    /**
     * @brief 磁场重叠回调
     * @param EnteredActor 进入磁场的Actor
     */
    UFUNCTION()
    void OnMagnetFieldActorEntered(AActor* EnteredActor);
};
