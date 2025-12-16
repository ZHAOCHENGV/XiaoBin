/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Character/XBCharacterBase.cpp

/**
 * @file XBCharacterBase.cpp
 * @brief 角色基类实现
 * 
 * @note 🔧 修改记录:
 *       1. 修复士兵计数同步问题 - 统一由 Soldiers 数组管理
 *       2. 修复将领死亡时循环回调问题 - 添加 bIsCleaningUpSoldiers 标记
 *       3. 使用项目专用日志类别
 *       4. 使用通用函数库进行阵营判断
 */

#include "Character/XBCharacterBase.h"
#include "Utils/XBLogCategories.h"
#include "Utils/XBBlueprintFunctionLibrary.h"
#include "AIController.h"
#include "Character/Components/XBCombatComponent.h"
#include "Character/Components/XBMagnetFieldComponent.h"
#include "Character/Components/XBFormationComponent.h"
#include "UI/XBWorldHealthBarComponent.h"
#include "GAS/XBAbilitySystemComponent.h"
#include "GAS/XBAttributeSet.h"
#include "Data/XBLeaderDataTable.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Engine/DataTable.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"
#include "XBCollisionChannels.h"
#include "AI/XBSoldierPerceptionSubsystem.h"
#include "Particles/ParticleSystemComponent.h"

AXBCharacterBase::AXBCharacterBase()
{
    PrimaryActorTick.bCanEverTick = true;

  
    // ✨ 新增 - 配置将领碰撞通道
    /**
     * @note 设置胶囊体使用将领专用碰撞通道
     *       与士兵通道配置为 Overlap，避免相互阻挡
     */
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        // 设置自己的身份是 Leader
        Capsule->SetCollisionObjectType(XBCollision::Leader);
        
        // 关键设置：对其他 Leader (将领) 必须是 Block (阻挡)
        Capsule->SetCollisionResponseToChannel(XBCollision::Leader, ECR_Block);
        
        // 对 Soldier (士兵) 是 Overlap (重叠/穿过)
        Capsule->SetCollisionResponseToChannel(XBCollision::Soldier, ECR_Overlap);
        
        // ✨ 新增 - 输出详细配置信息
        UE_LOG(LogXBCharacter, Warning, TEXT("将领碰撞配置: ObjectType=%d, 对Soldier(%d)响应=%d, 对Leader(%d)响应=%d"),
            (int32)Capsule->GetCollisionObjectType(),
            (int32)XBCollision::Soldier,
            (int32)Capsule->GetCollisionResponseToChannel(XBCollision::Soldier),
            (int32)XBCollision::Leader,
            (int32)Capsule->GetCollisionResponseToChannel(XBCollision::Leader));
   
    }
    // 🔧 关键修复 - 配置网格体碰撞忽略
    /**
     * @note 解决碰撞阻挡问题的核心：
     * 默认的 CharacterMesh 预设没有处理自定义通道，默认会 Block。
     * 这里必须显式让网格体忽略 Soldier 和 Leader 通道，防止 Mesh 产生物理推挤。
     */
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        // 关键设置：网格体忽略 Leader 和 Soldier
        // 这样即使模型穿模，也不会产生物理推挤力
        MeshComp->SetCollisionResponseToChannel(XBCollision::Leader, ECR_Ignore);
        MeshComp->SetCollisionResponseToChannel(XBCollision::Soldier, ECR_Ignore);
    }
    // 创建 ASC
    AbilitySystemComponent = CreateDefaultSubobject<UXBAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

    // 创建属性集
    AttributeSet = CreateDefaultSubobject<UXBAttributeSet>(TEXT("AttributeSet"));

    // 创建战斗组件
    CombatComponent = CreateDefaultSubobject<UXBCombatComponent>(TEXT("CombatComponent"));

    // 创建头顶血条组件
    HealthBarComponent = CreateDefaultSubobject<UXBWorldHealthBarComponent>(TEXT("HealthBarComponent"));
    HealthBarComponent->SetupAttachment(RootComponent);

    // 创建磁场组件
    MagnetFieldComponent = CreateDefaultSubobject<UXBMagnetFieldComponent>(TEXT("MagnetFieldComponent"));
    MagnetFieldComponent->SetupAttachment(RootComponent);

    // 创建编队组件
    FormationComponent = CreateDefaultSubobject<UXBFormationComponent>(TEXT("FormationComponent"));

    // 禁用控制器旋转
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;
}

