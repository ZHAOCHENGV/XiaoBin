// Copyright XiaoBing Project. All Rights Reserved.

/**
 * @file XBPlayerCharacter.cpp
 * @brief 玩家角色类实现文件
 * 
 * 功能说明：
 * - 实现玩家角色的移动、冲刺、镜头控制等功能
 * - 冲刺系统采用长按加速、松开恢复的设计
 * 
 * 详细流程：
 * 1. 构造函数初始化所有组件
 * 2. BeginPlay 设置移动参数和事件绑定
 * 3. Tick 中持续更新冲刺状态和速度插值
 * 4. StartDash/StopDash 由输入系统调用
 */

#include "Character/XBPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Character/Components/XBMagnetFieldComponent.h"
#include "Character/Components/XBFormationComponent.h"

AXBPlayerCharacter::AXBPlayerCharacter()
{
    // 启用Tick
    PrimaryActorTick.bCanEverTick = true;

    // ========== 弹簧臂配置 ==========
    // 创建弹簧臂组件
    SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
    // 附加到根组件
    SpringArmComponent->SetupAttachment(RootComponent);
    // 设置默认臂长
    SpringArmComponent->TargetArmLength = 600.0f;
    // 设置默认俯角
    SpringArmComponent->SetRelativeRotation(FRotator(-50.0f, 0.0f, 0.0f));
    // 禁用Pawn控制旋转（我们手动控制）
    SpringArmComponent->bUsePawnControlRotation = false;
    // 禁用继承旋转
    SpringArmComponent->bInheritPitch = false;
    SpringArmComponent->bInheritYaw = false;
    SpringArmComponent->bInheritRoll = false;
    // 启用镜头延迟
    SpringArmComponent->bEnableCameraLag = true;
    SpringArmComponent->bEnableCameraRotationLag = true;
    SpringArmComponent->CameraLagSpeed = 10.0f;
    SpringArmComponent->CameraRotationLagSpeed = 10.0f;
    // 启用碰撞检测
    SpringArmComponent->bDoCollisionTest = true;
    SpringArmComponent->ProbeSize = 12.0f;
    SpringArmComponent->ProbeChannel = ECC_Camera;

    // ========== 摄像机配置 ==========
    // 创建摄像机组件
    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
    // 附加到弹簧臂末端
    CameraComponent->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);
    // 禁用Pawn控制旋转
    CameraComponent->bUsePawnControlRotation = false;

    // ========== 磁场组件配置 ==========
    // 创建磁场组件
    MagnetFieldComponent = CreateDefaultSubobject<UXBMagnetFieldComponent>(TEXT("MagnetFieldComponent"));
    // 附加到根组件
    MagnetFieldComponent->SetupAttachment(RootComponent);

    // ========== 编队组件配置 ==========
    // 创建编队组件
    FormationComponent = CreateDefaultSubobject<UXBFormationComponent>(TEXT("FormationComponent"));

    // ========== 角色旋转配置 ==========
    // 禁用控制器旋转（角色朝向由移动方向决定）
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // ========== 默认阵营 ==========
    // 设置为玩家阵营
    Faction = EXBFaction::Player;
}

void AXBPlayerCharacter::BeginPlay()
{
    // 调用父类BeginPlay
    Super::BeginPlay();

    // 配置移动组件
    SetupMovementComponent();

    // 初始化目标速度为基础速度
    TargetMoveSpeed = BaseMoveSpeed;

    // 绑定磁场事件
    if (MagnetFieldComponent)
    {
        // 检查事件是否已绑定
        if (!MagnetFieldComponent->OnActorEnteredField.IsBound())
        {
            // 绑定进入磁场事件
            MagnetFieldComponent->OnActorEnteredField.AddDynamic(
                this, &AXBPlayerCharacter::OnMagnetFieldActorEntered);
        }
        // 启用磁场
        MagnetFieldComponent->SetFieldEnabled(true);
    }

    // 初始化镜头
    if (SpringArmComponent)
    {
        // 设置臂长
        SpringArmComponent->TargetArmLength = 1200.0f;
        // 设置俯角
        SpringArmComponent->SetRelativeRotation(FRotator(DefaultCameraPitch, 0.0f, 0.0f));
    }

    // 打印调试信息
    UE_LOG(LogTemp, Log, TEXT("XBPlayerCharacter BeginPlay - BaseMoveSpeed: %.1f, DashMultiplier: %.1f"), 
        BaseMoveSpeed, DashSpeedMultiplier);
}

