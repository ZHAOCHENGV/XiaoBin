/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Character/XBCharacterBase.cpp

/**
 * @file XBCharacterBase.cpp
 * @brief 角色基类实现
 * 
 * @note 🔧 修改记录:
 *       1. 修复士兵计数同步问题
 *       2. 修复将领死亡时循环回调问题
 *       3. ✨ 新增 掉落士兵抛物线系统（落地自动入列）
 *       4. 🔧 修改 使用 FullInitialize 完整初始化掉落士兵
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
#include "Soldier/Component/XBSoldierPoolSubsystem.h"
#include "AI/XBSoldierAIController.h"

AXBCharacterBase::AXBCharacterBase()
{
    PrimaryActorTick.bCanEverTick = true;

    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionObjectType(XBCollision::Leader);
        Capsule->SetCollisionResponseToChannel(XBCollision::Leader, ECR_Block);
        Capsule->SetCollisionResponseToChannel(XBCollision::Soldier, ECR_Overlap);
    }

    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetCollisionResponseToChannel(XBCollision::Leader, ECR_Ignore);
        MeshComp->SetCollisionResponseToChannel(XBCollision::Soldier, ECR_Ignore);
    }

    AbilitySystemComponent = CreateDefaultSubobject<UXBAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AttributeSet = CreateDefaultSubobject<UXBAttributeSet>(TEXT("AttributeSet"));
    CombatComponent = CreateDefaultSubobject<UXBCombatComponent>(TEXT("CombatComponent"));

    HealthBarComponent = CreateDefaultSubobject<UXBWorldHealthBarComponent>(TEXT("HealthBarComponent"));
    HealthBarComponent->SetupAttachment(RootComponent);

    MagnetFieldComponent = CreateDefaultSubobject<UXBMagnetFieldComponent>(TEXT("MagnetFieldComponent"));
    MagnetFieldComponent->SetupAttachment(RootComponent);

    FormationComponent = CreateDefaultSubobject<UXBFormationComponent>(TEXT("SoldierFormationComponent"));

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;
}

void AXBCharacterBase::BeginPlay()
{
    Super::BeginPlay();

    // 🔧 修改 - 将主将注册到感知子系统，确保士兵可以感知到主将
    if (UWorld* World = GetWorld())
    {
        // 🔧 修改 - 仅在子系统有效时执行注册
        if (UXBSoldierPerceptionSubsystem* Perception = World->GetSubsystem<UXBSoldierPerceptionSubsystem>())
        {
            // 🔧 修改 - 使用主将阵营注册，便于阵营筛选
            Perception->RegisterActor(this, Faction);
        }
    }

    InitializeAbilitySystem();
    SetupMovementComponent();

    TargetMoveSpeed = BaseMoveSpeed;

    BindCombatEvents();

    if (MagnetFieldComponent)
    {
        if (!MagnetFieldComponent->OnActorEnteredField.IsBound())
        {
            MagnetFieldComponent->OnActorEnteredField.AddDynamic(this, &AXBCharacterBase::OnMagnetFieldActorEntered);
        }
        MagnetFieldComponent->SetFieldEnabled(true);
    }

    if (ConfigDataTable && !ConfigRowName.IsNone())
    {
        InitializeFromDataTable(ConfigDataTable, ConfigRowName);
    }
}

/**
 * @brief 结束播放时处理感知子系统注销
 * @param EndPlayReason 结束原因
 * @return 无
 * @note 功能说明: 退出时将主将从感知子系统中移除
 * @note 详细流程: 获取世界 -> 获取感知子系统 -> 注销 Actor -> 调用父类 EndPlay
 * @note 注意事项: 需要在注销后再调用父类 EndPlay，避免访问已销毁对象
 */
void AXBCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 🔧 修改 - 获取世界实例用于感知注销
    if (UWorld* World = GetWorld())
    {
        // 🔧 修改 - 子系统有效时执行注销
        if (UXBSoldierPerceptionSubsystem* Perception = World->GetSubsystem<UXBSoldierPerceptionSubsystem>())
        {
            // 🔧 修改 - 注销当前主将
            Perception->UnregisterActor(this);
        }
    }

    // 🔧 修改 - 调用父类 EndPlay
    Super::EndPlay(EndPlayReason);
}

void AXBCharacterBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
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
    }
}

