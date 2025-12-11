/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Character/XBCharacterBase.cpp

/**
 * @file XBCharacterBase.cpp
 * @brief 角色基类实现
 * 
 * @note 🔧 修改记录:
 *       1. 将 MagnetFieldComponent 和 FormationComponent 从 PlayerCharacter 移入
 *       2. 添加共用的冲刺系统
 *       3. 添加磁场回调
 */

#include "Character/XBCharacterBase.h"
#include "Character/Components/XBCombatComponent.h"
#include "Character/Components/XBMagnetFieldComponent.h"
#include "Character/Components/XBFormationComponent.h"
#include "UI/XBWorldHealthBarComponent.h"
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

    // 创建 ASC
    AbilitySystemComponent = CreateDefaultSubobject<UXBAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

    // 创建属性集
    AttributeSet = CreateDefaultSubobject<UXBAttributeSet>(TEXT("AttributeSet"));

    // 创建战斗组件
    CombatComponent = CreateDefaultSubobject<UXBCombatComponent>(TEXT("CombatComponent"));

    // 创建头顶血条组件
    HealthBarComponent = CreateDefaultSubobject<UXBWorldHealthBarComponent>(TEXT("HealthBarComponent"));
    HealthBarComponent->SetupAttachment(RootComponent);

    // ✨ 新增 - 创建磁场组件
    MagnetFieldComponent = CreateDefaultSubobject<UXBMagnetFieldComponent>(TEXT("MagnetFieldComponent"));
    MagnetFieldComponent->SetupAttachment(RootComponent);

    // ✨ 新增 - 创建编队组件
    FormationComponent = CreateDefaultSubobject<UXBFormationComponent>(TEXT("FormationComponent"));

    // 禁用控制器旋转（角色朝向由移动方向决定）
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

    // ✨ 新增 - 绑定磁场事件
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
        UE_LOG(LogTemp, Warning, TEXT("%s: 未配置数据表或行名，跳过数据表初始化"), *GetName());
    }
}

void AXBCharacterBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 更新冲刺状态
    UpdateSprint(DeltaTime);
}

// ✨ 新增 - 配置移动组件
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

// ✨ 新增 - 磁场回调（虚函数，子类可重写）
void AXBCharacterBase::OnMagnetFieldActorEntered(AActor* EnteredActor)
{
    if (!EnteredActor)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("%s: Actor 进入磁场: %s"), *GetName(), *EnteredActor->GetName());

    // 基类的默认招募逻辑已在 MagnetFieldComponent 中实现
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
        UE_LOG(LogTemp, Log, TEXT("%s: ASC 初始化完成"), *GetName());
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
        UE_LOG(LogTemp, Error, TEXT("%s: InitializeFromDataTable - 找不到行 '%s'"), *GetName(), *RowName.ToString());
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

    UE_LOG(LogTemp, Log, TEXT("%s: 从数据表加载配置成功"), *GetName());
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
    // ✨ 新增 - 死亡后不能冲刺
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

    UE_LOG(LogTemp, Log, TEXT("%s: 开始冲刺，目标速度: %.1f"), *GetName(), TargetMoveSpeed);
}

void AXBCharacterBase::StopSprint()
{
    if (!bIsSprinting)
    {
        return;
    }

    bIsSprinting = false;
    TargetMoveSpeed = BaseMoveSpeed;

    // 通知士兵恢复正常速度
    SetSoldiersEscaping(false);

    // 广播事件
    OnSprintStateChanged.Broadcast(false);

    UE_LOG(LogTemp, Log, TEXT("%s: 停止冲刺，目标速度: %.1f"), *GetName(), TargetMoveSpeed);
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

// ==================== 士兵管理实现 ====================

void AXBCharacterBase::AddSoldier(AXBSoldierActor* Soldier)
{
    // ✨ 新增 - 死亡后不能添加士兵
    if (bIsDead)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 角色已死亡，无法添加士兵"), *GetName());
        return;
    }

    if (!Soldier)
    {
        return;
    }

    if (!Soldiers.Contains(Soldier))
    {
        int32 OldCount = Soldiers.Num();
        
        int32 SlotIndex = Soldiers.Num();
        Soldier->SetFormationSlotIndex(SlotIndex);
        Soldier->SetFollowTarget(this, SlotIndex);
        Soldier->InitializeSoldier(Soldier->GetSoldierConfig(), Faction);
        
        Soldiers.Add(Soldier);
        
        OnSoldiersAdded(1);
        OnSoldierCountChanged.Broadcast(OldCount, Soldiers.Num());
        
        if (FormationComponent)
        {
            FormationComponent->RegenerateFormation(Soldiers.Num());
        }
        
        UE_LOG(LogTemp, Log, TEXT("%s: 添加士兵 %s，槽位: %d，当前数量: %d"), 
            *GetName(), *Soldier->GetName(), SlotIndex, Soldiers.Num());
    }
}

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

    Soldiers.RemoveAt(RemovedIndex);
    ReassignSoldierSlots(RemovedIndex);

    // 更新编队
    if (FormationComponent)
    {
        FormationComponent->RegenerateFormation(Soldiers.Num());
    }

    OnSoldierCountChanged.Broadcast(OldCount, Soldiers.Num());
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