/**
 * @brief 配置移动组件
 * 
 * 设置角色移动组件的初始参数
 */
void AXBPlayerCharacter::SetupMovementComponent()
{
    // 获取角色移动组件
    UCharacterMovementComponent* CMC = GetCharacterMovement();
    if (!CMC)
    {
        return;
    }

    // 设置基础移动速度
    CMC->MaxWalkSpeed = BaseMoveSpeed;
    // 设置制动减速度
    CMC->BrakingDecelerationWalking = 2000.0f;
    // 设置地面摩擦力
    CMC->GroundFriction = 8.0f;
    
    // 启用朝向移动方向
    CMC->bOrientRotationToMovement = true;
    // 设置转向速率
    CMC->RotationRate = FRotator(0.0f, BaseRotationRate, 0.0f);
    
    // 设置加速度
    CMC->MaxAcceleration = 2048.0f;
    // 设置制动摩擦因子
    CMC->BrakingFrictionFactor = 2.0f;
}

void AXBPlayerCharacter::Tick(float DeltaTime)
{
    // 调用父类Tick
    Super::Tick(DeltaTime);

    // 🔧 修改 - 每帧更新冲刺状态
    // 更新冲刺逻辑（速度插值）
    UpdateDash(DeltaTime);
}

void AXBPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    // 调用父类设置
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // 输入绑定在 PlayerController 中完成
}

// ==================== 镜头控制实现 ====================

void AXBPlayerCharacter::SetCameraDistance(float NewDistance)
{
    // 检查弹簧臂是否有效
    if (SpringArmComponent)
    {
        // 设置臂长
        SpringArmComponent->TargetArmLength = NewDistance;
    }
}

float AXBPlayerCharacter::GetCameraDistance() const
{
    // 检查弹簧臂是否有效
    if (SpringArmComponent)
    {
        // 返回当前臂长
        return SpringArmComponent->TargetArmLength;
    }
    // 无效时返回0
    return 0.0f;
}

void AXBPlayerCharacter::SetCameraYawOffset(float YawOffset)
{
    // 保存当前Yaw偏移
    CurrentCameraYawOffset = YawOffset;

    // 检查弹簧臂是否有效
    if (SpringArmComponent)
    {
        // 获取当前旋转
        FRotator CurrentRotation = SpringArmComponent->GetRelativeRotation();
        // 设置新的Yaw值
        CurrentRotation.Yaw = YawOffset;
        // 应用旋转
        SpringArmComponent->SetRelativeRotation(CurrentRotation);
    }
}

// ==================== 冲刺系统实现 ====================

// 🔧 修改 - 完全重写冲刺开始逻辑
/**
 * @brief 开始冲刺
 * 
 * 功能说明：
 * - 设置冲刺状态为true
 * - 计算并设置目标速度（基础速度 × 冲刺倍率）
 * - 广播冲刺状态变化事件
 * 
 * 注意事项：
 * - 实际速度变化在 UpdateDash 中通过插值实现
 * - 可以在冲刺过程中随时调用（幂等操作）
 */
void AXBPlayerCharacter::StartDash()
{
    if (bIsDashing)
    {
        return;
    }

    bIsDashing = true;
    TargetMoveSpeed = BaseMoveSpeed * DashSpeedMultiplier;

    if (UCharacterMovementComponent* CMC = GetCharacterMovement())
    {
        if (bCanSteerWhileDashing)
        {
            CMC->RotationRate = FRotator(0.0f, BaseRotationRate * DashSteerRateMultiplier, 0.0f);
        }
        else
        {
            CMC->RotationRate = FRotator(0.0f, 100.0f, 0.0f);
        }
    }

    // ✨ 新增 - 通知士兵进入逃跑加速状态
    SetSoldiersEscaping(true);

    OnDashStateChanged.Broadcast(true);

    UE_LOG(LogTemp, Log, TEXT("Dash Started - Target Speed: %.1f"), TargetMoveSpeed);
}