void AXBCharacterBase::InitializeFromDataTable(UDataTable* DataTable, FName RowName)
{
    if (!DataTable || RowName.IsNone())
    {
        return;
    }

    FXBLeaderTableRow* LeaderRow = DataTable->FindRow<FXBLeaderTableRow>(RowName, TEXT("AXBCharacterBase::InitializeFromDataTable"));
    if (!LeaderRow)
    {
        return;
    }

    CachedLeaderData = *LeaderRow;

    GrowthConfigCache.HealthPerSoldier = LeaderRow->HealthPerSoldier;
    GrowthConfigCache.ScalePerSoldier = LeaderRow->ScalePerSoldier;
    GrowthConfigCache.MaxScale = LeaderRow->MaxScale;
    GrowthConfigCache.DamageMultiplierPerSoldier = LeaderRow->DamageMultiplierPerSoldier;
    GrowthConfigCache.MaxDamageMultiplier = LeaderRow->MaxDamageMultiplier;

    // 🔧 修改 - 从数据表加载骨骼网格/动画蓝图/死亡蒙太奇，体现数据驱动
    if (!LeaderRow->SkeletalMesh.IsNull())
    {
        if (USkeletalMesh* LoadedMesh = LeaderRow->SkeletalMesh.LoadSynchronous())
        {
            if (USkeletalMeshComponent* MeshComp = GetMesh())
            {
                MeshComp->SetSkeletalMesh(LoadedMesh);
            }
        }
    }

    if (!LeaderRow->AnimClass.IsNull())
    {
        AnimClass = LeaderRow->AnimClass.LoadSynchronous();
        if (USkeletalMeshComponent* MeshComp = GetMesh())
        {
            if (AnimClass)
            {
                MeshComp->SetAnimInstanceClass(AnimClass);
            }
        }
    }

    if (!LeaderRow->DeathMontage.IsNull())
    {
        DeathMontage = LeaderRow->DeathMontage.LoadSynchronous();
    }

    UE_LOG(LogXBCharacter, Log, TEXT("主将 %s 视觉配置加载完成: Mesh=%s, AnimClass=%s, DeathMontage=%s"),
        *GetName(),
        GetMesh() && GetMesh()->GetSkeletalMeshAsset() ? *GetMesh()->GetSkeletalMeshAsset()->GetName() : TEXT("无"),
        AnimClass ? *AnimClass->GetName() : TEXT("无"),
        DeathMontage ? *DeathMontage->GetName() : TEXT("无"));

    if (CombatComponent)
    {
        CombatComponent->InitializeFromDataTable(DataTable, RowName);
    }

    ApplyInitialAttributes();

    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->MaxWalkSpeed = LeaderRow->MoveSpeed;
        BaseMoveSpeed = LeaderRow->MoveSpeed;
        TargetMoveSpeed = BaseMoveSpeed;
    }
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
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetDamageMultiplierAttribute(), CachedLeaderData.DamageMultiplier);
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetMoveSpeedAttribute(), CachedLeaderData.MoveSpeed);
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetScaleAttribute(), CachedLeaderData.Scale);
}

// ==================== 冲刺系统实现 ====================

void AXBCharacterBase::StartSprint()
{
    if (bIsDead || bIsSprinting)
    {
        return;
    }

    bIsSprinting = true;
    TargetMoveSpeed = BaseMoveSpeed * SprintSpeedMultiplier;

    SetSoldiersEscaping(true);
    OnSprintStateChanged.Broadcast(true);
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
}

float AXBCharacterBase::GetCurrentMoveSpeed() const
{
    if (const UCharacterMovementComponent* CMC = GetCharacterMovement())
    {
        return CMC->MaxWalkSpeed;
    }
    return BaseMoveSpeed;
}

/**
 * @brief 获取最近攻击的敌方阵营
 * @param OutFaction 输出的敌方阵营
 * @return 是否有有效的敌方阵营记录
 * @note 功能说明: 将主将最近攻击到的敌方阵营暴露给士兵，用于优先选敌
 * @note 详细流程: 检查是否有记录 -> 输出阵营 -> 返回结果
 * @note 注意事项: 若没有记录，OutFaction 不会被修改
 */
bool AXBCharacterBase::GetLastAttackedEnemyFaction(EXBFaction& OutFaction) const
{
    // 🔧 修改 - 无记录时直接返回失败
    if (!bHasLastAttackedEnemyFaction)
    {
        return false;
    }

    // 🔧 修改 - 输出记录的敌方阵营
    OutFaction = LastAttackedEnemyFaction;

    // 🔧 修改 - 返回成功
    return true;
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

    return UXBBlueprintFunctionLibrary::AreFactionsHostile(Faction, Other->Faction);
}