void AXBCharacterBase::BeginPlay()
{
    Super::BeginPlay();

    // 初始化 ASC
    InitializeAbilitySystem();

    // 配置移动组件
    SetupMovementComponent();

    // 初始化目标速度
    TargetMoveSpeed = BaseMoveSpeed;

    // 绑定磁场事件
    if (MagnetFieldComponent)
    {
        if (!MagnetFieldComponent->OnActorEnteredField.IsBound())
        {
            MagnetFieldComponent->OnActorEnteredField.AddDynamic(this, &AXBCharacterBase::OnMagnetFieldActorEntered);
        }
        MagnetFieldComponent->SetFieldEnabled(true);
    }

    // 从配置的数据表初始化
    if (ConfigDataTable && !ConfigRowName.IsNone())
    {
        InitializeFromDataTable(ConfigDataTable, ConfigRowName);
    }
    else
    {
        UE_LOG(LogXBCharacter, Warning, TEXT("%s: 未配置数据表或行名，跳过数据表初始化"), *GetName());
    }
}

void AXBCharacterBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 更新冲刺状态
    UpdateSprint(DeltaTime);
}

void AXBCharacterBase::SetupMovementComponent()
{
    UCharacterMovementComponent* CMC = GetCharacterMovement();
    if (!CMC)
    {
        return;
    }

    CMC->MaxWalkSpeed = BaseMoveSpeed;
    CMC->BrakingDecelerationWalking = 2000.0f;
    CMC->GroundFriction = 8.0f;
    CMC->bOrientRotationToMovement = true;
    CMC->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
    CMC->MaxAcceleration = 2048.0f;
    CMC->BrakingFrictionFactor = 2.0f;
}

void AXBCharacterBase::OnMagnetFieldActorEntered(AActor* EnteredActor)
{
    if (!EnteredActor)
    {
        return;
    }

    UE_LOG(LogXBRecruit, Log, TEXT("%s: Actor 进入磁场: %s"), *GetName(), *EnteredActor->GetName());
}

void AXBCharacterBase::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
    }
}

UAbilitySystemComponent* AXBCharacterBase::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AXBCharacterBase::InitializeAbilitySystem()
{
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
        UE_LOG(LogXBCharacter, Log, TEXT("%s: ASC 初始化完成"), *GetName());
    }
}

void AXBCharacterBase::InitializeFromDataTable(UDataTable* DataTable, FName RowName)
{
    if (!DataTable)
    {
        UE_LOG(LogXBCharacter, Error, TEXT("%s: InitializeFromDataTable - 数据表为空"), *GetName());
        return;
    }

    if (RowName.IsNone())
    {
        UE_LOG(LogXBCharacter, Error, TEXT("%s: InitializeFromDataTable - 行名为空"), *GetName());
        return;
    }

    FXBLeaderTableRow* LeaderRow = DataTable->FindRow<FXBLeaderTableRow>(RowName, TEXT("AXBCharacterBase::InitializeFromDataTable"));
    if (!LeaderRow)
    {
        UE_LOG(LogXBCharacter, Error, TEXT("%s: InitializeFromDataTable - 找不到行 '%s'"), *GetName(), *RowName.ToString());
        return;
    }

    CachedLeaderData = *LeaderRow;

    GrowthConfigCache.HealthPerSoldier = LeaderRow->HealthPerSoldier;
    GrowthConfigCache.ScalePerSoldier = LeaderRow->ScalePerSoldier;
    GrowthConfigCache.MaxScale = LeaderRow->MaxScale;

    // 初始化战斗组件
    if (CombatComponent)
    {
        CombatComponent->InitializeFromDataTable(DataTable, RowName);
    }

    // 应用属性到 ASC
    ApplyInitialAttributes();

    // 应用移动速度
    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->MaxWalkSpeed = LeaderRow->MoveSpeed;
        BaseMoveSpeed = LeaderRow->MoveSpeed;
        TargetMoveSpeed = BaseMoveSpeed;
    }

    UE_LOG(LogXBCharacter, Log, TEXT("%s: 从数据表加载配置成功"), *GetName());
}

