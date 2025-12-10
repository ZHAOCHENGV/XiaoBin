/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Character/XBCharacterBase.cpp

/**
 * @file XBCharacterBase.cpp
 * @brief 角色基类实现
 * 
 * @note 🔧 修改记录:
 *       1. 修复血量成长逻辑 - 区分回复和溢出提升上限
 *       2. 新增战斗状态系统 - 攻击命中后触发士兵战斗
 *       3. 新增士兵补位逻辑 - 死亡后后面士兵向前补
 *       4. 新增士兵掉落系统 - 将领死亡后掉落士兵
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
#include "Kismet/GameplayStatics.h"

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
        int32 OldCount = Soldiers.Num();
        
        // 分配槽位索引
        int32 SlotIndex = Soldiers.Num();
        Soldier->SetFormationSlotIndex(SlotIndex);
        Soldier->SetFollowTarget(this, SlotIndex);
        
        // 设置士兵阵营与将领一致
        Soldier->InitializeSoldier(Soldier->GetSoldierConfig(), Faction);
        
        Soldiers.Add(Soldier);
        
        // 触发成长效果
        OnSoldiersAdded(1);
        
        // 广播士兵数量变化
        OnSoldierCountChanged.Broadcast(OldCount, Soldiers.Num());
        
        UE_LOG(LogTemp, Log, TEXT("%s: 添加士兵 %s，槽位: %d，当前数量: %d"), 
            *GetName(), *Soldier->GetName(), SlotIndex, Soldiers.Num());
    }
}

/**
 * @brief 从队列移除士兵
 * @param Soldier 士兵Actor
 * @note 🔧 修改 - 实现补位逻辑，后面的士兵向前补
 */
void AXBCharacterBase::RemoveSoldier(AXBSoldierActor* Soldier)
{
    if (!Soldier)
    {
        return;
    }

    int32 RemovedIndex = Soldiers.Find(Soldier);
    if (RemovedIndex == INDEX_NONE)
    {
        return;
    }

    int32 OldCount = Soldiers.Num();

    // 移除士兵
    Soldiers.RemoveAt(RemovedIndex);
    
    // ✨ 新增 - 重新分配槽位索引（补位逻辑）
    ReassignSoldierSlots(RemovedIndex);

    // 广播士兵数量变化
    OnSoldierCountChanged.Broadcast(OldCount, Soldiers.Num());

    UE_LOG(LogTemp, Log, TEXT("%s: 移除士兵，从索引 %d，剩余数量: %d"), *GetName(), RemovedIndex, Soldiers.Num());
}

/**
 * @brief 重新分配士兵槽位（补位逻辑）
 * @param StartIndex 从哪个索引开始重新分配
 */
void AXBCharacterBase::ReassignSoldierSlots(int32 StartIndex)
{
    // 从移除位置开始，所有后面的士兵槽位前移
    for (int32 i = StartIndex; i < Soldiers.Num(); ++i)
    {
        if (Soldiers[i])
        {
            Soldiers[i]->SetFormationSlotIndex(i);
            
            UE_LOG(LogTemp, Log, TEXT("士兵 %s 补位到槽位 %d"), *Soldiers[i]->GetName(), i);
        }
    }
}

/**
 * @brief 士兵死亡回调
 * @param DeadSoldier 死亡的士兵
 * @note 🔧 修改 - 实现补位逻辑和只缩小不扣血
 */
void AXBCharacterBase::OnSoldierDied(AXBSoldierActor* DeadSoldier)
{
    if (!DeadSoldier)
    {
        return;
    }

    // 从队列移除（会触发补位）
    RemoveSoldier(DeadSoldier);
    
    // 更新士兵计数
    CurrentSoldierCount = FMath::Max(0, CurrentSoldierCount - 1);
    
    // ✨ 新增 - 士兵死亡只缩小不扣血
    UpdateLeaderScale();

    UE_LOG(LogTemp, Log, TEXT("%s: 一名士兵阵亡，剩余士兵: %d"), *GetName(), Soldiers.Num());
}

