/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/UI/XBWorldHealthBarComponent.cpp

/**
 * @file XBWorldHealthBarComponent.cpp
 * @brief 世界空间血条组件实现
 *
 * @note 🔧 修改 - 使用 ForceRefreshDisplay 确保数据正确刷新
 */

#include "UI/XBWorldHealthBarComponent.h"
#include "Character/XBCharacterBase.h"
#include "GAS/XBAttributeSet.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/XBLeaderHealthWidget.h"


UXBWorldHealthBarComponent::UXBWorldHealthBarComponent() {
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = true;

  SetCollisionEnabled(ECollisionEnabled::NoCollision);
  SetAbsolute(false, false, true);
}

void UXBWorldHealthBarComponent::BeginPlay() {
  Super::BeginPlay();

  CachedOwner = Cast<AXBCharacterBase>(GetOwner());

  if (!CachedOwner.IsValid()) {
    UE_LOG(LogTemp, Warning,
           TEXT("WorldHealthBarComponent: Owner 不是 XBCharacterBase"));
    return;
  }

  CachedOwnerScale = CachedOwner->GetActorScale3D().Z;

  if (bHideForLocalPlayer) {
    APawn *OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn && OwnerPawn->IsLocallyControlled()) {
      SetVisibility(false);
      UE_LOG(LogTemp, Log, TEXT("WorldHealthBarComponent: 对本地玩家隐藏血条"));
      return;
    }
  }

  ApplyConfiguration();
  InitializeHealthWidget();

  UE_LOG(LogTemp, Log, TEXT("WorldHealthBarComponent: 初始化完成"));
}

void UXBWorldHealthBarComponent::TickComponent(
    float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  // ✨ 新增 - 如果将领死亡，停止所有更新
  if (CachedOwner.IsValid() && CachedOwner->IsDead()) {
    // 确保血条已隐藏
    if (IsVisible()) {
      SetVisibility(false);
    }
    return;
  }

  UpdatePositionWithScaleCompensation();

  if (bEnableDistanceFade) {
    UpdateDistanceBasedVisibility();
  }

  if (bShowOnlyWhenDamaged && CachedOwner.IsValid()) {
    UAbilitySystemComponent *ASC = CachedOwner->GetAbilitySystemComponent();
    if (ASC) {
      float CurrentHealth =
          ASC->GetNumericAttribute(UXBAttributeSet::GetHealthAttribute());
      float MaxHealth =
          ASC->GetNumericAttribute(UXBAttributeSet::GetMaxHealthAttribute());

      if (LastHealthValue > 0.0f && CurrentHealth < LastHealthValue) {
        DamageShowTimer = DamageShowDuration;
        if (!bManuallyHidden) {
          SetHealthBarVisible(true);
        }
      }

      LastHealthValue = CurrentHealth;

      if (DamageShowTimer > 0.0f) {
        DamageShowTimer -= DeltaTime;

        if (DamageShowTimer <= 0.0f &&
            FMath::IsNearlyEqual(CurrentHealth, MaxHealth, 1.0f)) {
          SetVisibility(false);
        }
      }
    }
  }
}

void UXBWorldHealthBarComponent::UpdatePositionWithScaleCompensation() {
  if (!CachedOwner.IsValid()) {
    return;
  }

  AActor *Owner = CachedOwner.Get();
  FVector OwnerLocation = Owner->GetActorLocation();
  FVector TargetWorldLocation = OwnerLocation + HealthBarOffset;
  SetWorldLocation(TargetWorldLocation);
}

void UXBWorldHealthBarComponent::ApplyConfiguration() {
  ApplySpaceMode();

  FVector2D FinalSize = HealthBarDrawSize * HealthBarScale;
  SetDrawSize(FinalSize);

  UpdatePositionWithScaleCompensation();
}

void UXBWorldHealthBarComponent::ApplySpaceMode() {
  switch (HealthBarSpaceMode) {
  case EXBHealthBarSpace::Screen:
    SetWidgetSpace(EWidgetSpace::Screen);
    break;

  case EXBHealthBarSpace::World:
    SetWidgetSpace(EWidgetSpace::World);
    SetTwoSided(bTwoSided);
    break;
  }
}

