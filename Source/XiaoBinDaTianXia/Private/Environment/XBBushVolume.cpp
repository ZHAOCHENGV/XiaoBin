/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Environment/XBBushVolume.cpp

/**
 * @file XBBushVolume.cpp
 * @brief 草丛体积触发器实现
 * 
 * @note ✨ 新增文件
 */

#include "Environment/XBBushVolume.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Character/XBCharacterBase.h"
#include "Utils/XBLogCategories.h"

AXBBushVolume::AXBBushVolume()
{
    PrimaryActorTick.bCanEverTick = false;

    BushBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BushBox"));
    BushBox->SetBoxExtent(FVector(200.0f, 200.0f, 150.0f));
    // 启用 Query 和 Physics 以同时支持射线检测和重叠事件
    BushBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    // 默认对所有通道 Overlap（用于角色进入检测）
    BushBox->SetCollisionResponseToAllChannels(ECR_Overlap);
    // 对 Visibility 和 Camera 通道 Block 响应（用于射线检测选中）
    BushBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    BushBox->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
    BushBox->SetGenerateOverlapEvents(true);
    RootComponent = BushBox;
}

void AXBBushVolume::BeginPlay()
{
    Super::BeginPlay();

    if (BushBox)
    {
        BushBox->OnComponentBeginOverlap.AddDynamic(this, &AXBBushVolume::OnBushOverlapBegin);
        BushBox->OnComponentEndOverlap.AddDynamic(this, &AXBBushVolume::OnBushOverlapEnd);
    }
}

void AXBBushVolume::OnBushOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    AXBCharacterBase* Leader = Cast<AXBCharacterBase>(OtherActor);
    if (!Leader)
    {
        return;
    }

    // 🔧 修复 - 只响应主将的胶囊体组件，防止磁场等附属组件产生额外计数
    if (OtherComp != Leader->GetCapsuleComponent())
    {
        return;
    }

    // 🔧 修复 - 死亡状态不处理草丛隐身
    if (Leader->IsDead())
    {
        return;
    }

    OverlappingLeaders.Add(Leader);

    // 🔧 修复 - 使用引用计数机制，支持连续穿过多个草丛
    Leader->IncrementBushOverlapCount();

    UE_LOG(LogXBCharacter, Log, TEXT("主将 %s 进入草丛，全军隐身"), *Leader->GetName());
}

void AXBBushVolume::OnBushOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    AXBCharacterBase* Leader = Cast<AXBCharacterBase>(OtherActor);
    if (!Leader)
    {
        return;
    }

    // 🔧 修复 - 只响应主将的胶囊体组件，与 BeginOverlap 保持一致
    if (OtherComp != Leader->GetCapsuleComponent())
    {
        return;
    }

    OverlappingLeaders.Remove(Leader);

    // 🔧 修复 - 检查主将是否仍在其他草丛中
    // 只有当主将完全离开所有草丛时才恢复可见
    // 通过使用 AXBCharacterBase 中的引用计数机制来处理
    Leader->DecrementBushOverlapCount();

    UE_LOG(LogXBCharacter, Log, TEXT("主将 %s 离开草丛"), *Leader->GetName());
}
