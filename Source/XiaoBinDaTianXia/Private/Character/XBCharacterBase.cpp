/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Character/XBCharacterBase.cpp

/**
 * @file XBCharacterBase.cpp
 * @brief 角色基类实现
 */

#include "Character/XBCharacterBase.h"
#include "Character/Components/XBCombatComponent.h"
#include "GAS/XBAbilitySystemComponent.h"
#include "GAS/XBAttributeSet.h"
#include "Data/XBLeaderDataTable.h"
#include "Soldier/XBSoldierActor.h"
#include "Engine/DataTable.h"
#include "GameFramework/CharacterMovementComponent.h"

AXBCharacterBase::AXBCharacterBase()
{
    PrimaryActorTick.bCanEverTick = true;

    // 创建ASC
    AbilitySystemComponent = CreateDefaultSubobject<UXBAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

    // 创建属性集
    AttributeSet = CreateDefaultSubobject<UXBAttributeSet>(TEXT("AttributeSet"));

    // 创建战斗组件
    CombatComponent = CreateDefaultSubobject<UXBCombatComponent>(TEXT("CombatComponent"));
}

UAbilitySystemComponent* AXBCharacterBase::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AXBCharacterBase::BeginPlay()
{
    Super::BeginPlay();

    // 初始化ASC
    InitializeAbilitySystem();

    // 从配置的数据表初始化
    if (ConfigDataTable && !ConfigRowName.IsNone())
    {
        InitializeFromDataTable(ConfigDataTable, ConfigRowName);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 未配置数据表或行名，跳过数据表初始化"), *GetName());
    }
}

void AXBCharacterBase::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
    }
}

void AXBCharacterBase::InitializeAbilitySystem()
{
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
        UE_LOG(LogTemp, Log, TEXT("%s: ASC初始化完成"), *GetName());
    }
}

void AXBCharacterBase::InitializeFromDataTable(UDataTable* DataTable, FName RowName)
{
    if (!DataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: InitializeFromDataTable - 数据表为空"), *GetName());
        return;
    }

    if (RowName.IsNone())
    {
        UE_LOG(LogTemp, Error, TEXT("%s: InitializeFromDataTable - 行名为空"), *GetName());
        return;
    }

    FXBLeaderTableRow* LeaderRow = DataTable->FindRow<FXBLeaderTableRow>(RowName, TEXT("AXBCharacterBase::InitializeFromDataTable"));
    if (!LeaderRow)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: InitializeFromDataTable - 找不到行 '%s'，可用行:"), *GetName(), *RowName.ToString());
        
        TArray<FName> RowNames = DataTable->GetRowNames();
        for (const FName& Name : RowNames)
        {
            UE_LOG(LogTemp, Error, TEXT("  - %s"), *Name.ToString());
        }
        return;
    }

    // 缓存数据
    CachedLeaderData = *LeaderRow;

    // 缓存成长配置
    GrowthConfigCache.HealthPerSoldier = LeaderRow->HealthPerSoldier;
    GrowthConfigCache.ScalePerSoldier = LeaderRow->ScalePerSoldier;
    GrowthConfigCache.MaxScale = LeaderRow->MaxScale;

    UE_LOG(LogTemp, Log, TEXT("%s: 从数据表加载配置成功 - 行: %s, MaxHealth: %.1f, BaseDamage: %.1f"), 
        *GetName(), *RowName.ToString(), LeaderRow->MaxHealth, LeaderRow->BaseDamage);

    // 初始化战斗组件
    if (CombatComponent)
    {
        CombatComponent->InitializeFromDataTable(DataTable, RowName);
        UE_LOG(LogTemp, Log, TEXT("%s: 战斗组件初始化完成"), *GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s: 战斗组件为空!"), *GetName());
    }

    // 应用属性到ASC
    ApplyInitialAttributes();

    // 应用移动速度
    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->MaxWalkSpeed = LeaderRow->MoveSpeed;
        UE_LOG(LogTemp, Log, TEXT("%s: 移动速度设置为 %.1f"), *GetName(), LeaderRow->MoveSpeed);
    }
}

void AXBCharacterBase::ApplyInitialAttributes()
{
    if (!AbilitySystemComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: ApplyInitialAttributes - ASC为空"), *GetName());
        return;
    }

    const UXBAttributeSet* LocalAttributeSet = AbilitySystemComponent->GetSet<UXBAttributeSet>();
    if (!LocalAttributeSet)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: ApplyInitialAttributes - AttributeSet为空"), *GetName());
        return;
    }

    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetMaxHealthAttribute(), CachedLeaderData.MaxHealth);
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetHealthAttribute(), CachedLeaderData.MaxHealth);
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetHealthMultiplierAttribute(), CachedLeaderData.HealthMultiplier);
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetBaseDamageAttribute(), CachedLeaderData.BaseDamage);
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetDamageMultiplierAttribute(), CachedLeaderData.DamageMultiplier);
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetMoveSpeedAttribute(), CachedLeaderData.MoveSpeed);
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetScaleAttribute(), CachedLeaderData.Scale);

    UE_LOG(LogTemp, Log, TEXT("%s: 属性应用完成 - MaxHealth: %.1f, BaseDamage: %.1f, MoveSpeed: %.1f, Scale: %.1f"),
        *GetName(),
        CachedLeaderData.MaxHealth,
        CachedLeaderData.BaseDamage,
        CachedLeaderData.MoveSpeed,
        CachedLeaderData.Scale);
}