/**
 * @brief 更新将领缩放（不更新血量）
 * @note 士兵死亡时只缩小不扣血
 */
void AXBCharacterBase::UpdateLeaderScale()
{
    const float BaseScale = CachedLeaderData.Scale;
    const float AdditionalScale = CurrentSoldierCount * GrowthConfigCache.ScalePerSoldier;
    const float NewScale = FMath::Min(BaseScale + AdditionalScale, GrowthConfigCache.MaxScale);

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetScaleAttribute(), NewScale);
    }

    SetActorScale3D(FVector(NewScale));

    UE_LOG(LogTemp, Log, TEXT("%s: 更新缩放 - 士兵数: %d, 新缩放: %.2f"), *GetName(), CurrentSoldierCount, NewScale);
}

/**
 * @brief 士兵添加后的成长处理
 * @param SoldierCount 新增士兵数量
 * @note 🔧 修改 - 实现设计文档的血量回复逻辑:
 *       1. 优先回复当前血量
 *       2. 溢出部分才提升最大血量
 *       3. 缩放使用累加而非累乘
 */
void AXBCharacterBase::OnSoldiersAdded(int32 SoldierCount)
{
    if (SoldierCount <= 0)
    {
        return;
    }

    CurrentSoldierCount += SoldierCount;

    // ========== 缩放处理（累加而非累乘）==========
    // ✅ 正确：1 + 0.1 + 0.1 + 0.1，而不是 1 * 1.1 * 1.1 * 1.1
    const float BaseScale = CachedLeaderData.Scale;
    const float AdditionalScale = CurrentSoldierCount * GrowthConfigCache.ScalePerSoldier;
    const float NewScale = FMath::Min(BaseScale + AdditionalScale, GrowthConfigCache.MaxScale);

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetScaleAttribute(), NewScale);
    }

    SetActorScale3D(FVector(NewScale));

    // ========== 🔧 修改 - 血量回复逻辑重写 ==========
    /**
     * @note 设计文档要求:
     *       如果将领血不满的情况下新增士兵只会回复，回复后如果超出最大血量则提升最大血量。
     *       例1: MaxHP=1000, HP=800, Bonus=100 => MaxHP=1000, HP=900 (只回复)
     *       例2: MaxHP=1000, HP=953, Bonus=100 => MaxHP=1053, HP=1053 (溢出提升上限)
     */
    const float HealthBonus = SoldierCount * GrowthConfigCache.HealthPerSoldier;
    
    if (AbilitySystemComponent)
    {
        float CurrentMaxHealth = AbilitySystemComponent->GetNumericAttribute(UXBAttributeSet::GetMaxHealthAttribute());
        float CurrentHealth = AbilitySystemComponent->GetNumericAttribute(UXBAttributeSet::GetHealthAttribute());
        
        // 计算回复后的血量
        float NewHealth = CurrentHealth + HealthBonus;
        
        // 🔧 修改 - 关键逻辑：判断是否溢出
        if (NewHealth > CurrentMaxHealth)
        {
            // 溢出情况：提升最大血量到新血量值
            // 例：MaxHP=1000, HP=953, Bonus=100 => MaxHP=1053, HP=1053
            AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetMaxHealthAttribute(), NewHealth);
            AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetHealthAttribute(), NewHealth);
            
            UE_LOG(LogTemp, Log, TEXT("%s: 血量溢出，提升上限 - MaxHP: %.1f -> %.1f, HP: %.1f -> %.1f"),
                *GetName(), CurrentMaxHealth, NewHealth, CurrentHealth, NewHealth);
        }
        else
        {
            // 未溢出情况：只回复血量，不提升上限
            // 例：MaxHP=1000, HP=800, Bonus=100 => MaxHP=1000, HP=900
            AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetHealthAttribute(), NewHealth);
            
            UE_LOG(LogTemp, Log, TEXT("%s: 血量回复 - MaxHP: %.1f (不变), HP: %.1f -> %.1f"),
                *GetName(), CurrentMaxHealth, CurrentHealth, NewHealth);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("%s: 士兵添加 +%d, 总数: %d, 新缩放: %.2f, 血量加成: %.1f"),
        *GetName(), SoldierCount, CurrentSoldierCount, NewScale, HealthBonus);
}

