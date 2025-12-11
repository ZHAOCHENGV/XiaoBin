// Copyright XiaoBing Project. All Rights Reserved.

/**
 * @file XBPlayerController.cpp
 * @brief 玩家控制器实现文件
 * 
 * 功能说明：
 * - 处理所有玩家输入
 * - 管理镜头距离和旋转
 * - 镜头旋转独立于角色朝向
 * 
 * 详细流程：
 * 1. BeginPlay 中添加输入映射上下文
 * 2. SetupInputComponent 中绑定所有输入动作
 * 3. PlayerTick 中更新镜头状态
 * 4. 移动输入时计算基于镜头的移动方向
 * 
 * 注意事项：
 * - 移动方向基于镜头Yaw计算，但不改变角色模型朝向
 * - 角色模型朝向由 CharacterMovementComponent 的 bOrientRotationToMovement 控制
 */

#include "Player/XBPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Input/XBInputConfig.h"
#include "Character/XBPlayerCharacter.h"
#include "Character/Components/XBCombatComponent.h"
AXBPlayerController::AXBPlayerController()
{
    // 初始化镜头参数
    CurrentCameraDistance = DefaultCameraDistance;
    TargetCameraDistance = DefaultCameraDistance;
    CurrentCameraYawOffset = 0.0f;
    TargetCameraYawOffset = 0.0f;
}

void AXBPlayerController::BeginPlay()
{
    // 调用父类BeginPlay
    Super::BeginPlay();

    // 添加输入映射上下文
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = 
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (DefaultMappingContext)
        {
            // 添加映射上下文
            Subsystem->AddMappingContext(DefaultMappingContext, MappingContextPriority);
            UE_LOG(LogTemp, Log, TEXT("Added Input Mapping Context: %s"), *DefaultMappingContext->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("DefaultMappingContext is not set in PlayerController!"));
        }
    }

    // 初始化镜头参数
    CurrentCameraDistance = DefaultCameraDistance;
    TargetCameraDistance = DefaultCameraDistance;
    CurrentCameraYawOffset = 0.0f;
    TargetCameraYawOffset = 0.0f;
    
    // 应用初始镜头设置
    ApplyCameraSettings();
}

void AXBPlayerController::OnPossess(APawn* InPawn)
{
    // 调用父类OnPossess
    Super::OnPossess(InPawn);

    // Possess 后立即应用镜头设置
    ApplyCameraSettings();
}

void AXBPlayerController::SetupInputComponent()
{
    // 调用父类设置
    Super::SetupInputComponent();

    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
    if (!EnhancedInput)
    {
        return;
    }
    // 绑定输入动作
    BindInputActions();

   
}

/**
 * @brief 绑定所有输入动作
 * 
 * 功能说明：
 * - 将输入配置中的动作绑定到对应的处理函数
 * - 使用 Enhanced Input 系统
 */
