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
#include "Components/CapsuleComponent.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"

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

    if (Faction == Other->Faction)
    {
        return false;
    }

    if (Faction == EXBFaction::Neutral || Other->Faction == EXBFaction::Neutral)
    {
        return false;
    }

    if ((Faction == EXBFaction::Player && Other->Faction == EXBFaction::Ally) ||
        (Faction == EXBFaction::Ally && Other->Faction == EXBFaction::Player))
    {
        return false;
    }

    return true;
}

bool AXBCharacterBase::IsFriendlyTo(const AXBCharacterBase* Other) const
{
    if (!Other)
    {
        return false;
    }

    if (Faction == Other->Faction)
    {
        return true;
    }

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
    UE_LOG(LogTemp, Log, TEXT("%s: 一名士兵阵亡，剩余士兵: %d"), *GetName(), Soldiers.Num());
}

void AXBCharacterBase::OnSoldiersAdded(int32 SoldierCount)
{
    if (SoldierCount <= 0)
    {
        return;
    }

    CurrentSoldierCount += SoldierCount;

    const float BaseScale = CachedLeaderData.Scale;
    const float AdditionalScale = CurrentSoldierCount * GrowthConfigCache.ScalePerSoldier;
    const float NewScale = FMath::Min(BaseScale + AdditionalScale, GrowthConfigCache.MaxScale);

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetScaleAttribute(), NewScale);
    }

    SetActorScale3D(FVector(NewScale));

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
}

void AXBCharacterBase::SetSoldiersEscaping(bool bEscaping)
{
    UE_LOG(LogTemp, Log, TEXT("%s: 设置士兵逃跑状态: %s"), *GetName(), bEscaping ? TEXT("是") : TEXT("否"));
}

// ============ 死亡系统实现 ============


void AXBCharacterBase::HandleDeath()
{
    // 防止重复处理死亡
    if (bIsDead)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: HandleDeath - 已经死亡，跳过"), *GetName());
        return;
    }

    bIsDead = true;

    UE_LOG(LogTemp, Log, TEXT(""));
    UE_LOG(LogTemp, Log, TEXT("╔══════════════════════════════════════════╗"));
    UE_LOG(LogTemp, Log, TEXT("║           角色死亡处理开始               ║"));
    UE_LOG(LogTemp, Log, TEXT("╠══════════════════════════════════════════╣"));
    UE_LOG(LogTemp, Log, TEXT("║ 角色: %s"), *GetName());
    UE_LOG(LogTemp, Log, TEXT("║ 阵营: %s"), 
        Faction == EXBFaction::Player ? TEXT("玩家") :
        Faction == EXBFaction::Enemy ? TEXT("敌人") :
        Faction == EXBFaction::Ally ? TEXT("友军") : TEXT("中立"));
    UE_LOG(LogTemp, Log, TEXT("║ 死亡蒙太奇: %s"), DeathMontage ? *DeathMontage->GetName() : TEXT("未配置"));
    UE_LOG(LogTemp, Log, TEXT("║ 消失延迟: %.2f秒"), DeathDestroyDelay);
    UE_LOG(LogTemp, Log, TEXT("╚══════════════════════════════════════════╝"));

    // 广播死亡事件
    OnCharacterDeath.Broadcast(this);

    // 禁用角色移动
    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->DisableMovement();
        MovementComp->StopMovementImmediately();
        UE_LOG(LogTemp, Log, TEXT("%s: 移动已禁用"), *GetName());
    }

    // 禁用碰撞（可选，根据需求调整）
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        UE_LOG(LogTemp, Log, TEXT("%s: 碰撞已禁用"), *GetName());
    }

    // 停止所有能力
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->CancelAllAbilities();
        UE_LOG(LogTemp, Log, TEXT("%s: 所有能力已取消"), *GetName());
    }

    // 播放死亡蒙太奇
    bool bMontageStarted = false;
    if (DeathMontage)
    {
        if (USkeletalMeshComponent* MeshComp = GetMesh())
        {
            if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
            {
                // 停止当前播放的蒙太奇
                AnimInstance->StopAllMontages(0.2f);

                // 播放死亡蒙太奇
                float Duration = AnimInstance->Montage_Play(DeathMontage, 1.0f);
                if (Duration > 0.0f)
                {
                    bMontageStarted = true;
                    UE_LOG(LogTemp, Log, TEXT("%s: 死亡蒙太奇开始播放，时长: %.2f秒"), *GetName(), Duration);

                    // 绑定蒙太奇结束回调
                    FOnMontageEnded EndDelegate;
                    EndDelegate.BindUObject(this, &AXBCharacterBase::OnDeathMontageEnded);
                    AnimInstance->Montage_SetEndDelegate(EndDelegate, DeathMontage);

                    // 如果不需要等蒙太奇结束，立即开始计时
                    if (!bDelayAfterMontage)
                    {
                        GetWorldTimerManager().SetTimer(
                            DeathDestroyTimerHandle,
                            this,
                            &AXBCharacterBase::OnDestroyTimerExpired,
                            DeathDestroyDelay,
                            false
                        );
                        UE_LOG(LogTemp, Log, TEXT("%s: 销毁计时器已启动（与蒙太奇并行）"), *GetName());
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("%s: 死亡蒙太奇播放失败"), *GetName());
                }
            }
        }
    }

    // 如果没有蒙太奇或播放失败，直接开始延迟销毁计时
    if (!bMontageStarted)
    {
        UE_LOG(LogTemp, Log, TEXT("%s: 无死亡蒙太奇，直接开始销毁倒计时"), *GetName());
        GetWorldTimerManager().SetTimer(
            DeathDestroyTimerHandle,
            this,
            &AXBCharacterBase::OnDestroyTimerExpired,
            DeathDestroyDelay,
            false
        );
    }
}

