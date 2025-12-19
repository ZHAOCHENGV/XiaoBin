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
    
    // ✨ 新增 - 创建 Zzz 特效组件
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
    
    // 组件校验
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

    // 加载 Zzz 特效资源
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

    // ✨ 关键 - 根据配置决定初始状态
    if (bStartAsDormant)
    {
        // 确保阵营是中立的
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
}

void AXBSoldierCharacter::EnableMovementAndTick()
{
    if (!IsValid(this) || IsPendingKillPending())
    {
        return;
    }
    
    // ✨ 新增 - 休眠态不启用移动
    if (CurrentState == EXBSoldierState::Dormant)
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

// ==================== ✨ 新增：休眠系统实现 ====================

/**
 * @brief 进入休眠态
 * @param DormantType 休眠类型
 * @note 禁用所有非必要组件，显示休眠视觉效果
 */
void AXBSoldierCharacter::EnterDormantState(EXBDormantType DormantType)
{
    if (CurrentState == EXBSoldierState::Dormant)
    {
        // 已经在休眠态，只切换类型
        SetDormantType(DormantType);
        return;
    }

    EXBSoldierState OldState = CurrentState;
    CurrentState = EXBSoldierState::Dormant;
    CurrentDormantType = DormantType;

    // 重置招募状态
    bIsRecruited = false;
    bIsDead = false;
    Faction = EXBFaction::Neutral;

    // 禁用激活态组件
    DisableActiveComponents();

    // 更新视觉效果
    if (DormantType != EXBDormantType::Hidden)
    {
        SetActorHiddenInGame(false);
        UpdateDormantAnimation();
        UpdateZzzEffect();
    }
    else
    {
        // Hidden 类型完全隐藏（对象池中）
        SetActorHiddenInGame(true);
        SetZzzEffectEnabled(false);
    }

    // 广播事件
    OnSoldierStateChanged.Broadcast(OldState, CurrentState);
    OnDormantStateChanged.Broadcast(this, true);

    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 进入休眠态，类型: %d"), 
        *GetName(), static_cast<int32>(DormantType));
}

/**
 * @brief 退出休眠态（激活）
 * @note 启用所有组件，准备进入战斗
 */
void AXBSoldierCharacter::ExitDormantState()
{
    if (CurrentState != EXBSoldierState::Dormant)
    {
        return;
    }

    EXBSoldierState OldState = CurrentState;
    CurrentState = EXBSoldierState::Idle;

    // 关闭休眠视觉效果
    SetZzzEffectEnabled(false);
    
    // 停止休眠动画
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->Stop();
    }

    // 显示角色
    SetActorHiddenInGame(false);

    // 启用激活态组件
    EnableActiveComponents();

    // 广播事件
    OnSoldierStateChanged.Broadcast(OldState, CurrentState);
    OnDormantStateChanged.Broadcast(this, false);

    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 退出休眠态"), *GetName());
}

/**
 * @brief 设置休眠视觉配置
 */
void AXBSoldierCharacter::SetDormantVisualConfig(const FXBDormantVisualConfig& NewConfig)
{
    DormantConfig = NewConfig;

    // 更新 Zzz 特效位置
    if (ZzzEffectComponent)
    {
        ZzzEffectComponent->SetRelativeLocation(DormantConfig.ZzzEffectOffset);
    }

    // 如果当前在休眠态，立即更新视觉效果
    if (CurrentState == EXBSoldierState::Dormant)
    {
        UpdateDormantAnimation();
        UpdateZzzEffect();
    }
}

/**
 * @brief 设置 Zzz 特效启用状态
 */
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

/**
 * @brief 切换休眠类型
 */
void AXBSoldierCharacter::SetDormantType(EXBDormantType NewType)
{
    if (CurrentDormantType == NewType)
    {
        return;
    }

    CurrentDormantType = NewType;

    // 只有在休眠态才更新视觉效果
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

/**
 * @brief 启用激活态组件
 */
void AXBSoldierCharacter::EnableActiveComponents()
{
    // 启用 Tick
    SetActorTickEnabled(true);

    // 启用移动组件
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->SetComponentTickEnabled(true);
        MoveComp->SetMovementMode(MOVE_Walking);
    }

    // 启用碰撞
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }

    // 启用跟随组件
    if (FollowComponent)
    {
        FollowComponent->SetComponentTickEnabled(true);
    }

    // 启用行为接口组件
    if (BehaviorInterface)
    {
        BehaviorInterface->SetComponentTickEnabled(true);
    }

    // 启用调试组件
    if (DebugComponent)
    {
        DebugComponent->SetComponentTickEnabled(true);
    }

    UE_LOG(LogXBSoldier, Verbose, TEXT("士兵 %s: 激活态组件已启用"), *GetName());
}

