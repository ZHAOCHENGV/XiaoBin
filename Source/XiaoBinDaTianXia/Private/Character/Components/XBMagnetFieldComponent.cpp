/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Character/Components/XBMagnetFieldComponent.cpp

/**
 * @file XBMagnetFieldComponent.cpp
 * @brief 磁场组件实现 - 增强村民招募
 * 
 * @note 🔧 修改记录:
 *       1. 新增 TryRecruitVillager 方法
 *       2. 在 OnSphereBeginOverlap 中处理村民检测
 */

#include "Character/Components/XBMagnetFieldComponent.h"
#include "GameplayEffectTypes.h"
#include "Character/XBCharacterBase.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Soldier/XBVillagerActor.h" // ✨ 新增
#include "GAS/XBAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "XBCollisionChannels.h"
#include "Data/XBSoldierDataTable.h"
#include "Engine/DataTable.h"

UXBMagnetFieldComponent::UXBMagnetFieldComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bWantsInitializeComponent = true;


    SetGenerateOverlapEvents(false);
    InitSphereRadius(300.0f);
    SetHiddenInGame(true);
}

void UXBMagnetFieldComponent::InitializeComponent()
{
    Super::InitializeComponent();

    // 启用碰撞查询
    SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    
    // 设置自身为 WorldDynamic 类型
    SetCollisionObjectType(ECC_WorldDynamic);
    
    // 🔧 修改 - 配置碰撞响应
    // 先忽略所有通道
    SetCollisionResponseToAllChannels(ECR_Ignore);
    
    // 检测默认 Pawn 通道（村民使用默认 Pawn）
    SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    
    // ✨ 新增 - 检测自定义 Soldier 通道（士兵使用此通道）
    SetCollisionResponseToChannel(XBCollision::Soldier, ECR_Overlap);
    
    // ✨ 新增 - 检测自定义 Leader 通道（其他将领，如果需要）
    SetCollisionResponseToChannel(XBCollision::Leader, ECR_Overlap);
    
    // ✨ 新增 - 检测 WorldDynamic（村民可能使用此通道）
    SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
    
    UE_LOG(LogTemp, Log, TEXT("磁场组件 %s: 碰撞配置完成 - 检测 Pawn/Soldier/Leader/WorldDynamic"), 
        *GetOwner()->GetName());
}

void UXBMagnetFieldComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!bOverlapEventsBound)
    {
        OnComponentBeginOverlap.AddDynamic(this, &UXBMagnetFieldComponent::OnSphereBeginOverlap);
        OnComponentEndOverlap.AddDynamic(this, &UXBMagnetFieldComponent::OnSphereEndOverlap);
        bOverlapEventsBound = true;
    }

    SetGenerateOverlapEvents(bIsFieldEnabled);
    // ✨ 新增 - 输出调试信息确认配置
    UE_LOG(LogTemp, Warning, TEXT("磁场组件 %s BeginPlay - 半径: %.1f, 启用: %s"), 
        *GetOwner()->GetName(), 
        GetScaledSphereRadius(),
        bIsFieldEnabled ? TEXT("是") : TEXT("否"));
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