void AXBPlayerController::BindInputActions()
{
    // 获取增强输入组件
    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
    if (!EnhancedInput)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get EnhancedInputComponent!"));
        return;
    }

    // 检查输入配置
    if (!InputConfig)
    {
        UE_LOG(LogTemp, Error, TEXT("InputConfig is not set in PlayerController!"));
        return;
    }

    // ========== 移动输入 ==========
    if (InputConfig->MoveAction)
    {
        // 绑定移动输入（持续触发）
        EnhancedInput->BindAction(
            InputConfig->MoveAction, 
            ETriggerEvent::Triggered, 
            this, 
            &AXBPlayerController::HandleMoveInput);
    }

    // ========== 冲刺输入 ==========
    if (InputConfig->DashAction)
    {
        // 按下时开始冲刺
        EnhancedInput->BindAction(
            InputConfig->DashAction, 
            ETriggerEvent::Started, 
            this, 
            &AXBPlayerController::HandleDashInputStarted);
        
        // 松开时结束冲刺
        EnhancedInput->BindAction(
            InputConfig->DashAction, 
            ETriggerEvent::Completed, 
            this, 
            &AXBPlayerController::HandleDashInputCompleted);
    }

    // ========== 镜头缩放输入 ==========
    if (InputConfig->CameraZoomAction)
    {
        // 绑定缩放输入
        EnhancedInput->BindAction(
            InputConfig->CameraZoomAction, 
            ETriggerEvent::Triggered, 
            this, 
            &AXBPlayerController::HandleCameraZoomInput);
    }

    // ========== 镜头左旋输入（Q键长按） ==========
    if (InputConfig->CameraRotateLeftAction)
    {
        // 按下时开始左旋
        EnhancedInput->BindAction(
            InputConfig->CameraRotateLeftAction, 
            ETriggerEvent::Started, 
            this, 
            &AXBPlayerController::HandleCameraRotateLeftStarted);
        
        // 松开时停止左旋
        EnhancedInput->BindAction(
            InputConfig->CameraRotateLeftAction, 
            ETriggerEvent::Completed, 
            this, 
            &AXBPlayerController::HandleCameraRotateLeftCompleted);
    }

    // ========== 镜头右旋输入（E键长按） ==========
    if (InputConfig->CameraRotateRightAction)
    {
        // 按下时开始右旋
        EnhancedInput->BindAction(
            InputConfig->CameraRotateRightAction, 
            ETriggerEvent::Started, 
            this, 
            &AXBPlayerController::HandleCameraRotateRightStarted);
        
        // 松开时停止右旋
        EnhancedInput->BindAction(
            InputConfig->CameraRotateRightAction, 
            ETriggerEvent::Completed, 
            this, 
            &AXBPlayerController::HandleCameraRotateRightCompleted);
    }

    // ========== 镜头重置输入（鼠标中键） ==========
    if (InputConfig->CameraResetAction)
    {
        // 按下时重置镜头
        EnhancedInput->BindAction(
            InputConfig->CameraResetAction, 
            ETriggerEvent::Started, 
            this, 
            &AXBPlayerController::HandleCameraResetInput);
    }

    // ========== 攻击输入 ==========
    if (InputConfig->AttackAction)
    {
        // 绑定攻击输入
        EnhancedInput->BindAction(
            InputConfig->AttackAction, 
            ETriggerEvent::Started, 
            this, 
            &AXBPlayerController::HandleAttackInput);
    }

    // ========== 技能输入 ==========
    if (InputConfig->SkillAction)
    {
        // 绑定技能输入
        EnhancedInput->BindAction(
            InputConfig->SkillAction, 
            ETriggerEvent::Started, 
            this, 
            &AXBPlayerController::HandleSkillInput);
    }

    // ========== 召回输入 ==========
    if (InputConfig->RecallAction)
    {
        // 绑定召回输入
        EnhancedInput->BindAction(
            InputConfig->RecallAction, 
            ETriggerEvent::Triggered, 
            this, 
            &AXBPlayerController::HandleDisengageCombat);
    }

  

    UE_LOG(LogTemp, Log, TEXT("输入操作已成功绑定!!!"));
}

void AXBPlayerController::PlayerTick(float DeltaTime)
{
    // 调用父类PlayerTick
    Super::PlayerTick(DeltaTime);

    // 更新镜头状态
    UpdateCamera(DeltaTime);
}

// ==================== 镜头控制实现 ====================

/**
 * @brief 设置镜头距离
 * @param NewDistance 新的镜头距离值
 */
void AXBPlayerController::SetCameraDistance(float NewDistance)
{
    // 限制距离范围
    TargetCameraDistance = FMath::Clamp(NewDistance, MinCameraDistance, MaxCameraDistance);
    // 取消距离重置状态
    bIsResettingDistance = false;
}

// 🔧 修改 - 重置镜头同时重置距离和旋转
/**
 * @brief 重置镜头到默认状态
 * 
 * 功能说明：
 * - 同时重置镜头距离和旋转
 * - 设置目标值，由 UpdateCamera 进行平滑插值
 */