bool AXBCharacterBase::IsFriendlyTo(const AXBCharacterBase* Other) const
{
    if (!Other)
    {
        return false;
    }

    return UXBBlueprintFunctionLibrary::AreFactionsFriendly(Faction, Other->Faction);
}

// ==================== 士兵管理实现 ====================

bool AXBCharacterBase::Internal_AddSoldierToArray(AXBSoldierCharacter* Soldier)
{
    if (!Soldier || Soldiers.Contains(Soldier))
    {
        return false;
    }

    Soldiers.Add(Soldier);
    return true;
}

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
 * @note 🔧 修复 - 检查士兵是否已在队伍中，避免重复添加
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
        UE_LOG(LogXBCharacter, Warning, TEXT("%s: 士兵指针为空"), *GetName());
        return;
    }

    // 🔧 修改 - 检查是否已在队伍中
    int32 ExistingIndex = Soldiers.Find(Soldier);
    if (ExistingIndex != INDEX_NONE)
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("%s: 士兵 %s 已在队伍中（索引: %d），同步槽位"),
            *GetName(), *Soldier->GetName(), ExistingIndex);
        
        // ✨ 新增 - 确保槽位索引正确同步
        if (Soldier->GetFormationSlotIndex() != ExistingIndex)
        {
            Soldier->SetFormationSlotIndex(ExistingIndex);
            UE_LOG(LogXBSoldier, Log, TEXT("%s: 同步士兵 %s 槽位索引为 %d"),
                *GetName(), *Soldier->GetName(), ExistingIndex);
        }
        return;
    }

    int32 OldCount = Soldiers.Num();

    // 添加到数组
    Soldiers.Add(Soldier);

    // ✨ 核心 - 槽位索引等于数组中的位置（最后一个）
    int32 SlotIndex = Soldiers.Num() - 1;
    
    // 设置士兵的槽位索引
    Soldier->SetFormationSlotIndex(SlotIndex);
    
    UE_LOG(LogXBSoldier, Log, TEXT("%s: 士兵 %s 添加成功，分配槽位: %d, 当前数量: %d"),
        *GetName(), *Soldier->GetName(), SlotIndex, Soldiers.Num());

    // 应用成长效果
    ApplyGrowthOnSoldiersAdded(1);

    // 更新计数
    UpdateSoldierCount(OldCount);

    // 更新编队组件
    if (FormationComponent)
    {
        FormationComponent->RegenerateFormation(Soldiers.Num());
        
        if (FormationComponent->GetFormationSlots().IsValidIndex(SlotIndex))
        {
            FormationComponent->OccupySlot(SlotIndex, Soldier->GetUniqueID());
        }
    }
}

void AXBCharacterBase::RemoveSoldier(AXBSoldierCharacter* Soldier)
{
    if (!Soldier)
    {
        return;
    }

    int32 OldCount = Soldiers.Num();
    int32 RemovedSlotIndex = Soldier->GetFormationSlotIndex();

    if (!Internal_RemoveSoldierFromArray(Soldier))
    {
        return;
    }

    UpdateSoldierCount(OldCount);

    if (FormationComponent)
    {
        FormationComponent->CompactSlots(Soldiers);
    }
}

void AXBCharacterBase::ReassignSoldierSlots(int32 StartIndex)
{
    for (int32 i = StartIndex; i < Soldiers.Num(); ++i)
    {
        if (Soldiers[i])
        {
            int32 OldSlot = Soldiers[i]->GetFormationSlotIndex();
            if (OldSlot != i)
            {
                Soldiers[i]->SetFormationSlotIndex(i);
            }
        }
    }
}

void AXBCharacterBase::OnSoldierDied(AXBSoldierCharacter* DeadSoldier)
{
    if (!DeadSoldier || bIsCleaningUpSoldiers)
    {
        return;
    }

    RemoveSoldier(DeadSoldier);
    ApplyGrowthOnSoldiersRemoved(1);
}

