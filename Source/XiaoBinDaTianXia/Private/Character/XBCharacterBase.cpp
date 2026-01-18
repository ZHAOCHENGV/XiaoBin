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
#include "Soldier/XBSoldierCharacter.h"
#include "Particles/ParticleSystemComponent.h"
#include "Soldier/Component/XBSoldierPoolSubsystem.h"
#include "AI/XBSoldierAIController.h"
#include "Kismet/GameplayStatics.h"
#include "Game/XBGameInstance.h"
#include "Character/XBPlayerCharacter.h"

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

/**
 * @brief  角色初始化入口
 * @return 无
 * @note   详细流程分析: 注册感知 -> 初始化组件 -> 初始化主将数据 -> 进入运行逻辑
 *         性能注意: 初始化仅在 BeginPlay 执行，避免运行期重复调用
 */
void AXBCharacterBase::BeginPlay()
{
    Super::BeginPlay();

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

    // 🔧 修改 - 统一初始化主将数据，子类可重写扩展
    InitializeLeaderData();
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
    // 🔧 修改 - 调用父类 EndPlay
    Super::EndPlay(EndPlayReason);
}

void AXBCharacterBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateSprint(DeltaTime);
    SmoothLeaderScale(DeltaTime);
}

/**
 * @brief  初始化主将数据
 * @return 无
 * @note   详细流程分析: 读取外部配置 -> 同步基础参数 -> 根据配置行名初始化数据表
 *         性能注意: 仅在 BeginPlay 阶段调用一次
 */
void AXBCharacterBase::InitializeLeaderData()
{
    FXBGameConfigData ExternalConfig;
    const bool bHasExternalConfig = GetExternalInitConfig(ExternalConfig);

    if (bHasExternalConfig)
    {
        // 🔧 修改 - 外部配置优先覆盖主将行名
        if (!ExternalConfig.LeaderConfigRowName.IsNone())
        {
            ConfigRowName = ExternalConfig.LeaderConfigRowName;
        }
    }

    // 🔧 修改 - 若配置行名有效，先从数据表初始化基础数据
    if (ConfigDataTable && !ConfigRowName.IsNone())
    {
        InitializeFromDataTable(ConfigDataTable, ConfigRowName);
    }

    // 🔧 修改 - 主将数据完成初始化后，刷新已招募士兵的跟随/编队状态
    RefreshRecruitedSoldiersAfterLeaderInit();
}

/**
 * @brief  获取外部初始化配置
 * @param  OutConfig 输出配置
 * @return 是否存在外部配置
 * @note   详细流程分析: 基类默认无外部配置
 */
bool AXBCharacterBase::GetExternalInitConfig(FXBGameConfigData& OutConfig) const
{
    return false;
}

/**
 * @brief  刷新已招募士兵的跟随状态
 * @return 无
 * @note   详细流程分析: 
 *         1) 保证编队槽位数量覆盖当前已招募士兵
 *         2) 校正士兵槽位索引
 *         3) 重新触发士兵跟随逻辑，避免初始化顺序导致的“已招募但不跟随”
 *         性能/架构注意事项: 仅在主将数据初始化完成时触发，避免重复刷新
 */