void AXBPlayerController::ResetCamera()
{
    // 设置目标距离为默认值
    TargetCameraDistance = DefaultCameraDistance;
    // 设置距离重置状态
    bIsResettingDistance = true;
    
    // ✨ 新增 - 同时重置旋转
    // 设置目标旋转为0
    TargetCameraYawOffset = 0.0f;
    // 设置旋转重置状态
    bIsResettingRotation = true;
    
    UE_LOG(LogTemp, Log, TEXT("Camera Reset - Distance: %.1f, Rotation: 0"), DefaultCameraDistance);
}

/**
 * @brief 脱离战斗输入处理
 * @note ✨ 新增方法
 */
void AXBPlayerController::HandleDisengageCombat()
{
    if (!CachedPlayerCharacter.IsValid())
    {
        return;
    }

    // 🔧 修改 - 重命名变量，避免与 APlayerController::Player 冲突
    AXBPlayerCharacter* PlayerChar = CachedPlayerCharacter.Get();
    if (!PlayerChar || PlayerChar->IsDead())
    {
        return;
    }

    // 调用角色的脱离战斗方法
    PlayerChar->DisengageFromCombat();

    UE_LOG(LogTemp, Log, TEXT("玩家触发脱离战斗"));
}

/**
 * @brief 更新镜头状态（每帧调用）
 * @param DeltaTime 帧间隔时间
 */
void AXBPlayerController::UpdateCamera(float DeltaTime)
{
    // 更新镜头距离
    UpdateCameraDistance(DeltaTime);
    // 更新镜头旋转
    UpdateCameraRotation(DeltaTime);
    // 应用镜头设置到角色
    ApplyCameraSettings();
}

/**
 * @brief 更新镜头距离（平滑插值）
 * @param DeltaTime 帧间隔时间
 */
void AXBPlayerController::UpdateCameraDistance(float DeltaTime)
{
    // 检查是否需要插值
    if (!FMath::IsNearlyEqual(CurrentCameraDistance, TargetCameraDistance, 1.0f))
    {
        // 根据是否正在重置选择插值速度
        float InterpSpeed = bIsResettingDistance ? CameraResetInterpSpeed : CameraZoomInterpSpeed;
        // 平滑插值到目标距离
        CurrentCameraDistance = FMath::FInterpTo(
            CurrentCameraDistance, 
            TargetCameraDistance, 
            DeltaTime, 
            InterpSpeed);
    }
    else
    {
        // 到达目标，清除重置状态
        CurrentCameraDistance = TargetCameraDistance;
        bIsResettingDistance = false;
    }
}

// 🔧 修改 - 完全重写旋转更新逻辑
/**
 * @brief 更新镜头旋转
 * @param DeltaTime 帧间隔时间
 * 
 * 功能说明：
 * - 处理Q/E键的持续旋转
 * - 处理重置时的平滑归零
 * - 旋转只影响镜头，不影响角色朝向
 */
void AXBPlayerController::UpdateCameraRotation(float DeltaTime)
{
    // 检查是否正在手动旋转
    bool bIsManuallyRotating = bIsRotatingLeft || bIsRotatingRight;
    
    // 如果正在手动旋转，取消重置状态
    if (bIsManuallyRotating)
    {
        bIsResettingRotation = false;
    }
    
    // 处理手动旋转
    if (bIsManuallyRotating)
    {
        // 计算旋转增量
        float RotationDelta = 0.0f;
        
        if (bIsRotatingLeft)
        {
            // 左旋（逆时针）
            RotationDelta -= CameraRotationSpeed * DeltaTime;
        }
        
        if (bIsRotatingRight)
        {
            // 右旋（顺时针）
            RotationDelta += CameraRotationSpeed * DeltaTime;
        }
        
        // 应用旋转增量
        CurrentCameraYawOffset += RotationDelta;
        // 同步目标值
        TargetCameraYawOffset = CurrentCameraYawOffset;
        
        // 标准化到 -180 ~ 180 范围
        if (CurrentCameraYawOffset > 180.0f)
        {
            CurrentCameraYawOffset -= 360.0f;
            TargetCameraYawOffset -= 360.0f;
        }
        else if (CurrentCameraYawOffset < -180.0f)
        {
            CurrentCameraYawOffset += 360.0f;
            TargetCameraYawOffset += 360.0f;
        }
    }
    // ✨ 新增 - 处理重置旋转
    else if (bIsResettingRotation)
    {
        // 检查是否需要插值
        if (!FMath::IsNearlyZero(CurrentCameraYawOffset, 1.0f))
        {
            // 平滑插值到0
            CurrentCameraYawOffset = FMath::FInterpTo(
                CurrentCameraYawOffset, 
                0.0f, 
                DeltaTime, 
                CameraResetInterpSpeed);
        }
        else
        {
            // 到达目标，清除重置状态
            CurrentCameraYawOffset = 0.0f;
            TargetCameraYawOffset = 0.0f;
            bIsResettingRotation = false;
        }
    }
}

