/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Soldier/XBSoldierCharacter.cpp

/**
 * @file XBSoldierCharacter.cpp
 * @brief 士兵Actor实现 - 统一角色系统
 * 
 * @note 🔧 修改记录:
 *       1. ✨ 新增 休眠态系统实现
 *       2. ✨ 新增 组件启用/禁用管理
 *       3. ✨ 新增 Zzz 特效系统
 *       4. 🔧 修改 CanBeRecruited 支持休眠态检查
 *       5. ✨ 新增 掉落抛物线飞行系统（支持落地自动入列）
 *       6. ✨ 新增 FullInitialize 完整初始化方法
 */

#include "Soldier/XBSoldierCharacter.h"
#include "Utils/XBLogCategories.h"
#include "Utils/XBBlueprintFunctionLibrary.h"
#include "Data/XBSoldierDataAccessor.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Soldier/Component/XBSoldierFollowComponent.h"
#include "Soldier/Component/XBSoldierDebugComponent.h"
#include "Soldier/Component/XBSoldierBehaviorInterface.h"
#include "Soldier/Component/XBSoldierPoolSubsystem.h"
#include "Character/XBCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "AI/XBSoldierAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/DataTable.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"
#include "XBCollisionChannels.h"

AXBSoldierCharacter::AXBSoldierCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    // ==================== 碰撞配置 ====================
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->InitCapsuleSize(34.0f, 88.0f);
        Capsule->SetCollisionObjectType(XBCollision::Soldier);
        Capsule->SetCollisionResponseToChannel(XBCollision::Leader, ECR_Overlap);
        Capsule->SetCollisionResponseToChannel(XBCollision::Soldier, ECR_Overlap);
    }

    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
        MeshComp->SetCollisionResponseToChannel(XBCollision::Soldier, ECR_Ignore);
        MeshComp->SetCollisionResponseToChannel(XBCollision::Leader, ECR_Ignore);
    }

    // ==================== 创建组件 ====================
    DataAccessor = CreateDefaultSubobject<UXBSoldierDataAccessor>(TEXT("DataAccessor"));
    FollowComponent = CreateDefaultSubobject<UXBSoldierFollowComponent>(TEXT("FollowComponent"));
    DebugComponent = CreateDefaultSubobject<UXBSoldierDebugComponent>(TEXT("DebugComponent"));
    BehaviorInterface = CreateDefaultSubobject<UXBSoldierBehaviorInterface>(TEXT("BehaviorInterface"));
    
    ZzzEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ZzzEffectComponent"));
    ZzzEffectComponent->SetupAttachment(RootComponent);
    ZzzEffectComponent->SetAutoActivate(false);
    
    // ==================== 移动组件配置 ====================
    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->bOrientRotationToMovement = true;
        MovementComp->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
        MovementComp->MaxWalkSpeed = 400.0f;
        MovementComp->BrakingDecelerationWalking = 2000.0f;
        MovementComp->SetComponentTickEnabled(false);
    }

    AutoPossessAI = EAutoPossessAI::Disabled;
    AIControllerClass = nullptr;
}

void AXBSoldierCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    
    bComponentsInitialized = true;
    
    UCapsuleComponent* Capsule = GetCapsuleComponent();
    if (Capsule)
    {
        FTransform CapsuleTransform = Capsule->GetComponentTransform();
        FVector Scale = CapsuleTransform.GetScale3D();
        
        if (Scale.IsNearlyZero() || Scale.ContainsNaN())
        {
            UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: Capsule Scale 无效，修正为 (1,1,1)"), *GetName());
            Capsule->SetWorldScale3D(FVector::OneVector);
        }
    }
    
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    if (MoveComp && !MoveComp->UpdatedComponent)
    {
        MoveComp->SetUpdatedComponent(Capsule);
    }
    
    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: PostInitializeComponents 完成"), *GetName());
}

void AXBSoldierCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (!ZzzEffectAsset.IsNull() && ZzzEffectComponent)
    {
        if (UNiagaraSystem* LoadedEffect = ZzzEffectAsset.LoadSynchronous())
        {
            ZzzEffectComponent->SetAsset(LoadedEffect);
        }
    }

    if (ZzzEffectComponent)
    {
        ZzzEffectComponent->SetRelativeLocation(DormantConfig.ZzzEffectOffset);
    }

    LoadDormantAnimations();

    if (IsDataAccessorValid())
    {
        CurrentHealth = DataAccessor->GetMaxHealth();
    }
    else
    {
        CurrentHealth = 100.0f;
    }

    if (bStartAsDormant)
    {
        Faction = EXBFaction::Neutral;
        EnterDormantState(DormantConfig.DormantType);
        
        UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: 初始化为休眠态，阵营: 中立"), *GetName());
    }
    else
    {
        GetWorldTimerManager().SetTimerForNextTick([this]()
        {
            EnableMovementAndTick();
        });
    }

    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s BeginPlay - 阵营: %d, 状态: %d, 休眠: %s"), 
        *GetName(), 
        static_cast<int32>(Faction), 
        static_cast<int32>(CurrentState),
        bStartAsDormant ? TEXT("是") : TEXT("否"));
}

void AXBSoldierCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 更新掉落飞行
    if (CurrentState == EXBSoldierState::Dropping)
    {
        UpdateDropFlight(DeltaTime);
    }
}

void AXBSoldierCharacter::EnableMovementAndTick()
{
    if (!IsValid(this) || IsPendingKillPending())
    {
        return;
    }
    
    // 休眠态和掉落态不启用移动
    if (CurrentState == EXBSoldierState::Dormant || CurrentState == EXBSoldierState::Dropping)
    {
        return;
    }
    
    UCapsuleComponent* Capsule = GetCapsuleComponent();
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    
    if (!Capsule || !MoveComp)
    {
        UE_LOG(LogXBSoldier, Error, TEXT("士兵 %s: 组件无效，无法启用移动"), *GetName());
        return;
    }
    
    FTransform CapsuleTransform = Capsule->GetComponentTransform();
    if (!CapsuleTransform.IsValid())
    {
        UE_LOG(LogXBSoldier, Error, TEXT("士兵 %s: Capsule Transform 无效"), *GetName());
        return;
    }
    
    MoveComp->SetComponentTickEnabled(true);
    SetActorTickEnabled(true);
    
    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: 移动组件和Tick已启用"), *GetName());
}

// ==================== ✨ 新增：完整初始化方法 ====================

