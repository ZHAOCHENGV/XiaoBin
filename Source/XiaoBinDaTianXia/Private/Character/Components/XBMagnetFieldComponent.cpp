// XBMagnetFieldComponent.cpp
#include "Character/Components/XBMagnetFieldComponent.h"

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