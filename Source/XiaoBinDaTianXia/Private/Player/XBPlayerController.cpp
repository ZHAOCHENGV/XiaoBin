/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Player/XBPlayerController.cpp

/**
 * @file XBPlayerController.cpp
 * @brief 玩家控制器实现文件
 * 
 * @note 🔧 修改记录:
 *       1. ✨ 新增 - 移动输入时检查角色是否可以移动（技能释放中禁止移动）
 */

#include "Player/XBPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Input/XBInputConfig.h"
#include "Character/XBPlayerCharacter.h"
#include "Character/XBConfigCameraPawn.h"
#include "Game/XBGameMode.h"
#include "Utils/XBGameplayTags.h"
#include "Character/Components/XBCombatComponent.h"

AXBPlayerController::AXBPlayerController()
{
    CurrentCameraDistance = DefaultCameraDistance;
    TargetCameraDistance = DefaultCameraDistance;
    CurrentCameraYawOffset = 0.0f;
    TargetCameraYawOffset = 0.0f;
    CurrentCameraPitch = -45.0f;
    TargetCameraPitch = -45.0f;
}

void AXBPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = 
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (DefaultMappingContext)
        {
            Subsystem->AddMappingContext(DefaultMappingContext, MappingContextPriority);
            UE_LOG(LogTemp, Log, TEXT("Added Input Mapping Context: %s"), *DefaultMappingContext->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("DefaultMappingContext is not set in PlayerController!"));
        }
    }

    CurrentCameraDistance = DefaultCameraDistance;
    TargetCameraDistance = DefaultCameraDistance;
    CurrentCameraYawOffset = 0.0f;
    TargetCameraYawOffset = 0.0f;
    
    ApplyCameraSettings();
}

void AXBPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    // ✨ 新增 - 缓存玩家角色引用
    CachedPlayerCharacter = Cast<AXBPlayerCharacter>(InPawn);
    CachedConfigPawn = Cast<AXBConfigCameraPawn>(InPawn);
    

    // 🔧 修改 - 进入新场景并开始控制角色时，切换为仅游戏输入模式
    FInputModeGameOnly GameOnlyInputMode;
    SetInputMode(GameOnlyInputMode);
    bShowMouseCursor = false;

    ApplyCameraSettings();
}

/**
 * @brief 生成主将后重置镜头旋转
 * @return 无
 * @note   详细流程分析: 清理配置阶段旋转缓存 -> 强制回到默认背后视角
 *         性能/架构注意事项: 仅在主将生成流程调用，避免影响战斗阶段镜头
 */
void AXBPlayerController::ResetCameraAfterSpawnLeader()
{
    // 🔧 修改 - 生成主将时强制重置Yaw/Pitch，避免继承配置阶段旋转
    CurrentCameraYawOffset = 0.0f;
    TargetCameraYawOffset = 0.0f;
    bIsRotatingLeft = false;
    bIsRotatingRight = false;
    bIsResettingRotation = false;

    // 🔧 修改 - 恢复默认俯角缓存，保证镜头回到主将背后视角
    CurrentCameraPitch = -45.0f;
    TargetCameraPitch = -45.0f;

    // 🔧 修改 - 立即应用镜头设置，确保切换控制后视角正确
    ApplyCameraSettings();
}

void AXBPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
    if (!EnhancedInput)
    {
        return;
    }
    
    BindInputActions();
}