/**
 * @brief 完整初始化士兵（数据 + 组件 + 视觉）
 * @param DataTable 数据表
 * @param RowName 行名
 * @param InFaction 阵营
 * @note 用于掉落士兵，在生成时立即完成所有初始化
 *       与 InitializeFromDataTable 的区别：
 *       1. 立即启用所有组件
 *       2. 立即启用 Tick
 *       3. 不进入休眠态
 */
void AXBSoldierCharacter::FullInitialize(UDataTable* DataTable, FName RowName, EXBFaction InFaction)
{
    // 1. 基础数据初始化
    InitializeFromDataTable(DataTable, RowName, InFaction);
    
    // 2. 确保组件初始化标记
    bComponentsInitialized = true;
    
    // 3. 启用 Tick（掉落状态需要）
    SetActorTickEnabled(true);
    
    // 4. 设置阵营
    Faction = InFaction;
    
    // 5. 重置状态标记
    bIsRecruited = false;
    bIsDead = false;
    bIsEscaping = false;
    
    // 6. 显示角色
    SetActorHiddenInGame(false);
    
    // 7. 启用碰撞（Query模式，用于被检测但不阻挡）
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }
    
    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: 完整初始化完成，阵营: %d"), 
        *GetName(), static_cast<int32>(InFaction));
}

// ==================== 掉落抛物线飞行系统 ====================

/**
 * @brief 开始掉落抛物线飞行
 * @param StartLocation 起始位置
 * @param TargetLocation 目标落地位置
 * @param ArcConfig 抛物线配置
 * @param TargetLeader 落地后要加入的将领
 * @note 🔧 修改 - 新增 TargetLeader 参数，支持落地自动入列
 */
void AXBSoldierCharacter::StartDropFlight(const FVector& StartLocation, const FVector& TargetLocation, 
    const FXBDropArcConfig& ArcConfig, AXBCharacterBase* TargetLeader)
{
    // 保存飞行参数
    DropStartLocation = StartLocation;
    DropTargetLocation = TargetLocation;
    DropFlightDuration = ArcConfig.FlightDuration;
    DropArcHeight = ArcConfig.ArcHeight;
    bPlayDropLandingEffect = ArcConfig.bPlayLandingEffect;
    
    // ✨ 新增 - 保存目标将领和自动入列配置
    DropTargetLeader = TargetLeader;
    bAutoRecruitOnLanding = ArcConfig.bAutoRecruitOnLanding;
    
    // 保存落地特效资源
    if (!ArcConfig.LandingEffect.IsNull())
    {
        DropLandingEffectAsset = ArcConfig.LandingEffect;
    }
    
    // 重置计时器
    DropElapsedTime = 0.0f;
    
    // 设置初始位置
    SetActorLocation(StartLocation);
    
    // 进入掉落状态
    SetSoldierState(EXBSoldierState::Dropping);
    
    // 确保 Tick 启用
    SetActorTickEnabled(true);
    
    // 禁用碰撞（飞行期间）
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    
    // 禁用移动组件（使用手动位置更新）
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->SetMovementMode(MOVE_Flying);
        MoveComp->StopMovementImmediately();
        MoveComp->SetComponentTickEnabled(false);
    }
    
    // 隐藏 Zzz 特效
    SetZzzEffectEnabled(false);
    
    // 显示角色
    SetActorHiddenInGame(false);
    
    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 开始掉落飞行: (%.0f, %.0f, %.0f) -> (%.0f, %.0f, %.0f), 目标将领: %s, 自动入列: %s"),
        *GetName(),
        StartLocation.X, StartLocation.Y, StartLocation.Z,
        TargetLocation.X, TargetLocation.Y, TargetLocation.Z,
        TargetLeader ? *TargetLeader->GetName() : TEXT("无"),
        bAutoRecruitOnLanding ? TEXT("是") : TEXT("否"));
}

float AXBSoldierCharacter::GetDropProgress() const
{
    if (DropFlightDuration <= KINDA_SMALL_NUMBER)
    {
        return 1.0f;
    }
    return FMath::Clamp(DropElapsedTime / DropFlightDuration, 0.0f, 1.0f);
}

void AXBSoldierCharacter::UpdateDropFlight(float DeltaTime)
{
    // 更新计时器
    DropElapsedTime += DeltaTime;
    
    // 计算进度
    float Progress = GetDropProgress();
    
    // 计算当前位置
    FVector CurrentPosition = CalculateArcPosition(Progress);
    SetActorLocation(CurrentPosition);
    
    // 计算面向方向（朝向目标）
    FVector Direction = (DropTargetLocation - DropStartLocation).GetSafeNormal2D();
    if (!Direction.IsNearlyZero())
    {
        SetActorRotation(Direction.Rotation());
    }
    
    // 检查是否完成
    if (Progress >= 1.0f)
    {
        OnDropLanded();
    }
}

FVector AXBSoldierCharacter::CalculateArcPosition(float Progress) const
{
    // XY 线性插值
    float X = FMath::Lerp(DropStartLocation.X, DropTargetLocation.X, Progress);
    float Y = FMath::Lerp(DropStartLocation.Y, DropTargetLocation.Y, Progress);
    
    // Z 抛物线计算
    float LinearZ = FMath::Lerp(DropStartLocation.Z, DropTargetLocation.Z, Progress);
    float ArcOffset = DropArcHeight * 4.0f * Progress * (1.0f - Progress);
    float Z = LinearZ + ArcOffset;
    
    return FVector(X, Y, Z);
}

/**
 * @brief 处理落地
 * @note 🔧 修改 - 落地后根据配置决定是自动入列还是进入休眠态
 */
void AXBSoldierCharacter::OnDropLanded()
{
    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 掉落落地"), *GetName());
    
    // 确保位置精确
    SetActorLocation(DropTargetLocation);
    
    // 恢复碰撞
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
    
    // 恢复移动组件
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->SetMovementMode(MOVE_Walking);
        MoveComp->SetComponentTickEnabled(true);
    }
    
    // 播放落地特效
    if (bPlayDropLandingEffect)
    {
        PlayLandingEffect();
    }
    
    // ✨ 核心逻辑 - 根据配置决定落地后的行为
    if (bAutoRecruitOnLanding && DropTargetLeader.IsValid())
    {
        // 自动入列到目标将领
        AutoRecruitToLeader();
    }
    else
    {
        // 没有目标将领，进入待机态（可被其他将领招募）
        SetSoldierState(EXBSoldierState::Idle);
        UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 落地后进入待机态，等待招募"), *GetName());
    }
    
    // 广播落地完成事件
    OnDropLandingComplete.Broadcast(this);
}