// ============ 阵营系统实现 ============

bool AXBCharacterBase::IsHostileTo(const AXBCharacterBase* Other) const
{
    if (!Other)
    {
        return false;
    }

    // 相同阵营不敌对
    if (Faction == Other->Faction)
    {
        return false;
    }

    // 中立对任何人都不敌对
    if (Faction == EXBFaction::Neutral || Other->Faction == EXBFaction::Neutral)
    {
        return false;
    }

    // 玩家和友军互相不敌对
    if ((Faction == EXBFaction::Player && Other->Faction == EXBFaction::Ally) ||
        (Faction == EXBFaction::Ally && Other->Faction == EXBFaction::Player))
    {
        return false;
    }

    // 其他情况（玩家vs敌人，友军vs敌人）视为敌对
    return true;
}

bool AXBCharacterBase::IsFriendlyTo(const AXBCharacterBase* Other) const
{
    if (!Other)
    {
        return false;
    }

    // 相同阵营友好
    if (Faction == Other->Faction)
    {
        return true;
    }

    // 玩家和友军友好
    if ((Faction == EXBFaction::Player && Other->Faction == EXBFaction::Ally) ||
        (Faction == EXBFaction::Ally && Other->Faction == EXBFaction::Player))
    {
        return true;
    }

    return false;
}

// ============ 士兵管理实现 ============

void AXBCharacterBase::AddSoldier(AXBSoldierActor* Soldier)
{
    if (!Soldier)
    {
        return;
    }

    if (!Soldiers.Contains(Soldier))
    {
        Soldiers.Add(Soldier);
        OnSoldiersAdded(1);
        UE_LOG(LogTemp, Log, TEXT("%s: 添加士兵，当前数量: %d"), *GetName(), Soldiers.Num());
    }
}

void AXBCharacterBase::RemoveSoldier(AXBSoldierActor* Soldier)
{
    if (!Soldier)
    {
        return;
    }

    if (Soldiers.Remove(Soldier) > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("%s: 移除士兵，当前数量: %d"), *GetName(), Soldiers.Num());
    }
}

void AXBCharacterBase::OnSoldierDied()
{
    // 士兵死亡时的处理逻辑
    UE_LOG(LogTemp, Log, TEXT("%s: 一名士兵阵亡，剩余士兵: %d"), *GetName(), Soldiers.Num());
}

void AXBCharacterBase::OnSoldiersAdded(int32 SoldierCount)
{
    if (SoldierCount <= 0)
    {
        return;
    }

    CurrentSoldierCount += SoldierCount;

    // 计算新的缩放值
    const float BaseScale = CachedLeaderData.Scale;
    const float AdditionalScale = CurrentSoldierCount * GrowthConfigCache.ScalePerSoldier;
    const float NewScale = FMath::Min(BaseScale + AdditionalScale, GrowthConfigCache.MaxScale);

    // 应用缩放
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetScaleAttribute(), NewScale);
    }
    SetActorScale3D(FVector(NewScale));

    // 增加生命值
    const float HealthBonus = SoldierCount * GrowthConfigCache.HealthPerSoldier;
    if (AbilitySystemComponent)
    {
        float CurrentMaxHealth = AbilitySystemComponent->GetNumericAttribute(UXBAttributeSet::GetMaxHealthAttribute());
        float CurrentHealth = AbilitySystemComponent->GetNumericAttribute(UXBAttributeSet::GetHealthAttribute());
        
        AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetMaxHealthAttribute(), CurrentMaxHealth + HealthBonus);
        AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetHealthAttribute(), CurrentHealth + HealthBonus);
    }

    UE_LOG(LogTemp, Log, TEXT("%s: 士兵添加 +%d, 总数: %d, 新缩放: %.2f, 生命加成: %.1f"),
        *GetName(), SoldierCount, CurrentSoldierCount, NewScale, HealthBonus);
}

void AXBCharacterBase::RecallAllSoldiers()
{
    UE_LOG(LogTemp, Log, TEXT("%s: 召回所有士兵"), *GetName());
    // 子类可以重写此方法实现具体的召回逻辑
}

void AXBCharacterBase::SetSoldiersEscaping(bool bEscaping)
{
    UE_LOG(LogTemp, Log, TEXT("%s: 设置士兵逃跑状态: %s"), *GetName(), bEscaping ? TEXT("是") : TEXT("否"));
    // 子类可以重写此方法实现具体的逃跑逻辑
}