/**
 * @brief 禁用激活态组件
 */
void AXBSoldierCharacter::DisableActiveComponents()
{
    // 禁用移动组件
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->StopMovementImmediately();
        MoveComp->SetComponentTickEnabled(false);
    }

    // 保留碰撞（用于被招募检测）
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }

    // 禁用跟随组件
    if (FollowComponent)
    {
        FollowComponent->SetFollowTarget(nullptr);
        FollowComponent->SetComponentTickEnabled(false);
    }

    // 禁用行为接口组件
    if (BehaviorInterface)
    {
        BehaviorInterface->SetComponentTickEnabled(false);
    }

    // 🔧 修改 - 重命名局部变量为 CurrentController
    if (AController* CurrentController = GetController())
    {
        CurrentController->UnPossess();
    }

    // 清除跟随目标
    FollowTarget = nullptr;
    FormationSlotIndex = INDEX_NONE;
    CurrentAttackTarget = nullptr;

    UE_LOG(LogXBSoldier, Verbose, TEXT("士兵 %s: 激活态组件已禁用"), *GetName());
}

/**
 * @brief 更新休眠动画
 */
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
        // Hidden 不播放动画，直接返回
        return;
    }

    PlayAnimationSequence(AnimToPlay, true);
}

/**
 * @brief 更新 Zzz 特效
 */
void AXBSoldierCharacter::UpdateZzzEffect()
{
    // 只有睡眠类型且配置启用时才显示 Zzz
    bool bShouldShowZzz = (CurrentDormantType == EXBDormantType::Sleeping) && 
                          DormantConfig.bShowZzzEffect;
    
    SetZzzEffectEnabled(bShouldShowZzz);
}

/**
 * @brief 播放指定动画序列
 */
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

/**
 * @brief 加载休眠动画资源
 */
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

/**
 * @brief 检查是否可以被招募
 * @return 是否可招募
 * @note 🔧 修改 - 增强检查条件，防止重复招募
 */
bool AXBSoldierCharacter::CanBeRecruited() const
{
    // 已被招募则不可再招募
    if (bIsRecruited)
    {
        return false;
    }
    
    // 已有跟随目标则不可招募
    if (FollowTarget.IsValid())
    {
        return false;
    }
    
    // 必须是中立阵营
    if (Faction != EXBFaction::Neutral)
    {
        return false;
    }
    
    // 必须处于休眠态或待机态
    if (CurrentState != EXBSoldierState::Dormant && CurrentState != EXBSoldierState::Idle)
    {
        return false;
    }
    
    // 已死亡不可招募
    if (bIsDead || CurrentHealth <= 0.0f)
    {
        return false;
    }
    
    // 组件必须初始化
    if (!bComponentsInitialized)
    {
        return false;
    }
    
    return true;
}

/**
 * @brief 士兵被招募
 * @param NewLeader 新将领
 * @param SlotIndex 槽位索引
 * @note 🔧 修改 - 增强防重复招募检查，修复旋转问题
 */