/**
 * @brief 落地后自动加入将领队伍
 * @note 🔧 修复版本 - 
 *       1. 正确的调用顺序：先标记招募 → 后添加到将领
 *       2. 手动启用所有必要组件
 *       3. 正确启动跟随过渡
 */
void AXBSoldierCharacter::AutoRecruitToLeader()
{
    AXBCharacterBase* Leader = DropTargetLeader.Get();
    if (!Leader || !IsValid(Leader))
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: 自动入列失败 - 目标将领无效"), *GetName());
        Faction = EXBFaction::Neutral;
        bIsRecruited = false;
        SetSoldierState(EXBSoldierState::Idle);
        return;
    }
    
    if (Leader->IsDead())
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: 自动入列失败 - 目标将领已死亡"), *GetName());
        Faction = EXBFaction::Neutral;
        bIsRecruited = false;
        SetSoldierState(EXBSoldierState::Idle);
        return;
    }
    
    // 防止重复入列
    if (bIsRecruited && FollowTarget.IsValid())
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: 已入列，跳过"), *GetName());
        return;
    }
    
    UE_LOG(LogXBSoldier, Log, TEXT(""));
    UE_LOG(LogXBSoldier, Log, TEXT("========================================"));
    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 开始自动入列"), *GetName());
    UE_LOG(LogXBSoldier, Log, TEXT("目标将领: %s"), *Leader->GetName());
    UE_LOG(LogXBSoldier, Log, TEXT("将领当前士兵数: %d"), Leader->GetSoldierCount());
    UE_LOG(LogXBSoldier, Log, TEXT("========================================"));
    
    // ==================== Step 1: 设置状态标记 ====================
    bIsRecruited = true;
    FollowTarget = Leader;
    Faction = Leader->GetFaction();
    
    // ==================== Step 2: 启用所有组件 ====================
    
    SetActorTickEnabled(true);
    
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
    
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->SetComponentTickEnabled(true);
        MoveComp->SetMovementMode(MOVE_Walking);
        MoveComp->MaxWalkSpeed = GetMoveSpeed();
        UE_LOG(LogXBSoldier, Log, TEXT("移动组件启用，速度: %.1f"), MoveComp->MaxWalkSpeed);
    }
    
    if (FollowComponent)
    {
        FollowComponent->SetComponentTickEnabled(true);
    }
    
    if (BehaviorInterface)
    {
        BehaviorInterface->SetComponentTickEnabled(true);
    }
    
    // ==================== Step 3: 添加到将领（自动分配槽位） ====================
    
    // AddSoldier 会：
    // - 将士兵添加到 Soldiers 数组
    // - 分配 FormationSlotIndex = Soldiers.Num() - 1
    // - 调用 SetFollowTarget (会更新 FollowComponent)
    // - 触发成长效果
    Leader->AddSoldier(this);
    
    int32 SlotIndex = FormationSlotIndex;
    UE_LOG(LogXBSoldier, Log, TEXT("已添加到将领，槽位: %d"), SlotIndex);
    
    // ==================== Step 4: 确保跟随组件正确配置 ====================
    
    if (FollowComponent)
    {
        // 确认跟随目标和槽位（可能已被 AddSoldier->SetFollowTarget 设置）
        if (FollowComponent->GetFollowTarget() != Leader)
        {
            FollowComponent->SetFollowTarget(Leader);
        }
        
        if (FollowComponent->GetFormationSlotIndex() != SlotIndex)
        {
            FollowComponent->SetFormationSlotIndex(SlotIndex);
        }
        
        // 同步将领冲刺状态
        FollowComponent->SyncLeaderSprintState(Leader->IsSprinting(), Leader->GetCurrentMoveSpeed());
        
        // ✨ 核心 - 开始招募过渡，士兵会移动到槽位位置
        FollowComponent->StartRecruitTransition();
        
        FVector TargetPos = FollowComponent->GetTargetPosition();
        UE_LOG(LogXBSoldier, Log, TEXT("开始移动到槽位位置: (%.1f, %.1f, %.1f)"), 
            TargetPos.X, TargetPos.Y, TargetPos.Z);
    }
    
    // ==================== Step 5: 设置状态并启动 AI ====================
    
    SetSoldierState(EXBSoldierState::Following);
    
    GetWorldTimerManager().SetTimer(
        DelayedAIStartTimerHandle,
        this,
        &AXBSoldierCharacter::SpawnAndPossessAIController,
        0.3f,
        false
    );
    
    OnSoldierRecruited.Broadcast(this, Leader);
    
    UE_LOG(LogXBSoldier, Log, TEXT("========================================"));
    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 自动入列完成，槽位: %d"), *GetName(), SlotIndex);
    UE_LOG(LogXBSoldier, Log, TEXT("将领最终士兵数: %d"), Leader->GetSoldierCount());
    UE_LOG(LogXBSoldier, Log, TEXT("========================================"));
    UE_LOG(LogXBSoldier, Log, TEXT(""));
}

void AXBSoldierCharacter::PlayLandingEffect()
{
    if (DropLandingEffectAsset.IsNull())
    {
        return;
    }
    
    UNiagaraSystem* EffectSystem = DropLandingEffectAsset.LoadSynchronous();
    if (!EffectSystem)
    {
        return;
    }
    
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    
    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        World,
        EffectSystem,
        DropTargetLocation,
        FRotator::ZeroRotator,
        FVector::OneVector,
        true,
        true,
        ENCPoolMethod::AutoRelease
    );
    
    UE_LOG(LogXBSoldier, Verbose, TEXT("士兵 %s 播放落地特效"), *GetName());
}

// ==================== 休眠系统实现 ====================

void AXBSoldierCharacter::EnterDormantState(EXBDormantType DormantType)
{
    if (CurrentState == EXBSoldierState::Dormant)
    {
        SetDormantType(DormantType);
        return;
    }

    EXBSoldierState OldState = CurrentState;
    CurrentState = EXBSoldierState::Dormant;
    CurrentDormantType = DormantType;

    bIsRecruited = false;
    bIsDead = false;
    Faction = EXBFaction::Neutral;

    DisableActiveComponents();

    if (DormantType != EXBDormantType::Hidden)
    {
        SetActorHiddenInGame(false);
        UpdateDormantAnimation();
        UpdateZzzEffect();
    }
    else
    {
        SetActorHiddenInGame(true);
        SetZzzEffectEnabled(false);
    }

    OnSoldierStateChanged.Broadcast(OldState, CurrentState);
    OnDormantStateChanged.Broadcast(this, true);

    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 进入休眠态，类型: %d"), 
        *GetName(), static_cast<int32>(DormantType));
}