void AXBPlayerController::BindInputActions()
{
    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
    if (!EnhancedInput)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get EnhancedInputComponent!"));
        return;
    }

    if (!InputConfig)
    {
        UE_LOG(LogTemp, Error, TEXT("InputConfig is not set in PlayerController!"));
        return;
    }

    if (InputConfig->MoveAction)
    {
        EnhancedInput->BindAction(
            InputConfig->MoveAction, 
            ETriggerEvent::Triggered, 
            this, 
            &AXBPlayerController::HandleMoveInput);
    }

    if (InputConfig->DashAction)
    {
        EnhancedInput->BindAction(
            InputConfig->DashAction, 
            ETriggerEvent::Started, 
            this, 
            &AXBPlayerController::HandleDashInputStarted);
        
        EnhancedInput->BindAction(
            InputConfig->DashAction, 
            ETriggerEvent::Completed, 
            this, 
            &AXBPlayerController::HandleDashInputCompleted);
    }

    if (InputConfig->CameraZoomAction)
    {
        EnhancedInput->BindAction(
            InputConfig->CameraZoomAction, 
            ETriggerEvent::Triggered, 
            this, 
            &AXBPlayerController::HandleCameraZoomInput);
    }

    if (InputConfig->LookAction)
    {
        EnhancedInput->BindAction(
            InputConfig->LookAction,
            ETriggerEvent::Triggered,
            this,
            &AXBPlayerController::HandleLookInput);
    }

    if (InputConfig->CameraRotateLeftAction)
    {
        EnhancedInput->BindAction(
            InputConfig->CameraRotateLeftAction, 
            ETriggerEvent::Started, 
            this, 
            &AXBPlayerController::HandleCameraRotateLeftStarted);
        
        EnhancedInput->BindAction(
            InputConfig->CameraRotateLeftAction, 
            ETriggerEvent::Completed, 
            this, 
            &AXBPlayerController::HandleCameraRotateLeftCompleted);
    }

    if (InputConfig->CameraRotateRightAction)
    {
        EnhancedInput->BindAction(
            InputConfig->CameraRotateRightAction, 
            ETriggerEvent::Started, 
            this, 
            &AXBPlayerController::HandleCameraRotateRightStarted);
        
        EnhancedInput->BindAction(
            InputConfig->CameraRotateRightAction, 
            ETriggerEvent::Completed, 
            this, 
            &AXBPlayerController::HandleCameraRotateRightCompleted);
    }

    if (InputConfig->CameraResetAction)
    {
        EnhancedInput->BindAction(
            InputConfig->CameraResetAction, 
            ETriggerEvent::Started, 
            this, 
            &AXBPlayerController::HandleCameraResetInput);
    }

    if (InputConfig->AttackAction)
    {
        EnhancedInput->BindAction(
            InputConfig->AttackAction, 
            ETriggerEvent::Started, 
            this, 
            &AXBPlayerController::HandleAttackInput);
    }

    if (InputConfig->SkillAction)
    {
        EnhancedInput->BindAction(
            InputConfig->SkillAction, 
            ETriggerEvent::Started, 
            this, 
            &AXBPlayerController::HandleSkillInput);
    }

    if (InputConfig->RecallAction)
    {
        EnhancedInput->BindAction(
            InputConfig->RecallAction, 
            ETriggerEvent::Triggered, 
            this, 
            &AXBPlayerController::HandleDisengageCombat);
    }

    // ✨ 新增 - 配置阶段生成主将（回车）
    if (InputConfig->SpawnLeaderAction)
    {
        EnhancedInput->BindAction(
            InputConfig->SpawnLeaderAction,
            ETriggerEvent::Started,
            this,
            &AXBPlayerController::HandleSpawnLeaderInput);
    }
    else
    {
        const FGameplayTag SpawnLeaderTag = FXBGameplayTags::Get().InputTag_SpawnLeader;
        if (const UInputAction* SpawnLeaderAction = InputConfig->FindInputActionByTag(SpawnLeaderTag))
        {
            EnhancedInput->BindAction(
                const_cast<UInputAction*>(SpawnLeaderAction),
                ETriggerEvent::Started,
                this,
                &AXBPlayerController::HandleSpawnLeaderInput);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("未配置生成主将输入，请在输入配置中绑定回车"));
        }
    }

    UE_LOG(LogTemp, Log, TEXT("输入操作已成功绑定!!!"));
}

void AXBPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    UpdateCamera(DeltaTime);
}

void AXBPlayerController::SetCameraDistance(float NewDistance)
{
    TargetCameraDistance = FMath::Clamp(NewDistance, MinCameraDistance, MaxCameraDistance);
    bIsResettingDistance = false;
}

void AXBPlayerController::ResetCamera()
{
    TargetCameraDistance = DefaultCameraDistance;
    bIsResettingDistance = true;
    
    TargetCameraYawOffset = 0.0f;
    bIsResettingRotation = true;
    
    UE_LOG(LogTemp, Log, TEXT("Camera Reset - Distance: %.1f, Rotation: 0"), DefaultCameraDistance);
}

void AXBPlayerController::HandleDisengageCombat()
{
    if (!CachedPlayerCharacter.IsValid())
    {
        return;
    }

    AXBPlayerCharacter* PlayerChar = CachedPlayerCharacter.Get();
    if (!PlayerChar || PlayerChar->IsDead())
    {
        return;
    }

    PlayerChar->DisengageFromCombat();

    UE_LOG(LogTemp, Log, TEXT("玩家触发脱离战斗"));
}

void AXBPlayerController::UpdateCamera(float DeltaTime)
{
    UpdateCameraDistance(DeltaTime);
    UpdateCameraRotation(DeltaTime);
    ApplyCameraSettings();
}

void AXBPlayerController::UpdateCameraDistance(float DeltaTime)
{
    if (!FMath::IsNearlyEqual(CurrentCameraDistance, TargetCameraDistance, 1.0f))
    {
        float InterpSpeed = bIsResettingDistance ? CameraResetInterpSpeed : CameraZoomInterpSpeed;
        CurrentCameraDistance = FMath::FInterpTo(
            CurrentCameraDistance, 
            TargetCameraDistance, 
            DeltaTime, 
            InterpSpeed);
    }
    else
    {
        CurrentCameraDistance = TargetCameraDistance;
        bIsResettingDistance = false;
    }
}