void AXBCharacterBase::OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    UE_LOG(LogTemp, Log, TEXT("%s: 死亡蒙太奇结束 - 被打断: %s"), 
        *GetName(), bInterrupted ? TEXT("是") : TEXT("否"));

    // 如果配置为蒙太奇结束后才开始计时
    if (bDelayAfterMontage)
    {
        UE_LOG(LogTemp, Log, TEXT("%s: 开始销毁倒计时 %.2f秒"), *GetName(), DeathDestroyDelay);
        GetWorldTimerManager().SetTimer(
            DeathDestroyTimerHandle,
            this,
            &AXBCharacterBase::OnDestroyTimerExpired,
            DeathDestroyDelay,
            false
        );
    }
}

void AXBCharacterBase::OnDestroyTimerExpired()
{
    UE_LOG(LogTemp, Log, TEXT("%s: 销毁计时器到期，准备销毁角色"), *GetName());

    // 执行销毁前清理
    PreDestroyCleanup();

    // 销毁Actor
    Destroy();
}

void AXBCharacterBase::PreDestroyCleanup()
{
    UE_LOG(LogTemp, Log, TEXT("%s: 执行销毁前清理"), *GetName());

    // 清除所有士兵的引用
    for (AXBSoldierActor* Soldier : Soldiers)
    {
        if (Soldier)
        {
            // 通知士兵主将已死亡
            // Soldier->OnLeaderDied();
        }
    }
    Soldiers.Empty();

   

    // ✨ 新增 - 清理 ASC 的所有激活状态
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->CancelAllAbilities();
        AbilitySystemComponent->RemoveAllGameplayCues();
        
        // 清除所有 GameplayEffects
        AbilitySystemComponent->RemoveActiveEffectsWithTags(FGameplayTagContainer());
        
        UE_LOG(LogTemp, Log, TEXT("%s: ASC 状态已清理"), *GetName());
    }

    // 清除定时器
    GetWorldTimerManager().ClearTimer(DeathDestroyTimerHandle);
}