void AXBSoldierCharacter::ExitDormantState()
{
    if (CurrentState != EXBSoldierState::Dormant)
    {
        return;
    }

    EXBSoldierState OldState = CurrentState;
    CurrentState = EXBSoldierState::Idle;

    SetZzzEffectEnabled(false);
    
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->Stop();
    }

    SetActorHiddenInGame(false);

    EnableActiveComponents();

    OnSoldierStateChanged.Broadcast(OldState, CurrentState);
    OnDormantStateChanged.Broadcast(this, false);

    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 退出休眠态"), *GetName());
}

void AXBSoldierCharacter::SetDormantVisualConfig(const FXBDormantVisualConfig& NewConfig)
{
    DormantConfig = NewConfig;

    if (ZzzEffectComponent)
    {
        ZzzEffectComponent->SetRelativeLocation(DormantConfig.ZzzEffectOffset);
    }

    if (CurrentState == EXBSoldierState::Dormant)
    {
        UpdateDormantAnimation();
        UpdateZzzEffect();
    }
}

void AXBSoldierCharacter::SetZzzEffectEnabled(bool bEnabled)
{
    if (!ZzzEffectComponent)
    {
        return;
    }

    if (bEnabled)
    {
        ZzzEffectComponent->Activate(true);
    }
    else
    {
        ZzzEffectComponent->Deactivate();
    }
}

void AXBSoldierCharacter::SetDormantType(EXBDormantType NewType)
{
    if (CurrentDormantType == NewType)
    {
        return;
    }

    CurrentDormantType = NewType;

    if (CurrentState == EXBSoldierState::Dormant)
    {
        if (NewType == EXBDormantType::Hidden)
        {
            SetActorHiddenInGame(true);
            SetZzzEffectEnabled(false);
        }
        else
        {
            SetActorHiddenInGame(false);
            UpdateDormantAnimation();
            UpdateZzzEffect();
        }
    }

    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 休眠类型切换为: %d"), 
        *GetName(), static_cast<int32>(NewType));
}

void AXBSoldierCharacter::EnableActiveComponents()
{
    SetActorTickEnabled(true);

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->SetComponentTickEnabled(true);
        MoveComp->SetMovementMode(MOVE_Walking);
    }

    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }

    if (FollowComponent)
    {
        FollowComponent->SetComponentTickEnabled(true);
    }

    if (BehaviorInterface)
    {
        BehaviorInterface->SetComponentTickEnabled(true);
    }

    if (DebugComponent)
    {
        DebugComponent->SetComponentTickEnabled(true);
    }

    UE_LOG(LogXBSoldier, Verbose, TEXT("士兵 %s: 激活态组件已启用"), *GetName());
}

void AXBSoldierCharacter::DisableActiveComponents()
{
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->StopMovementImmediately();
        MoveComp->SetComponentTickEnabled(false);
    }

    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }

    if (FollowComponent)
    {
        FollowComponent->SetFollowTarget(nullptr);
        FollowComponent->SetComponentTickEnabled(false);
    }

    if (BehaviorInterface)
    {
        BehaviorInterface->SetComponentTickEnabled(false);
    }

    if (AController* CurrentController = GetController())
    {
        CurrentController->UnPossess();
    }

    FollowTarget = nullptr;
    FormationSlotIndex = INDEX_NONE;
    CurrentAttackTarget = nullptr;

    UE_LOG(LogXBSoldier, Verbose, TEXT("士兵 %s: 激活态组件已禁用"), *GetName());
}

void AXBSoldierCharacter::UpdateDormantAnimation()
{
    UAnimSequence* AnimToPlay = nullptr;

    switch (CurrentDormantType)
    {
    case EXBDormantType::Sleeping:
        AnimToPlay = LoadedSleepingAnimation;
        break;
        
    case EXBDormantType::Standing:
        AnimToPlay = LoadedStandingAnimation;
        break;
        
    case EXBDormantType::Hidden:
        return;
    }

    PlayAnimationSequence(AnimToPlay, true);
}

void AXBSoldierCharacter::UpdateZzzEffect()
{
    bool bShouldShowZzz = (CurrentDormantType == EXBDormantType::Sleeping) && 
                          DormantConfig.bShowZzzEffect;
    
    SetZzzEffectEnabled(bShouldShowZzz);
}

void AXBSoldierCharacter::PlayAnimationSequence(UAnimSequence* Animation, bool bLoop)
{
    USkeletalMeshComponent* MeshComp = GetMesh();
    if (!MeshComp)
    {
        return;
    }

    if (Animation)
    {
        MeshComp->PlayAnimation(Animation, bLoop);
    }
    else
    {
        MeshComp->Stop();
    }
}

void AXBSoldierCharacter::LoadDormantAnimations()
{
    if (!DormantConfig.SleepingAnimation.IsNull())
    {
        LoadedSleepingAnimation = DormantConfig.SleepingAnimation.LoadSynchronous();
    }
    
    if (!DormantConfig.StandingAnimation.IsNull())
    {
        LoadedStandingAnimation = DormantConfig.StandingAnimation.LoadSynchronous();
    }
}

// ==================== 数据访问器接口 ====================

bool AXBSoldierCharacter::IsDataAccessorValid() const
{
    return DataAccessor && DataAccessor->IsInitialized();
}