/**
 * @brief 应用镜头设置到角色
 * 
 * 功能说明：
 * - 将当前镜头距离和旋转应用到角色的弹簧臂
 * - 只修改弹簧臂，不影响角色本身的朝向
 */
void AXBPlayerController::ApplyCameraSettings()
{
    // 获取玩家角色
    AXBPlayerCharacter* PlayerChar = Cast<AXBPlayerCharacter>(GetPawn());
    if (!PlayerChar)
    {
        return;
    }

    // 应用镜头距离
    PlayerChar->SetCameraDistance(CurrentCameraDistance);
    // 应用镜头Yaw偏移
    PlayerChar->SetCameraYawOffset(CurrentCameraYawOffset);
}

/**
 * @brief 计算移动方向（基于世界坐标，不受镜头旋转影响）
 * @param InputVector 输入向量（WASD）
 * @return 世界空间中的移动方向
 * 
 * 功能说明：
 * - WASD 对应固定的世界方向
 * - 镜头旋转只影响视角，不影响移动方向
 * - W = 世界前方（+X），S = 世界后方（-X）
 * - D = 世界右方（+Y），A = 世界左方（-Y）
 * 
 * 注意事项：
 * - 角色朝向由 CharacterMovementComponent 的 bOrientRotationToMovement 控制
 * - 角色会自动朝向移动方向
 */
FVector AXBPlayerController::CalculateMoveDirection(const FVector2D& InputVector) const
{
    // ✨ 新增 - 使用世界坐标方向，不受镜头旋转影响
    // 世界前方向（+X轴）
    const FVector WorldForward = FVector::ForwardVector;  // (1, 0, 0)
    // 世界右方向（+Y轴）
    const FVector WorldRight = FVector::RightVector;      // (0, 1, 0)

    // 组合移动方向
    // InputVector.Y = W(+1) / S(-1) 对应世界前后
    // InputVector.X = D(+1) / A(-1) 对应世界左右
    FVector MoveDirection = WorldForward * InputVector.Y + WorldRight * InputVector.X;
    
    // 确保在水平面上（Z = 0）
    MoveDirection.Z = 0.0f;

    // 标准化方向向量
    if (!MoveDirection.IsNearlyZero())
    {
        MoveDirection.Normalize();
    }

    return MoveDirection;
}

// ==================== 输入回调实现 ====================

// 🔧 修改 - 移动输入处理
/**
 * @brief 处理移动输入
 * @param InputValue 输入值（2D向量）
 * 
 * 功能说明：
 * - 根据镜头旋转计算移动方向
 * - 只设置移动方向，不设置角色朝向
 * - 角色朝向由移动组件自动处理
 */
void AXBPlayerController::HandleMoveInput(const FInputActionValue& InputValue)
{
    // 获取输入向量
    const FVector2D MoveValue = InputValue.Get<FVector2D>();

    // 获取控制的Pawn
    if (APawn* ControlledPawn = GetPawn())
    {
        // 计算基于镜头的移动方向
        FVector MoveDirection = CalculateMoveDirection(MoveValue);
        
        // 应用移动输入
        // 注意：这里只是添加移动输入，角色朝向由 CharacterMovementComponent 控制
        // CharacterMovementComponent 的 bOrientRotationToMovement = true 会让角色朝向移动方向
        ControlledPawn->AddMovementInput(MoveDirection, 1.0f);
    }
}

/**
 * @brief 处理冲刺输入开始
 */