void AXBCharacterBase::ApplyGrowthOnSoldiersAdded(int32 SoldierCount)
{
    if (bIsDead || SoldierCount <= 0)
    {
        return;
    }

    UpdateLeaderScale();

    const float HealthBonus = SoldierCount * GrowthConfigCache.HealthPerSoldier;
    AddHealthWithOverflow(HealthBonus);

    UpdateDamageMultiplier();

    if (GrowthConfigCache.bEnableSkillEffectScaling)
    {
        UpdateSkillEffectScaling();
    }

    if (GrowthConfigCache.bEnableAttackRangeScaling)
    {
        UpdateAttackRangeScaling();
    }
}

void AXBCharacterBase::UpdateDamageMultiplier()
{
    if (!AbilitySystemComponent)
    {
        return;
    }

    const float BaseDamageMultiplier = CachedLeaderData.DamageMultiplier;
    const float AdditionalMultiplier = Soldiers.Num() * GrowthConfigCache.DamageMultiplierPerSoldier;
    
    const float NewMultiplier = FMath::Min(
        BaseDamageMultiplier + AdditionalMultiplier,
        GrowthConfigCache.MaxDamageMultiplier
    );

    AbilitySystemComponent->SetNumericAttributeBase(
        UXBAttributeSet::GetDamageMultiplierAttribute(),
        NewMultiplier
    );
}

float AXBCharacterBase::GetCurrentDamageMultiplier() const
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->GetNumericAttribute(UXBAttributeSet::GetDamageMultiplierAttribute());
    }
    return CachedLeaderData.DamageMultiplier;
}

void AXBCharacterBase::ApplyGrowthOnSoldiersRemoved(int32 SoldierCount)
{
    if (SoldierCount <= 0)
    {
        return;
    }

    UpdateLeaderScale();
    UpdateDamageMultiplier();

    if (GrowthConfigCache.bEnableSkillEffectScaling)
    {
        UpdateSkillEffectScaling();
    }

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

void AXBCharacterBase::OnCombatAttackStateChanged(bool bIsAttacking)
{
    if (!CombatComponent)
    {
        return;
    }

    bool bShouldBlock = CombatComponent->ShouldBlockMovement();
    
    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        if (bShouldBlock)
        {
            MovementComp->DisableMovement();
        }
        else
        {
            MovementComp->SetMovementMode(MOVE_Walking);
        }
    }
}

void AXBCharacterBase::BindCombatEvents()
{
    if (CombatComponent)
    {
        CombatComponent->OnAttackStateChanged.AddDynamic(this, &AXBCharacterBase::OnCombatAttackStateChanged);
    }
}