void AXBSoldierCharacter::InitializeFromDataTable(UDataTable* DataTable, FName RowName, EXBFaction InFaction)
{
    if (!DataTable)
    {
        UE_LOG(LogXBSoldier, Error, TEXT("士兵初始化失败: 数据表为空"));
        return;
    }

    if (RowName.IsNone())
    {
        UE_LOG(LogXBSoldier, Error, TEXT("士兵初始化失败: 行名为空"));
        return;
    }

    if (!DataAccessor)
    {
        UE_LOG(LogXBSoldier, Error, TEXT("士兵初始化失败: DataAccessor 组件未创建"));
        return;
    }

    bool bInitSuccess = DataAccessor->Initialize(
        DataTable, 
        RowName, 
        EXBResourceLoadStrategy::Synchronous
    );

    if (!bInitSuccess)
    {
        UE_LOG(LogXBSoldier, Error, TEXT("士兵初始化失败: DataAccessor 初始化失败"));
        return;
    }

    SoldierType = DataAccessor->GetSoldierType();
    Faction = InFaction;
    CurrentHealth = DataAccessor->GetMaxHealth();

    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->MaxWalkSpeed = DataAccessor->GetMoveSpeed();
        MovementComp->RotationRate = FRotator(0.0f, DataAccessor->GetRotationSpeed(), 0.0f);
    }

    if (FollowComponent)
    {
        FollowComponent->SetFollowSpeed(DataAccessor->GetMoveSpeed());
    }

    BehaviorTreeAsset = DataAccessor->GetBehaviorTree();
    ApplyVisualConfig();

    UE_LOG(LogXBSoldier, Log, TEXT("士兵初始化成功: %s (类型=%s, 血量=%.1f)"), 
        *RowName.ToString(),
        *UEnum::GetValueAsString(SoldierType),
        CurrentHealth);
}

void AXBSoldierCharacter::ApplyVisualConfig()
{
    if (!IsDataAccessorValid())
    {
        return;
    }

    USkeletalMesh* SoldierMesh = DataAccessor->GetSkeletalMesh();
    if (SoldierMesh)
    {
        GetMesh()->SetSkeletalMesh(SoldierMesh);
    }

    TSubclassOf<UAnimInstance> AnimClass = DataAccessor->GetAnimClass();
    if (AnimClass)
    {
        GetMesh()->SetAnimInstanceClass(AnimClass);
    }

    float MeshScale = DataAccessor->GetRawData().VisualConfig.MeshScale;
    if (!FMath::IsNearlyEqual(MeshScale, 1.0f))
    {
        SetActorScale3D(FVector(MeshScale));
    }
}

// ==================== 配置属性访问 ====================

FText AXBSoldierCharacter::GetDisplayName() const
{
    return IsDataAccessorValid() ? DataAccessor->GetDisplayName() : FText::FromString(TEXT("未命名士兵"));
}

FGameplayTagContainer AXBSoldierCharacter::GetSoldierTags() const
{
    return IsDataAccessorValid() ? DataAccessor->GetSoldierTags() : FGameplayTagContainer();
}

float AXBSoldierCharacter::GetMaxHealth() const
{
    return IsDataAccessorValid() ? DataAccessor->GetMaxHealth() : 100.0f;
}

float AXBSoldierCharacter::GetBaseDamage() const
{
    return IsDataAccessorValid() ? DataAccessor->GetBaseDamage() : 10.0f;
}

float AXBSoldierCharacter::GetAttackRange() const
{
    return IsDataAccessorValid() ? DataAccessor->GetAttackRange() : 150.0f;
}

float AXBSoldierCharacter::GetAttackInterval() const
{
    return IsDataAccessorValid() ? DataAccessor->GetAttackInterval() : 1.0f;
}

float AXBSoldierCharacter::GetMoveSpeed() const
{
    return IsDataAccessorValid() ? DataAccessor->GetMoveSpeed() : 400.0f;
}

float AXBSoldierCharacter::GetSprintSpeedMultiplier() const
{
    return IsDataAccessorValid() ? DataAccessor->GetSprintSpeedMultiplier() : 2.0f;
}

float AXBSoldierCharacter::GetFollowInterpSpeed() const
{
    return IsDataAccessorValid() ? DataAccessor->GetFollowInterpSpeed() : 5.0f;
}

float AXBSoldierCharacter::GetRotationSpeed() const
{
    return IsDataAccessorValid() ? DataAccessor->GetRotationSpeed() : 360.0f;
}

float AXBSoldierCharacter::GetVisionRange() const
{
    return IsDataAccessorValid() ? DataAccessor->GetVisionRange() : 800.0f;
}

float AXBSoldierCharacter::GetDisengageDistance() const
{
    return IsDataAccessorValid() ? DataAccessor->GetDisengageDistance() : 1000.0f;
}

float AXBSoldierCharacter::GetReturnDelay() const
{
    return IsDataAccessorValid() ? DataAccessor->GetReturnDelay() : 2.0f;
}

float AXBSoldierCharacter::GetArrivalThreshold() const
{
    return IsDataAccessorValid() ? DataAccessor->GetArrivalThreshold() : 50.0f;
}

float AXBSoldierCharacter::GetAvoidanceRadius() const
{
    return IsDataAccessorValid() ? DataAccessor->GetAvoidanceRadius() : 50.0f;
}

float AXBSoldierCharacter::GetAvoidanceWeight() const
{
    return IsDataAccessorValid() ? DataAccessor->GetAvoidanceWeight() : 0.3f;
}

// ==================== 招募系统 ====================

bool AXBSoldierCharacter::CanBeRecruited() const
{
    if (bIsRecruited)
    {
        return false;
    }
    
    if (FollowTarget.IsValid())
    {
        return false;
    }
    
    if (Faction != EXBFaction::Neutral)
    {
        return false;
    }
    
    // 掉落中不可招募
    if (CurrentState == EXBSoldierState::Dropping)
    {
        return false;
    }
    
    // 必须处于休眠态或待机态
    if (CurrentState != EXBSoldierState::Dormant && CurrentState != EXBSoldierState::Idle)
    {
        return false;
    }
    
    if (bIsDead || CurrentHealth <= 0.0f)
    {
        return false;
    }
    
    if (!bComponentsInitialized)
    {
        return false;
    }
    
    return true;
}