/**
 * @brief 碰撞开始回调
 * @note 🔧 修改 - 新增村民检测逻辑
 */
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
    // ✨ 新增 - 调试日志
    UE_LOG(LogTemp, Log, TEXT("磁场检测到: %s (类型: %s)"), 
        *OtherActor->GetName(), 
        *OtherActor->GetClass()->GetName());
    AXBCharacterBase* Leader = Cast<AXBCharacterBase>(GetOwner());
    if (!Leader || Leader->IsDead())
    {
        UE_LOG(LogTemp, Log, TEXT("磁场组件: 将领已死亡，忽略招募"));
        return;
    }

    // ✨ 新增 - 优先检测村民
    if (AXBVillagerActor* Villager = Cast<AXBVillagerActor>(OtherActor))
    {
        if (TryRecruitVillager(Villager))
        {
            return; // 成功招募村民，跳过后续逻辑
        }
    }

    // 原有逻辑：招募已存在的士兵
    if (AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(OtherActor))
    {
        if (Soldier->CanBeRecruited())
        {
            UDataTable* SoldierDT = Leader->GetSoldierDataTable();
            FName SoldierRowName = Leader->GetRecruitSoldierRowName();

            if (SoldierDT && !SoldierRowName.IsNone())
            {
                Soldier->InitializeFromDataTable(SoldierDT, SoldierRowName, Leader->GetFaction());
            }
            else
            {
                FXBSoldierConfig DefaultConfig;
                Soldier->InitializeSoldier(DefaultConfig, Leader->GetFaction());
            }

            int32 SlotIndex = Leader->GetSoldierCount();
            Soldier->OnRecruited(Leader, SlotIndex);
            Leader->AddSoldier(Soldier);
            ApplyRecruitEffect(Leader, Soldier);

            UE_LOG(LogTemp, Log, TEXT("士兵被招募，将领当前士兵数: %d"), Leader->GetSoldierCount());
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

/**
 * @brief 尝试招募村民
 * @param Villager 村民Actor
 * @return 是否成功招募
 * @note ✨ 新增方法
 *       功能流程：
 *       1. 检查村民是否可招募
 *       2. 生成对应兵种的士兵Actor
 *       3. 调用村民的 OnRecruited（村民会自毁）
 *       4. 将新士兵加入将领队列
 */
bool UXBMagnetFieldComponent::TryRecruitVillager(AXBVillagerActor* Villager)
{
    if (!Villager || !Villager->CanBeRecruited())
    {
        return false;
    }

    AXBCharacterBase* Leader = Cast<AXBCharacterBase>(GetOwner());
    if (!Leader)
    {
        return false;
    }

    // 获取将领的默认兵种配置
    UDataTable* SoldierDT = Leader->GetSoldierDataTable();
    FName SoldierRowName = Leader->GetRecruitSoldierRowName();

    if (!SoldierDT || SoldierRowName.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("将领 %s 未配置士兵数据表"), *Leader->GetName());
        return false;
    }

    // 生成士兵Actor
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    // 🔧 修改 - 使用公开访问器代替直接访问 protected 成员
    TSubclassOf<AXBSoldierCharacter> SoldierClass = Leader->GetSoldierActorClass();
    if (!SoldierClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("将领 %s 未配置士兵Actor类"), *Leader->GetName());
        return false;
    }

    // 在村民位置生成士兵
    FVector SpawnLocation = Villager->GetActorLocation();
    FRotator SpawnRotation = Villager->GetActorRotation();

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AXBSoldierCharacter* NewSoldier = World->SpawnActor<AXBSoldierCharacter>(
        SoldierClass,
        SpawnLocation,
        SpawnRotation,
        SpawnParams
    );

    if (!NewSoldier)
    {
        UE_LOG(LogTemp, Error, TEXT("生成士兵失败"));
        return false;
    }

    // 初始化士兵
    NewSoldier->InitializeFromDataTable(SoldierDT, SoldierRowName, Leader->GetFaction());

    // 招募士兵
    int32 SlotIndex = Leader->GetSoldierCount();
    NewSoldier->OnRecruited(Leader, SlotIndex);
    Leader->AddSoldier(NewSoldier);

    // 应用招募效果
    ApplyRecruitEffect(Leader, NewSoldier);

    // 通知村民被招募（村民会自毁）
    Villager->OnRecruited(Leader);

    UE_LOG(LogTemp, Log, TEXT("村民 %s 转化为士兵 %s，将领当前士兵数: %d"),
        *Villager->GetName(), *NewSoldier->GetName(), Leader->GetSoldierCount());

    return true;
}

void UXBMagnetFieldComponent::ApplyRecruitEffect(AXBCharacterBase* Leader, AXBSoldierCharacter* Soldier)
{
    if (!Leader)
    {
        return;
    }

    UAbilitySystemComponent* LeaderASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Leader);
    if (!LeaderASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("将领没有 AbilitySystemComponent"));
        return;
    }

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

    FGameplayEventData EventData;
    EventData.Instigator = Leader;
    EventData.Target = Soldier;
    LeaderASC->HandleGameplayEvent(
        FGameplayTag::RequestGameplayTag(FName("Event.Soldier.Recruited")), &EventData);
}
