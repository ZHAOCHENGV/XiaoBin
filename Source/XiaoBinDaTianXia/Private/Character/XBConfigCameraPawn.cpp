/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Character/XBConfigCameraPawn.cpp

/**
 * @file XBConfigCameraPawn.cpp
 * @brief 配置阶段浮空相机Pawn实现
 * 
 * @note ✨ 新增 - 支持自由飞行与镜头控制
 * @note 🔧 修改 - 添加 Actor 放置组件
 * @note 🔧 修改 - 添加按键触发显示放置菜单（使用增强输入系统）
 */

#include "Character/XBConfigCameraPawn.h"
#include "Config/XBActorPlacementComponent.h"
#include "Config/XBPlacementConfigAsset.h"
#include "Input/XBInputConfig.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Utils/XBLogCategories.h"

AXBConfigCameraPawn::AXBConfigCameraPawn()
{
	// 创建放置组件
	PlacementComponent = CreateDefaultSubobject<UXBActorPlacementComponent>(TEXT("PlacementComponent"));
}

void AXBConfigCameraPawn::BeginPlay()
{
	Super::BeginPlay();

	// 将放置配置传递给组件
	if (PlacementComponent && PlacementConfig)
	{
		PlacementComponent->SetPlacementConfig(PlacementConfig);
	}

	// 初始隐藏鼠标光标（配置阶段默认漫游模式）
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void AXBConfigCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 获取增强输入组件
	UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComp)
	{
		UE_LOG(LogXBConfig, Warning, TEXT("[配置Pawn] 未找到 EnhancedInputComponent"));
		return;
	}

	// 检查输入配置
	if (!InputConfig)
	{
		UE_LOG(LogXBConfig, Warning, TEXT("[配置Pawn] 未配置 InputConfig"));
		return;
	}

	// 绑定切换放置菜单
	if (InputConfig->TogglePlacementMenuAction)
	{
		EnhancedInputComp->BindAction(
			InputConfig->TogglePlacementMenuAction,
			ETriggerEvent::Triggered,
			this,
			&AXBConfigCameraPawn::Input_TogglePlacementMenu
		);
	}

	// 绑定放置点击
	if (InputConfig->PlacementClickAction)
	{
		EnhancedInputComp->BindAction(
			InputConfig->PlacementClickAction,
			ETriggerEvent::Triggered,
			this,
			&AXBConfigCameraPawn::Input_PlacementClick
		);
	}

	// 绑定放置取消
	if (InputConfig->PlacementCancelAction)
	{
		EnhancedInputComp->BindAction(
			InputConfig->PlacementCancelAction,
			ETriggerEvent::Triggered,
			this,
			&AXBConfigCameraPawn::Input_PlacementCancel
		);
	}

	// 绑定放置删除
	if (InputConfig->PlacementDeleteAction)
	{
		EnhancedInputComp->BindAction(
			InputConfig->PlacementDeleteAction,
			ETriggerEvent::Triggered,
			this,
			&AXBConfigCameraPawn::Input_PlacementDelete
		);
	}

	// 绑定放置旋转
	if (InputConfig->PlacementRotateAction)
	{
		EnhancedInputComp->BindAction(
			InputConfig->PlacementRotateAction,
			ETriggerEvent::Triggered,
			this,
			&AXBConfigCameraPawn::Input_PlacementRotate
		);
	}

	// 🔧 修改 - SpawnLeaderAction 的处理已移至 XBPlayerController::HandleSpawnLeaderInput()
	// 该函数会在生成主将之前触发 OnConfigConfirmed 事件

	UE_LOG(LogXBConfig, Log, TEXT("[配置Pawn] 放置系统输入绑定完成"));
}

void AXBConfigCameraPawn::AddMovementInput(FVector WorldDirection, float ScaleValue, bool bForce)
{
	// 根据 bCanMove 决定是否允许移动
	if (!bCanMove)
	{
		return;
	}
	Super::AddMovementInput(WorldDirection, ScaleValue, bForce);
}