void AXBSoldierCharacter::OnRecruited(AActor* NewLeader, int32 SlotIndex)
{
    if (!NewLeader)
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: 招募失败 - 将领为空"), *GetName());
        return;
    }
    
    if (bIsRecruited)
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: 已被招募，忽略来自 %s 的重复招募请求"), 
            *GetName(), *NewLeader->GetName());
        return;
    }

    if (FollowTarget.IsValid() && FollowTarget.Get() != NewLeader)
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: 已跟随 %s，拒绝 %s 的招募"), 
            *GetName(), *FollowTarget->GetName(), *NewLeader->GetName());
        return;
    }
    
    if (!bComponentsInitialized)
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: 组件未初始化，延迟招募"), *GetName());
        FTimerHandle TempHandle;
        GetWorldTimerManager().SetTimer(TempHandle, [this, NewLeader, SlotIndex]()
        {
            OnRecruited(NewLeader, SlotIndex);
        }, 0.1f, false);
        return;
    }
    
    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: 被将领 %s 招募，槽位: %d，当前状态: %d"), 
        *GetName(), *NewLeader->GetName(), SlotIndex, static_cast<int32>(CurrentState));
    
    bIsRecruited = true;
    FollowTarget = NewLeader;
    FormationSlotIndex = SlotIndex;
    
    if (CurrentState == EXBSoldierState::Dormant)
    {
        ExitDormantState();
    }
    
    if (AXBCharacterBase* LeaderChar = Cast<AXBCharacterBase>(NewLeader))
    {
        Faction = LeaderChar->GetFaction();
    }
    
    FVector DirectionToLeader = (NewLeader->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    if (!DirectionToLeader.IsNearlyZero())
    {
        SetActorRotation(DirectionToLeader.Rotation());
    }
    
    if (FollowComponent)
    {
        FollowComponent->SetFollowTarget(NewLeader);
        FollowComponent->SetFormationSlotIndex(SlotIndex);
        
        if (AXBCharacterBase* LeaderChar = Cast<AXBCharacterBase>(NewLeader))
        {
            bool bLeaderSprinting = LeaderChar->IsSprinting();
            float LeaderSpeed = LeaderChar->GetCurrentMoveSpeed();
            FollowComponent->SyncLeaderSprintState(bLeaderSprinting, LeaderSpeed);
        }
        
        FollowComponent->StartRecruitTransition();
    }
    
    SetSoldierState(EXBSoldierState::Following);
    
    GetWorldTimerManager().SetTimer(
        DelayedAIStartTimerHandle,
        this,
        &AXBSoldierCharacter::SpawnAndPossessAIController,
        0.3f,
        false
    );
    
    OnSoldierRecruited.Broadcast(this, NewLeader);
}

// ==================== 跟随系统 ====================

void AXBSoldierCharacter::SetFollowTarget(AActor* NewLeader, int32 SlotIndex)
{
    FollowTarget = NewLeader;
    FormationSlotIndex = SlotIndex;

    if (FollowComponent)
    {
        FollowComponent->SetFollowTarget(NewLeader);
        FollowComponent->SetFormationSlotIndex(SlotIndex);
    }

    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BBComp = AICtrl->GetBlackboardComponent())
        {
            BBComp->SetValueAsObject(TEXT("Leader"), NewLeader);
            BBComp->SetValueAsInt(TEXT("FormationSlot"), SlotIndex);
        }
    }

    if (NewLeader)
    {
        SetSoldierState(EXBSoldierState::Following);
    }
    else
    {
        SetSoldierState(EXBSoldierState::Idle);
    }
}

AXBCharacterBase* AXBSoldierCharacter::GetLeaderCharacter() const
{
    return Cast<AXBCharacterBase>(FollowTarget.Get());
}

void AXBSoldierCharacter::SetFormationSlotIndex(int32 NewIndex)
{
    FormationSlotIndex = NewIndex;

    if (FollowComponent)
    {
        FollowComponent->SetFormationSlotIndex(NewIndex);
    }

    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BBComp = AICtrl->GetBlackboardComponent())
        {
            BBComp->SetValueAsInt(TEXT("FormationSlot"), NewIndex);
        }
    }
}

// ==================== 状态管理 ====================

void AXBSoldierCharacter::SetSoldierState(EXBSoldierState NewState)
{
    if (CurrentState == NewState)
    {
        return;
    }

    EXBSoldierState OldState = CurrentState;
    CurrentState = NewState;

    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BBComp = AICtrl->GetBlackboardComponent())
        {
            BBComp->SetValueAsInt(TEXT("SoldierState"), static_cast<int32>(NewState));
        }
    }

    OnSoldierStateChanged.Broadcast(OldState, NewState);

    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 状态变化: %d -> %d"), 
        *GetName(), static_cast<int32>(OldState), static_cast<int32>(NewState));
}

// ==================== 战斗系统 ====================

void AXBSoldierCharacter::EnterCombat()
{
    if (CurrentState == EXBSoldierState::Dead || CurrentState == EXBSoldierState::Dormant || CurrentState == EXBSoldierState::Dropping)
    {
        return;
    }

    if (!bIsRecruited)
    {
        return;
    }

    if (FollowComponent)
    {
        FollowComponent->EnterCombatMode();
    }

    SetSoldierState(EXBSoldierState::Combat);
    
    if (BehaviorInterface)
    {
        AActor* FoundEnemy = nullptr;
        if (BehaviorInterface->SearchForEnemy(FoundEnemy))
        {
            CurrentAttackTarget = FoundEnemy;
        }
    }

    UE_LOG(LogXBCombat, Log, TEXT("士兵 %s 进入战斗, 目标: %s"), 
        *GetName(), CurrentAttackTarget.IsValid() ? *CurrentAttackTarget->GetName() : TEXT("无"));
}

void AXBSoldierCharacter::ExitCombat()
{
    if (CurrentState == EXBSoldierState::Dead || CurrentState == EXBSoldierState::Dormant || CurrentState == EXBSoldierState::Dropping)
    {
        return;
    }

    CurrentAttackTarget = nullptr;
    
    if (FollowComponent)
    {
        FollowComponent->ExitCombatMode();
    }
    
    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        AICtrl->StopMovement();
    }

    SetSoldierState(EXBSoldierState::Following);

    UE_LOG(LogXBCombat, Log, TEXT("士兵 %s 退出战斗"), *GetName());
}

float AXBSoldierCharacter::TakeSoldierDamage(float DamageAmount, AActor* DamageSource)
{
    if (bIsDead || CurrentState == EXBSoldierState::Dead)
    {
        return 0.0f;
    }

    if (DamageAmount <= 0.0f)
    {
        return 0.0f;
    }

    float ActualDamage = FMath::Min(DamageAmount, CurrentHealth);
    CurrentHealth -= ActualDamage;

    UE_LOG(LogXBCombat, Log, TEXT("士兵 %s 受到 %.1f 伤害, 剩余血量: %.1f"), 
        *GetName(), ActualDamage, CurrentHealth);

    if (CurrentHealth <= 0.0f)
    {
        HandleDeath();
    }

    return ActualDamage;
}

bool AXBSoldierCharacter::PerformAttack(AActor* Target)
{
    if (BehaviorInterface)
    {
        EXBBehaviorResult Result = BehaviorInterface->ExecuteAttack(Target);
        return Result == EXBBehaviorResult::Success;
    }
    return false;
}

