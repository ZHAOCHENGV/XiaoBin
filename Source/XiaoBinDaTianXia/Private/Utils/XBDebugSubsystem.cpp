/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Debug/XBDebugSubsystem.cpp

/**
 * @file XBDebugSubsystem.cpp
 * @brief 调试子系统实现
 * 
 * @note ✨ 新增文件
 * @note 🔧 修改 - 适配新的调试组件接口
 */

#include "Utils/XBDebugSubsystem.h"
#include "Utils/XBLogCategories.h"
#include "Soldier/Component/XBSoldierDebugComponent.h"
#include "Character/XBCharacterBase.h"
#include "Character/Components/XBFormationComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

void UXBDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 注册控制台命令
    RegisterConsoleCommands();

    UE_LOG(LogXBSoldier, Log, TEXT("调试子系统初始化完成"));
}

void UXBDebugSubsystem::Deinitialize()
{
    Super::Deinitialize();

    UE_LOG(LogXBSoldier, Log, TEXT("调试子系统关闭"));
}

void UXBDebugSubsystem::RegisterConsoleCommands()
{
    // 注册士兵调试命令
    IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("XB.Debug.Soldier.Toggle"),
        TEXT("切换士兵调试显示"),
        FConsoleCommandDelegate::CreateUObject(this, &UXBDebugSubsystem::ToggleSoldierDebug),
        ECVF_Default
    );

    // 注册编队调试命令
    IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("XB.Debug.Formation.Toggle"),
        TEXT("切换编队调试显示"),
        FConsoleCommandDelegate::CreateUObject(this, &UXBDebugSubsystem::ToggleFormationDebug),
        ECVF_Default
    );
}

// ==================== 士兵调试控制实现 ====================

void UXBDebugSubsystem::ToggleSoldierDebug()
{
    bool bCurrentState = UXBSoldierDebugComponent::IsGlobalDebugEnabled();
    SetSoldierDebugEnabled(!bCurrentState);
}

void UXBDebugSubsystem::SetSoldierDebugEnabled(bool bEnable)
{
    UXBSoldierDebugComponent::SetGlobalDebugEnabled(bEnable);

    UE_LOG(LogXBSoldier, Warning, TEXT("===== 士兵调试显示: %s ====="), 
        bEnable ? TEXT("启用") : TEXT("禁用"));
}

bool UXBDebugSubsystem::IsSoldierDebugEnabled() const
{
    return UXBSoldierDebugComponent::IsGlobalDebugEnabled();
}

// ==================== 编队调试控制实现 ====================

void UXBDebugSubsystem::ToggleFormationDebug()
{
    SetFormationDebugEnabled(!bFormationDebugEnabled);
}

void UXBDebugSubsystem::SetFormationDebugEnabled(bool bEnable)
{
    bFormationDebugEnabled = bEnable;

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // 遍历所有角色，切换编队调试
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AXBCharacterBase::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        if (AXBCharacterBase* Character = Cast<AXBCharacterBase>(Actor))
        {
            if (UXBFormationComponent* FormationComp = Character->GetFormationComponent())
            {
                FormationComp->SetDebugDrawEnabled(bEnable);
            }
        }
    }

    UE_LOG(LogXBSoldier, Warning, TEXT("===== 编队调试显示: %s ====="), 
        bEnable ? TEXT("启用") : TEXT("禁用"));
}
