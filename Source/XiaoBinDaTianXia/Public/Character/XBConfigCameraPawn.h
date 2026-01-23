/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Character/XBConfigCameraPawn.h

/**
 * @file XBConfigCameraPawn.h
 * @brief 配置阶段浮空相机Pawn
 * 
 * @note ✨ 新增 - 配置阶段先行操控
 * @note 🔧 修改 - 添加 Actor 放置组件
 * @note 🔧 修改 - 添加按键触发显示放置菜单（使用增强输入系统）
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DefaultPawn.h"
#include "XBConfigCameraPawn.generated.h"

class UXBActorPlacementComponent;
class UXBPlacementConfigAsset;
class UXBInputConfig;
class UInputAction;
struct FInputActionValue;

// 请求显示放置菜单代理（供蓝图绑定）
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRequestShowPlacementMenu);

// 请求隐藏放置菜单代理
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRequestHidePlacementMenu);

// 配置确认代理（回车键按下时触发，在销毁 Pawn 之前执行）
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConfigConfirmed);

/**
 * @brief 配置阶段浮空相机Pawn
 * @note 继承 DefaultPawn，复用其移动与旋转逻辑
 *       🔧 修改 - 添加 Actor 放置功能
 *       🔧 修改 - 添加按键触发显示放置菜单（使用增强输入系统）
 */
UCLASS()
class XIAOBINDATIANXIA_API AXBConfigCameraPawn : public ADefaultPawn
{
	GENERATED_BODY()

public:
	AXBConfigCameraPawn();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

public:
	// ============ 覆盖移动/旋转输入 ============

	/** 覆盖移动输入，根据 bCanMove 决定是否允许移动 */
	virtual void AddMovementInput(FVector WorldDirection, float ScaleValue = 1.0f, bool bForce = false) override;

	/** 覆盖控制器Yaw输入，根据 bCanRotate 决定是否允许旋转 */
	virtual void AddControllerYawInput(float Val) override;

	/** 覆盖控制器Pitch输入，根据 bCanRotate 决定是否允许旋转 */
	virtual void AddControllerPitchInput(float Val) override;

	// ============ 放置系统 ============

	/**
	 * @brief 获取放置组件
	 * @return 放置组件指针
	 */
	UFUNCTION(BlueprintPure, Category = "XB|放置", meta = (DisplayName = "获取放置组件"))
	UXBActorPlacementComponent* GetPlacementComponent() const { return PlacementComponent; }

	/**
	 * @brief 切换放置菜单显示状态
	 * @note 按键触发，显示/隐藏放置菜单
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|放置", meta = (DisplayName = "切换放置菜单"))
	void TogglePlacementMenu();

	/**
	 * @brief 显示放置菜单
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|放置", meta = (DisplayName = "显示放置菜单"))
	void ShowPlacementMenu();

	/**
	 * @brief 隐藏放置菜单
	 */
	UFUNCTION(BlueprintCallable, Category = "XB|放置", meta = (DisplayName = "隐藏放置菜单"))
	void HidePlacementMenu();

	/**
	 * @brief 放置菜单是否正在显示
	 */
	UFUNCTION(BlueprintPure, Category = "XB|放置", meta = (DisplayName = "菜单是否显示"))
	bool IsPlacementMenuVisible() const { return bIsMenuVisible; }

	/**
	 * @brief 当前是否可以移动
	 */
	UFUNCTION(BlueprintPure, Category = "XB|放置", meta = (DisplayName = "是否可移动"))
	bool CanMove() const { return bCanMove; }

	/**
	 * @brief 当前是否可以旋转
	 */
	UFUNCTION(BlueprintPure, Category = "XB|放置", meta = (DisplayName = "是否可旋转"))
	bool CanRotate() const { return bCanRotate; }

	// ============ 事件代理 ============

	/** 请求显示放置菜单事件（蓝图绑定 UI 创建） */
	UPROPERTY(BlueprintAssignable, Category = "XB|放置|事件")
	FOnRequestShowPlacementMenu OnRequestShowPlacementMenu;

	/** 请求隐藏放置菜单事件 */
	UPROPERTY(BlueprintAssignable, Category = "XB|放置|事件")
	FOnRequestHidePlacementMenu OnRequestHidePlacementMenu;

	/** 配置确认事件（回车键按下时触发，在销毁 ConfigCameraPawn 之前执行） */
	UPROPERTY(BlueprintAssignable, Category = "XB|放置|事件")
	FOnConfigConfirmed OnConfigConfirmed;

protected:
	// ============ 增强输入回调 ============

	/** 切换放置菜单 */
	void Input_TogglePlacementMenu(const FInputActionValue& Value);

	/** 处理点击事件（用于放置确认） */
	void Input_PlacementClick(const FInputActionValue& Value);

	/** 处理取消事件（ESC 取消操作） */
	void Input_PlacementCancel(const FInputActionValue& Value);

	/** 处理删除事件（Delete 删除选中） */
	void Input_PlacementDelete(const FInputActionValue& Value);

	/** 处理旋转事件（滚轮旋转预览/选中 Actor） */
	void Input_PlacementRotate(const FInputActionValue& Value);

	/** 处理回车确认事件（配置完成） */
	void Input_ConfigConfirm(const FInputActionValue& Value);

	// ============ 配置引用 ============

	/** 放置管理组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "XB|放置", meta = (DisplayName = "放置组件"))
	TObjectPtr<UXBActorPlacementComponent> PlacementComponent;

	/** 放置配置 DataAsset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|放置", meta = (DisplayName = "放置配置"))
	TObjectPtr<UXBPlacementConfigAsset> PlacementConfig;

	/** 输入配置 DataAsset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|输入", meta = (DisplayName = "输入配置"))
	TObjectPtr<UXBInputConfig> InputConfig;

private:
	/** 菜单是否正在显示 */
	bool bIsMenuVisible = false;

	/** 是否允许移动（菜单显示时仍允许移动） */
	bool bCanMove = true;

	/** 是否允许旋转（菜单显示时禁止旋转） */
	bool bCanRotate = true;
};