void AXBCharacterBase::ApplyInitialAttributes()
{
    if (!AbilitySystemComponent)
    {
        return;
    }

    const UXBAttributeSet* LocalAttributeSet = AbilitySystemComponent->GetSet<UXBAttributeSet>();
    if (!LocalAttributeSet)
    {
        return;
    }

    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetMaxHealthAttribute(), CachedLeaderData.MaxHealth);
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetHealthAttribute(), CachedLeaderData.MaxHealth);
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetHealthMultiplierAttribute(), CachedLeaderData.HealthMultiplier);
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetBaseDamageAttribute(), CachedLeaderData.BaseDamage);
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetDamageMultiplierAttribute(), CachedLeaderData.DamageMultiplier);
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetMoveSpeedAttribute(), CachedLeaderData.MoveSpeed);
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetScaleAttribute(), CachedLeaderData.Scale);
}

// ==================== 冲刺系统实现 ====================

void AXBCharacterBase::StartSprint()
{
    if (bIsDead)
    {
        return;
    }

    if (bIsSprinting)
    {
        return;
    }

    bIsSprinting = true;
    TargetMoveSpeed = BaseMoveSpeed * SprintSpeedMultiplier;

    SetSoldiersEscaping(true);
    OnSprintStateChanged.Broadcast(true);

    UE_LOG(LogXBCharacter, Log, TEXT("%s: 开始冲刺，目标速度: %.1f"), *GetName(), TargetMoveSpeed);
}

void AXBCharacterBase::StopSprint()
{
    if (!bIsSprinting)
    {
        return;
    }

    bIsSprinting = false;
    TargetMoveSpeed = BaseMoveSpeed;

    SetSoldiersEscaping(false);
    OnSprintStateChanged.Broadcast(false);

    UE_LOG(LogXBCharacter, Log, TEXT("%s: 停止冲刺，目标速度: %.1f"), *GetName(), TargetMoveSpeed);
}

float AXBCharacterBase::GetCurrentMoveSpeed() const
{
    if (const UCharacterMovementComponent* CMC = GetCharacterMovement())
    {
        return CMC->MaxWalkSpeed;
    }
    return BaseMoveSpeed;
}

void AXBCharacterBase::UpdateSprint(float DeltaTime)
{
    UCharacterMovementComponent* CMC = GetCharacterMovement();
    if (!CMC)
    {
        return;
    }

    float CurrentSpeed = CMC->MaxWalkSpeed;

    if (!FMath::IsNearlyEqual(CurrentSpeed, TargetMoveSpeed, 1.0f))
    {
        float NewSpeed = FMath::FInterpTo(CurrentSpeed, TargetMoveSpeed, DeltaTime, SpeedInterpRate);
        CMC->MaxWalkSpeed = NewSpeed;
    }
}

// ==================== 阵营系统实现 ====================

bool AXBCharacterBase::IsHostileTo(const AXBCharacterBase* Other) const
{
    if (!Other)
    {
        return false;
    }

    // 🔧 修改 - 使用通用函数库
    return UXBBlueprintFunctionLibrary::AreFactionsHostile(Faction, Other->Faction);
}

bool AXBCharacterBase::IsFriendlyTo(const AXBCharacterBase* Other) const
{
    if (!Other)
    {
        return false;
    }

    // 🔧 修改 - 使用通用函数库
    return UXBBlueprintFunctionLibrary::AreFactionsFriendly(Faction, Other->Faction);
}

// ==================== 士兵管理实现（🔧 重点修改） ====================

/**
 * @brief 内部添加士兵到数组
 * @param Soldier 士兵
 * @return 是否添加成功
 * @note ✨ 新增 - 纯粹的数组操作，不触发成长逻辑
 */
