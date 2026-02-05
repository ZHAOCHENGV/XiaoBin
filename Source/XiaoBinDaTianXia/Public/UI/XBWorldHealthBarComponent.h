/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/UI/XBWorldHealthBarComponent.h

/**
 * @file XBWorldHealthBarComponent.h
 * @brief 世界空间血条组件 - 在角色头顶显示血量 UI
 *
 * @note 🔧 修改 - 修复距离隐藏后重新显示时数据丢失的问题
 */

#pragma once

#include "Components/WidgetComponent.h"
#include "CoreMinimal.h"
#include "XBWorldHealthBarComponent.generated.h"


class UXBLeaderHealthWidget;
class AXBCharacterBase;

/**
 * @brief 血条显示空间模式
 */
UENUM(BlueprintType)
enum class EXBHealthBarSpace : uint8 {
  Screen UMETA(DisplayName = "屏幕空间"),
  World UMETA(DisplayName = "世界空间")
};

/**
 * @brief 世界空间血条组件
 */
UCLASS(ClassGroup = (Custom),
       meta = (BlueprintSpawnableComponent, DisplayName = "XB 世界血条组件"))
class XIAOBINDATIANXIA_API UXBWorldHealthBarComponent
    : public UWidgetComponent {
  GENERATED_BODY()

public:
  UXBWorldHealthBarComponent();

  virtual void BeginPlay() override;
  virtual void
  TickComponent(float DeltaTime, ELevelTick TickType,
                FActorComponentTickFunction *ThisTickFunction) override;

  // ==================== 空间模式控制 ====================

  UFUNCTION(BlueprintCallable, Category = "XB|UI",
            meta = (DisplayName = "设置显示空间"))
  void SetHealthBarSpaceMode(EXBHealthBarSpace NewSpaceMode);

  UFUNCTION(BlueprintCallable, Category = "XB|UI",
            meta = (DisplayName = "获取显示空间"))
  EXBHealthBarSpace GetHealthBarSpaceMode() const { return HealthBarSpaceMode; }

  // ==================== 配置接口 ====================

  UFUNCTION(BlueprintCallable, Category = "XB|UI",
            meta = (DisplayName = "设置血条偏移"))
  void SetHealthBarOffset(const FVector &NewOffset);

  UFUNCTION(BlueprintCallable, Category = "XB|UI",
            meta = (DisplayName = "获取血条偏移"))
  FVector GetHealthBarOffset() const { return HealthBarOffset; }

  UFUNCTION(BlueprintCallable, Category = "XB|UI",
            meta = (DisplayName = "设置血条尺寸"))
  void SetHealthBarSize(const FVector2D &NewSize);

  UFUNCTION(BlueprintCallable, Category = "XB|UI",
            meta = (DisplayName = "获取血条尺寸"))
  FVector2D GetHealthBarSize() const { return HealthBarDrawSize; }

  UFUNCTION(BlueprintCallable, Category = "XB|UI",
            meta = (DisplayName = "设置血条缩放"))
  void SetHealthBarScale(float NewScale);

  UFUNCTION(BlueprintCallable, Category = "XB|UI",
            meta = (DisplayName = "获取血条缩放"))
  float GetHealthBarScale() const { return HealthBarScale; }

  UFUNCTION(BlueprintCallable, Category = "XB|UI",
            meta = (DisplayName = "设置血条可见"))
  void SetHealthBarVisible(bool bNewVisible);

  UFUNCTION(BlueprintCallable, Category = "XB|UI",
            meta = (DisplayName = "血条是否可见"))
  bool IsHealthBarVisible() const;

  // ==================== Widget 访问 ====================

  UFUNCTION(BlueprintCallable, Category = "XB|UI",
            meta = (DisplayName = "获取血条Widget"))
  UXBLeaderHealthWidget *GetHealthWidget() const;

  UFUNCTION(BlueprintCallable, Category = "XB|UI",
            meta = (DisplayName = "刷新血条"))
  void RefreshHealthBar();

  /** @brief 刷新名称显示（用于角色名称变更后通知刷新） */
  UFUNCTION(BlueprintCallable, Category = "XB|UI",
            meta = (DisplayName = "刷新名称显示"))
  void RefreshNameDisplay();

  // ✨ 新增 - 血条颜色控制
  UFUNCTION(BlueprintCallable, Category = "XB|UI",
            meta = (DisplayName = "设置血条颜色"))
  void SetHealthBarColor(FLinearColor InColor);

  UFUNCTION(BlueprintCallable, Category = "XB|UI",
            meta = (DisplayName = "应用随机颜色"))
  void ApplyRandomHealthBarColor();

protected:
  void InitializeHealthWidget();
  void UpdateDistanceBasedVisibility();
  void ApplyConfiguration();
  void ApplySpaceMode();
  void UpdatePositionWithScaleCompensation();

  // ✨ 新增 - 确保 Widget 初始化
  void EnsureWidgetInitialized();

  /**
   * @brief 强制刷新血条（清除缓存并重新获取数据）
   */
  UFUNCTION(BlueprintCallable, Category = "XB|UI",
            meta = (DisplayName = "强制刷新血条"))
  void ForceRefreshHealthBar();

protected:
  // ==================== 空间模式配置 ====================

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|UI|空间模式",
            meta = (DisplayName = "显示空间"))
  EXBHealthBarSpace HealthBarSpaceMode = EXBHealthBarSpace::Screen;

  UPROPERTY(
      EditAnywhere, BlueprintReadWrite, Category = "XB|UI|空间模式",
      meta = (DisplayName = "双面渲染",
              EditCondition = "HealthBarSpaceMode == EXBHealthBarSpace::World"))
  bool bTwoSided = false;

  // ==================== 配置属性 ====================

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|UI|配置",
            meta = (DisplayName = "血条Widget类"))
  TSubclassOf<UXBLeaderHealthWidget> HealthWidgetClass;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|UI|配置",
            meta = (DisplayName = "位置偏移"))
  FVector HealthBarOffset = FVector(0.0f, 0.0f, 120.0f);

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|UI|配置",
            meta = (DisplayName = "基础尺寸(像素)"))
  FVector2D HealthBarDrawSize = FVector2D(200.0f, 30.0f);

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|UI|配置",
            meta = (DisplayName = "缩放倍率", ClampMin = "0.1",
                    ClampMax = "10.0"))
  float HealthBarScale = 1.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|UI|距离",
            meta = (DisplayName = "启用距离隐藏"))
  bool bEnableDistanceFade = true;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|UI|距离",
            meta = (DisplayName = "最大可见距离", ClampMin = "100.0",
                    EditCondition = "bEnableDistanceFade"))
  float MaxVisibleDistance = 3000.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|UI|距离",
            meta = (DisplayName = "淡出起始距离", ClampMin = "100.0",
                    EditCondition = "bEnableDistanceFade"))
  float FadeStartDistance = 2000.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|UI|配置",
            meta = (DisplayName = "对自己隐藏"))
  bool bHideForLocalPlayer = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|UI|配置",
            meta = (DisplayName = "仅受伤时显示"))
  bool bShowOnlyWhenDamaged = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|UI|配置",
            meta = (DisplayName = "受伤显示时长", ClampMin = "0.0",
                    EditCondition = "bShowOnlyWhenDamaged"))
  float DamageShowDuration = 5.0f;

  // ✨ 新增 - 血条颜色配置
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|UI|颜色",
            meta = (DisplayName = "血条填充颜色"))
  FLinearColor HealthBarFillColor = FLinearColor(0.0f, 0.8f, 0.2f, 1.0f);

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|UI|颜色",
            meta = (DisplayName = "启用随机颜色", ToolTip = "启用后初始化时会自动应用随机颜色"))
  bool bUseRandomHealthBarColor = false;

private:
  UPROPERTY()
  TWeakObjectPtr<AXBCharacterBase> CachedOwner;

  bool bManuallyHidden = false;
  float DamageShowTimer = 0.0f;
  float LastHealthValue = -1.0f;
  float CachedOwnerScale = 1.0f;
};
