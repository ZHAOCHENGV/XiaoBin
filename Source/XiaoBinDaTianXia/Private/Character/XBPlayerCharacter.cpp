/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Character/XBPlayerCharacter.cpp

/**
 * @file XBPlayerCharacter.cpp
 * @brief 玩家角色类实现 - 仅包含镜头控制
 */

#include "Character/XBPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

AXBPlayerCharacter::AXBPlayerCharacter()
{
    // ========== 弹簧臂配置 ==========
    SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
    SpringArmComponent->SetupAttachment(RootComponent);
    SpringArmComponent->TargetArmLength = 1200.0f;
    SpringArmComponent->SetRelativeRotation(FRotator(-50.0f, 0.0f, 0.0f));
    SpringArmComponent->bUsePawnControlRotation = false;
    SpringArmComponent->bInheritPitch = false;
    SpringArmComponent->bInheritYaw = false;
    SpringArmComponent->bInheritRoll = false;
    SpringArmComponent->bEnableCameraLag = true;
    SpringArmComponent->bEnableCameraRotationLag = true;
    SpringArmComponent->CameraLagSpeed = 10.0f;
    SpringArmComponent->CameraRotationLagSpeed = 10.0f;
    SpringArmComponent->bDoCollisionTest = true;
    SpringArmComponent->ProbeSize = 12.0f;
    SpringArmComponent->ProbeChannel = ECC_Camera;

    // ========== 摄像机配置 ==========
    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
    CameraComponent->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);
    CameraComponent->bUsePawnControlRotation = false;

    // ========== 默认阵营 ==========
    Faction = EXBFaction::Player;
}

void AXBPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    // 初始化镜头
    if (SpringArmComponent)
    {
        SpringArmComponent->TargetArmLength = 1200.0f;
        SpringArmComponent->SetRelativeRotation(FRotator(DefaultCameraPitch, 0.0f, 0.0f));
    }
}

void AXBPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    // 输入绑定在 PlayerController 中完成
}

// ==================== 镜头控制实现 ====================

void AXBPlayerCharacter::SetCameraDistance(float NewDistance)
{
    if (SpringArmComponent)
    {
        SpringArmComponent->TargetArmLength = NewDistance;
    }
}

float AXBPlayerCharacter::GetCameraDistance() const
{
    if (SpringArmComponent)
    {
        return SpringArmComponent->TargetArmLength;
    }
    return 0.0f;
}

void AXBPlayerCharacter::SetCameraYawOffset(float YawOffset)
{
    CurrentCameraYawOffset = YawOffset;

    if (SpringArmComponent)
    {
        FRotator CurrentRotation = SpringArmComponent->GetRelativeRotation();
        CurrentRotation.Yaw = YawOffset;
        SpringArmComponent->SetRelativeRotation(CurrentRotation);
    }
}