bool AXBCharacterBase::Internal_AddSoldierToArray(AXBSoldierCharacter* Soldier)
{
    if (!Soldier)
    {
        return false;
    }

    if (Soldiers.Contains(Soldier))
    {
        return false;
    }

    Soldiers.Add(Soldier);
    return true;
}

/**
 * @brief 内部从数组移除士兵
 * @param Soldier 士兵
 * @return 是否移除成功
 * @note ✨ 新增 - 纯粹的数组操作，不触发缩减逻辑
 */
bool AXBCharacterBase::Internal_RemoveSoldierFromArray(AXBSoldierCharacter* Soldier)
{
    if (!Soldier)
    {
        return false;
    }

    int32 RemovedIndex = Soldiers.Find(Soldier);
    if (RemovedIndex == INDEX_NONE)
    {
        return false;
    }

    Soldiers.RemoveAt(RemovedIndex);
    ReassignSoldierSlots(RemovedIndex);
    return true;
}

/**
 * @brief 更新士兵计数并广播事件
 * @param OldCount 旧计数
 * @note ✨ 新增 - 统一的计数更新入口
 */
void AXBCharacterBase::UpdateSoldierCount(int32 OldCount)
{
    int32 NewCount = Soldiers.Num();

    if (OldCount != NewCount)
    {
        OnSoldierCountChanged.Broadcast(OldCount, NewCount);
    }
}

/**
 * @brief 添加士兵
 * @param Soldier 士兵
 * @note 🔧 修改 - 重构，统一计数管理
 */
void AXBCharacterBase::AddSoldier(AXBSoldierCharacter* Soldier)
{
    if (bIsDead)
    {
        UE_LOG(LogXBCharacter, Warning, TEXT("%s: 角色已死亡，无法添加士兵"), *GetName());
        return;
    }

    if (!Soldier)
    {
        return;
    }

    int32 OldCount = Soldiers.Num();

    if (!Internal_AddSoldierToArray(Soldier))
    {
        return;
    }

    int32 SlotIndex = Soldiers.Num() - 1;
    Soldier->SetFormationSlotIndex(SlotIndex);
    Soldier->SetFollowTarget(this, SlotIndex);


    ApplyGrowthOnSoldiersAdded(1);

    UpdateSoldierCount(OldCount);

    if (FormationComponent)
    {
        FormationComponent->RegenerateFormation(Soldiers.Num());
    }

    UE_LOG(LogXBSoldier, Log, TEXT("%s: 添加士兵 %s，槽位: %d，当前数量: %d"),
        *GetName(), *Soldier->GetName(), SlotIndex, Soldiers.Num());
}

/**
 * @brief 移除士兵
 * @param Soldier 士兵
 * @note 🔧 修改 - 重构，不直接处理成长缩减（由 OnSoldierDied 处理）
 */
void AXBCharacterBase::RemoveSoldier(AXBSoldierCharacter* Soldier)
{
    if (!Soldier)
    {
        return;
    }

    int32 OldCount = Soldiers.Num();

    // 从数组移除
    if (!Internal_RemoveSoldierFromArray(Soldier))
    {
        return; // 不存在，跳过
    }

    // 更新计数并广播
    UpdateSoldierCount(OldCount);

    // 更新编队
    if (FormationComponent)
    {
        FormationComponent->RegenerateFormation(Soldiers.Num());
    }

    UE_LOG(LogXBSoldier, Log, TEXT("%s: 移除士兵 %s，剩余数量: %d"),
        *GetName(), *Soldier->GetName(), Soldiers.Num());
}

void AXBCharacterBase::ReassignSoldierSlots(int32 StartIndex)
{
    for (int32 i = StartIndex; i < Soldiers.Num(); ++i)
    {
        if (Soldiers[i])
        {
            Soldiers[i]->SetFormationSlotIndex(i);
        }
    }
}

/**
 * @brief 士兵死亡回调
 * @param DeadSoldier 死亡的士兵
 * @note 🔧 修改 - 修复计数同步问题
 *       1. 先从数组移除
 *       2. 再应用缩减效果
 *       3. 不再手动修改计数
 */