// ============ 战斗状态系统实现 ============

/**
 * @brief 进入战斗状态
 * @note ✨ 新增 - 通知所有士兵进入战斗
 */
void AXBCharacterBase::EnterCombat()
{
    if (bIsInCombat)
    {
        // 已经在战斗中，刷新超时计时器
        GetWorldTimerManager().ClearTimer(CombatTimeoutHandle);
        GetWorldTimerManager().SetTimer(
            CombatTimeoutHandle,
            this,
            &AXBCharacterBase::OnCombatTimeout,
            CombatTimeoutDuration,
            false
        );
        return;
    }

    bIsInCombat = true;

    UE_LOG(LogTemp, Log, TEXT("%s: 进入战斗状态，士兵数: %d"), *GetName(), Soldiers.Num());

    // 通知所有士兵进入战斗
    for (AXBSoldierActor* Soldier : Soldiers)
    {
        if (Soldier && Soldier->GetSoldierState() != EXBSoldierState::Dead)
        {
            Soldier->EnterCombat();
        }
    }

    // 设置战斗超时计时器
    GetWorldTimerManager().SetTimer(
        CombatTimeoutHandle,
        this,
        &AXBCharacterBase::OnCombatTimeout,
        CombatTimeoutDuration,
        false
    );

    // 广播战斗状态变化
    OnCombatStateChanged.Broadcast(true);
}

/**
 * @brief 退出战斗状态
 * @note ✨ 新增 - 通知所有士兵返回队列
 */
void AXBCharacterBase::ExitCombat()
{
    if (!bIsInCombat)
    {
        return;
    }

    bIsInCombat = false;

    UE_LOG(LogTemp, Log, TEXT("%s: 退出战斗状态"), *GetName());

    // 清除超时计时器
    GetWorldTimerManager().ClearTimer(CombatTimeoutHandle);

    // 通知所有士兵退出战斗，返回队列
    for (AXBSoldierActor* Soldier : Soldiers)
    {
        if (Soldier && Soldier->GetSoldierState() != EXBSoldierState::Dead)
        {
            Soldier->ExitCombat();
        }
    }

    // 广播战斗状态变化
    OnCombatStateChanged.Broadcast(false);
}

/**
 * @brief 战斗超时回调
 */
void AXBCharacterBase::OnCombatTimeout()
{
    UE_LOG(LogTemp, Log, TEXT("%s: 战斗超时，自动退出战斗"), *GetName());
    ExitCombat();
}

/**
 * @brief 攻击命中目标时调用
 * @param HitTarget 命中的目标
 * @note ✨ 新增 - 触发士兵进入战斗的关键函数
 */
void AXBCharacterBase::OnAttackHit(AActor* HitTarget)
{
    if (!HitTarget)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("%s: 攻击命中目标 %s"), *GetName(), *HitTarget->GetName());

    // 进入战斗状态（会刷新超时计时器）
    EnterCombat();
}

void AXBCharacterBase::RecallAllSoldiers()
{
    UE_LOG(LogTemp, Log, TEXT("%s: 召回所有士兵"), *GetName());

    // 退出战斗状态
    ExitCombat();

    // 强制所有士兵返回
    for (AXBSoldierActor* Soldier : Soldiers)
    {
        if (Soldier && Soldier->GetSoldierState() != EXBSoldierState::Dead)
        {
            Soldier->SetSoldierState(EXBSoldierState::Returning);
        }
    }
}