void AXBPlayerController::UpdateCameraRotation(float DeltaTime)
{
    bool bIsManuallyRotating = bIsRotatingLeft || bIsRotatingRight;
    
    if (bIsManuallyRotating)
    {
        bIsResettingRotation = false;
    }
    
    if (bIsManuallyRotating)
    {
        float RotationDelta = 0.0f;
        
        if (bIsRotatingLeft)
        {
            RotationDelta -= CameraRotationSpeed * DeltaTime;
        }
        
        if (bIsRotatingRight)
        {
            RotationDelta += CameraRotationSpeed * DeltaTime;
        }
        
        CurrentCameraYawOffset += RotationDelta;
        TargetCameraYawOffset = CurrentCameraYawOffset;
        
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
    else if (bIsResettingRotation)
    {
        if (!FMath::IsNearlyZero(CurrentCameraYawOffset, 1.0f))
        {
            CurrentCameraYawOffset = FMath::FInterpTo(
                CurrentCameraYawOffset, 
                0.0f, 
                DeltaTime, 
                CameraResetInterpSpeed);
        }
        else
        {
            CurrentCameraYawOffset = 0.0f;
            TargetCameraYawOffset = 0.0f;
            bIsResettingRotation = false;
        }
    }
}

void AXBPlayerController::ApplyCameraSettings()
{
    if (AXBPlayerCharacter* PlayerChar = Cast<AXBPlayerCharacter>(GetPawn()))
    {
        PlayerChar->SetCameraDistance(CurrentCameraDistance);
        PlayerChar->SetCameraYawOffset(CurrentCameraYawOffset);
        return;
    }
    
}

FVector AXBPlayerController::CalculateMoveDirection(const FVector2D& InputVector) const
{
    // 🔧 修改 - 根据视角朝向计算移动方向，保证前进跟随相机朝向
    const FRotator CurrentControlRotation = GetControlRotation();
    const FRotator YawRotation(0.0f, CurrentControlRotation.Yaw, 0.0f);
    const FVector WorldForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector WorldRight = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    FVector MoveDirection = WorldForward * InputVector.Y + WorldRight * InputVector.X;
    
    MoveDirection.Z = 0.0f;

    if (!MoveDirection.IsNearlyZero())
    {
        MoveDirection.Normalize();
    }

    return MoveDirection;
}

/**
 * @brief 处理移动输入
 * @param InputValue 输入值（2D向量）
 * 
 * @note 🔧 修改 - 增加技能释放中禁止移动的检查
 *       当战斗组件的 ShouldBlockMovement() 返回 true 时，忽略移动输入
 */
void AXBPlayerController::HandleMoveInput(const FInputActionValue& InputValue)
{
    const FVector2D MoveValue = InputValue.Get<FVector2D>();

    // 🔧 修改 - 配置相机Pawn期间不使用自定义移动逻辑，直接走基础控制器方向
    if (CachedConfigPawn.IsValid())
    {
        // 🔧 修改 - 使用控制器当前朝向生成移动方向，保持基础Pawn移动一致性
        // 🔧 修改 - 避免与AController::ControlRotation成员同名遮蔽
        const FRotator CurrentControlRotation = GetControlRotation();
        const FRotator YawRotation(0.0f, CurrentControlRotation.Yaw, 0.0f);
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        CachedConfigPawn->AddMovementInput(ForwardDirection, MoveValue.Y);
        CachedConfigPawn->AddMovementInput(RightDirection, MoveValue.X);
        return;
    }

    if (APawn* ControlledPawn = GetPawn())
    {
        // ✨ 新增 - 检查角色是否正在释放技能（技能期间禁止移动）
        if (AXBCharacterBase* CharBase = Cast<AXBCharacterBase>(ControlledPawn))
        {
            if (UXBCombatComponent* CombatComp = CharBase->GetCombatComponent())
            {
                // 🔧 修改 - 如果战斗组件要求阻止移动，则忽略输入
                if (CombatComp->ShouldBlockMovement())
                {
                    // 技能释放中，不响应移动输入
                    return;
                }
            }
        }

        FVector MoveDirection = CalculateMoveDirection(MoveValue);
        
        ControlledPawn->AddMovementInput(MoveDirection, 1.0f);
    }
}

void AXBPlayerController::HandleDashInputStarted()
{
    if (AXBCharacterBase* CharBase = Cast<AXBCharacterBase>(GetPawn()))
    {
        // ✨ 新增 - 冲刺时也检查是否正在释放技能
        if (UXBCombatComponent* CombatComp = CharBase->GetCombatComponent())
        {
            if (CombatComp->ShouldBlockMovement())
            {
                return;
            }
        }
        // 🔧 修改 - 按键触发冲刺改为持续时间制
        CharBase->TriggerSprint();
    }
}

void AXBPlayerController::HandleDashInputCompleted()
{
    // 🔧 修改 - 冲刺为持续时间制，松开按键不再立即结束
}

/**
 * @brief 处理镜头视角输入
 * @param InputValue 输入值（2D向量）
 * @note   详细流程分析: 读取输入 -> 旋转Yaw/Pitch -> 写入目标偏移
 *         性能/架构注意事项: 仅在配置Pawn期间处理Pitch，避免影响战斗镜头
 */
void AXBPlayerController::HandleLookInput(const FInputActionValue& InputValue)
{
    const FVector2D LookValue = InputValue.Get<FVector2D>();

    // 🔧 修改 - 配置相机Pawn期间使用基类控制器旋转逻辑
    if (CachedConfigPawn.IsValid())
    {
        AddYawInput(LookValue.X);
        AddPitchInput(LookValue.Y);
        return;
    }
}
void AXBPlayerController::HandleCameraZoomInput(const FInputActionValue& InputValue)
{
    // 🔧 修改 - 配置相机Pawn期间不使用自定义镜头缩放
    if (CachedConfigPawn.IsValid())
    {
        return;
    }

    const float ZoomValue = InputValue.Get<float>();
    
    float NewDistance = TargetCameraDistance - (ZoomValue * CameraZoomStep);
    SetCameraDistance(NewDistance);
}

void AXBPlayerController::HandleCameraRotateLeftStarted()
{
    // 🔧 修改 - 配置相机Pawn期间不使用自定义镜头旋转
    if (CachedConfigPawn.IsValid())
    {
        return;
    }

    bIsRotatingLeft = true;
    bIsResettingRotation = false;
}

void AXBPlayerController::HandleCameraRotateLeftCompleted()
{
    bIsRotatingLeft = false;
}

void AXBPlayerController::HandleCameraRotateRightStarted()
{
    // 🔧 修改 - 配置相机Pawn期间不使用自定义镜头旋转
    if (CachedConfigPawn.IsValid())
    {
        return;
    }

    bIsRotatingRight = true;
    bIsResettingRotation = false;
}

void AXBPlayerController::HandleCameraRotateRightCompleted()
{
    bIsRotatingRight = false;
}

void AXBPlayerController::HandleCameraResetInput()
{
    // 🔧 修改 - 配置相机Pawn期间不使用自定义镜头重置
    if (CachedConfigPawn.IsValid())
    {
        return;
    }

    ResetCamera();
}

void AXBPlayerController::HandleAttackInput()
{
    if (AXBPlayerCharacter* PlayerChar = Cast<AXBPlayerCharacter>(GetPawn()))
    {
        if (UXBCombatComponent* CombatComp = PlayerChar->GetCombatComponent())
        {
            bool bSuccess = CombatComp->PerformBasicAttack();
            UE_LOG(LogTemp, Log, TEXT("攻击输入触发 - 结果: %s"), 
                bSuccess ? TEXT("成功") : TEXT("失败/冷却中"));
        }
    }
}

void AXBPlayerController::HandleSkillInput()
{
    if (AXBPlayerCharacter* PlayerChar = Cast<AXBPlayerCharacter>(GetPawn()))
    {
        if (UXBCombatComponent* CombatComp = PlayerChar->GetCombatComponent())
        {
            bool bSuccess = CombatComp->PerformSpecialSkill();
            UE_LOG(LogTemp, Log, TEXT("技能输入触发 - 结果: %s"), 
                bSuccess ? TEXT("成功") : TEXT("失败/冷却中"));
        }
    }
}

void AXBPlayerController::HandleRecallInput()
{
    if (AXBPlayerCharacter* PlayerChar = Cast<AXBPlayerCharacter>(GetPawn()))
    {
        PlayerChar->RecallAllSoldiers();
        UE_LOG(LogTemp, Log, TEXT("Recall Input Triggered"));
    }
}

/**
 * @brief 处理生成主将输入
 * @return 无
 * @note   详细流程分析: 校验配置Pawn -> 调用GameMode生成主将 -> 切换控制
 *         性能/架构注意事项: 仅在配置阶段触发，避免重复生成
 */
void AXBPlayerController::HandleSpawnLeaderInput()
{
    if (UWorld* World = GetWorld())
    {
        if (AXBGameMode* GameMode = World->GetAuthGameMode<AXBGameMode>())
        {
            // 🔧 修改 - 仅在配置阶段且控制配置Pawn时生成主将
            if (!CachedConfigPawn.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("当前未控制配置Pawn，忽略生成主将请求"));
                return;
            }

            if (GameMode->SpawnPlayerLeader(this))
            {
                UE_LOG(LogTemp, Log, TEXT("已确认进入游戏阶段，主将生成完成"));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("生成主将失败或条件未满足"));
            }
        }
    }
}
