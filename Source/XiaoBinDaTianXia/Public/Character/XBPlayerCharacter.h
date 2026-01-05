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

    /**
     * @brief  初始化主将数据（玩家）
     * @return 无
     * @note   详细流程分析: 调用父类通用初始化 -> 可追加玩家专属逻辑
     */
    virtual void InitializeLeaderData() override;

    /**
     * @brief  获取外部初始化配置（玩家从配置界面）
     * @param  OutConfig 输出配置
     * @return 是否存在外部配置
     * @note   详细流程分析: 从 GameInstance 读取配置数据
     */
    virtual bool GetExternalInitConfig(FXBGameConfigData& OutConfig) const override;

    /**
     * @brief  从配置数据初始化玩家主将
     * @param  GameConfig 配置数据
     * @param  bApplyInitialSoldiers 是否生成初始士兵
     * @return 无
     * @note   详细流程分析: 应用行名/名称/倍率/成长参数 -> 应用基础缩放 -> 初始化属性 -> 应用运行时配置
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "应用主将配置"))
    void ApplyConfigFromGameConfig(const FXBGameConfigData& GameConfig, bool bApplyInitialSoldiers = true);
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