void UXBWorldHealthBarComponent::SetHealthBarSpaceMode(
    EXBHealthBarSpace NewSpaceMode) {
  if (HealthBarSpaceMode == NewSpaceMode) {
    return;
  }

  HealthBarSpaceMode = NewSpaceMode;
  ApplySpaceMode();
}

void UXBWorldHealthBarComponent::InitializeHealthWidget() {
  if (!HealthWidgetClass) {
    UE_LOG(LogTemp, Warning,
           TEXT("WorldHealthBarComponent: 未配置 HealthWidgetClass"));
    return;
  }

  SetWidgetClass(HealthWidgetClass);

  UXBLeaderHealthWidget *HealthWidget = GetHealthWidget();
  if (HealthWidget && CachedOwner.IsValid()) {
    // ✨ 新增 - 先应用颜色配置
    if (bUseRandomHealthBarColor) {
      HealthWidget->bUseRandomColor = true;
    } else {
      HealthWidget->HealthBarFillColor = HealthBarFillColor;
    }
    
    HealthWidget->SetOwningLeader(CachedOwner.Get());
    UE_LOG(LogTemp, Log,
           TEXT("WorldHealthBarComponent: 血条 Widget 初始化成功"));
  }
}

// 🔧 修改 - 完全重写距离可见性更新
void UXBWorldHealthBarComponent::UpdateDistanceBasedVisibility() {
  if (bManuallyHidden) {
    return;
  }

  APlayerController *PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
  if (!PC) {
    return;
  }

  FVector CameraLocation;
  FRotator CameraRotation;
  PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

  float Distance = FVector::Dist(GetComponentLocation(), CameraLocation);

  bool bWasVisible = IsVisible();

  if (Distance > MaxVisibleDistance) {
    if (bWasVisible) {
      SetVisibility(false);
    }
    return;
  }

  float Alpha = 1.0f;

  if (Distance > FadeStartDistance) {
    Alpha = 1.0f - (Distance - FadeStartDistance) /
                       (MaxVisibleDistance - FadeStartDistance);
    Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
  }

  UUserWidget *WidgetInstance = GetWidget();
  if (WidgetInstance) {
    WidgetInstance->SetRenderOpacity(Alpha);
  }

  // 🔧 修改 - 从隐藏变为显示时强制刷新
  if (!bWasVisible) {
    SetVisibility(true);

    // 确保 Widget 初始化并强制刷新
    EnsureWidgetInitialized();
    ForceRefreshHealthBar();

    UE_LOG(LogTemp, Log,
           TEXT("WorldHealthBarComponent: 从隐藏恢复显示，强制刷新数据"));
  } else {
    SetVisibility(true);
  }
}

void UXBWorldHealthBarComponent::EnsureWidgetInitialized() {
  UXBLeaderHealthWidget *HealthWidget = GetHealthWidget();

  if (!HealthWidget) {
    UE_LOG(LogTemp, Warning,
           TEXT("WorldHealthBarComponent: Widget 不存在，重新初始化"));
    InitializeHealthWidget();
    return;
  }

  if (HealthWidget->GetOwningLeader() != CachedOwner.Get()) {
    if (CachedOwner.IsValid()) {
      HealthWidget->SetOwningLeader(CachedOwner.Get());
      UE_LOG(LogTemp, Log, TEXT("WorldHealthBarComponent: 重新关联 Owner"));
    }
  }
}

void UXBWorldHealthBarComponent::SetHealthBarOffset(const FVector &NewOffset) {
  HealthBarOffset = NewOffset;
  UpdatePositionWithScaleCompensation();
}

void UXBWorldHealthBarComponent::SetHealthBarSize(const FVector2D &NewSize) {
  HealthBarDrawSize = NewSize;

  FVector2D FinalSize = HealthBarDrawSize * HealthBarScale;
  SetDrawSize(FinalSize);
}

void UXBWorldHealthBarComponent::SetHealthBarScale(float NewScale) {
  HealthBarScale = FMath::Clamp(NewScale, 0.1f, 10.0f);

  FVector2D FinalSize = HealthBarDrawSize * HealthBarScale;
  SetDrawSize(FinalSize);
}