void AXBCharacterBase::UpdateLeaderScale()
{
    const float AdditionalScale = Soldiers.Num() * GrowthConfigCache.ScalePerSoldier;
    const float NewScale = FMath::Min(BaseScale + AdditionalScale, GrowthConfigCache.MaxScale);
    // 🔧 修改 - 缩放前记录胶囊高度，保证缩放后脚底贴地
    const float OldHalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.0f;

    SetActorScale3D(FVector(NewScale));

    // 🔧 修改 - 根据高度差调整位置，避免缩放导致角色悬空/穿地
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        const float NewHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
        const float HeightDelta = NewHalfHeight - OldHalfHeight;
        if (!FMath::IsNearlyZero(HeightDelta))
        {
            const FVector AdjustedLocation = GetActorLocation() + FVector(0.0f, 0.0f, HeightDelta);
            SetActorLocation(AdjustedLocation);
        }
    }

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetScaleAttribute(), NewScale);
    }

    if (CombatComponent && GrowthConfigCache.bEnableAttackRangeScaling)
    {
        float RangeScale = NewScale * GrowthConfigCache.AttackRangeScaleMultiplier;
        CombatComponent->SetAttackRangeScale(RangeScale);
    }
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
    }
    else
    {
        AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetHealthAttribute(), NewHealth);
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
        // 🔧 修改 - 战斗中重新触发时保持战斗定时器逻辑
        CancelNoEnemyDisengage();
        bHasEnemiesInCombat = true;
        // 🔧 修改 - 战斗中二次触发时同步士兵状态，避免士兵因超距回队后无法再次入战
        for (AXBSoldierCharacter* Soldier : Soldiers)
        {
            if (Soldier && Soldier->GetSoldierState() != EXBSoldierState::Dead)
            {
                if (Soldier->GetSoldierState() != EXBSoldierState::Combat)
                {
                    Soldier->EnterCombat();
                    UE_LOG(LogXBCombat, Verbose, TEXT("将领 %s 同步士兵 %s 再次进入战斗"),
                        *GetName(), *Soldier->GetName());
                }
            }
        }

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
    bHasEnemiesInCombat = true;

    // 🔧 修改 - 进入战斗时取消无敌人脱战计时
    CancelNoEnemyDisengage();

    if (UWorld* World = GetWorld())
    {
        if (UXBSoldierPerceptionSubsystem* Perception = World->GetSubsystem<UXBSoldierPerceptionSubsystem>())
        {
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
    bHasEnemiesInCombat = false;

    GetWorldTimerManager().ClearTimer(CombatTimeoutHandle);
    CancelNoEnemyDisengage();

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

void AXBCharacterBase::SetHasEnemiesInCombat(bool bInCombat)
{
    bHasEnemiesInCombat = bInCombat;
}

void AXBCharacterBase::DisengageFromCombat()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastDisengageTime < DisengageCooldown)
    {
        return;
    }

    LastDisengageTime = CurrentTime;

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

// 🔧 修改 - 无敌人延迟脱战调度
void AXBCharacterBase::ScheduleNoEnemyDisengage()
{
    if (!bIsInCombat)
    {
        return;
    }

    if (NoEnemyDisengageDelay <= 0.0f)
    {
        ExitCombat();
        return;
    }

    GetWorldTimerManager().ClearTimer(NoEnemyDisengageHandle);
    GetWorldTimerManager().SetTimer(
        NoEnemyDisengageHandle,
        this,
        &AXBCharacterBase::ExitCombat,
        NoEnemyDisengageDelay,
        false
    );
}

void AXBCharacterBase::CancelNoEnemyDisengage()
{
    GetWorldTimerManager().ClearTimer(NoEnemyDisengageHandle);
}

/**
 * @brief ??????????
 * @param HitTarget ?????
 * @return ?
 * @note ????: ????????????/???????????
 * @note ????: ???? -> ???? -> ????/?? -> ???? -> ????????
 * @note ????: ???????????????
 */
void AXBCharacterBase::OnAttackHit(AActor* HitTarget)
{
    // ?? ?? - ???????
    if (!HitTarget)
    {
        return;
    }

    // ?? ?? - ?????????
    EnterCombat();

    // ?? ?? - ?????????
    AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(HitTarget);
    if (TargetLeader && IsHostileTo(TargetLeader))
    {
        // 🔧 修改 - 命中敌方主将时取消脱战计时，保持战斗
        CancelNoEnemyDisengage();
        bHasEnemiesInCombat = true;
        // ?? ?? - ????????????
        LastAttackedEnemyLeader = TargetLeader;
        // ?? ?? - ????????????
        bHasLastAttackedEnemyFaction = true;
        LastAttackedEnemyFaction = TargetLeader->GetFaction();
        // ?? ?? - ??????????
        TargetLeader->EnterCombat();

        UE_LOG(LogXBCombat, Log, TEXT("?? %s ?????? %s??????????????"),
            *GetName(), *TargetLeader->GetName());
        return;
    }

    // ?? ?? - ???????????
    AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(HitTarget);
    if (TargetSoldier && UXBBlueprintFunctionLibrary::AreFactionsHostile(Faction, TargetSoldier->GetFaction()))
    {
        // 🔧 修改 - 命中敌方士兵时取消脱战计时，保持战斗
        CancelNoEnemyDisengage();
        bHasEnemiesInCombat = true;
        // 🔧 修改 - 若命中敌方士兵，优先锁定其所属主将，避免跨主将误选目标
        // 🔧 修改 - 避免与上方 TargetLeader 变量遮蔽
        if (AXBCharacterBase* TargetSoldierLeader = TargetSoldier->GetLeaderCharacter())
        {
            LastAttackedEnemyLeader = TargetSoldierLeader;
        }
        // ?? ?? - ????????????
        bHasLastAttackedEnemyFaction = true;
        LastAttackedEnemyFaction = TargetSoldier->GetFaction();

        UE_LOG(LogXBCombat, Log, TEXT("?? %s ?????? %s?????????????"),
            *GetName(), *TargetSoldier->GetName());
    }
}

void AXBCharacterBase::RecallAllSoldiers()

{
    ExitCombat();

    for (AXBSoldierCharacter* Soldier : Soldiers)
    {
        if (Soldier && Soldier->GetSoldierState() != EXBSoldierState::Dead)
        {
            // 🔧 修改 - 召回时切换为跟随状态并关闭行为树
            Soldier->SetSoldierState(EXBSoldierState::Following);
            Soldier->CurrentAttackTarget = nullptr;

            if (AXBSoldierAIController* SoldierAI = Cast<AXBSoldierAIController>(Soldier->GetController()))
            {
                SoldierAI->StopBehaviorTreeLogic();
                SoldierAI->StopMovement();
            }
            else if (AAIController* AICtrl = Cast<AAIController>(Soldier->GetController()))
            {
                AICtrl->StopMovement();
            }
        }
    }
}

/**
 * @brief  设置草丛隐身状态
 * @param  bHidden 是否隐身
 * @note   详细流程分析: 更新标记 -> 缓存碰撞响应 -> 设置半透明 -> 同步士兵
 *         性能/架构注意事项: 仅在状态变化时执行，避免频繁材质更新
 */
void AXBCharacterBase::SetHiddenInBush(bool bEnableHidden)
{
    if (bIsHiddenInBush == bEnableHidden)
    {
        return;
    }

    bIsHiddenInBush = bEnableHidden;

    // 🔧 修改 - 设置覆层材质（草丛半透明效果）
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        if (!CachedOverlayMaterial)
        {
            CachedOverlayMaterial = MeshComp->GetOverlayMaterial();
        }

        if (bEnableHidden)
        {
            if (BushOverlayMaterial)
            {
                MeshComp->SetOverlayMaterial(BushOverlayMaterial);
            }
        }
        else
        {
            // 🔧 修改 - 离开草丛时清理覆层材质
            MeshComp->SetOverlayMaterial(nullptr);
            CachedOverlayMaterial = nullptr;
        }
    }

    // 🔧 修改 - 关闭与敌人的碰撞（简化为忽略Leader/Soldier通道）
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        if (!bCachedBushCollisionResponse)
        {
            CachedLeaderCollisionResponse = Capsule->GetCollisionResponseToChannel(XBCollision::Leader);
            CachedSoldierCollisionResponse = Capsule->GetCollisionResponseToChannel(XBCollision::Soldier);
            bCachedBushCollisionResponse = true;
        }

        Capsule->SetCollisionResponseToChannel(XBCollision::Leader,
            bEnableHidden ? ECR_Ignore : CachedLeaderCollisionResponse.GetValue());
        Capsule->SetCollisionResponseToChannel(XBCollision::Soldier,
            bEnableHidden ? ECR_Ignore : CachedSoldierCollisionResponse.GetValue());
    }

    // 🔧 修改 - 同步所有士兵隐身状态（即便士兵在草丛外）
    for (AXBSoldierCharacter* Soldier : Soldiers)
    {
        if (Soldier && Soldier->GetSoldierState() != EXBSoldierState::Dead)
        {
            // 🔧 修改 - 草丛隐身时强制士兵脱离战斗并回归跟随
            if (bEnableHidden && Soldier->GetSoldierState() == EXBSoldierState::Combat)
            {
                Soldier->ExitCombat();
                Soldier->ReturnToFormation();
            }
            Soldier->SetHiddenInBush(bEnableHidden);
        }
    }

    UE_LOG(LogXBCharacter, Log, TEXT("主将 %s 草丛隐身状态=%s"),
        *GetName(), bEnableHidden ? TEXT("开启") : TEXT("关闭"));
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
    }

    if (HealthBarComponent)
    {
        HealthBarComponent->SetHealthBarVisible(false);
        HealthBarComponent->SetComponentTickEnabled(false);
    }

    OnCharacterDeath.Broadcast(this);

    // ✨ 核心 - 先生成掉落士兵（使用击杀者配置，落地自动入列）
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

    // 🔧 修改 - 保持死亡时当前缩放，避免死亡瞬间体型变化

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->CancelAllAbilities();
    }

    ExitCombat();

    if (bIsSprinting)
    {
        StopSprint();
    }

    KillAllSoldiers();

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