void AXBConfigCameraPawn::AddControllerYawInput(float Val)
{
	// 根据 bCanRotate 决定是否允许旋转
	if (!bCanRotate)
	{
		return;
	}
	Super::AddControllerYawInput(Val);
}

void AXBConfigCameraPawn::AddControllerPitchInput(float Val)
{
	// 根据 bCanRotate 决定是否允许旋转
	if (!bCanRotate)
	{
		return;
	}
	Super::AddControllerPitchInput(Val);
}

void AXBConfigCameraPawn::TogglePlacementMenu()
{
	if (bIsMenuVisible)
	{
		HidePlacementMenu();
	}
	else
	{
		ShowPlacementMenu();
	}
}

void AXBConfigCameraPawn::ShowPlacementMenu()
{
	if (bIsMenuVisible)
	{
		return;
	}

	bIsMenuVisible = true;
	// 菜单显示时：允许移动，禁止旋转
	bCanMove = true;
	bCanRotate = false;

	// 显示鼠标光标
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;
		// 使用 GameAndUI 模式保持按键响应能力
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}

	// 广播事件，让蓝图处理 UI 创建
	OnRequestShowPlacementMenu.Broadcast();

	UE_LOG(LogXBConfig, Log, TEXT("[配置Pawn] 显示放置菜单"));
}

void AXBConfigCameraPawn::HidePlacementMenu()
{
	if (!bIsMenuVisible)
	{
		return;
	}

	bIsMenuVisible = false;
	// 恢复移动和旋转
	bCanMove = true;
	bCanRotate = true;

	// 隐藏鼠标光标
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}

	// 取消当前放置操作
	if (PlacementComponent)
	{
		PlacementComponent->CancelOperation();
	}

	// 广播事件，让蓝图处理 UI 销毁
	OnRequestHidePlacementMenu.Broadcast();

	UE_LOG(LogXBConfig, Log, TEXT("[配置Pawn] 隐藏放置菜单"));
}

void AXBConfigCameraPawn::Input_TogglePlacementMenu(const FInputActionValue& Value)
{
	TogglePlacementMenu();
}

void AXBConfigCameraPawn::Input_PlacementClick(const FInputActionValue& Value)
{
	// 只有在菜单显示时处理点击
	if (bIsMenuVisible && PlacementComponent)
	{
		PlacementComponent->HandleClick();
	}
}

void AXBConfigCameraPawn::Input_PlacementCancel(const FInputActionValue& Value)
{
	// ESC 只取消当前预览或编辑操作，不关闭菜单
	// 菜单的关闭通过 Tab 键（TogglePlacementMenu）来触发
	if (PlacementComponent && PlacementComponent->GetPlacementState() != EXBPlacementState::Idle)
	{
		PlacementComponent->CancelOperation();
	}
}

void AXBConfigCameraPawn::Input_PlacementDelete(const FInputActionValue& Value)
{
	if (!PlacementComponent)
	{
		return;
	}

	// 根据当前状态决定删除行为
	const EXBPlacementState State = PlacementComponent->GetPlacementState();

	if (State == EXBPlacementState::Idle)
	{
		// 空闲状态删除悬停的 Actor
		PlacementComponent->DeleteHoveredActor();
	}
	else if (State == EXBPlacementState::Editing)
	{
		// 编辑状态删除选中的 Actor
		PlacementComponent->DeleteSelectedActor();
	}
}

void AXBConfigCameraPawn::Input_PlacementRotate(const FInputActionValue& Value)
{
	// 获取滚轮值并旋转
	const float RotateValue = Value.Get<float>();
	if (PlacementComponent && FMath::Abs(RotateValue) > KINDA_SMALL_NUMBER)
	{
		PlacementComponent->RotateActor(RotateValue);
	}
}

void AXBConfigCameraPawn::Input_ConfigConfirm(const FInputActionValue& Value)
{
	// 广播配置确认事件（在销毁 Pawn 之前执行蓝图绑定的逻辑）
	OnConfigConfirmed.Broadcast();
	UE_LOG(LogXBConfig, Log, TEXT("[配置Pawn] 配置确认事件已广播"));
}