void AXBCharacterBase::RefreshRecruitedSoldiersAfterLeaderInit()
{
    if (Soldiers.Num() == 0)
    {
        return;
    }

    UE_LOG(LogXBCharacter, Log, TEXT("主将 %s 数据初始化完成，刷新已招募士兵跟随状态，数量: %d"),
        *GetName(), Soldiers.Num());

    // 🔧 修改 - 先确保编队槽位覆盖当前士兵数量，避免槽位缺失导致目标位置无效
    if (FormationComponent && FormationComponent->GetFormationSlots().Num() < Soldiers.Num())
    {
        FormationComponent->RegenerateFormation(Soldiers.Num());
    }

    for (int32 Index = 0; Index < Soldiers.Num(); ++Index)
    {
        AXBSoldierCharacter* Soldier = Soldiers[Index];
        if (!Soldier || !IsValid(Soldier))
        {
            continue;
        }

        // 🔧 修改 - 只处理已招募且归属于当前主将的士兵
        if (!Soldier->IsRecruited() || Soldier->GetLeaderCharacter() != this)
        {
            continue;
        }

        // 🔧 修改 - 校正槽位索引，保证编队位置计算一致
        if (Soldier->GetFormationSlotIndex() != Index)
        {
            Soldier->SetFormationSlotIndex(Index);
        }

        // 🔧 修改 - 使用公开入口刷新跟随状态，避免访问受保护成员
        Soldier->RefreshFollowingAfterLeaderInit(this, Soldier->GetFormationSlotIndex());
    }
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

    // 🔧 修改 - 生命值需要同时应用基础值与倍率，确保初始化即反映配置
    const float EffectiveMaxHealth = FMath::Max(0.01f, CachedLeaderData.MaxHealth * CachedLeaderData.HealthMultiplier);
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetMaxHealthAttribute(), EffectiveMaxHealth);
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetHealthAttribute(), EffectiveMaxHealth);
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetHealthMultiplierAttribute(), CachedLeaderData.HealthMultiplier);
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetDamageMultiplierAttribute(), CachedLeaderData.DamageMultiplier);
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetMoveSpeedAttribute(), CachedLeaderData.MoveSpeed);
    AbilitySystemComponent->SetNumericAttributeBase(UXBAttributeSet::GetScaleAttribute(), CachedLeaderData.Scale);
}

void AXBCharacterBase::ApplyRuntimeConfig(const FXBGameConfigData& GameConfig, bool bApplyInitialSoldiers)
{
    // ==================== 主将配置覆盖 ====================
    // 🔧 修改 - 主将名称/倍率仅在初始阶段写入，运行时不再覆盖

    if (GameConfig.LeaderMoveSpeed > 0.0f)
    {
        CachedLeaderData.MoveSpeed = GameConfig.LeaderMoveSpeed;
    }

    // 🔧 修改 - 冲刺倍率由配置直接覆盖
    SprintSpeedMultiplier = GameConfig.LeaderSprintSpeedMultiplier;

    // ✨ 新增 - 冲刺持续时间由配置直接覆盖
    SprintDuration = GameConfig.LeaderSprintDuration;

    // 🔧 修改 - 提升伤害倍率上限，确保高倍率配置不会被上限截断
    GrowthConfigCache.MaxDamageMultiplier = FMath::Max(
        GrowthConfigCache.MaxDamageMultiplier,
        GameConfig.LeaderDamageMultiplier
    );

    // 🔧 修改 - 掉落数量由配置覆盖
    SoldierDropConfig.DropCount = GameConfig.LeaderDeathDropCount;

    // ==================== 招募/成长配置 ====================
    if (!GameConfig.InitialSoldierRowName.IsNone())
    {
        RecruitSoldierRowName = GameConfig.InitialSoldierRowName;
    }

    GrowthConfigCache.ScalePerSoldier = GameConfig.SoldierScalePerRecruit;
    GrowthConfigCache.HealthPerSoldier = GameConfig.SoldierHealthPerRecruit;

    // ==================== 磁场配置 ====================
    if (MagnetFieldComponent)
    {
        MagnetFieldComponent->SetFieldRadius(GameConfig.MagnetFieldRadius);
    }

    // ==================== 属性刷新 ====================
    ApplyInitialAttributes();

    // 🔧 修改 - 同步移动速度到移动组件
    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->MaxWalkSpeed = CachedLeaderData.MoveSpeed;
        BaseMoveSpeed = CachedLeaderData.MoveSpeed;
        TargetMoveSpeed = BaseMoveSpeed;
    }

    // 🔧 修改 - 根据新配置刷新成长效果
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

    // ==================== 初始士兵 ====================
    if (bApplyInitialSoldiers)
    {
        SpawnInitialSoldiers(GameConfig.InitialSoldierCount);
    }
}

/**
 * @brief  生成初始士兵
 * @param  DesiredCount 期望生成数量
 * @return 无
 * @note   详细流程分析: 计算缺失数量 -> 预生成编队槽位 -> 按槽位位置生成士兵 -> 写入队列槽位并完成招募
 */
