/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Character/XBPlayerCharacter.h

/**
 * @file XBPlayerCharacter.h
 * @brief 玩家角色类 - 仅包含玩家专用功能（镜头控制）
 * 
 * @note 🔧 修改 - 将共用组件和方法移到基类
 *       现在只保留镜头系统
 */

#pragma once

#include "CoreMinimal.h"
#include "Character/XBCharacterBase.h"
#include "XBPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;

/**
 * @brief 玩家角色类
 */
UCLASS()
class XIAOBINDATIANXIA_API AXBPlayerCharacter : public AXBCharacterBase
{
    GENERATED_BODY()

public:
    AXBPlayerCharacter();

    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    // ==================== 镜头控制 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Camera", meta = (DisplayName = "设置镜头距离"))
    void SetCameraDistance(float NewDistance);

    UFUNCTION(BlueprintCallable, Category = "XB|Camera", meta = (DisplayName = "获取镜头距离"))
    float GetCameraDistance() const;

    UFUNCTION(BlueprintCallable, Category = "XB|Camera", meta = (DisplayName = "设置镜头Yaw偏移"))
    void SetCameraYawOffset(float YawOffset);

    UFUNCTION(BlueprintCallable, Category = "XB|Camera", meta = (DisplayName = "获取镜头Yaw偏移"))
    float GetCameraYawOffset() const { return CurrentCameraYawOffset; }

    // ==================== 组件访问 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Components", meta = (DisplayName = "获取弹簧臂组件"))
    USpringArmComponent* GetSpringArmComponent() const { return SpringArmComponent; }

    UFUNCTION(BlueprintCallable, Category = "XB|Components", meta = (DisplayName = "获取摄像机组件"))
    UCameraComponent* GetCameraComponent() const { return CameraComponent; }

protected:
    // ==================== 镜头组件（玩家专用） ====================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "XB|Camera", meta = (DisplayName = "弹簧臂组件"))
    TObjectPtr<USpringArmComponent> SpringArmComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "XB|Camera", meta = (DisplayName = "摄像机组件"))
    TObjectPtr<UCameraComponent> CameraComponent;

    // ==================== 镜头配置 ====================

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XB|Camera", meta = (DisplayName = "默认镜头俯角"))
    float DefaultCameraPitch = -50.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XB|Camera", meta = (DisplayName = "镜头跟随延迟"))
    float CameraLagSpeed = 10.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XB|Camera", meta = (DisplayName = "镜头旋转延迟"))
    float CameraRotationLagSpeed = 10.0f;

private:
    float CurrentCameraYawOffset = 0.0f;
};