void AXBSoldierCharacter::OnRecruited(AActor* NewLeader, int32 SlotIndex)
{
   if (!NewLeader)
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: 招募失败 - 将领为空"), *GetName());
        return;
    }
    
    // 🔧 核心修复 - 严格检查是否已被招募
    if (bIsRecruited)
    {
        UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: 已被招募，忽略来自 %s 的重复招募请求"), 
            *GetName(), *NewLeader->GetName());
        return;
    }

    // 🔧 新增 - 检查是否已有跟随目标（可能是被其他将领招募中）
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
    
    // 🔧 核心 - 立即标记为已招募，防止其他将领抢夺
    bIsRecruited = true;
    FollowTarget = NewLeader;
    FormationSlotIndex = SlotIndex;
    
    // 如果处于休眠态，先退出休眠
    if (CurrentState == EXBSoldierState::Dormant)
    {
        ExitDormantState();
    }
    
    // 设置阵营
    if (AXBCharacterBase* LeaderChar = Cast<AXBCharacterBase>(NewLeader))
    {
        Faction = LeaderChar->GetFaction();
    }
    
    // 🔧 修复旋转 - 先面向将领
    FVector DirectionToLeader = (NewLeader->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    if (!DirectionToLeader.IsNearlyZero())
    {
        SetActorRotation(DirectionToLeader.Rotation());
    }
    
    // 配置跟随组件
    if (FollowComponent)
    {
        FollowComponent->SetFollowTarget(NewLeader);
        FollowComponent->SetFormationSlotIndex(SlotIndex);
        
        // 同步将领冲刺状态
        if (AXBCharacterBase* LeaderChar = Cast<AXBCharacterBase>(NewLeader))
        {
            bool bLeaderSprinting = LeaderChar->IsSprinting();
            float LeaderSpeed = LeaderChar->GetCurrentMoveSpeed();
            FollowComponent->SyncLeaderSprintState(bLeaderSprinting, LeaderSpeed);
        }
        
        FollowComponent->StartRecruitTransition();
    }
    
    // 设置状态
    SetSoldierState(EXBSoldierState::Following);
    
    // 延迟启动 AI
    GetWorldTimerManager().SetTimer(
        DelayedAIStartTimerHandle,
        this,
        &AXBSoldierCharacter::SpawnAndPossessAIController,
        0.3f,
        false
    );
    
    // 广播事件
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
    if (CurrentState == EXBSoldierState::Dead || CurrentState == EXBSoldierState::Dormant)
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
    if (CurrentState == EXBSoldierState::Dead || CurrentState == EXBSoldierState::Dormant)
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

    // ✨ 新增 - 休眠态也可以受伤
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
    if (CurrentState == EXBSoldierState::Dead || CurrentState == EXBSoldierState::Dormant)
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

    float AttackRange = GetAttackRange();
    return GetDistanceToTarget(Target) <= AttackRange;
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
    float AvoidanceRadius = GetAvoidanceRadius();
    float AvoidanceWeightVal = GetAvoidanceWeight();

    if (AvoidanceRadius <= 0.0f)
    {
        return DesiredDirection;
    }

    FVector AvoidanceForce = FVector::ZeroVector;
    FVector MyLocation = GetActorLocation();

    FXBDetectionResult AlliesResult;
    UXBBlueprintFunctionLibrary::DetectAlliesInRadius(
        this,
        MyLocation,
        AvoidanceRadius,
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
            float Strength = 1.0f - (Distance / AvoidanceRadius);
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

/**
 * @brief 重置士兵状态（用于对象池回收）
 * @note 🔧 修改 - 进入 Hidden 休眠态
 */
void AXBSoldierCharacter::ResetForPooling()
{
    // 进入 Hidden 休眠态
    EnterDormantState(EXBDormantType::Hidden);

    // 清除跟随目标
    FollowTarget = nullptr;
    FormationSlotIndex = INDEX_NONE;

    // 重置状态变量
    bIsRecruited = false;
    bIsDead = false;
    bIsEscaping = false;

    // 重置血量
    CurrentHealth = 100.0f;

    // 清除攻击目标
    CurrentAttackTarget = nullptr;

    // 重置冷却
    AttackCooldownTimer = 0.0f;
    TargetSearchTimer = 0.0f;

    // 清除 AI 定时器
    GetWorldTimerManager().ClearTimer(DelayedAIStartTimerHandle);

    UE_LOG(LogXBSoldier, Verbose, TEXT("士兵 %s 状态已重置，进入池化休眠"), *GetName());
}

// ==================== 死亡系统 ====================

/**
 * @brief 处理士兵死亡
 * @note 🔧 修改 - 支持对象池回收
 */
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

    // 播放死亡蒙太奇
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

    // 🔧 修改 - 统一使用对象池回收
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
                    // 没有对象池，直接重置为休眠态
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

    // 休眠态不需要 AI
    if (CurrentState == EXBSoldierState::Dormant)
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