void AXBCharacterBase::OnSoldierDied(AXBSoldierCharacter* DeadSoldier)
{
    if (!DeadSoldier)
    {
        return;
    }

    // ✨ 新增 - 检查是否正在清理（防止循环回调）
    if (bIsCleaningUpSoldiers)
    {
        UE_LOG(LogXBSoldier, Verbose, TEXT("%s: 正在清理士兵，跳过 OnSoldierDied 回调"), *GetName());
        return;
    }

    // 从队列移除（不触发成长逻辑）
    RemoveSoldier(DeadSoldier);

    // 🔧 修改 - 应用缩减效果
    ApplyGrowthOnSoldiersRemoved(1);

    UE_LOG(LogXBSoldier, Log, TEXT("将领 %s 失去士兵，剩余: %d，体型: %.2f"),
        *GetName(), Soldiers.Num(), GetCurrentScale());
}

/**
 * @brief 应用士兵增加带来的成长效果
 * @param SoldierCount 增加的士兵数量
 * @note ✨ 新增 - 原 OnSoldiersAdded 的核心逻辑
 */
void AXBCharacterBase::ApplyGrowthOnSoldiersAdded(int32 SoldierCount)
{
    if (bIsDead || SoldierCount <= 0)
    {
        return;
    }

    // 1. 更新体型缩放
    UpdateLeaderScale();

    // 2. 更新血量（支持溢出）
    const float HealthBonus = SoldierCount * GrowthConfigCache.HealthPerSoldier;
    AddHealthWithOverflow(HealthBonus);

    // 3. 更新技能特效缩放
    if (GrowthConfigCache.bEnableSkillEffectScaling)
    {
        UpdateSkillEffectScaling();
    }

    // 4. 更新攻击范围缩放
    if (GrowthConfigCache.bEnableAttackRangeScaling)
    {
        UpdateAttackRangeScaling();
    }

    UE_LOG(LogXBCharacter, Log, TEXT("将领 %s 招募 %d 个士兵，当前总数: %d，体型: %.2f"),
        *GetName(), SoldierCount, Soldiers.Num(), GetCurrentScale());
}

/**
 * @brief 应用士兵减少带来的缩减效果
 * @param SoldierCount 减少的士兵数量
 * @note ✨ 新增 - 分离出缩减逻辑
 */
void AXBCharacterBase::ApplyGrowthOnSoldiersRemoved(int32 SoldierCount)
{
    if (SoldierCount <= 0)
    {
        return;
    }

    // 1. 缩小体型
    UpdateLeaderScale();

    // 2. 不减少血量（按需求）

    // 3. 更新技能特效缩放
    if (GrowthConfigCache.bEnableSkillEffectScaling)
    {
        UpdateSkillEffectScaling();
    }

    // 4. 更新攻击范围缩放
    if (GrowthConfigCache.bEnableAttackRangeScaling)
    {
        UpdateAttackRangeScaling();
    }
}

float AXBCharacterBase::GetCurrentScale() const
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->GetNumericAttribute(UXBAttributeSet::GetScaleAttribute());
    }
    return BaseScale;
}

float AXBCharacterBase::GetScaledAttackRange() const
{
    float CurrentScale = GetCurrentScale();
    return BaseAttackRange * CurrentScale * GrowthConfigCache.AttackRangeScaleMultiplier;
}

/**
 * @brief 更新角色体型
 * @note 使用累加方式计算缩放
 *       公式：最终缩放 = BaseScale + (士兵数 × 每士兵加成)
 */
void AXBCharacterBase::UpdateLeaderScale()
{
    // 🔧 修改 - 直接使用 Soldiers.Num()
    const float AdditionalScale = Soldiers.Num() * GrowthConfigCache.ScalePerSoldier;
    const float NewScale = FMath::Min(BaseScale + AdditionalScale, GrowthConfigCache.MaxScale);

    // 应用到Actor
    SetActorScale3D(FVector(NewScale));

    // 同步到ASC属性
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetScaleAttribute(), NewScale);
    }

    // 同步更新战斗组件的攻击范围缩放
    if (CombatComponent && GrowthConfigCache.bEnableAttackRangeScaling)
    {
        float RangeScale = NewScale * GrowthConfigCache.AttackRangeScaleMultiplier;
        CombatComponent->SetAttackRangeScale(RangeScale);
    }

    UE_LOG(LogXBCharacter, Verbose, TEXT("体型更新: BaseScale=%.2f, 士兵数=%d, 最终缩放=%.2f"),
        BaseScale, Soldiers.Num(), NewScale);
}