void AXBCharacterBase::OnSoldierDied(AXBSoldierActor* DeadSoldier)
{
    if (!DeadSoldier)
    {
        return;
    }

    RemoveSoldier(DeadSoldier);
    CurrentSoldierCount = FMath::Max(0, CurrentSoldierCount - 1);
    UpdateLeaderScale();
}

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
}

void AXBCharacterBase::OnSoldiersAdded(int32 SoldierCount)
{
    // ✨ 新增 - 死亡后不处理成长
    if (bIsDead)
    {
        return;
    }

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
        
        float NewHealth = CurrentHealth + HealthBonus;
        
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
}

// ==================== 战斗状态系统实现 ====================

void AXBCharacterBase::EnterCombat()
{
    // ✨ 新增 - 死亡后不能进入战斗
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

    for (AXBSoldierActor* Soldier : Soldiers)
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

    for (AXBSoldierActor* Soldier : Soldiers)
    {
        if (Soldier && Soldier->GetSoldierState() != EXBSoldierState::Dead)
        {
            Soldier->ExitCombat();
        }
    }

    OnCombatStateChanged.Broadcast(false);
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
    for (AXBSoldierActor* Soldier : Soldiers)
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

    UE_LOG(LogTemp, Log, TEXT("%s: 角色死亡"), *GetName());

    // ✨ 新增 - 禁用磁场组件，阻止招募新士兵
    if (MagnetFieldComponent)
    {
        MagnetFieldComponent->SetFieldEnabled(false);
        UE_LOG(LogTemp, Log, TEXT("%s: 磁场组件已禁用"), *GetName());
    }

    // ✨ 新增 - 隐藏血条
    if (HealthBarComponent)
    {
        HealthBarComponent->SetHealthBarVisible(false);
        HealthBarComponent->SetComponentTickEnabled(false);
        UE_LOG(LogTemp, Log, TEXT("%s: 血条已隐藏"), *GetName());
    }

    // 广播死亡事件
    OnCharacterDeath.Broadcast(this);

    // 生成掉落的士兵
    SpawnDroppedSoldiers();

    // 禁用移动
    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->DisableMovement();
        MovementComp->StopMovementImmediately();
    }

    // 禁用碰撞
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // 停止所有技能
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->CancelAllAbilities();
    }

    // 退出战斗状态
    ExitCombat();

    // 停止冲刺
    if (bIsSprinting)
    {
        StopSprint();
    }

    // 播放死亡蒙太奇
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
        
        AXBSoldierActor* DroppedSoldier = World->SpawnActor<AXBSoldierActor>(
            SoldierDropConfig.DropSoldierClass,
            TargetLocation,
            FRotator::ZeroRotator,
            SpawnParams
        );
        
        if (DroppedSoldier)
        {
            DroppedSoldier->InitializeSoldier(DroppedSoldier->GetSoldierConfig(), EXBFaction::Neutral);
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

void AXBCharacterBase::PreDestroyCleanup()
{
    GetWorldTimerManager().ClearTimer(CombatTimeoutHandle);

    for (AXBSoldierActor* Soldier : Soldiers)
    {
        if (Soldier)
        {
            Soldier->SetSoldierState(EXBSoldierState::Dead);
            Soldier->SetLifeSpan(2.0f);
        }
    }
    Soldiers.Empty();

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->CancelAllAbilities();
        AbilitySystemComponent->RemoveAllGameplayCues();
        AbilitySystemComponent->RemoveActiveEffectsWithTags(FGameplayTagContainer());
    }

    GetWorldTimerManager().ClearTimer(DeathDestroyTimerHandle);
}