bool AXBSoldierCharacter::PlayAttackMontage()
{
    if (!IsDataAccessorValid())
    {
        return false;
    }

    UAnimMontage* AttackMontage = DataAccessor->GetBasicAttackMontage();

    if (!AttackMontage)
    {
        return false;
    }

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        return AnimInstance->Montage_Play(AttackMontage) > 0.0f;
    }

    return false;
}

bool AXBSoldierCharacter::CanAttack() const
{
    if (CurrentState == EXBSoldierState::Dead || CurrentState == EXBSoldierState::Dormant || CurrentState == EXBSoldierState::Dropping)
    {
        return false;
    }

    if (BehaviorInterface)
    {
        return BehaviorInterface->GetAttackCooldownRemaining() <= 0.0f;
    }
    return false;
}

// ==================== AI系统 ====================

bool AXBSoldierCharacter::HasEnemiesInRadius(float Radius) const
{
    FXBDetectionResult Result;
    return UXBBlueprintFunctionLibrary::DetectEnemiesInRadius(
        this,
        GetActorLocation(),
        Radius,
        Faction,
        true,
        Result
    );
}

float AXBSoldierCharacter::GetDistanceToTarget(AActor* Target) const
{
    if (!Target || !IsValid(Target))
    {
        return MAX_FLT;
    }
    return FVector::Dist(GetActorLocation(), Target->GetActorLocation());
}

bool AXBSoldierCharacter::IsInAttackRange(AActor* Target) const
{
    if (!Target || !IsValid(Target))
    {
        return false;
    }

    float AttackRangeVal = GetAttackRange();
    return GetDistanceToTarget(Target) <= AttackRangeVal;
}

void AXBSoldierCharacter::ReturnToFormation()
{
    CurrentAttackTarget = nullptr;
    
    if (FollowComponent)
    {
        FollowComponent->ExitCombatMode();
    }
    
    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        AICtrl->StopMovement();
    }
    
    SetSoldierState(EXBSoldierState::Following);
}

FVector AXBSoldierCharacter::CalculateAvoidanceDirection(const FVector& DesiredDirection)
{
    float AvoidanceRadiusVal = GetAvoidanceRadius();
    float AvoidanceWeightVal = GetAvoidanceWeight();

    if (AvoidanceRadiusVal <= 0.0f)
    {
        return DesiredDirection;
    }

    FVector AvoidanceForce = FVector::ZeroVector;
    FVector MyLocation = GetActorLocation();

    FXBDetectionResult AlliesResult;
    UXBBlueprintFunctionLibrary::DetectAlliesInRadius(
        this,
        MyLocation,
        AvoidanceRadiusVal,
        Faction,
        true,
        AlliesResult
    );

    int32 AvoidanceCount = 0;

    for (AActor* OtherActor : AlliesResult.DetectedActors)
    {
        if (OtherActor == this)
        {
            continue;
        }

        float Distance = FVector::Dist2D(MyLocation, OtherActor->GetActorLocation());
        if (Distance > KINDA_SMALL_NUMBER)
        {
            FVector AwayDirection = (MyLocation - OtherActor->GetActorLocation()).GetSafeNormal2D();
            float Strength = 1.0f - (Distance / AvoidanceRadiusVal);
            AvoidanceForce += AwayDirection * Strength;
            AvoidanceCount++;
        }
    }

    if (AvoidanceCount == 0)
    {
        return DesiredDirection;
    }

    AvoidanceForce.Normalize();

    FVector BlendedDirection = DesiredDirection * (1.0f - AvoidanceWeightVal) + 
                               AvoidanceForce * AvoidanceWeightVal;

    return BlendedDirection.GetSafeNormal();
}

void AXBSoldierCharacter::MoveToFormationPosition()
{
    if (FollowComponent)
    {
        FollowComponent->StartInterpolateToFormation();
    }
}

FVector AXBSoldierCharacter::GetFormationWorldPosition() const
{
    if (!FollowTarget.IsValid())
    {
        return GetActorLocation();
    }

    if (FollowComponent)
    {
        return FollowComponent->GetTargetPosition();
    }

    return FollowTarget->GetActorLocation();
}

FVector AXBSoldierCharacter::GetFormationWorldPositionSafe() const
{
    if (!FollowTarget.IsValid())
    {
        return FVector::ZeroVector;
    }
    
    AActor* Target = FollowTarget.Get();
    if (!Target || !IsValid(Target))
    {
        return FVector::ZeroVector;
    }
    
    if (!FollowComponent)
    {
        return Target->GetActorLocation();
    }
    
    FVector TargetPos = FollowComponent->GetTargetPosition();
    if (!TargetPos.IsZero() && !TargetPos.ContainsNaN())
    {
        return TargetPos;
    }
    
    return Target->GetActorLocation();
}

bool AXBSoldierCharacter::IsAtFormationPosition() const
{
    if (FollowComponent)
    {
        return FollowComponent->IsAtFormationPosition();
    }
    
    FVector TargetPos = GetFormationWorldPosition();
    float ArrivalThresholdVal = GetArrivalThreshold();
    return FVector::Dist2D(GetActorLocation(), TargetPos) <= ArrivalThresholdVal;
}

bool AXBSoldierCharacter::IsAtFormationPositionSafe() const
{
    if (!FollowTarget.IsValid() || FormationSlotIndex == INDEX_NONE)
    {
        return true;
    }
    
    if (FollowComponent)
    {
        return FollowComponent->IsAtFormationPosition();
    }
    
    return true;
}

// ==================== 逃跑系统 ====================

void AXBSoldierCharacter::SetEscaping(bool bEscaping)
{
    bIsEscaping = bEscaping;

    if (bEscaping)
    {
        if (FollowComponent)
        {
            FollowComponent->SetCombatState(false);
            
            if (CurrentState == EXBSoldierState::Combat)
            {
                CurrentAttackTarget = nullptr;
                SetSoldierState(EXBSoldierState::Following);
            }
            
            FollowComponent->StartInterpolateToFormation();
        }
        
        if (AAIController* AICtrl = Cast<AAIController>(GetController()))
        {
            AICtrl->StopMovement();
        }
    }

    float BaseSpeed = GetMoveSpeed();
    float SprintMultiplier = GetSprintSpeedMultiplier();

    float NewSpeed = bEscaping ? BaseSpeed * SprintMultiplier : BaseSpeed;

    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->MaxWalkSpeed = NewSpeed;
    }
}