/**
 * @brief 生成掉落的士兵（抛物线飞行 + 落地自动入列）
 * @note 🔧 重构 - 核心修改点:
 *       1. 使用 FullInitialize 完整初始化士兵（数据+组件+视觉）
 *       2. 传入目标将领，落地后自动入列
 *       3. 设置士兵阵营与目标将领一致
 */
void AXBCharacterBase::SpawnDroppedSoldiers()
{
    if (SoldierDropConfig.DropCount <= 0)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // 获取击杀者作为目标将领
    AXBCharacterBase* TargetLeader = nullptr;
    UDataTable* DropSoldierDataTable = nullptr;
    FName DropSoldierRowName = NAME_None;
    TSubclassOf<AXBSoldierCharacter> DropSoldierClass = SoldierDropConfig.DropSoldierClass;
    EXBFaction DropFaction = EXBFaction::Neutral;

    if (LastDamageInstigator.IsValid())
    {
        // 🔧 修改 - 击杀者可能是士兵或主将，统一映射到对应主将
        if (AXBCharacterBase* InstigatorLeader = Cast<AXBCharacterBase>(LastDamageInstigator.Get()))
        {
            TargetLeader = InstigatorLeader;
        }
        else if (AXBSoldierCharacter* InstigatorSoldier = Cast<AXBSoldierCharacter>(LastDamageInstigator.Get()))
        {
            TargetLeader = InstigatorSoldier->GetLeaderCharacter();
            UE_LOG(LogXBCharacter, Log, TEXT("掉落士兵：击杀者为士兵 %s，归属主将=%s"),
                *InstigatorSoldier->GetName(),
                TargetLeader ? *TargetLeader->GetName() : TEXT("无"));
        }
        
        if (TargetLeader && !TargetLeader->IsDead())
        {
            DropSoldierDataTable = TargetLeader->GetSoldierDataTable();
            DropSoldierRowName = TargetLeader->GetRecruitSoldierRowName();
            DropFaction = TargetLeader->GetFaction();
            
            if (TargetLeader->GetSoldierActorClass())
            {
                DropSoldierClass = TargetLeader->GetSoldierActorClass();
            }
            
            UE_LOG(LogXBCharacter, Log, TEXT("掉落士兵将自动入列到击杀者 %s，行名: %s"), 
                *TargetLeader->GetName(), *DropSoldierRowName.ToString());
        }
        else
        {
            TargetLeader = nullptr;
        }
    }

    // 回退逻辑
    if (!DropSoldierDataTable || DropSoldierRowName.IsNone())
    {
        DropSoldierDataTable = SoldierDataTable;
        DropSoldierRowName = RecruitSoldierRowName;
        DropFaction = EXBFaction::Neutral;
    }

    if (!DropSoldierClass)
    {
        DropSoldierClass = SoldierActorClass;
    }

    if (!DropSoldierClass)
    {
        UE_LOG(LogXBCharacter, Error, TEXT("掉落士兵失败: 未配置士兵类"));
        return;
    }

    FVector SpawnOrigin = GetActorLocation();
    // 🔧 修改 - 若有击杀者，强制落地自动入列
    FXBDropArcConfig ArcConfig = SoldierDropConfig.ArcConfig;
    if (TargetLeader)
    {
        ArcConfig.bAutoRecruitOnLanding = true;
    }

    UXBSoldierPoolSubsystem* PoolSubsystem = World->GetSubsystem<UXBSoldierPoolSubsystem>();

    // 🔧 修改 - 使用士兵胶囊半高参与落点校正，避免悬空
    float DropCapsuleHalfHeight = 88.0f;
    if (DropSoldierClass)
    {
        const AXBSoldierCharacter* SoldierCDO = DropSoldierClass->GetDefaultObject<AXBSoldierCharacter>();
        if (SoldierCDO && SoldierCDO->GetCapsuleComponent())
        {
            DropCapsuleHalfHeight = SoldierCDO->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        }
    }

    UE_LOG(LogXBCharacter, Log, TEXT("将领 %s 死亡，生成 %d 个掉落士兵，目标将领: %s"),
        *GetName(), 
        SoldierDropConfig.DropCount,
        TargetLeader ? *TargetLeader->GetName() : TEXT("无"));

    for (int32 i = 0; i < SoldierDropConfig.DropCount; ++i)
    {
        // 计算抛物线终点位置
        float BaseAngle = (360.0f / SoldierDropConfig.DropCount) * i;
        float RandomAngleOffset = FMath::RandRange(-20.0f, 20.0f);
        float Angle = BaseAngle + RandomAngleOffset;

        float Distance = FMath::RandRange(ArcConfig.MinDropDistance, ArcConfig.MaxDropDistance);

        FVector Direction = FRotator(0.0f, Angle, 0.0f).RotateVector(FVector::ForwardVector);
        FVector TargetLocation = SpawnOrigin + Direction * Distance;

        // 地面检测
        FHitResult HitResult;
        FVector TraceStart = FVector(TargetLocation.X, TargetLocation.Y, SpawnOrigin.Z + ArcConfig.GroundTraceUpDistance);
        FVector TraceEnd = FVector(TargetLocation.X, TargetLocation.Y, SpawnOrigin.Z - ArcConfig.GroundTraceDownDistance);

        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this);

        bool bHit = World->LineTraceSingleByChannel(
            HitResult,
            TraceStart,
            TraceEnd,
            ECC_WorldStatic,
            QueryParams
        );

        if (bHit)
        {
            // ✨ 新增 - 落地位置加上半高，确保胶囊体底部触地
            TargetLocation = HitResult.Location + FVector(0.0f, 0.0f, DropCapsuleHalfHeight + ArcConfig.LandingExtraZOffset);
        }
        else
        {
            TargetLocation.Z = SpawnOrigin.Z + ArcConfig.LandingExtraZOffset;
        }

        AXBSoldierCharacter* DroppedSoldier = nullptr;

        // 尝试从对象池获取
        if (PoolSubsystem && PoolSubsystem->HasAvailableSoldier())
        {
            DroppedSoldier = PoolSubsystem->AcquireSoldier(SpawnOrigin, FRotator::ZeroRotator);
        }

        // 池中没有则生成新的
        if (!DroppedSoldier)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            // ✨ 新增 - 延迟 BeginPlay，避免在配置完成前触发
            SpawnParams.bDeferConstruction = true;

            DroppedSoldier = World->SpawnActor<AXBSoldierCharacter>(
                DropSoldierClass,
                SpawnOrigin,
                FRotator::ZeroRotator,
                SpawnParams
            );
            
            if (DroppedSoldier)
            {
                DroppedSoldier->MarkAsPooledSoldier();
                
                // ✨ 新增 - 在 BeginPlay 前禁用碰撞，避免触发磁场
                if (UCapsuleComponent* Capsule = DroppedSoldier->GetCapsuleComponent())
                {
                    Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                }
                
                // 完成构造（触发 BeginPlay）
                DroppedSoldier->FinishSpawning(FTransform(FRotator::ZeroRotator, SpawnOrigin));
            }
        }

        if (DroppedSoldier)
        {
            // ✨ 新增 - 确保碰撞禁用（对象池获取的士兵也需要）
            if (UCapsuleComponent* Capsule = DroppedSoldier->GetCapsuleComponent())
            {
                Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            }
            
            // 完整初始化（不会触发磁场，因为碰撞已禁用）
            if (DropSoldierDataTable && !DropSoldierRowName.IsNone())
            {
                DroppedSoldier->FullInitialize(DropSoldierDataTable, DropSoldierRowName, DropFaction);
            }
            
            // 启动抛物线飞行
            DroppedSoldier->StartDropFlight(SpawnOrigin, TargetLocation, ArcConfig, TargetLeader);
            
            UE_LOG(LogXBCharacter, Log, TEXT("掉落士兵 [%d] %s 开始飞行"),
                i, *DroppedSoldier->GetName());
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

void AXBCharacterBase::KillAllSoldiers()
{
    bIsCleaningUpSoldiers = true;

    UE_LOG(LogXBSoldier, Log, TEXT("将领 %s 死亡，开始处理 %d 个士兵的死亡"), 
        *GetName(), Soldiers.Num());

    for (AXBSoldierCharacter* Soldier : Soldiers)
    {
        if (Soldier && IsValid(Soldier))
        {
            if (Soldier->GetSoldierState() != EXBSoldierState::Dead)
            {
                Soldier->TakeSoldierDamage(Soldier->GetCurrentHealth() + 100.0f, this);
            }
        }
    }

    Soldiers.Empty();

    bIsCleaningUpSoldiers = false;
}

void AXBCharacterBase::PreDestroyCleanup()
{
    GetWorldTimerManager().ClearTimer(CombatTimeoutHandle);

    Soldiers.Empty();

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->CancelAllAbilities();
        AbilitySystemComponent->RemoveAllGameplayCues();
        AbilitySystemComponent->RemoveActiveEffectsWithTags(FGameplayTagContainer());
    }

    GetWorldTimerManager().ClearTimer(DeathDestroyTimerHandle);
}
