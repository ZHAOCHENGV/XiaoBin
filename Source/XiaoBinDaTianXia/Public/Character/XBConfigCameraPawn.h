/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Character/XBConfigCameraPawn.h

/**
 * @file XBConfigCameraPawn.h
 * @brief 配置阶段浮空相机Pawn
 * 
 * @note ✨ 新增 - 配置阶段先行操控
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "XBConfigCameraPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UFloatingPawnMovement;

/**
 * @brief 配置阶段浮空相机Pawn
 * @note 提供自由飞行移动与镜头控制
 */
UCLASS()
class XIAOBINDATIANXIA_API AXBConfigCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	AXBConfigCameraPawn();

	/**
	 * @brief 设置镜头距离
	 * @param NewDistance 新镜头距离
	 * @return 无
	 * @note   详细流程分析: 夹取范围 -> 写入弹簧臂
	 *         性能/架构注意事项: 仅在本地输入驱动时调用
	 */
	void SetCameraDistance(float NewDistance);

	/**
	 * @brief 设置镜头Yaw偏移
	 * @param NewYawOffset 新Yaw偏移
	 * @return 无
	 * @note   详细流程分析: 写入弹簧臂旋转 -> 仅影响视角
	 *         性能/架构注意事项: 与移动方向解耦
	 */
	void SetCameraYawOffset(float NewYawOffset);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "根组件"))
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "弹簧臂"))
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "摄像机"))
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "浮空移动"))
	TObjectPtr<UFloatingPawnMovement> FloatingMovement;

	// ✨ 新增 - 镜头距离限制
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "最小镜头距离", ClampMin = "100.0"))
	float MinCameraDistance = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "最大镜头距离", ClampMin = "200.0"))
	float MaxCameraDistance = 2500.0f;
};