void AXBCharacterBase::AddHealthWithOverflow(float HealthToAdd)
{
    if (!AbilitySystemComponent)
    {
        return;
    }

    float CurrentHealth = AbilitySystemComponent->GetNumericAttribute(UXBAttributeSet::GetHealthAttribute());
    float CurrentMaxHealth = AbilitySystemComponent->GetNumericAttribute(UXBAttributeSet::GetMaxHealthAttribute());

    float NewHealth = CurrentHealth + HealthToAdd;

    if (NewHealth > CurrentMaxHealth)
    {
        AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetMaxHealthAttribute(), NewHealth);
        AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetHealthAttribute(), NewHealth);

        UE_LOG(LogXBCharacter, Log, TEXT("血量溢出：最大血量提升 %.0f → %.0f"), CurrentMaxHealth, NewHealth);
    }
    else
    {
        AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetHealthAttribute(), NewHealth);

        UE_LOG(LogXBCharacter, Verbose, TEXT("血量回复：%.0f → %.0f (最大%.0f)"),
            CurrentHealth, NewHealth, CurrentMaxHealth);
    }
}

void AXBCharacterBase::UpdateSkillEffectScaling()
{
    if (!AbilitySystemComponent)
    {
        return;
    }

    float CurrentScale = GetCurrentScale();
    float EffectScale = CurrentScale * GrowthConfigCache.SkillEffectScaleMultiplier;

    UE_LOG(LogXBCharacter, Verbose, TEXT("技能特效缩放更新: %.2f"), EffectScale);

    TArray<UActorComponent*> Components;
    GetComponents(UParticleSystemComponent::StaticClass(), Components);

    for (UActorComponent* Comp : Components)
    {
        if (UParticleSystemComponent* PSC = Cast<UParticleSystemComponent>(Comp))
        {
            if (PSC->ComponentHasTag(FName("SkillEffect")))
            {
                PSC->SetWorldScale3D(FVector(EffectScale));
            }
        }
    }
}

void AXBCharacterBase::UpdateAttackRangeScaling()
{
    if (!CombatComponent)
    {
        return;
    }

    float CurrentScale = GetCurrentScale();
    float ScaledRange = BaseAttackRange * CurrentScale * GrowthConfigCache.AttackRangeScaleMultiplier;

    UE_LOG(LogXBCharacter, Verbose, TEXT("攻击范围更新: %.0f → %.0f"), BaseAttackRange, ScaledRange);
}

// ==================== 战斗状态系统实现 ====================