void AXBPlayerController::HandleDashInputStarted()
{
    if (AXBCharacterBase* CharBase = Cast<AXBCharacterBase>(GetPawn()))
    {
        CharBase->StartSprint();
    }
}

/**
 * @brief 处理冲刺输入结束
 */
void AXBPlayerController::HandleDashInputCompleted()
{
    if (AXBCharacterBase* CharBase = Cast<AXBCharacterBase>(GetPawn()))
    {
        CharBase->StopSprint();
    }
}

/**
 * @brief 处理镜头缩放输入
 * @param InputValue 输入值（滚轮增量）
 */
void AXBPlayerController::HandleCameraZoomInput(const FInputActionValue& InputValue)
{
    // 获取滚轮值
    const float ZoomValue = InputValue.Get<float>();
    
    // 计算新距离（滚轮向上拉近，向下拉远）
    float NewDistance = TargetCameraDistance - (ZoomValue * CameraZoomStep);
    // 设置新距离
    SetCameraDistance(NewDistance);
}

/**
 * @brief 处理镜头左旋输入开始
 */
void AXBPlayerController::HandleCameraRotateLeftStarted()
{
    // 设置左旋状态
    bIsRotatingLeft = true;
    // 取消重置状态
    bIsResettingRotation = false;
}

/**
 * @brief 处理镜头左旋输入结束
 */
void AXBPlayerController::HandleCameraRotateLeftCompleted()
{
    // 清除左旋状态
    bIsRotatingLeft = false;
}

/**
 * @brief 处理镜头右旋输入开始
 */
void AXBPlayerController::HandleCameraRotateRightStarted()
{
    // 设置右旋状态
    bIsRotatingRight = true;
    // 取消重置状态
    bIsResettingRotation = false;
}

/**
 * @brief 处理镜头右旋输入结束
 */
void AXBPlayerController::HandleCameraRotateRightCompleted()
{
    // 清除右旋状态
    bIsRotatingRight = false;
}

// 🔧 修改 - 重置输入处理
/**
 * @brief 处理镜头重置输入
 * 
 * 功能说明：
 * - 同时重置镜头距离和旋转
 */
void AXBPlayerController::HandleCameraResetInput()
{
    // 调用重置函数（同时重置距离和旋转）
    ResetCamera();
}

/**
 * @brief 处理攻击输入
 * @note 通过 GAS 系统触发攻击技能，使用 Tag 匹配方式
 */
void AXBPlayerController::HandleAttackInput()
{
    if (AXBPlayerCharacter* PlayerChar = Cast<AXBPlayerCharacter>(GetPawn()))
    {
        // ✨ 新增 - 使用战斗组件执行攻击
        if (UXBCombatComponent* CombatComp = PlayerChar->GetCombatComponent())
        {
            bool bSuccess = CombatComp->PerformBasicAttack();
            UE_LOG(LogTemp, Log, TEXT("攻击输入触发 - 结果: %s"), 
                bSuccess ? TEXT("成功") : TEXT("失败/冷却中"));
        }
    }
}






/**
 * @brief 处理技能输入
 * @note 通过 GAS 系统触发特殊技能
 */
void AXBPlayerController::HandleSkillInput()
{
    if (AXBPlayerCharacter* PlayerChar = Cast<AXBPlayerCharacter>(GetPawn()))
    {
        // ✨ 新增 - 使用战斗组件执行技能
        if (UXBCombatComponent* CombatComp = PlayerChar->GetCombatComponent())
        {
            bool bSuccess = CombatComp->PerformSpecialSkill();
            UE_LOG(LogTemp, Log, TEXT("技能输入触发 - 结果: %s"), 
                bSuccess ? TEXT("成功") : TEXT("失败/冷却中"));
        }
    }
}
/**
 * @brief 处理召回输入
 */
void AXBPlayerController::HandleRecallInput()
{
    // 获取玩家角色
    if (AXBPlayerCharacter* PlayerChar = Cast<AXBPlayerCharacter>(GetPawn()))
    {
        // 召回所有士兵
        PlayerChar->RecallAllSoldiers();
        UE_LOG(LogTemp, Log, TEXT("Recall Input Triggered"));
    }
}
