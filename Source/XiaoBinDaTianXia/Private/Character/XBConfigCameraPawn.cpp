/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Character/XBConfigCameraPawn.cpp

/**
 * @file XBConfigCameraPawn.cpp
 * @brief 配置阶段浮空相机Pawn实现
 * 
 * @note ✨ 新增 - 支持自由飞行与镜头控制
 */

#include "Character/XBConfigCameraPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

AXBConfigCameraPawn::AXBConfigCameraPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	// ✨ 新增 - 根组件
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// ✨ 新增 - 弹簧臂
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(Root);
	SpringArm->TargetArmLength = 1200.0f;
	SpringArm->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bInheritRoll = false;

	// ✨ 新增 - 摄像机
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	// ✨ 新增 - 浮空移动
	FloatingMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("FloatingMovement"));
	FloatingMovement->MaxSpeed = 1500.0f;
}

/**
 * @brief 设置镜头距离
 * @param NewDistance 新镜头距离
 * @return 无
 * @note   详细流程分析: 夹取范围 -> 写入弹簧臂
 *         性能/架构注意事项: 仅在本地输入驱动时调用
 */
void AXBConfigCameraPawn::SetCameraDistance(float NewDistance)
{
	// 🔧 修改 - 夹取范围，避免过近或过远
	const float ClampedDistance = FMath::Clamp(NewDistance, MinCameraDistance, MaxCameraDistance);
	SpringArm->TargetArmLength = ClampedDistance;
}

/**
 * @brief 设置镜头Yaw偏移
 * @param NewYawOffset 新Yaw偏移
 * @return 无
 * @note   详细流程分析: 写入弹簧臂旋转 -> 仅影响视角
 *         性能/架构注意事项: 与移动方向解耦
 */
void AXBConfigCameraPawn::SetCameraYawOffset(float NewYawOffset)
{
	// 🔧 修改 - 保持Pitch，避免视角抖动
	const FRotator CurrentRotation = SpringArm->GetRelativeRotation();
	SpringArm->SetRelativeRotation(FRotator(CurrentRotation.Pitch, NewYawOffset, 0.0f));
}