// 🔧 修改 - 设置可见性时强制刷新
void UXBWorldHealthBarComponent::SetHealthBarVisible(bool bNewVisible) {
  // ✨ 新增 - 如果将领死亡，强制隐藏
  if (CachedOwner.IsValid() && CachedOwner->IsDead()) {
    bManuallyHidden = true;
    SetVisibility(false);
    return;
  }

  bool bWasVisible = IsVisible() && !bManuallyHidden;

  bManuallyHidden = !bNewVisible;
  SetVisibility(bNewVisible);

  // 从隐藏变为显示时强制刷新
  if (bNewVisible && !bWasVisible) {
    EnsureWidgetInitialized();
    ForceRefreshHealthBar();
  }
}

bool UXBWorldHealthBarComponent::IsHealthBarVisible() const {
  return IsVisible() && !bManuallyHidden;
}

UXBLeaderHealthWidget *UXBWorldHealthBarComponent::GetHealthWidget() const {
  return Cast<UXBLeaderHealthWidget>(GetWidget());
}

void UXBWorldHealthBarComponent::RefreshHealthBar() {
  // ✨ 新增 - 如果将领死亡，不刷新
  if (CachedOwner.IsValid() && CachedOwner->IsDead()) {
    return;
  }

  UXBLeaderHealthWidget *HealthWidget = GetHealthWidget();
  if (HealthWidget) {
    if (!HealthWidget->GetOwningLeader() && CachedOwner.IsValid()) {
      HealthWidget->SetOwningLeader(CachedOwner.Get());
    }

    HealthWidget->RefreshDisplay();
  }
}

// 🔧 新增 - 刷新名称显示（用于角色名称变更后通知刷新）
void UXBWorldHealthBarComponent::RefreshNameDisplay() {
  // 如果将领死亡，不刷新
  if (CachedOwner.IsValid() && CachedOwner->IsDead()) {
    return;
  }

  UXBLeaderHealthWidget *HealthWidget = GetHealthWidget();
  if (HealthWidget) {
    // 清除名称缓存并强制刷新名称
    HealthWidget->ClearCache();
    HealthWidget->ForceRefreshDisplay();

    UE_LOG(LogTemp, Log,
           TEXT("WorldHealthBarComponent: 刷新名称显示完成, CharacterName=%s"),
           CachedOwner.IsValid() ? *CachedOwner->CharacterName
                                 : TEXT("[无效]"));
  }
}

// ✨ 新增 - 强制刷新血条
void UXBWorldHealthBarComponent::ForceRefreshHealthBar() {
  // ✨ 新增 - 如果将领死亡，不刷新
  if (CachedOwner.IsValid() && CachedOwner->IsDead()) {
    return;
  }

  UXBLeaderHealthWidget *HealthWidget = GetHealthWidget();
  if (HealthWidget) {
    // 确保 Owner 已设置
    if (!HealthWidget->GetOwningLeader() && CachedOwner.IsValid()) {
      HealthWidget->SetOwningLeader(CachedOwner.Get());
    }

    // 清除缓存并强制刷新
    HealthWidget->ClearCache();
    HealthWidget->ForceRefreshDisplay();

    UE_LOG(LogTemp, Log, TEXT("WorldHealthBarComponent: 强制刷新血条完成"));
  } else {
    UE_LOG(
        LogTemp, Warning,
        TEXT("WorldHealthBarComponent: ForceRefreshHealthBar - Widget 为空"));
  }
}

void UXBWorldHealthBarComponent::SetHealthBarColor(FLinearColor InColor) {
  HealthBarFillColor = InColor;
  
  UXBLeaderHealthWidget *HealthWidget = GetHealthWidget();
  if (HealthWidget) {
    HealthWidget->SetHealthBarColor(InColor);
  }
}

void UXBWorldHealthBarComponent::ApplyRandomHealthBarColor() {
  UXBLeaderHealthWidget *HealthWidget = GetHealthWidget();
  if (HealthWidget) {
    HealthWidget->ApplyRandomColor();
    // 同步颜色到组件缓存
    HealthBarFillColor = HealthWidget->HealthBarFillColor;
  }
}
