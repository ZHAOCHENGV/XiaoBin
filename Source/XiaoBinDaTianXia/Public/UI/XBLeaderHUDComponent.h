/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/UI/XBLeaderHUDComponent.h

/**
 * @file XBLeaderHUDComponent.h
 * @brief 将领 HUD 组件 - 管理将领头顶/屏幕上的 UI 显示
 * 
 * @note ✨ 新增 - 用于管理将领相关的 UI 显示
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "XBLeaderHUDComponent.generated.h"

class UXBLeaderHealthWidget;
class AXBCharacterBase;

/**
 * @brief 将领 HUD 组件
 * 
 * @details 挂载在将领角色上，负责创建和管理血量 UI
 *          支持两种模式：
 *          1. 屏幕 UI（始终显示在屏幕固定位置）
 *          2. 世界 UI（显示在角色头顶，需要 WidgetComponent）
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName = "XB Leader HUD"))
class XIAOBINDATIANXIA_API UXBLeaderHUDComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UXBLeaderHUDComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // ==================== UI 控制 ====================

    /**
     * @brief 显示血量 UI
     */
    UFUNCTION(BlueprintCallable, Category = "XB|HUD", meta = (DisplayName = "显示血量UI"))
    void ShowHealthUI();

    /**
     * @brief 隐藏血量 UI
     */
    UFUNCTION(BlueprintCallable, Category = "XB|HUD", meta = (DisplayName = "隐藏血量UI"))
    void HideHealthUI();

    /**
     * @brief 切换血量 UI 显示状态
     */
    UFUNCTION(BlueprintCallable, Category = "XB|HUD", meta = (DisplayName = "切换血量UI"))
    void ToggleHealthUI();

    /**
     * @brief 获取血量 Widget 实例
     */
    UFUNCTION(BlueprintCallable, Category = "XB|HUD", meta = (DisplayName = "获取血量Widget"))
    UXBLeaderHealthWidget* GetHealthWidget() const { return HealthWidget; }

    /**
     * @brief 强制刷新 UI 显示
     */
    UFUNCTION(BlueprintCallable, Category = "XB|HUD", meta = (DisplayName = "刷新UI"))
    void RefreshUI();

protected:
    /**
     * @brief 创建血量 UI
     */
    void CreateHealthUI();

    /**
     * @brief 销毁血量 UI
     */
    void DestroyHealthUI();

protected:
    // ==================== 配置 ====================

    /**
     * @brief 血量 Widget 类
     * @note 需要设置为继承自 UXBLeaderHealthWidget 的蓝图类
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|HUD|配置", meta = (DisplayName = "血量Widget类"))
    TSubclassOf<UXBLeaderHealthWidget> HealthWidgetClass;

    /**
     * @brief 是否在游戏开始时自动创建 UI
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|HUD|配置", meta = (DisplayName = "自动创建UI"))
    bool bAutoCreateUI = true;

    /**
     * @brief UI 显示的 ZOrder（越大越靠前）
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|HUD|配置", meta = (DisplayName = "ZOrder"))
    int32 UIZOrder = 0;

    // ==================== 运行时数据 ====================

    /**
     * @brief 血量 Widget 实例
     */
    UPROPERTY(BlueprintReadOnly, Category = "XB|HUD")
    TObjectPtr<UXBLeaderHealthWidget> HealthWidget;

    /**
     * @brief 缓存的将领引用
     */
    UPROPERTY()
    TWeakObjectPtr<AXBCharacterBase> CachedLeader;
};
