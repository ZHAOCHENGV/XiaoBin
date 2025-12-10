// XBMagnetFieldComponent.cpp
#include "Character/Components/XBMagnetFieldComponent.h"

#include "GameplayEffectTypes.h"
#include "Character/XBCharacterBase.h"
#include "Soldier/XBSoldierActor.h"
#include "GAS/XBAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Data/XBSoldierDataTable.h"
#include "Engine/DataTable.h"

UXBMagnetFieldComponent::UXBMagnetFieldComponent()
{
    // 构造函数中只设置基本属性，不做复杂操作
    PrimaryComponentTick.bCanEverTick = false;
    
    // 设置组件需要初始化
    bWantsInitializeComponent = true;
    
    // 基础碰撞设置
    SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
    SetGenerateOverlapEvents(false);  // 先禁用，在 BeginPlay 中启用
    
    // 设置球体半径
    InitSphereRadius(300.0f);
    
    // 默认不可见
    SetHiddenInGame(true);
}

void UXBMagnetFieldComponent::InitializeComponent()
{
    Super::InitializeComponent();
    
    // 在这里设置碰撞配置
    SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SetCollisionObjectType(ECC_WorldDynamic);
    SetCollisionResponseToAllChannels(ECR_Ignore);
    SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
}

void UXBMagnetFieldComponent::BeginPlay()
{
    Super::BeginPlay();

    // 在 BeginPlay 中绑定碰撞事件（安全）
    if (!bOverlapEventsBound)
    {
        OnComponentBeginOverlap.AddDynamic(this, &UXBMagnetFieldComponent::OnSphereBeginOverlap);
        OnComponentEndOverlap.AddDynamic(this, &UXBMagnetFieldComponent::OnSphereEndOverlap);
        bOverlapEventsBound = true;
    }
    
    // 现在启用重叠事件
    SetGenerateOverlapEvents(bIsFieldEnabled);
}

void UXBMagnetFieldComponent::SetFieldRadius(float NewRadius)
{
    SetSphereRadius(NewRadius);
}

void UXBMagnetFieldComponent::SetFieldEnabled(bool bEnabled)
{
    bIsFieldEnabled = bEnabled;
    SetGenerateOverlapEvents(bEnabled);
}

void UXBMagnetFieldComponent::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!bIsFieldEnabled || !OtherActor)
    {
        return;
    }

    if (OtherActor == GetOwner())
    {
        return;
    }

    // 🔧 修改 - 使用新的招募流程
    if (AXBSoldierActor* Soldier = Cast<AXBSoldierActor>(OtherActor))
    {
        // 使用新的 CanBeRecruited 检查
        if (Soldier->CanBeRecruited())
        {
            if (AXBCharacterBase* Leader = Cast<AXBCharacterBase>(GetOwner()))
            {
                // 从将领配置的数据表初始化士兵
                UDataTable* SoldierDT = Leader->GetSoldierDataTable();
                FName SoldierRowName = Leader->GetRecruitSoldierRowName();
                
                if (SoldierDT && !SoldierRowName.IsNone())
                {
                    Soldier->InitializeFromDataTable(SoldierDT, SoldierRowName, Leader->GetFaction());
                    UE_LOG(LogTemp, Log, TEXT("士兵从数据表初始化: %s"), *SoldierRowName.ToString());
                }
                else
                {
                    FXBSoldierConfig DefaultConfig;
                    Soldier->InitializeSoldier(DefaultConfig, Leader->GetFaction());
                    UE_LOG(LogTemp, Warning, TEXT("将领未配置士兵数据表，使用默认配置"));
                }
                
                // ✨ 核心改动 - 获取槽位并调用 OnRecruited
                int32 SlotIndex = Leader->GetSoldierCount();  // 当前数量即为新槽位
                
                // 调用新的招募方法（内部会启动AI控制器）
                Soldier->OnRecruited(Leader, SlotIndex);
                
                // 添加到将领的士兵列表
                Leader->AddSoldier(Soldier);
                
                // 应用招募效果
                ApplyRecruitEffect(Leader, Soldier);
                
                UE_LOG(LogTemp, Log, TEXT("士兵被招募，将领当前士兵数: %d"), Leader->GetSoldierCount());
            }
        }
    }

    if (IsActorDetectable(OtherActor))
    {
        OnActorEnteredField.Broadcast(OtherActor);
    }
}

void UXBMagnetFieldComponent::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!bIsFieldEnabled || !OtherActor)
    {
        return;
    }

    if (OtherActor == GetOwner())
    {
        return;
    }

    if (IsActorDetectable(OtherActor))
    {
        OnActorExitedField.Broadcast(OtherActor);
    }
}

bool UXBMagnetFieldComponent::IsActorDetectable(AActor* Actor) const
{
    if (!Actor)
    {
        return false;
    }

    if (DetectableActorClasses.Num() == 0)
    {
        return true;
    }

    for (const TSubclassOf<AActor>& ActorClass : DetectableActorClasses)
    {
        if (ActorClass && Actor->IsA(ActorClass))
        {
            return true;
        }
    }

    return false;
}

void UXBMagnetFieldComponent::ApplyRecruitEffect(AXBCharacterBase* Leader, AXBSoldierActor* Soldier)
{
    if (!Leader)
    {
        return;
    }

    // 获取将领的 ASC
    UAbilitySystemComponent* LeaderASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Leader);
    if (!LeaderASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("将领没有 AbilitySystemComponent"));
        return;
    }

    // 如果配置了招募效果，应用它
    if (RecruitBonusEffectClass)
    {
        FGameplayEffectContextHandle ContextHandle = LeaderASC->MakeEffectContext();
        ContextHandle.AddSourceObject(Soldier);

        FGameplayEffectSpecHandle SpecHandle = LeaderASC->MakeOutgoingSpec(
            RecruitBonusEffectClass, 1, ContextHandle);

        if (SpecHandle.IsValid())
        {
            LeaderASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
            UE_LOG(LogTemp, Log, TEXT("已应用招募增益效果"));
        }
    }

    // 发送招募事件（供其他系统监听）
    FGameplayEventData EventData;
    EventData.Instigator = Leader;
    EventData.Target = Soldier;
    LeaderASC->HandleGameplayEvent(
        FGameplayTag::RequestGameplayTag(FName("Event.Soldier.Recruited")), &EventData);
}