// 🔧 修改 - 完全重写冲刺结束逻辑
/**
 * @brief 结束冲刺
 * 
 * 功能说明：
 * - 设置冲刺状态为false
 * - 将目标速度恢复为基础速度
 * - 恢复正常转向速率
 * - 广播冲刺状态变化事件
 * 
 * 注意事项：
 * - 实际速度变化在 UpdateDash 中通过插值实现
 * - 无冷却时间，可立即再次冲刺
 */
void AXBPlayerCharacter::StopDash()
{
    if (!bIsDashing)
    {
        return;
    }

    bIsDashing = false;
    TargetMoveSpeed = BaseMoveSpeed;

    if (UCharacterMovementComponent* CMC = GetCharacterMovement())
    {
        CMC->RotationRate = FRotator(0.0f, BaseRotationRate, 0.0f);
    }

    // ✨ 新增 - 取消士兵逃跑加速状态
    SetSoldiersEscaping(false);

    OnDashStateChanged.Broadcast(false);

    UE_LOG(LogTemp, Log, TEXT("Dash Stopped - Target Speed: %.1f"), TargetMoveSpeed);
}

/**
 * @brief 获取当前实际移动速度
 * @return 当前角色移动组件的最大行走速度
 */
float AXBPlayerCharacter::GetCurrentMoveSpeed() const
{
    // 获取角色移动组件
    if (const UCharacterMovementComponent* CMC = GetCharacterMovement())
    {
        // 返回当前最大行走速度
        return CMC->MaxWalkSpeed;
    }
    // 无效时返回基础速度
    return BaseMoveSpeed;
}

// 🔧 修改 - 完全重写冲刺更新逻辑
/**
 * @brief 更新冲刺状态（每帧调用）
 * @param DeltaTime 帧间隔时间
 * 
 * 功能说明：
 * - 平滑插值当前速度到目标速度
 * - 应用新速度到角色移动组件
 * 
 * 详细流程：
 * 1. 获取当前移动速度
 * 2. 使用 FMath::FInterpTo 平滑插值到目标速度
 * 3. 应用新速度到移动组件
 */
void AXBPlayerCharacter::UpdateDash(float DeltaTime)
{
    // 获取角色移动组件
    UCharacterMovementComponent* CMC = GetCharacterMovement();
    if (!CMC)
    {
        return;
    }

    // 获取当前移动速度
    float CurrentSpeed = CMC->MaxWalkSpeed;

    // 检查是否需要插值（当前速度与目标速度不同）
    if (!FMath::IsNearlyEqual(CurrentSpeed, TargetMoveSpeed, 1.0f))
    {
        // 平滑插值到目标速度
        float NewSpeed = FMath::FInterpTo(CurrentSpeed, TargetMoveSpeed, DeltaTime, SpeedInterpRate);
        
        // 应用新速度
        ApplyMoveSpeed(NewSpeed);
    }
}

// ✨ 新增 - 应用移动速度的辅助函数
/**
 * @brief 应用移动速度到角色
 * @param NewSpeed 新的移动速度
 * 
 * 功能说明：
 * - 设置角色移动组件的最大行走速度
 */
void AXBPlayerCharacter::ApplyMoveSpeed(float NewSpeed)
{
    // 获取角色移动组件
    if (UCharacterMovementComponent* CMC = GetCharacterMovement())
    {
        // 设置最大行走速度
        CMC->MaxWalkSpeed = NewSpeed;
    }
}

// ==================== 磁场回调实现 ====================

/**
 * @brief 磁场重叠回调
 * @param EnteredActor 进入磁场的Actor
 * 
 * 功能说明：
 * - 当有Actor进入磁场范围时触发
 * - 用于检测和招募村民
 */
void AXBPlayerCharacter::OnMagnetFieldActorEntered(AActor* EnteredActor)
{
    // 检查Actor是否有效
    if (!EnteredActor)
    {
        return;
    }

    // TODO: 检查是否是村民，进行招募逻辑
    UE_LOG(LogTemp, Log, TEXT("Actor entered magnet field: %s"), *EnteredActor->GetName());
}