// ==================== 对象池支持 ====================

void AXBSoldierCharacter::ResetForPooling()
{
    EnterDormantState(EXBDormantType::Hidden);

    FollowTarget = nullptr;
    FormationSlotIndex = INDEX_NONE;

    bIsRecruited = false;
    bIsDead = false;
    bIsEscaping = false;

    CurrentHealth = 100.0f;

    CurrentAttackTarget = nullptr;

    AttackCooldownTimer = 0.0f;
    TargetSearchTimer = 0.0f;

    // 重置掉落状态
    DropElapsedTime = 0.0f;
    DropTargetLeader = nullptr;
    bAutoRecruitOnLanding = true;

    GetWorldTimerManager().ClearTimer(DelayedAIStartTimerHandle);

    UE_LOG(LogXBSoldier, Verbose, TEXT("士兵 %s 状态已重置，进入池化休眠"), *GetName());
}

// ==================== 死亡系统 ====================

void AXBSoldierCharacter::HandleDeath()
{
    if (bIsDead)
    {
        return;
    }

    GetWorldTimerManager().ClearTimer(DelayedAIStartTimerHandle);
    
    bIsDead = true;
    
    if (FollowComponent)
    {
        FollowComponent->SetFollowMode(EXBFollowMode::Free);
        FollowComponent->SetComponentTickEnabled(false);
    }
    
    SetSoldierState(EXBSoldierState::Dead);

    OnSoldierDied.Broadcast(this);

    if (AXBCharacterBase* LeaderCharacter = GetLeaderCharacter())
    {
        LeaderCharacter->OnSoldierDied(this);
    }

    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        AICtrl->StopMovement();
    }

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->StopMovementImmediately();
        MoveComp->DisableMovement();
        MoveComp->SetComponentTickEnabled(false);
    }

    bool bMontageStarted = false;
    float DeathAnimDuration = 1.5f;
    
    if (IsDataAccessorValid())
    {
        UAnimMontage* DeathMontage = DataAccessor->GetDeathMontage();
        if (DeathMontage)
        {
            if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
            {
                float Duration = AnimInstance->Montage_Play(DeathMontage);
                if (Duration > 0.0f)
                {
                    bMontageStarted = true;
                    DeathAnimDuration = Duration;
                }
            }
        }
    }

    FTimerHandle RecycleTimerHandle;
    GetWorldTimerManager().SetTimer(
        RecycleTimerHandle,
        [this]()
        {
            if (!IsValid(this))
            {
                return;
            }
            
            if (UWorld* World = GetWorld())
            {
                if (UXBSoldierPoolSubsystem* PoolSubsystem = World->GetSubsystem<UXBSoldierPoolSubsystem>())
                {
                    PoolSubsystem->ReleaseSoldier(this);
                    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 已回收到对象池"), *GetName());
                }
                else
                {
                    ResetForPooling();
                    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 已重置为休眠态（无对象池）"), *GetName());
                }
            }
        },
        DeathAnimDuration + 0.5f,
        false
    );

    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 死亡，%.1f秒后回收"), *GetName(), DeathAnimDuration + 0.5f);
}

void AXBSoldierCharacter::FaceTarget(AActor* Target, float DeltaTime)
{
    if (!Target || !IsValid(Target))
    {
        return;
    }

    FVector Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    if (!Direction.IsNearlyZero())
    {
        FRotator TargetRotation = Direction.Rotation();
        FRotator CurrentRotation = GetActorRotation();
        
        float RotationSpeedVal = GetRotationSpeed();
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotationSpeedVal / 90.0f);
        SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
    }
}

// ==================== AI控制器初始化 ====================

void AXBSoldierCharacter::SpawnAndPossessAIController()
{
    if (!IsValid(this) || IsPendingKillPending())
    {
        return;
    }

    if (CurrentState == EXBSoldierState::Dormant || CurrentState == EXBSoldierState::Dropping)
    {
        return;
    }
    
    UCapsuleComponent* Capsule = GetCapsuleComponent();
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    
    if (!Capsule || !MoveComp)
    {
        UE_LOG(LogXBSoldier, Error, TEXT("士兵 %s: 组件无效，无法启动AI"), *GetName());
        return;
    }
    
    FTransform CapsuleTransform = Capsule->GetComponentTransform();
    if (!CapsuleTransform.IsValid())
    {
        GetWorldTimerManager().SetTimer(
            DelayedAIStartTimerHandle,
            this,
            &AXBSoldierCharacter::SpawnAndPossessAIController,
            0.1f,
            false
        );
        return;
    }
    
    if (GetController())
    {
        InitializeAI();
        return;
    }
    
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    
    UClass* ControllerClassToUse = nullptr;
    if (SoldierAIControllerClass)
    {
        ControllerClassToUse = SoldierAIControllerClass.Get();
    }
    else
    {
        ControllerClassToUse = AXBSoldierAIController::StaticClass();
    }
    
    if (!ControllerClassToUse)
    {
        return;
    }
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    AAIController* NewController = World->SpawnActor<AAIController>(
        ControllerClassToUse,
        GetActorLocation(),
        GetActorRotation(),
        SpawnParams
    );
    
    if (NewController)
    {
        NewController->Possess(this);
        InitializeAI();
    }
}

void AXBSoldierCharacter::InitializeAI()
{
    AAIController* AICtrl = Cast<AAIController>(GetController());
    if (!AICtrl)
    {
        return;
    }
    
    if (BehaviorTreeAsset)
    {
        AICtrl->RunBehaviorTree(BehaviorTreeAsset);
        
        if (UBlackboardComponent* BBComp = AICtrl->GetBlackboardComponent())
        {
            BBComp->SetValueAsObject(TEXT("Self"), this);
            BBComp->SetValueAsObject(TEXT("Leader"), FollowTarget.Get());
            BBComp->SetValueAsInt(TEXT("SoldierState"), static_cast<int32>(CurrentState));
            BBComp->SetValueAsInt(TEXT("FormationSlot"), FormationSlotIndex);
            BBComp->SetValueAsFloat(TEXT("AttackRange"), GetAttackRange());
            BBComp->SetValueAsFloat(TEXT("VisionRange"), GetVisionRange());
            BBComp->SetValueAsFloat(TEXT("DetectionRange"), GetVisionRange());
            BBComp->SetValueAsBool(TEXT("IsAtFormation"), true);
            BBComp->SetValueAsBool(TEXT("CanAttack"), true);
        }
    }
}