void AXBCharacterBase::EnterCombat()
{
    if (bIsDead)
    {
        return;
    }

    if (bIsInCombat)
    {
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

    // ✨ 新增 - 标记当前位置为热点区域
    if (UWorld* World = GetWorld())
    {
        if (UXBSoldierPerceptionSubsystem* Perception = World->GetSubsystem<UXBSoldierPerceptionSubsystem>())
        {
            // 以将领为中心，标记 1500 单位半径为热点
            Perception->MarkHotspotRegion(GetActorLocation(), 1500.0f);
        }
    }

    for (AXBSoldierCharacter* Soldier : Soldiers)
    {
        if (Soldier && Soldier->GetSoldierState() != EXBSoldierState::Dead)
        {
            Soldier->EnterCombat();
        }
    }

    GetWorldTimerManager().SetTimer(
        CombatTimeoutHandle,
        this,
        &AXBCharacterBase::OnCombatTimeout,
        CombatTimeoutDuration,
        false
    );

    OnCombatStateChanged.Broadcast(true);
}

void AXBCharacterBase::ExitCombat()
{
    if (!bIsInCombat)
    {
        return;
    }

    bIsInCombat = false;

    GetWorldTimerManager().ClearTimer(CombatTimeoutHandle);

    // ✨ 新增 - 清除热点区域标记
    if (UWorld* World = GetWorld())
    {
        if (UXBSoldierPerceptionSubsystem* Perception = World->GetSubsystem<UXBSoldierPerceptionSubsystem>())
        {
            Perception->ClearHotspotRegion(GetActorLocation());
        }
    }

    for (AXBSoldierCharacter* Soldier : Soldiers)
    {
        if (Soldier && Soldier->GetSoldierState() != EXBSoldierState::Dead)
        {
            Soldier->ExitCombat();
        }
    }

    OnCombatStateChanged.Broadcast(false);
}

void AXBCharacterBase::DisengageFromCombat()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastDisengageTime < DisengageCooldown)
    {
        UE_LOG(LogXBCombat, Verbose, TEXT("脱离战斗冷却中，剩余: %.1f秒"),
            DisengageCooldown - (CurrentTime - LastDisengageTime));
        return;
    }

    LastDisengageTime = CurrentTime;

    UE_LOG(LogXBCombat, Warning, TEXT(">>> 将领 %s 脱离战斗（逃跑） <<<"), *GetName());

    ExitCombat();
    RecallAllSoldiers();

    if (bSprintWhenDisengaging)
    {
        StartSprint();

        GetWorldTimerManager().ClearTimer(DisengageSprintTimerHandle);
        GetWorldTimerManager().SetTimer(
            DisengageSprintTimerHandle,
            this,
            &AXBCharacterBase::StopSprint,
            DisengageSprintDuration,
            false
        );

        UE_LOG(LogXBCombat, Log, TEXT("逃跑冲刺启动，持续时间: %.1f秒"), DisengageSprintDuration);
    }

    SetSoldiersEscaping(true);

    FTimerHandle TempHandle;
    GetWorldTimerManager().SetTimer(
        TempHandle,
        [this]()
        {
            SetSoldiersEscaping(false);
        },
        DisengageSprintDuration,
        false
    );
}

void AXBCharacterBase::OnCombatTimeout()
{
    ExitCombat();
}

void AXBCharacterBase::OnAttackHit(AActor* HitTarget)
{
    if (!HitTarget)
    {
        return;
    }

    EnterCombat();
}

void AXBCharacterBase::RecallAllSoldiers()
{
    ExitCombat();

    for (AXBSoldierCharacter* Soldier : Soldiers)
    {
        if (Soldier && Soldier->GetSoldierState() != EXBSoldierState::Dead)
        {
            Soldier->SetSoldierState(EXBSoldierState::Returning);
            Soldier->CurrentAttackTarget = nullptr;

            if (AAIController* AICtrl = Cast<AAIController>(Soldier->GetController()))
            {
                AICtrl->StopMovement();
            }
        }
    }

    UE_LOG(LogXBSoldier, Log, TEXT("将领 %s 召回所有士兵"), *GetName());
}

void AXBCharacterBase::SetSoldiersEscaping(bool bEscaping)
{
    for (AXBSoldierCharacter* Soldier : Soldiers)
    {
        if (Soldier)
        {
            Soldier->SetEscaping(bEscaping);
        }
    }
}

// ==================== 死亡系统实现 ====================

