/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/UI/XBLeaderHUDComponent.cpp

/**
 * @file XBLeaderHUDComponent.cpp
 * @brief 将领 HUD 组件实现
 */

#include "UI/XBLeaderHUDComponent.h"
#include "UI/XBLeaderHealthWidget.h"
#include "Character/XBCharacterBase.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

UXBLeaderHUDComponent::UXBLeaderHUDComponent()
{
    // 此组件不需要 Tick
    PrimaryComponentTick.bCanEverTick = false;
}

void UXBLeaderHUDComponent::BeginPlay()
{
    Super::BeginPlay();

    // 缓存将领引用
    CachedLeader = Cast<AXBCharacterBase>(GetOwner());

    if (!CachedLeader.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("LeaderHUDComponent: Owner 不是 XBCharacterBase 类型"));
        return;
    }

    // 自动创建 UI
    if (bAutoCreateUI)
    {
        CreateHealthUI();
    }
}

void UXBLeaderHUDComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 清理 UI
    DestroyHealthUI();

    Super::EndPlay(EndPlayReason);
}

/**
 * @brief 创建血量 UI
 * @note 只为本地玩家控制的角色创建 UI
 */
void UXBLeaderHUDComponent::CreateHealthUI()
{
    // 检查是否已创建
    if (HealthWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("LeaderHUDComponent: 血量 UI 已存在"));
        return;
    }

    // 检查 Widget 类是否配置
    if (!HealthWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("LeaderHUDComponent: 未配置 HealthWidgetClass"));
        return;
    }

    // 获取玩家控制器
    APlayerController* PC = nullptr;

    // 检查 Owner 是否是玩家控制的角色
    if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
    {
        PC = Cast<APlayerController>(OwnerPawn->GetController());
    }

    // 如果不是玩家控制的角色，尝试获取第一个本地玩家控制器
    // （用于显示敌方将领的血条等情况）
    if (!PC)
    {
        PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    }

    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("LeaderHUDComponent: 无法获取 PlayerController"));
        return;
    }

    // 创建 Widget
    HealthWidget = CreateWidget<UXBLeaderHealthWidget>(PC, HealthWidgetClass);

    if (!HealthWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("LeaderHUDComponent: 创建血量 Widget 失败"));
        return;
    }

    // 关联将领
    if (CachedLeader.IsValid())
    {
        HealthWidget->SetOwningLeader(CachedLeader.Get());
    }

    // 添加到视口
    HealthWidget->AddToViewport(UIZOrder);

    UE_LOG(LogTemp, Log, TEXT("LeaderHUDComponent: 血量 UI 创建成功"));
}

/**
 * @brief 销毁血量 UI
 */
void UXBLeaderHUDComponent::DestroyHealthUI()
{
    if (HealthWidget)
    {
        HealthWidget->RemoveFromParent();
        HealthWidget = nullptr;

        UE_LOG(LogTemp, Log, TEXT("LeaderHUDComponent: 血量 UI 已销毁"));
    }
}

/**
 * @brief 显示血量 UI
 */
void UXBLeaderHUDComponent::ShowHealthUI()
{
    if (!HealthWidget)
    {
        CreateHealthUI();
    }

    if (HealthWidget)
    {
        HealthWidget->SetVisibility(ESlateVisibility::Visible);
    }
}

/**
 * @brief 隐藏血量 UI
 */
void UXBLeaderHUDComponent::HideHealthUI()
{
    if (HealthWidget)
    {
        HealthWidget->SetVisibility(ESlateVisibility::Collapsed);
    }
}

/**
 * @brief 切换血量 UI 显示状态
 */
void UXBLeaderHUDComponent::ToggleHealthUI()
{
    if (!HealthWidget)
    {
        ShowHealthUI();
        return;
    }

    if (HealthWidget->GetVisibility() == ESlateVisibility::Visible)
    {
        HideHealthUI();
    }
    else
    {
        ShowHealthUI();
    }
}

/**
 * @brief 强制刷新 UI 显示
 */
void UXBLeaderHUDComponent::RefreshUI()
{
    if (HealthWidget)
    {
        HealthWidget->RefreshDisplay();
    }
}