void AXBCharacterBase::SetSoldiersEscaping(bool bEscaping)
{
    UE_LOG(LogTemp, Log, TEXT("%s: 设置士兵逃跑状态: %s"), *GetName(), bEscaping ? TEXT("是") : TEXT("否"));

    for (AXBSoldierActor* Soldier : Soldiers)
    {
        if (Soldier)
        {
            Soldier->SetEscaping(bEscaping);
        }
    }
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
    UE_LOG(LogTemp, Log, TEXT("║ 掉落士兵数: %d"), SoldierDropConfig.DropCount);
    UE_LOG(LogTemp, Log, TEXT("╚══════════════════════════════════════════╝"));

    // 广播死亡事件
    OnCharacterDeath.Broadcast(this);

    // ✨ 新增 - 生成掉落的士兵
    SpawnDroppedSoldiers();

    // 禁用角色移动
    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->DisableMovement();
        MovementComp->StopMovementImmediately();
        UE_LOG(LogTemp, Log, TEXT("%s: 移动已禁用"), *GetName());
    }

    // 禁用碰撞
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

    // 退出战斗状态
    ExitCombat();

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

/**
 * @brief 生成掉落的士兵
 * @note ✨ 新增 - 将领死亡后从中心向四周掉落士兵
 */
void AXBCharacterBase::SpawnDroppedSoldiers()
{
    if (SoldierDropConfig.DropCount <= 0 || !SoldierDropConfig.DropSoldierClass)
    {
        UE_LOG(LogTemp, Log, TEXT("%s: 未配置士兵掉落，跳过"), *GetName());
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FVector SpawnOrigin = GetActorLocation();
    
    UE_LOG(LogTemp, Log, TEXT("%s: 开始掉落 %d 个士兵"), *GetName(), SoldierDropConfig.DropCount);

    for (int32 i = 0; i < SoldierDropConfig.DropCount; ++i)
    {
        // 计算均匀分布的方向 + 随机偏移
        float BaseAngle = (360.0f / SoldierDropConfig.DropCount) * i;
        float RandomAngleOffset = FMath::RandRange(-15.0f, 15.0f);
        float Angle = BaseAngle + RandomAngleOffset;
        
        // 随机距离
        float Distance = FMath::RandRange(SoldierDropConfig.DropRadius * 0.5f, SoldierDropConfig.DropRadius);
        
        // 计算目标位置
        FVector Direction = FRotator(0.0f, Angle, 0.0f).RotateVector(FVector::ForwardVector);
        FVector TargetLocation = SpawnOrigin + Direction * Distance;
        
        // 从上方掉落
        FVector SpawnLocation = TargetLocation + FVector(0.0f, 0.0f, 500.0f);
        
        // 生成士兵
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        
        AXBSoldierActor* DroppedSoldier = World->SpawnActor<AXBSoldierActor>(
            SoldierDropConfig.DropSoldierClass,
            SpawnLocation,
            FRotator::ZeroRotator,
            SpawnParams
        );
        
        if (DroppedSoldier)
        {
            // 设置为中立阵营（可被拾取）
            DroppedSoldier->InitializeSoldier(DroppedSoldier->GetSoldierConfig(), EXBFaction::Neutral);
            DroppedSoldier->SetSoldierState(EXBSoldierState::Idle);
            
            // TODO: 播放掉落动画（使用Timeline或物理模拟）
            // 简化实现：直接设置到目标位置
            DroppedSoldier->SetActorLocation(TargetLocation);
            
            UE_LOG(LogTemp, Log, TEXT("掉落士兵 %d: %s 到位置 %s"), 
                i, *DroppedSoldier->GetName(), *TargetLocation.ToString());
        }
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

    // 清除战斗超时计时器
    GetWorldTimerManager().ClearTimer(CombatTimeoutHandle);

    // 通知所有士兵主将已死亡（士兵也死亡）
    for (AXBSoldierActor* Soldier : Soldiers)
    {
        if (Soldier)
        {
            // 将领死亡后所有剩余士兵一起死亡
            Soldier->SetSoldierState(EXBSoldierState::Dead);
            Soldier->SetLifeSpan(2.0f);
        }
    }
    Soldiers.Empty();

    // 清理 ASC 的所有激活状态
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