void AXBCharacterBase::SpawnInitialSoldiers(int32 DesiredCount)
{
    if (DesiredCount <= 0)
    {
        return;
    }

    const int32 MissingCount = FMath::Max(0, DesiredCount - Soldiers.Num());
    if (MissingCount <= 0)
    {
        return;
    }

    if (!SoldierDataTable || RecruitSoldierRowName.IsNone())
    {
        UE_LOG(LogXBCharacter, Warning, TEXT("初始士兵生成失败：未配置士兵数据表或行名"));
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UXBSoldierPoolSubsystem* PoolSubsystem = World->GetSubsystem<UXBSoldierPoolSubsystem>();
    const FVector LeaderLocation = GetActorLocation();

    // 🔧 修改 - 预先生成编队槽位，保证初始士兵直接落位到队列插槽
    if (FormationComponent)
    {
        FormationComponent->RegenerateFormation(DesiredCount);
    }

    // 🔧 修改 - 缓存初始数量，保证生成与分配槽位保持一致
    const int32 BaseSoldierCount = Soldiers.Num();

    for (int32 i = 0; i < MissingCount; ++i)
    {
        const int32 SlotIndex = BaseSoldierCount + i;
        FVector SpawnLocation = LeaderLocation;

        if (FormationComponent)
        {
            // 🔧 修改 - 直接使用编队槽位位置，确保初始士兵在队列插槽中生成
            SpawnLocation = FormationComponent->GetSlotWorldPosition(SlotIndex);
        }
        else
        {
            // 🔧 修改 - 无编队组件时使用环形分布作为兜底，避免重叠
            const float Angle = (360.0f / MissingCount) * i;
            const float Distance = 150.0f;
            const FVector Offset = FVector(
                FMath::Cos(FMath::DegreesToRadians(Angle)) * Distance,
                FMath::Sin(FMath::DegreesToRadians(Angle)) * Distance,
                0.0f
            );
            SpawnLocation = LeaderLocation + Offset;
        }

        AXBSoldierCharacter* Soldier = nullptr;

        if (PoolSubsystem)
        {
            Soldier = PoolSubsystem->AcquireSoldier(SpawnLocation, FRotator::ZeroRotator);
        }

        if (!Soldier)
        {
            // 🔧 修改 - 使用显式分支避免 TSubclassOf 与 UClass* 的三元表达式歧义
            TSubclassOf<AXBSoldierCharacter> SpawnClass = SoldierActorClass;
            if (!SpawnClass)
            {
                SpawnClass = AXBSoldierCharacter::StaticClass();
            }

            Soldier = World->SpawnActor<AXBSoldierCharacter>(SpawnClass, SpawnLocation, FRotator::ZeroRotator);
        }

        if (!Soldier)
        {
            UE_LOG(LogXBCharacter, Warning, TEXT("初始士兵生成失败：SpawnActor 为空"));
            continue;
        }

        // 🔧 修改 - 使用完整初始化确保数据/组件一致
        Soldier->FullInitialize(SoldierDataTable, RecruitSoldierRowName, Faction);

        // 🔧 修改 - 按顺序分配槽位并进入跟随
        Soldier->OnRecruited(this, SlotIndex);
        AddSoldier(Soldier);
    }
}

// ==================== 冲刺系统实现 ====================

void AXBCharacterBase::TriggerSprint()
{
    // 🔧 修改 - 冲刺中或死亡时禁止重复触发，避免无意义计时器
    if (bIsDead || bIsSprinting)
    {
        return;
    }

    // 🔧 修改 - 无移动输入时启用自动前进，满足静止触发冲刺也能移动
    bAutoSprintMove = GetLastMovementInputVector().IsNearlyZero() && GetVelocity().IsNearlyZero();

    // 🔧 修改 - 先启动冲刺，再按配置持续时间安排结束
    StartSprint();

    if (SprintDuration > 0.0f)
    {
        GetWorldTimerManager().ClearTimer(SprintDurationTimerHandle);
        GetWorldTimerManager().SetTimer(
            SprintDurationTimerHandle,
            this,
            &AXBCharacterBase::StopSprint,
            SprintDuration,
            false
        );
    }
    else
    {
        // 🔧 修改 - 配置为 0 时视为不启用持续冲刺，立即恢复
        StopSprint();
    }
}

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

    // 🔧 修改 - 停止冲刺时清理按键冲刺计时器
    GetWorldTimerManager().ClearTimer(SprintDurationTimerHandle);

    bIsSprinting = false;
    TargetMoveSpeed = BaseMoveSpeed;

    // 🔧 修改 - 冲刺结束时关闭自动前进开关
    bAutoSprintMove = false;

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

    // 🔧 修改 - 玩家开始输入时关闭自动前进，避免覆盖玩家方向
    if (bIsSprinting && bAutoSprintMove && !GetLastMovementInputVector().IsNearlyZero())
    {
        bAutoSprintMove = false;
    }

    // 🔧 修改 - 静止触发冲刺时持续给前进输入，保证冲刺期间保持移动
    if (bIsSprinting && bAutoSprintMove && CMC->MovementMode == MOVE_Walking)
    {
        const FVector ForwardDirection = GetActorForwardVector();
        AddMovementInput(ForwardDirection, 1.0f);
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
        // 🔧 修改 - 仅在槽位不足时扩容，避免缩小导致后续初始士兵落位失败
        if (FormationComponent->GetFormationSlots().Num() < Soldiers.Num())
        {
            FormationComponent->RegenerateFormation(Soldiers.Num());
        }
        
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
    
    // 🔧 修改 - 取消伤害倍率上限，确保士兵加成全部生效
    const float NewMultiplier = BaseDamageMultiplier + AdditionalMultiplier;

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

    // 🔧 修改 - 冲刺中被攻击/技能打断移动时，立即退出冲刺
    if (bShouldBlock && bIsSprinting)
    {
        StopSprint();
    }
    
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

    // 🔧 修改 - 设置目标缩放，由 Tick 平滑过渡
    TargetLeaderScale = NewScale;
    bHasTargetLeaderScale = true;
}

void AXBCharacterBase::SmoothLeaderScale(float DeltaTime)
{
    if (!bHasTargetLeaderScale)
    {
        return;
    }

    const float CurrentScale = GetActorScale3D().X;
    const float InterpSpeed = FMath::Max(0.0f, LeaderScaleInterpSpeed);
    const float NewScale = InterpSpeed > 0.0f
        ? FMath::FInterpTo(CurrentScale, TargetLeaderScale, DeltaTime, InterpSpeed)
        : TargetLeaderScale;

    ApplyLeaderScale(NewScale);

    if (FMath::IsNearlyEqual(NewScale, TargetLeaderScale, 0.001f))
    {
        bHasTargetLeaderScale = false;
    }
}

void AXBCharacterBase::ApplyLeaderScale(float NewScale)
{
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
        const float RangeScale = NewScale * GrowthConfigCache.AttackRangeScaleMultiplier;
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

/**
 * @brief  主将开始攻击时通知士兵进入战斗
 * @param  无
 * @return 无
 * @note   详细流程分析: 校验死亡状态 -> 触发进入战斗 -> 取消无敌人脱战计时
 *         性能/架构注意事项: 复用 EnterCombat 统一管理战斗计时与士兵状态，避免重复逻辑
 */
void AXBCharacterBase::NotifyAttackStarted()
{
    // 🔧 修改 - 死亡状态下不触发战斗逻辑，避免无效状态切换
    if (bIsDead)
    {
        return;
    }

    // 🔧 修改 - 进入战斗由统一入口处理，确保士兵同步与计时器复用
    EnterCombat();

    // 🔧 修改 - 主将主动攻击时取消无敌人脱战计时，避免刚出手就退出战斗
    CancelNoEnemyDisengage();

    UE_LOG(LogXBCombat, Verbose, TEXT("主将 %s 开始攻击，已触发战斗状态"), *GetName());
}

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

  
        return;
    }

    bIsInCombat = true;
    bHasEnemiesInCombat = true;

    // 🔧 修改 - 进入战斗时取消无敌人脱战计时
    CancelNoEnemyDisengage();

    for (AXBSoldierCharacter* Soldier : Soldiers)
    {
        if (Soldier && Soldier->GetSoldierState() != EXBSoldierState::Dead)
        {
            Soldier->EnterCombat();
        }
    }


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
 * @brief  处理受到伤害的回调（基类默认空实现）
 * @param  DamageSource 伤害来源
 * @param  DamageAmount 伤害数值
 * @return 无
 * @note   详细流程分析: 基类保持空实现 -> 子类按需扩展
 *         性能/架构注意事项: 避免在基类中绑定具体AI逻辑
 */
void AXBCharacterBase::HandleDamageReceived(AActor* DamageSource, float DamageAmount)
{
    // 🔧 修改 - 基类不处理，避免影响所有主将逻辑
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
        // 🔧 修改 - 草丛隐身目标不可被命中
        if (TargetLeader->IsHiddenInBush())
        {
            return;
        }
        // 🔧 修改 - 命中敌方主将时取消脱战计时，保持战斗
        CancelNoEnemyDisengage();
        bHasEnemiesInCombat = true;
        // ?? ?? - ????????????
        LastAttackedEnemyLeader = TargetLeader;
        // ?? ?? - ????????????
        bHasLastAttackedEnemyFaction = true;
        LastAttackedEnemyFaction = TargetLeader->GetFaction();
        // 🔧 修改 - 敌方主将被命中不自动进入战斗，避免其士兵被动参战

        UE_LOG(LogXBCombat, Log, TEXT("?? %s ?????? %s??????????????"),
            *GetName(), *TargetLeader->GetName());
        return;
    }

    // ?? ?? - ???????????
    AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(HitTarget);
    if (TargetSoldier && UXBBlueprintFunctionLibrary::AreFactionsHostile(Faction, TargetSoldier->GetFaction()))
    {
        // 🔧 修改 - 草丛隐身目标不可被命中
        if (TargetSoldier->IsHiddenInBush())
        {
            return;
        }
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

    // 🔧 修改 - 设置覆层材质（草丛隐身效果）
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
            // 🔧 修改 - 草丛中对非友军不可见，仅对本地玩家做可见性过滤
            bool bShouldHideForLocal = false;
            if (APawn* LocalPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
            {
                if (const AXBCharacterBase* LocalLeader = Cast<AXBCharacterBase>(LocalPawn))
                {
                    bShouldHideForLocal = (LocalLeader->GetFaction() != Faction);
                }
            }
            MeshComp->SetVisibility(!bShouldHideForLocal, true);
            if (HealthBarComponent)
            {
                HealthBarComponent->SetHealthBarVisible(!bShouldHideForLocal);
            }
        }
        else
        {
            // 🔧 修改 - 离开草丛时清理覆层材质
            MeshComp->SetOverlayMaterial(nullptr);
            CachedOverlayMaterial = nullptr;
            MeshComp->SetVisibility(true, true);
            if (HealthBarComponent)
            {
                HealthBarComponent->SetHealthBarVisible(true);
            }
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
        // ✨ 新增 - 死亡时禁用旋转跟随移动方向，防止尸体继续转向
        MovementComp->bOrientRotationToMovement = false;
    }

    // ✨ 新增 - 清除 AI 控制器焦点，防止死亡后继续因 SetFocus 而转向
    if (AAIController* AIController = Cast<AAIController>(GetController()))
    {
        AIController->ClearFocus(EAIFocusPriority::Gameplay);
        AIController->StopMovement();
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

    // ✨ 修改 - 根据配置决定是否杀死麾下士兵
    // 若不杀死，解除士兵与主将的绑定关系并允许其被其他主将招募
    if (bKillSoldiersOnDeath)
    {
        KillAllSoldiers();
    }
    else
    {
        // 解除士兵绑定但不杀死
        bIsCleaningUpSoldiers = true;
        UE_LOG(LogXBCharacter, Log, TEXT("将领 %s 死亡，释放 %d 个士兵（不杀死）"), 
            *GetName(), Soldiers.Num());
        for (AXBSoldierCharacter* Soldier : Soldiers)
        {
            if (Soldier && IsValid(Soldier) && Soldier->GetSoldierState() != EXBSoldierState::Dead)
            {
                // 解除主将绑定
                Soldier->SetLeaderCharacter(nullptr);
                // 退出战斗状态
                Soldier->ExitCombat();
                // 设置为休眠态，可被其他主将招募
                Soldier->EnterDormantState(EXBDormantType::Sleeping);
            }
        }
        Soldiers.Empty();
        bIsCleaningUpSoldiers = false;
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
                //Soldier->TakeSoldierDamage(Soldier->GetCurrentHealth() + 100.0f, this);
                Soldier->HandleDeath();
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