void AXBCharacterBase::HandleDeath()
{
    if (bIsDead)
    {
        return;
    }

    bIsDead = true;

    UE_LOG(LogXBCharacter, Log, TEXT("%s: 角色死亡"), *GetName());

    if (MagnetFieldComponent)
    {
        MagnetFieldComponent->SetFieldEnabled(false);
        UE_LOG(LogXBCharacter, Log, TEXT("%s: 磁场组件已禁用"), *GetName());
    }

    if (HealthBarComponent)
    {
        HealthBarComponent->SetHealthBarVisible(false);
        HealthBarComponent->SetComponentTickEnabled(false);
        UE_LOG(LogXBCharacter, Log, TEXT("%s: 血条已隐藏"), *GetName());
    }

    OnCharacterDeath.Broadcast(this);

    SpawnDroppedSoldiers();

    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->DisableMovement();
        MovementComp->StopMovementImmediately();
    }

    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->CancelAllAbilities();
    }

    ExitCombat();

    if (bIsSprinting)
    {
        StopSprint();
    }

    bool bMontageStarted = false;
    if (DeathMontage)
    {
        if (USkeletalMeshComponent* MeshComp = GetMesh())
        {
            if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
            {
                AnimInstance->StopAllMontages(0.2f);

                float Duration = AnimInstance->Montage_Play(DeathMontage, 1.0f);
                if (Duration > 0.0f)
                {
                    bMontageStarted = true;

                    FOnMontageEnded EndDelegate;
                    EndDelegate.BindUObject(this, &AXBCharacterBase::OnDeathMontageEnded);
                    AnimInstance->Montage_SetEndDelegate(EndDelegate, DeathMontage);

                    if (!bDelayAfterMontage)
                    {
                        GetWorldTimerManager().SetTimer(
                            DeathDestroyTimerHandle,
                            this,
                            &AXBCharacterBase::OnDestroyTimerExpired,
                            DeathDestroyDelay,
                            false
                        );
                    }
                }
            }
        }
    }

    if (!bMontageStarted)
    {
        GetWorldTimerManager().SetTimer(
            DeathDestroyTimerHandle,
            this,
            &AXBCharacterBase::OnDestroyTimerExpired,
            DeathDestroyDelay,
            false
        );
    }
}

void AXBCharacterBase::SpawnDroppedSoldiers()
{
    if (SoldierDropConfig.DropCount <= 0 || !SoldierDropConfig.DropSoldierClass)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FVector SpawnOrigin = GetActorLocation();

    for (int32 i = 0; i < SoldierDropConfig.DropCount; ++i)
    {
        float BaseAngle = (360.0f / SoldierDropConfig.DropCount) * i;
        float RandomAngleOffset = FMath::RandRange(-15.0f, 15.0f);
        float Angle = BaseAngle + RandomAngleOffset;

        float Distance = FMath::RandRange(SoldierDropConfig.DropRadius * 0.5f, SoldierDropConfig.DropRadius);

        FVector Direction = FRotator(0.0f, Angle, 0.0f).RotateVector(FVector::ForwardVector);
        FVector TargetLocation = SpawnOrigin + Direction * Distance;

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        AXBSoldierCharacter* DroppedSoldier = World->SpawnActor<AXBSoldierCharacter>(
            SoldierDropConfig.DropSoldierClass,
            TargetLocation,
            FRotator::ZeroRotator,
            SpawnParams
        );

        if (DroppedSoldier)
        {
            // 🔧 修复 - 使用 InitializeFromDataTable
            if (SoldierDataTable && !RecruitSoldierRowName.IsNone())
            {
                DroppedSoldier->InitializeFromDataTable(SoldierDataTable, RecruitSoldierRowName, EXBFaction::Neutral);
            }
            DroppedSoldier->SetSoldierState(EXBSoldierState::Idle);
        }
    }
}

void AXBCharacterBase::OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (bDelayAfterMontage)
    {
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
    PreDestroyCleanup();
    Destroy();
}

/**
 * @brief 销毁前清理
 * @note 🔧 修改 - 添加 bIsCleaningUpSoldiers 标记防止循环回调
 */
void AXBCharacterBase::PreDestroyCleanup()
{
    GetWorldTimerManager().ClearTimer(CombatTimeoutHandle);

    // ✨ 新增 - 设置清理标记，防止士兵死亡回调
    bIsCleaningUpSoldiers = true;

    for (AXBSoldierCharacter* Soldier : Soldiers)
    {
        if (Soldier && IsValid(Soldier))
        {
            // 直接设置状态，不触发回调
            Soldier->SetSoldierState(EXBSoldierState::Dead);
            Soldier->SetLifeSpan(2.0f);
        }
    }
    Soldiers.Empty();

    // ✨ 新增 - 清除标记
    bIsCleaningUpSoldiers = false;

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->CancelAllAbilities();
        AbilitySystemComponent->RemoveAllGameplayCues();
        AbilitySystemComponent->RemoveActiveEffectsWithTags(FGameplayTagContainer());
    }

    GetWorldTimerManager().ClearTimer(DeathDestroyTimerHandle);
}
