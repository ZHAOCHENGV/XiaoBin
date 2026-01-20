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
#include "AI/XBSoldierAIController.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/Components/XBFormationComponent.h"
#include "Character/XBCharacterBase.h"
#include "Character/XBPlayerCharacter.h"
#include "Combat/XBProjectilePoolSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/XBSoldierDataAccessor.h"
#include "DrawDebugHelpers.h"
#include "Engine/DataTable.h"
#include "GAS/Abilities/XBGameplayAbility_Attack.h"
#include "GAS/XBAbilitySystemComponent.h"
#include "Game/XBGameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Soldier/Component/XBSoldierBehaviorInterface.h"
#include "Soldier/Component/XBSoldierDebugComponent.h"
#include "Soldier/Component/XBSoldierFollowComponent.h"
#include "Soldier/Component/XBSoldierPoolSubsystem.h"
#include "TimerManager.h"
#include "Utils/XBBlueprintFunctionLibrary.h"
#include "Utils/XBLogCategories.h"
#include "XBCollisionChannels.h"

AXBSoldierCharacter::AXBSoldierCharacter() {
  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.bStartWithTickEnabled = false;

  // ==================== 碰撞配置 ====================
  if (UCapsuleComponent *Capsule = GetCapsuleComponent()) {
    Capsule->InitCapsuleSize(34.0f, 88.0f);
    Capsule->SetCollisionObjectType(XBCollision::Soldier);
    Capsule->SetCollisionResponseToChannel(XBCollision::Leader, ECR_Overlap);
    Capsule->SetCollisionResponseToChannel(XBCollision::Soldier, ECR_Overlap);
  }

  if (USkeletalMeshComponent *MeshComp = GetMesh()) {
    MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
    MeshComp->SetCollisionResponseToChannel(XBCollision::Soldier, ECR_Ignore);
    MeshComp->SetCollisionResponseToChannel(XBCollision::Leader, ECR_Ignore);
  }

  // ==================== 创建组件 ====================
  DataAccessor =
      CreateDefaultSubobject<UXBSoldierDataAccessor>(TEXT("DataAccessor"));
  FollowComponent = CreateDefaultSubobject<UXBSoldierFollowComponent>(
      TEXT("FollowComponent"));
  DebugComponent =
      CreateDefaultSubobject<UXBSoldierDebugComponent>(TEXT("DebugComponent"));
  BehaviorInterface = CreateDefaultSubobject<UXBSoldierBehaviorInterface>(
      TEXT("BehaviorInterface"));

  // ✨ 新增 - 士兵ASC用于近战Tag触发GA
  AbilitySystemComponent = CreateDefaultSubobject<UXBAbilitySystemComponent>(
      TEXT("AbilitySystemComponent"));
  MeleeHitAbilityClass = UXBGameplayAbility_Attack::StaticClass();

  ZzzEffectComponent =
      CreateDefaultSubobject<UNiagaraComponent>(TEXT("ZzzEffectComponent"));
  ZzzEffectComponent->SetupAttachment(RootComponent);
  ZzzEffectComponent->SetAutoActivate(false);

  // ==================== 移动组件配置 ====================
  if (UCharacterMovementComponent *MovementComp = GetCharacterMovement()) {
    MovementComp->bOrientRotationToMovement = true;
    MovementComp->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
    MovementComp->MaxWalkSpeed = 400.0f;
    MovementComp->BrakingDecelerationWalking = 2000.0f;
    MovementComp->SetComponentTickEnabled(false);

    // ✨ 新增 - 初始化RVO避让系统参数
    // 默认关闭，仅在战斗状态(SetSoldierState)时开启
    MovementComp->bUseRVOAvoidance = false;

    // 🔧 修改 - 移除 SetAvoidanceEnabled 调用，避让启用/禁用在 SetSoldierState
    // 中控制
    MovementComp->AvoidanceConsiderationRadius = 500.0f;
    MovementComp->AvoidanceWeight = 0.5f;
    MovementComp->AvoidanceGroup.SetFlagsDirectly(1); // 避让组1
    MovementComp->GroupsToAvoid.SetFlagsDirectly(1);  // 避让组1
    MovementComp->GroupsToIgnore.SetFlagsDirectly(0); // 不忽略任何组
  }

  AutoPossessAI = EAutoPossessAI::Disabled;
  AIControllerClass = nullptr;
}

void AXBSoldierCharacter::PostInitializeComponents() {
  Super::PostInitializeComponents();

  bComponentsInitialized = true;

  UCapsuleComponent *Capsule = GetCapsuleComponent();
  if (Capsule) {
    FTransform CapsuleTransform = Capsule->GetComponentTransform();
    FVector Scale = CapsuleTransform.GetScale3D();

    if (Scale.IsNearlyZero() || Scale.ContainsNaN()) {
      UE_LOG(LogXBSoldier, Warning,
             TEXT("士兵 %s: Capsule Scale 无效，修正为 (1,1,1)"), *GetName());
      Capsule->SetWorldScale3D(FVector::OneVector);
    }
  }

  UCharacterMovementComponent *MoveComp = GetCharacterMovement();
  if (MoveComp && !MoveComp->UpdatedComponent) {
    MoveComp->SetUpdatedComponent(Capsule);
  }

  UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: PostInitializeComponents 完成"),
         *GetName());
}

void AXBSoldierCharacter::BeginPlay() {
  Super::BeginPlay();

  // 🔧 修改 - 初始化ASC信息，确保GA可被触发
  if (AbilitySystemComponent) {
    AbilitySystemComponent->InitAbilityActorInfo(this, this);
  }

  // 🔧 修改 - 使用数据表配置刷新近战GA
  RefreshMeleeHitAbilityFromData();

  // 🔧 修改 - 近战GA授予由 RefreshMeleeHitAbilityFromData 统一处理

  if (!ZzzEffectAsset.IsNull() && ZzzEffectComponent) {
    if (UNiagaraSystem *LoadedEffect = ZzzEffectAsset.LoadSynchronous()) {
      ZzzEffectComponent->SetAsset(LoadedEffect);
    }
  }

  if (ZzzEffectComponent) {
    ZzzEffectComponent->SetRelativeLocation(DormantConfig.ZzzEffectOffset);
  }

  LoadDormantAnimations();

  if (IsDataAccessorValid()) {
    CurrentHealth = DataAccessor->GetMaxHealth();
  } else {
    CurrentHealth = 100.0f;
  }

  if (bStartAsDormant) {
    Faction = EXBFaction::Neutral;
    EnterDormantState(DormantConfig.DormantType);

    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: 初始化为休眠态，阵营: 中立"),
           *GetName());
  } else {
    GetWorldTimerManager().SetTimerForNextTick(
        [this]() { EnableMovementAndTick(); });
  }

  UE_LOG(LogXBSoldier, Log,
         TEXT("士兵 %s BeginPlay - 阵营: %d, 状态: %d, 休眠: %s"), *GetName(),
         static_cast<int32>(Faction), static_cast<int32>(CurrentState),
         bStartAsDormant ? TEXT("是") : TEXT("否"));
}

UAbilitySystemComponent *
AXBSoldierCharacter::GetAbilitySystemComponent() const {
  return AbilitySystemComponent;
}

void AXBSoldierCharacter::RefreshMeleeHitAbilityFromData() {
  if (!IsDataAccessorValid()) {
    UE_LOG(LogXBSoldier, Warning,
           TEXT("刷新近战GA失败：DataAccessor无效，Soldier=%s"), *GetName());
    return;
  }

  // 🔧 修改 - 从数据表读取普攻GA作为近战命中GA
  const TSubclassOf<UGameplayAbility> DataAttackGA =
      DataAccessor->GetRawData().BasicAttack.AbilityClass;
  if (DataAttackGA) {
    MeleeHitAbilityClass = DataAttackGA;
    UE_LOG(LogXBSoldier, Log, TEXT("读取数据表近战GA成功: %s, Soldier=%s"),
           *MeleeHitAbilityClass->GetName(), *GetName());
  } else {
    UE_LOG(LogXBSoldier, Warning, TEXT("数据表未配置近战GA，Soldier=%s"),
           *GetName());
  }

  if (!AbilitySystemComponent) {
    UE_LOG(LogXBSoldier, Warning, TEXT("刷新近战GA失败：ASC无效，Soldier=%s"),
           *GetName());
    return;
  }

  AbilitySystemComponent->InitAbilityActorInfo(this, this);

  if (!HasAuthority() || !MeleeHitAbilityClass) {
    UE_LOG(LogXBSoldier, Verbose,
           TEXT("刷新近战GA跳过：无权限或GA无效，Soldier=%s"), *GetName());
    return;
  }

  if (!AbilitySystemComponent->FindAbilitySpecFromClass(MeleeHitAbilityClass)) {
    FGameplayAbilitySpec HitSpec(MeleeHitAbilityClass, 1, INDEX_NONE, this);
    AbilitySystemComponent->GiveAbility(HitSpec);
    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 刷新近战GA: %s"), *GetName(),
           *MeleeHitAbilityClass->GetName());
  } else {
    UE_LOG(LogXBSoldier, Verbose, TEXT("士兵 %s 已拥有近战GA: %s"), *GetName(),
           *MeleeHitAbilityClass->GetName());
  }
}

void AXBSoldierCharacter::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);

  // 更新掉落飞行
  if (CurrentState == EXBSoldierState::Dropping) {
    UpdateDropFlight(DeltaTime);
  }

  // 🔧 修改 - 跟随/待机状态下尝试自动反击，修复无主将战斗不响应问题
  TryAutoEngage(DeltaTime);

  // ✨ 新增 - 技能/攻击动作期间禁止移动和旋转
  // 🔧 修复 - 士兵没有 CombatComponent，改为直接检查蒙太奇播放状态
  if (IsDataAccessorValid()) {
    if (UAnimMontage *AttackMontage = DataAccessor->GetBasicAttackMontage()) {
      if (GetMesh() && GetMesh()->GetAnimInstance() &&
          GetMesh()->GetAnimInstance()->Montage_IsPlaying(AttackMontage)) {
        if (UCharacterMovementComponent *MoveComp = GetCharacterMovement()) {
          MoveComp->StopMovementImmediately();      // 停止移动
          MoveComp->Velocity = FVector::ZeroVector; // 强制速度为0
          MoveComp->ClearAccumulatedForces();       // 清除累积力
        }

        // 禁止通过控制器旋转（如果AI正在Focus目标）
        if (AController *ContextController = GetController()) {
          ContextController->StopMovement();
        }
      }
    }
  }
}

void AXBSoldierCharacter::EnableMovementAndTick() {
  if (!IsValid(this) || IsPendingKillPending()) {
    return;
  }

  // 休眠态和掉落态不启用移动
  if (CurrentState == EXBSoldierState::Dormant ||
      CurrentState == EXBSoldierState::Dropping) {
    return;
  }

  UCapsuleComponent *Capsule = GetCapsuleComponent();
  UCharacterMovementComponent *MoveComp = GetCharacterMovement();

  if (!Capsule || !MoveComp) {
    UE_LOG(LogXBSoldier, Error, TEXT("士兵 %s: 组件无效，无法启用移动"),
           *GetName());
    return;
  }

  FTransform CapsuleTransform = Capsule->GetComponentTransform();
  if (!CapsuleTransform.IsValid()) {
    UE_LOG(LogXBSoldier, Error, TEXT("士兵 %s: Capsule Transform 无效"),
           *GetName());
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
void AXBSoldierCharacter::FullInitialize(UDataTable *DataTable, FName RowName,
                                         EXBFaction InFaction) {
  // ✨ 新增 - 在初始化前先清理之前的归属关系
  if (FollowTarget.IsValid()) {
    if (AXBCharacterBase *OldLeader =
            Cast<AXBCharacterBase>(FollowTarget.Get())) {
      OldLeader->RemoveSoldier(this);
      UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: 从旧将领 %s 队伍中移除"),
             *GetName(), *OldLeader->GetName());
    }
  }

  // 1. 基础数据初始化
  InitializeFromDataTable(DataTable, RowName, InFaction);

  // 2. 确保组件初始化标记
  bComponentsInitialized = true;

  // 3. 启用 Tick（掉落状态需要）
  SetActorTickEnabled(true);

  // 4. 设置阵营
  Faction = InFaction;

  // 5. 彻底重置归属状态
  bIsRecruited = false;
  bIsDead = false;
  bIsEscaping = false;
  FollowTarget = nullptr;
  FormationSlotIndex = INDEX_NONE;
  CurrentAttackTarget = nullptr;
  GetWorldTimerManager().ClearTimer(TargetRequestTimerHandle);
  UnbindAssignedTargetEvents();

  // 6. 重置跟随组件状态
  if (FollowComponent) {
    FollowComponent->SetFollowTarget(nullptr);
    FollowComponent->SetFormationSlotIndex(INDEX_NONE);
  }

  // 7. 显示角色
  SetActorHiddenInGame(false);

  // 🔧 修改 - 不在这里启用碰撞，让调用者控制
  // 掉落士兵需要在飞行结束后才启用碰撞

  UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: 完整初始化完成，阵营: %d"),
         *GetName(), static_cast<int32>(InFaction));
}

// ==================== 掉落抛物线飞行系统 ====================

/**
 * @brief 开始掉落抛物线飞行
 * @param StartLocation 起始位置（将领死亡位置）
 * @param TargetLocation 目标落地位置
 * @param ArcConfig 抛物线配置
 * @param TargetLeader 落地后要加入的将领（可选）
 * @note 🔧 修改 - 优化移动组件状态管理
 */
void AXBSoldierCharacter::StartDropFlight(const FVector &StartLocation,
                                          const FVector &TargetLocation,
                                          const FXBDropArcConfig &ArcConfig,
                                          AXBCharacterBase *TargetLeader) {
  // 🔧 修改 - 缓存抛物线配置，便于蓝图实时调试
  ActiveDropArcConfig = ArcConfig;

  // 保存飞行参数
  DropStartLocation = StartLocation;
  DropTargetLocation =
      ComputeGroundSnappedLocation(TargetLocation, ActiveDropArcConfig);
  DropFlightDuration = ActiveDropArcConfig.FlightDuration;
  DropArcHeight = ActiveDropArcConfig.ArcHeight;
  bPlayDropLandingEffect = ActiveDropArcConfig.bPlayLandingEffect;
  DropTargetLeader = TargetLeader;
  bAutoRecruitOnLanding = ActiveDropArcConfig.bAutoRecruitOnLanding;

  // 保存落地特效资源
  if (!ActiveDropArcConfig.LandingEffect.IsNull()) {
    DropLandingEffectAsset = ActiveDropArcConfig.LandingEffect;
  }

  // 重置计时器
  DropElapsedTime = 0.0f;

  // 设置初始位置
  SetActorLocation(StartLocation);

  // 进入掉落状态
  SetSoldierState(EXBSoldierState::Dropping);

  // 确保 Tick 启用
  SetActorTickEnabled(true);

  // 🔧 修改 - 确保碰撞完全禁用（避免触发磁场）
  if (UCapsuleComponent *Capsule = GetCapsuleComponent()) {
    Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
    Capsule->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
  }

  // 完全禁用移动组件
  if (UCharacterMovementComponent *MoveComp = GetCharacterMovement()) {
    MoveComp->DisableMovement();
    MoveComp->StopMovementImmediately();
    MoveComp->SetComponentTickEnabled(false);
    MoveComp->GravityScale = 0.0f;
  }

  // 隐藏 Zzz 特效
  SetZzzEffectEnabled(false);

  // 显示角色
  SetActorHiddenInGame(false);

  // ✨ 新增 - 启用调试绘制帮助调参
  if (ActiveDropArcConfig.bEnableDebugDraw) {
    DrawDropDebugArc(ActiveDropArcConfig.DebugDrawDuration,
                     ActiveDropArcConfig.DebugArcSegments);
  }

  UE_LOG(LogXBSoldier, Log,
         TEXT("士兵 %s 开始掉落飞行: (%.0f, %.0f, %.0f) -> (%.0f, %.0f, %.0f)"),
         *GetName(), StartLocation.X, StartLocation.Y, StartLocation.Z,
         DropTargetLocation.X, DropTargetLocation.Y, DropTargetLocation.Z);
}

float AXBSoldierCharacter::GetDropProgress() const {
  if (DropFlightDuration <= KINDA_SMALL_NUMBER) {
    return 1.0f;
  }
  return FMath::Clamp(DropElapsedTime / DropFlightDuration, 0.0f, 1.0f);
}

void AXBSoldierCharacter::UpdateDropFlight(float DeltaTime) {
  // 更新计时器
  DropElapsedTime += DeltaTime;

  // 计算进度
  float Progress = GetDropProgress();

  // 计算当前位置
  FVector CurrentPosition = CalculateArcPosition(Progress);
  SetActorLocation(CurrentPosition);

  // 计算面向方向（朝向目标）
  FVector Direction =
      (DropTargetLocation - DropStartLocation).GetSafeNormal2D();
  if (!Direction.IsNearlyZero()) {
    SetActorRotation(Direction.Rotation());
  }

  // 检查是否完成
  if (Progress >= 1.0f) {
    OnDropLanded();
  }
}

FVector AXBSoldierCharacter::CalculateArcPosition(float Progress) const {
  // XY 线性插值
  float X = FMath::Lerp(DropStartLocation.X, DropTargetLocation.X, Progress);
  float Y = FMath::Lerp(DropStartLocation.Y, DropTargetLocation.Y, Progress);

  // Z 抛物线计算
  float LinearZ =
      FMath::Lerp(DropStartLocation.Z, DropTargetLocation.Z, Progress);
  float ArcOffset = DropArcHeight * 4.0f * Progress * (1.0f - Progress);
  float Z = LinearZ + ArcOffset;

  return FVector(X, Y, Z);
}

/**
 * @brief 将期望落地点投射到地面
 * @param DesiredLocation 期望落地世界坐标
 * @param ArcConfig 抛物线配置（包含检测距离与偏移）
 * @return 贴合地面的落点
 * @note 🔧 修改 - 统一地面检测，消除悬空落地问题
 */
FVector AXBSoldierCharacter::ComputeGroundSnappedLocation(
    const FVector &DesiredLocation, const FXBDropArcConfig &ArcConfig) const {
  // 🔧 修改 - 默认使用当前高度，若检测失败至少应用额外偏移
  FVector Result =
      DesiredLocation + FVector(0.0f, 0.0f, ArcConfig.LandingExtraZOffset);

  UWorld *World = GetWorld();
  if (!World) {
    return Result;
  }

  // 🔧 修改 - 使用胶囊半高，确保底部贴地
  float CapsuleHalfHeight = 88.0f;
  if (const UCapsuleComponent *Capsule = GetCapsuleComponent()) {
    CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
  }

  // 🔧 修改 - 按配置构造自上而下的射线
  FVector TraceStart =
      FVector(DesiredLocation.X, DesiredLocation.Y,
              DesiredLocation.Z + ArcConfig.GroundTraceUpDistance);
  FVector TraceEnd =
      FVector(DesiredLocation.X, DesiredLocation.Y,
              DesiredLocation.Z - ArcConfig.GroundTraceDownDistance);

  FCollisionQueryParams QueryParams(
      SCENE_QUERY_STAT(ComputeGroundSnappedLocation), false, this);
  FHitResult HitResult;
  if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd,
                                      ECC_WorldStatic, QueryParams)) {
    // 在检测命中时，补上半高与额外偏移
    Result =
        HitResult.Location +
        FVector(0.0f, 0.0f, CapsuleHalfHeight + ArcConfig.LandingExtraZOffset);
  }

  return Result;
}

/**
 * @brief 绘制掉落抛物线调试轨迹
 * @param DurationOverride 调试持续时间（<0 使用配置）
 * @param SegmentOverride 调试段数（<=0 使用配置）
 * @note ✨ 新增 - 可视化轨迹便于蓝图调参
 */
void AXBSoldierCharacter::DrawDropDebugArc(float DurationOverride,
                                           int32 SegmentOverride) const {
  // 仅在需要时绘制，避免无意义的性能开销
  if (!ActiveDropArcConfig.bEnableDebugDraw && DurationOverride < 0.0f &&
      SegmentOverride <= 0) {
    return;
  }

  UWorld *World = GetWorld();
  if (!World) {
    return;
  }

  const int32 SegmentCount =
      SegmentOverride > 0 ? SegmentOverride
                          : FMath::Max(ActiveDropArcConfig.DebugArcSegments, 2);
  const float Step = 1.0f / static_cast<float>(SegmentCount);
  const float DrawDuration = DurationOverride >= 0.0f
                                 ? DurationOverride
                                 : ActiveDropArcConfig.DebugDrawDuration;
  const FColor DebugColor = ActiveDropArcConfig.DebugArcColor.ToFColor(true);
  const float DebugPointRadius = ActiveDropArcConfig.DebugPointSize * 0.5f;

  // 🔧 修改 - 逐段采样，确保曲线连续
  for (int32 Index = 0; Index <= SegmentCount; ++Index) {
    float CurrentProgress = FMath::Clamp(Step * Index, 0.0f, 1.0f);
    FVector CurrentPoint = CalculateArcPosition(CurrentProgress);

    DrawDebugSphere(World, CurrentPoint, DebugPointRadius, 8, DebugColor, false,
                    DrawDuration);

    if (Index > 0) {
      float PrevProgress = FMath::Clamp(Step * (Index - 1), 0.0f, 1.0f);
      FVector PrevPoint = CalculateArcPosition(PrevProgress);

      DrawDebugLine(World, PrevPoint, CurrentPoint, DebugColor, false,
                    DrawDuration, 0, 1.5f);
    }
  }
}

/**
 * @brief 处理落地
 * @note 🔧 修改 - 正确恢复移动组件，让物理系统控制贴地
 */
void AXBSoldierCharacter::OnDropLanded() {
  UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 掉落落地"), *GetName());

  // 🔧 修改 - 落地点再次对齐地面，避免悬空
  DropTargetLocation =
      ComputeGroundSnappedLocation(DropTargetLocation, ActiveDropArcConfig);

  // 设置落地位置
  SetActorLocation(DropTargetLocation);

  // ✨ Step 1: 恢复碰撞
  if (UCapsuleComponent *Capsule = GetCapsuleComponent()) {
    Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Capsule->SetCollisionProfileName(XBCollision::SoldierPresetName);
  }

  // ✨ Step 2: 恢复移动组件（让物理系统接管）
  if (UCharacterMovementComponent *MoveComp = GetCharacterMovement()) {
    MoveComp->GravityScale = 1.0f;
    MoveComp->SetComponentTickEnabled(true);
    MoveComp->SetMovementMode(
        MOVE_Falling); // 🔧 修改 - 先设为 Falling，让物理检测地面
    MoveComp->MaxWalkSpeed = GetMoveSpeed();

    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: 移动组件已恢复"), *GetName());
  }

  // 播放落地特效
  if (bPlayDropLandingEffect) {
    PlayLandingEffect();
  }

  // ✨ Step 3: 延迟处理入列，等物理稳定
  if (bAutoRecruitOnLanding && DropTargetLeader.IsValid()) {
    const float AutoRecruitDelay =
        FMath::Max(0.0f, ActiveDropArcConfig.AutoRecruitDelay);
    if (AutoRecruitDelay <= KINDA_SMALL_NUMBER) {
      AutoRecruitToLeader();
    } else {
      FTimerHandle TimerHandle;
      GetWorldTimerManager().SetTimer(TimerHandle, this,
                                      &AXBSoldierCharacter::AutoRecruitToLeader,
                                      AutoRecruitDelay, false);
    }
  } else {
    SetSoldierState(EXBSoldierState::Idle);
    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 落地后进入待机态"), *GetName());
  }

  // 广播落地完成事件
  OnDropLandingComplete.Broadcast(this);
}

/**
 * @brief 落地后自动加入将领队伍
 * @note 🔧 修复版本 -
 *       1. 修正调用顺序：先 AddSoldier，后设置本地状态
 *       2. 确保槽位索引正确获取
 *       3. 正确启动跟随过渡
 */
void AXBSoldierCharacter::AutoRecruitToLeader() {
  AXBCharacterBase *Leader = DropTargetLeader.Get();
  if (!Leader || !IsValid(Leader)) {
    UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: 自动入列失败 - 目标将领无效"),
           *GetName());
    Faction = EXBFaction::Neutral;
    bIsRecruited = false;
    FollowTarget = nullptr;
    SetSoldierState(EXBSoldierState::Idle);
    return;
  }

  if (Leader->IsDead()) {
    UE_LOG(LogXBSoldier, Warning,
           TEXT("士兵 %s: 自动入列失败 - 目标将领已死亡"), *GetName());
    Faction = EXBFaction::Neutral;
    bIsRecruited = false;
    FollowTarget = nullptr;
    SetSoldierState(EXBSoldierState::Idle);
    return;
  }

  // 检查是否已在该将领的队伍中（可能被磁场提前招募）
  const TArray<AXBSoldierCharacter *> &LeaderSoldiers = Leader->GetSoldiers();
  int32 ExistingIndex = LeaderSoldiers.Find(this);

  if (ExistingIndex != INDEX_NONE) {
    UE_LOG(LogXBSoldier, Log,
           TEXT("士兵 %s: 已在将领队伍中（索引: %d），同步状态并开始移动"),
           *GetName(), ExistingIndex);

    // 同步状态
    bIsRecruited = true;
    FollowTarget = Leader;
    Faction = Leader->GetFaction();
    FormationSlotIndex = ExistingIndex;

    // 配置并启动跟随
    SetupFollowingAndStartMoving(Leader, ExistingIndex);
    return;
  }

  // 如果跟随其他将领，需要先离开
  if (FollowTarget.IsValid() && FollowTarget.Get() != Leader) {
    if (AXBCharacterBase *OldLeader =
            Cast<AXBCharacterBase>(FollowTarget.Get())) {
      OldLeader->RemoveSoldier(this);
    }
    FollowTarget = nullptr;
    FormationSlotIndex = INDEX_NONE;
  }

  UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 开始自动入列到 %s"), *GetName(),
         *Leader->GetName());

  // 添加到将领
  Leader->AddSoldier(this);

  // 获取分配的槽位
  int32 SlotIndex = FormationSlotIndex;

  // 设置本地状态
  bIsRecruited = true;
  FollowTarget = Leader;
  Faction = Leader->GetFaction();

  // 配置并启动跟随
  SetupFollowingAndStartMoving(Leader, SlotIndex);

  OnSoldierRecruited.Broadcast(this, Leader);

  UE_LOG(LogXBSoldier, Log, TEXT("========================================"));
  UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 自动入列完成，槽位: %d"), *GetName(),
         SlotIndex);
  UE_LOG(LogXBSoldier, Log, TEXT("将领最终士兵数: %d"),
         Leader->GetSoldierCount());
  UE_LOG(LogXBSoldier, Log, TEXT("========================================"));
  UE_LOG(LogXBSoldier, Log, TEXT(""));
}

/**
 * @brief 获取用于动画的移动速度
 * @return 当前移动速度
 * @note ✨ 新增 - 核心逻辑：
 *       1. 未招募 → 返回0
 *       2. 招募过渡中 → 返回0（避免过渡期间动画异常）
 *       3. 锁定跟随/战斗状态且已到位 → 返回实际速度
 */
float AXBSoldierCharacter::GetAnimationMoveSpeed() const {
  // 条件1：必须已被招募
  if (!bIsRecruited) {
    return 0.0f;
  }

  // 条件2：必须处于跟随或战斗状态
  if (CurrentState != EXBSoldierState::Following &&
      CurrentState != EXBSoldierState::Combat) {
    return 0.0f;
  }

  // 条件3：检查跟随组件状态
  if (FollowComponent) {
    EXBFollowMode FollowMode = FollowComponent->GetFollowMode();

    // 招募过渡中不返回速度（避免过渡动画）
    if (FollowMode == EXBFollowMode::RecruitTransition) {
      return 0.0f;
    }

    // 锁定模式或自由模式（战斗）：返回实际速度
    if (FollowMode == EXBFollowMode::Locked ||
        FollowMode == EXBFollowMode::Free) {
      // 使用跟随组件缓存的移动速度
      return FollowComponent->GetCurrentMoveSpeed();
    }
  }

  // 回退：使用移动组件速度
  if (UCharacterMovementComponent *MoveComp = GetCharacterMovement()) {
    FVector Velocity = MoveComp->Velocity;
    Velocity.Z = 0.0f;
    return Velocity.Size();
  }

  return 0.0f;
}

/**
 * @brief 检查是否应该播放移动动画
 * @return 是否应该播放
 * @note ✨ 新增 - 简化版判断，供动画蓝图使用
 */
bool AXBSoldierCharacter::ShouldPlayMoveAnimation() const {
  // 必须已招募
  if (!bIsRecruited) {
    return false;
  }

  // 必须处于正确状态
  if (CurrentState != EXBSoldierState::Following &&
      CurrentState != EXBSoldierState::Combat) {
    return false;
  }

  // 必须不在招募过渡中
  if (FollowComponent &&
      FollowComponent->GetFollowMode() == EXBFollowMode::RecruitTransition) {
    return false;
  }

  // 速度大于阈值才播放
  return GetAnimationMoveSpeed() > 10.0f;
}

/**
 * @brief 配置跟随组件并开始移动
 * @param Leader 将领
 * @param SlotIndex 槽位索引
 * @note ✨ 新增 - 抽取公共逻辑
 */
void AXBSoldierCharacter::SetupFollowingAndStartMoving(AXBCharacterBase *Leader,
                                                       int32 SlotIndex) {
  // 确保移动组件正确配置
  if (UCharacterMovementComponent *MoveComp = GetCharacterMovement()) {
    MoveComp->GravityScale = 1.0f;
    MoveComp->SetComponentTickEnabled(true);
    MoveComp->SetMovementMode(MOVE_Walking);
    MoveComp->MaxWalkSpeed = GetMoveSpeed();
  }

  // 确保碰撞启用
  if (UCapsuleComponent *Capsule = GetCapsuleComponent()) {
    Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
  }

  // 配置跟随组件
  if (FollowComponent) {
    FollowComponent->SetComponentTickEnabled(true);
    FollowComponent->SetFollowTarget(Leader);
    FollowComponent->SetFormationSlotIndex(SlotIndex);
    FollowComponent->SyncLeaderSprintState(Leader->IsSprinting(),
                                           Leader->GetCurrentMoveSpeed());
    FollowComponent->StartRecruitTransition();

    FVector TargetPos = FollowComponent->GetTargetPosition();
    UE_LOG(LogXBSoldier, Log,
           TEXT("士兵 %s: 开始移动到槽位 %d，目标: (%.1f, %.1f, %.1f)"),
           *GetName(), SlotIndex, TargetPos.X, TargetPos.Y, TargetPos.Z);
  }

  // 🔧 修改 - 绑定编队更新事件，确保队形变化时触发平滑补位
  if (Leader) {
    BindLeaderFormationEvents(Leader);
  }

  if (BehaviorInterface) {
    BehaviorInterface->SetComponentTickEnabled(true);
  }

  // 设置状态
  SetSoldierState(EXBSoldierState::Following);

  // 延迟启动 AI
  GetWorldTimerManager().SetTimer(
      DelayedAIStartTimerHandle, this,
      &AXBSoldierCharacter::SpawnAndPossessAIController, 0.3f, false);
}

/**
 * @brief 当槽位变化时触发补位移动
 * @param bForceRecruitTransition 是否强制使用招募过渡模式
 * @note ✨ 新增 - 统一补位逻辑，避免瞬移
 */
void AXBSoldierCharacter::RequestRelocateToSlot(bool bForceRecruitTransition) {
  // 🔧 修改 - 仅在跟随或待机状态下执行补位
  if (CurrentState != EXBSoldierState::Following &&
      CurrentState != EXBSoldierState::Idle) {
    return;
  }

  if (FollowComponent) {
    // 使用招募过渡模式移动到新槽位
    if (bForceRecruitTransition ||
        FollowComponent->GetFollowMode() != EXBFollowMode::RecruitTransition) {
      FollowComponent->StartRecruitTransition();
    }
  }
}

void AXBSoldierCharacter::PlayLandingEffect() {
  if (DropLandingEffectAsset.IsNull()) {
    return;
  }

  UNiagaraSystem *EffectSystem = DropLandingEffectAsset.LoadSynchronous();
  if (!EffectSystem) {
    return;
  }

  UWorld *World = GetWorld();
  if (!World) {
    return;
  }

  UNiagaraFunctionLibrary::SpawnSystemAtLocation(
      World, EffectSystem, DropTargetLocation, FRotator::ZeroRotator,
      FVector::OneVector, true, true, ENCPoolMethod::AutoRelease);

  UE_LOG(LogXBSoldier, Verbose, TEXT("士兵 %s 播放落地特效"), *GetName());
}

// ==================== 休眠系统实现 ====================

void AXBSoldierCharacter::EnterDormantState(EXBDormantType DormantType) {
  if (CurrentState == EXBSoldierState::Dormant) {
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

  if (DormantType != EXBDormantType::Hidden) {
    SetActorHiddenInGame(false);
    UpdateDormantAnimation();
    UpdateZzzEffect();
  } else {
    SetActorHiddenInGame(true);
    SetZzzEffectEnabled(false);
  }

  OnSoldierStateChanged.Broadcast(OldState, CurrentState);
  OnDormantStateChanged.Broadcast(this, true);

  UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 进入休眠态，类型: %d"), *GetName(),
         static_cast<int32>(DormantType));
}

void AXBSoldierCharacter::ExitDormantState() {
  if (CurrentState != EXBSoldierState::Dormant) {
    return;
  }

  EXBSoldierState OldState = CurrentState;
  CurrentState = EXBSoldierState::Idle;

  SetZzzEffectEnabled(false);

  if (USkeletalMeshComponent *MeshComp = GetMesh()) {
    MeshComp->Stop();
  }

  SetActorHiddenInGame(false);

  EnableActiveComponents();

  OnSoldierStateChanged.Broadcast(OldState, CurrentState);
  OnDormantStateChanged.Broadcast(this, false);

  UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 退出休眠态"), *GetName());
}

void AXBSoldierCharacter::SetDormantVisualConfig(
    const FXBDormantVisualConfig &NewConfig) {
  DormantConfig = NewConfig;

  if (ZzzEffectComponent) {
    ZzzEffectComponent->SetRelativeLocation(DormantConfig.ZzzEffectOffset);
  }

  if (CurrentState == EXBSoldierState::Dormant) {
    UpdateDormantAnimation();
    UpdateZzzEffect();
  }
}

void AXBSoldierCharacter::SetZzzEffectEnabled(bool bEnabled) {
  if (!ZzzEffectComponent) {
    return;
  }

  if (bEnabled) {
    ZzzEffectComponent->Activate(true);
  } else {
    ZzzEffectComponent->Deactivate();
  }
}

void AXBSoldierCharacter::SetDormantType(EXBDormantType NewType) {
  if (CurrentDormantType == NewType) {
    return;
  }

  CurrentDormantType = NewType;

  if (CurrentState == EXBSoldierState::Dormant) {
    if (NewType == EXBDormantType::Hidden) {
      SetActorHiddenInGame(true);
      SetZzzEffectEnabled(false);
    } else {
      SetActorHiddenInGame(false);
      UpdateDormantAnimation();
      UpdateZzzEffect();
    }
  }

  UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 休眠类型切换为: %d"), *GetName(),
         static_cast<int32>(NewType));
}

void AXBSoldierCharacter::EnableActiveComponents() {
  SetActorTickEnabled(true);

  if (UCharacterMovementComponent *MoveComp = GetCharacterMovement()) {
    MoveComp->SetComponentTickEnabled(true);
    MoveComp->SetMovementMode(MOVE_Walking);
  }

  if (UCapsuleComponent *Capsule = GetCapsuleComponent()) {
    Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
  }

  if (FollowComponent) {
    FollowComponent->SetComponentTickEnabled(true);
  }

  if (BehaviorInterface) {
    BehaviorInterface->SetComponentTickEnabled(true);
  }

  if (DebugComponent) {
    DebugComponent->SetComponentTickEnabled(true);
  }

  UE_LOG(LogXBSoldier, Verbose, TEXT("士兵 %s: 激活态组件已启用"), *GetName());
}

void AXBSoldierCharacter::DisableActiveComponents() {
  if (UCharacterMovementComponent *MoveComp = GetCharacterMovement()) {
    MoveComp->StopMovementImmediately();
    MoveComp->SetComponentTickEnabled(false);
  }

  if (UCapsuleComponent *Capsule = GetCapsuleComponent()) {
    Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
  }

  if (FollowComponent) {
    FollowComponent->SetFollowTarget(nullptr);
    FollowComponent->SetComponentTickEnabled(false);
  }

  if (BehaviorInterface) {
    BehaviorInterface->SetComponentTickEnabled(false);
  }

  if (AController *CurrentController = GetController()) {
    CurrentController->UnPossess();
  }

  FollowTarget = nullptr;
  FormationSlotIndex = INDEX_NONE;
  CurrentAttackTarget = nullptr;
  GetWorldTimerManager().ClearTimer(TargetRequestTimerHandle);
  UnbindAssignedTargetEvents();

  UE_LOG(LogXBSoldier, Verbose, TEXT("士兵 %s: 激活态组件已禁用"), *GetName());
}

void AXBSoldierCharacter::UpdateDormantAnimation() {
  UAnimSequence *AnimToPlay = nullptr;

  switch (CurrentDormantType) {
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

void AXBSoldierCharacter::UpdateZzzEffect() {
  bool bShouldShowZzz = (CurrentDormantType == EXBDormantType::Sleeping) &&
                        DormantConfig.bShowZzzEffect;

  SetZzzEffectEnabled(bShouldShowZzz);
}

void AXBSoldierCharacter::PlayAnimationSequence(UAnimSequence *Animation,
                                                bool bLoop) {
  USkeletalMeshComponent *MeshComp = GetMesh();
  if (!MeshComp) {
    return;
  }

  if (Animation) {
    MeshComp->PlayAnimation(Animation, bLoop);
  } else {
    MeshComp->Stop();
  }
}

void AXBSoldierCharacter::LoadDormantAnimations() {
  if (!DormantConfig.SleepingAnimation.IsNull()) {
    LoadedSleepingAnimation = DormantConfig.SleepingAnimation.LoadSynchronous();
  }

  if (!DormantConfig.StandingAnimation.IsNull()) {
    LoadedStandingAnimation = DormantConfig.StandingAnimation.LoadSynchronous();
  }
}

// ==================== 数据访问器接口 ====================

bool AXBSoldierCharacter::IsDataAccessorValid() const {
  return DataAccessor && DataAccessor->IsInitialized();
}

void AXBSoldierCharacter::InitializeFromDataTable(UDataTable *DataTable,
                                                  FName RowName,
                                                  EXBFaction InFaction) {
  if (!DataTable) {
    UE_LOG(LogXBSoldier, Error, TEXT("士兵初始化失败: 数据表为空"));
    return;
  }

  if (RowName.IsNone()) {
    UE_LOG(LogXBSoldier, Error, TEXT("士兵初始化失败: 行名为空"));
    return;
  }

  if (!DataAccessor) {
    UE_LOG(LogXBSoldier, Error,
           TEXT("士兵初始化失败: DataAccessor 组件未创建"));
    return;
  }

  bool bInitSuccess = DataAccessor->Initialize(
      DataTable, RowName, EXBResourceLoadStrategy::Synchronous);

  if (!bInitSuccess) {
    UE_LOG(LogXBSoldier, Error,
           TEXT("士兵初始化失败: DataAccessor 初始化失败"));
    return;
  }

  // 🔧 修改 - 应用运行时配置（倍率/覆盖值）以保证数据驱动一致
  if (const UXBGameInstance *GameInstance =
          GetGameInstance<UXBGameInstance>()) {
    ApplyRuntimeConfig(GameInstance->GetGameConfig());
  } else {
    // 🔧 修改 - 无 GameInstance 时回退默认倍率
    CachedHealthMultiplier = 1.0f;
    CachedDamageMultiplier = 1.0f;
    CachedHealthOverride = 0.0f;
    CurrentHealth = GetMaxHealth();
  }

  SoldierType = DataAccessor->GetSoldierType();
  Faction = InFaction;

  // 🔧 修改 - 弓手初始化发射物配置，便于动画通知读取
  if (SoldierType == EXBSoldierType::Archer) {
    ProjectileConfig = DataAccessor->GetProjectileConfig();
    UE_LOG(LogXBSoldier, Log, TEXT("弓手 %s 载入发射物配置，投射物类=%s"),
           *GetName(),
           ProjectileConfig.ProjectileClass
               ? *ProjectileConfig.ProjectileClass->GetName()
               : TEXT("未配置"));

    // ✨ 新增 - 预加载弓手投射物到对象池
    if (ProjectileConfig.ProjectileClass && GetWorld()) {
      if (UXBProjectilePoolSubsystem *PoolSubsystem =
              GetWorld()->GetSubsystem<UXBProjectilePoolSubsystem>()) {
        PoolSubsystem->PrewarmProjectiles(ProjectileConfig.ProjectileClass,
                                          ProjectileConfig.PreloadCount);
      }
    }
  } else {
    ProjectileConfig = FXBProjectileConfig();
  }

  if (UCharacterMovementComponent *MovementComp = GetCharacterMovement()) {
    MovementComp->MaxWalkSpeed = DataAccessor->GetMoveSpeed();
    MovementComp->RotationRate =
        FRotator(0.0f, DataAccessor->GetRotationSpeed(), 0.0f);
  }

  if (FollowComponent) {
    FollowComponent->SetFollowSpeed(DataAccessor->GetMoveSpeed());
  }

  BehaviorTreeAsset = DataAccessor->GetBehaviorTree();
  ApplyVisualConfig();

  // 🔧 修改 - 数据表初始化完成后刷新近战GA配置
  RefreshMeleeHitAbilityFromData();

  UE_LOG(LogXBSoldier, Log, TEXT("士兵初始化成功: %s (类型=%s, 血量=%.1f)"),
         *RowName.ToString(), *UEnum::GetValueAsString(SoldierType),
         CurrentHealth);
}

void AXBSoldierCharacter::ApplyVisualConfig() {
  if (!IsDataAccessorValid()) {
    return;
  }

  USkeletalMesh *SoldierMesh = DataAccessor->GetSkeletalMesh();
  if (SoldierMesh) {
    GetMesh()->SetSkeletalMesh(SoldierMesh);

    // ✨ 新增 - 自适应胶囊体大小
    // 1. 获取网格体边界
    FBoxSphereBounds MeshBounds = SoldierMesh->GetBounds();
    FVector BoxExtent = MeshBounds.BoxExtent;

    // 2. 计算新尺寸
    // Radius取X/Y最大值的0.6倍以适配人体圆柱 (避免过宽导致穿模或无法通过)
    float NewRadius = FMath::Max(BoxExtent.X, BoxExtent.Y) * 0.6f;
    // HalfHeight直接取Z轴范围的一半
    float NewHalfHeight = BoxExtent.Z;

    // 3. 设定最小/最大限制防止异常
    NewRadius = FMath::Clamp(NewRadius, 15.0f, 60.0f);
    NewHalfHeight = FMath::Max(NewHalfHeight, 30.0f);

    if (UCapsuleComponent *Capsule = GetCapsuleComponent()) {
      Capsule->SetCapsuleSize(NewRadius, NewHalfHeight);

      // 4. 调整Mesh位置到底部对齐
      // 胶囊体原点在中心，Mesh原点在脚底，所以Z轴偏移为 -HalfHeight
      GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -NewHalfHeight));
      UE_LOG(LogXBSoldier, Log,
             TEXT("士兵 %s 胶囊体自适应调整: MeshBounds=%.1f, Radius=%.1f, "
                  "HalfHeight=%.1f"),
             *GetName(), BoxExtent.Z * 2.0f, NewRadius, NewHalfHeight);
    }
  }

  TSubclassOf<UAnimInstance> AnimClass = DataAccessor->GetAnimClass();
  if (AnimClass) {
    GetMesh()->SetAnimInstanceClass(AnimClass);
  }

  float MeshScale = DataAccessor->GetRawData().VisualConfig.MeshScale;
  if (!FMath::IsNearlyEqual(MeshScale, 1.0f)) {
    SetActorScale3D(FVector(MeshScale));
  }
}

// ==================== 配置属性访问 ====================

FText AXBSoldierCharacter::GetDisplayName() const {
  return IsDataAccessorValid() ? DataAccessor->GetDisplayName()
                               : FText::FromString(TEXT("未命名士兵"));
}

FGameplayTagContainer AXBSoldierCharacter::GetSoldierTags() const {
  return IsDataAccessorValid() ? DataAccessor->GetSoldierTags()
                               : FGameplayTagContainer();
}

float AXBSoldierCharacter::GetMaxHealth() const {
  const float BaseHealth =
      CachedHealthOverride > 0.0f
          ? CachedHealthOverride
          : (IsDataAccessorValid() ? DataAccessor->GetMaxHealth() : 100.0f);
  return BaseHealth * CachedHealthMultiplier;
}

float AXBSoldierCharacter::GetBaseDamage() const {
  const float BaseDamage =
      IsDataAccessorValid() ? DataAccessor->GetBaseDamage() : 10.0f;
  return BaseDamage * CachedDamageMultiplier;
}

void AXBSoldierCharacter::ApplyRuntimeConfig(
    const FXBGameConfigData &GameConfig) {
  // 🔧 修改 - 缓存倍率/覆盖值，确保运行时一致
  CachedHealthMultiplier =
      FMath::Max(0.01f, GameConfig.SoldierHealthMultiplier);
  CachedDamageMultiplier =
      FMath::Max(0.01f, GameConfig.SoldierDamageMultiplier);

  // 🔧 修改 - 以配置初始化士兵大小，确保出生尺寸一致
  const float InitialScale = FMath::Max(0.01f, GameConfig.SoldierInitialScale);
  const float BaseScale =
      IsDataAccessorValid() ? DataAccessor->GetRawData().VisualConfig.MeshScale
                            : 1.0f;
  SetActorScale3D(FVector(BaseScale * InitialScale));

  // 🔧 修改 - 刷新当前血量为新上限，避免配置变化导致负值
  CurrentHealth = GetMaxHealth();
}

float AXBSoldierCharacter::GetAttackRange() const {
  return IsDataAccessorValid() ? DataAccessor->GetAttackRange() : 150.0f;
}

float AXBSoldierCharacter::GetAttackInterval() const {
  if (!IsDataAccessorValid()) {
    return 1.0f;
  }

  // 🔧 修改 -
  // 优先使用数据表中普攻Cooldown，避免AttackInterval与技能冷却语义冲突
  const float BasicAttackCooldown = DataAccessor->GetBasicAttackCooldown();
  if (BasicAttackCooldown > 0.0f) {
    return BasicAttackCooldown;
  }

  return DataAccessor->GetAttackInterval();
}

float AXBSoldierCharacter::GetMoveSpeed() const {
  return IsDataAccessorValid() ? DataAccessor->GetMoveSpeed() : 400.0f;
}

float AXBSoldierCharacter::GetSprintSpeedMultiplier() const {
  return IsDataAccessorValid() ? DataAccessor->GetSprintSpeedMultiplier()
                               : 2.0f;
}

float AXBSoldierCharacter::GetFollowInterpSpeed() const {
  return IsDataAccessorValid() ? DataAccessor->GetFollowInterpSpeed() : 5.0f;
}

float AXBSoldierCharacter::GetRotationSpeed() const {
  return IsDataAccessorValid() ? DataAccessor->GetRotationSpeed() : 360.0f;
}

float AXBSoldierCharacter::GetVisionRange() const {
  return IsDataAccessorValid() ? DataAccessor->GetVisionRange() : 800.0f;
}

float AXBSoldierCharacter::GetDisengageDistance() const {
  return IsDataAccessorValid() ? DataAccessor->GetDisengageDistance() : 1000.0f;
}

float AXBSoldierCharacter::GetReturnDelay() const {
  return IsDataAccessorValid() ? DataAccessor->GetReturnDelay() : 2.0f;
}

float AXBSoldierCharacter::GetArrivalThreshold() const {
  return IsDataAccessorValid() ? DataAccessor->GetArrivalThreshold() : 50.0f;
}

float AXBSoldierCharacter::GetAvoidanceRadius() const {
  return IsDataAccessorValid() ? DataAccessor->GetAvoidanceRadius() : 50.0f;
}

float AXBSoldierCharacter::GetAvoidanceWeight() const {
  return IsDataAccessorValid() ? DataAccessor->GetAvoidanceWeight() : 0.3f;
}

// ==================== 招募系统 ====================

/**
 * @brief 检查士兵是否可以被招募
 * @return 是否可招募
 * @note 🔧 修改 - 增加更多状态检查，防止掉落中或已入列的士兵被磁场抢走
 */
bool AXBSoldierCharacter::CanBeRecruited() const {
  // 已招募
  if (bIsRecruited) {
    return false;
  }

  // 已有跟随目标
  if (FollowTarget.IsValid()) {
    return false;
  }

  // 非中立阵营（已属于某个阵营）
  if (Faction != EXBFaction::Neutral) {
    return false;
  }

  // 掉落中不可招募（由抛物线系统控制入列）
  if (CurrentState == EXBSoldierState::Dropping) {
    return false;
  }

  // ✨ 新增 - 跟随状态不可招募（已在某个队伍中）
  if (CurrentState == EXBSoldierState::Following) {
    return false;
  }

  // ✨ 新增 - 战斗状态不可招募
  if (CurrentState == EXBSoldierState::Combat) {
    return false;
  }

  // 必须处于休眠态或待机态
  if (CurrentState != EXBSoldierState::Dormant &&
      CurrentState != EXBSoldierState::Idle) {
    return false;
  }

  // 已死亡
  if (bIsDead || CurrentHealth <= 0.0f) {
    return false;
  }

  // 组件未初始化
  if (!bComponentsInitialized) {
    return false;
  }

  return true;
}

/**
 * @brief 被招募回调
 * @param NewLeader 新将领
 * @param SlotIndex 槽位索引
 * @note 🔧 修改 - 消除招募延迟，立即开始移动
 *       1. 移除AI启动延迟对移动的阻塞
 *       2. 立即配置跟随组件并开始移动
 *       3. AI控制器仍可延迟初始化（不影响移动）
 */
void AXBSoldierCharacter::OnRecruited(AActor *NewLeader, int32 SlotIndex) {

  if (!NewLeader) {
    UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: 招募失败 - 将领为空"),
           *GetName());
    return;
  }

  if (bIsRecruited) {
    UE_LOG(LogXBSoldier, Warning,
           TEXT("士兵 %s: 已被招募，忽略来自 %s 的重复招募请求"), *GetName(),
           *NewLeader->GetName());
    return;
  }

  if (FollowTarget.IsValid() && FollowTarget.Get() != NewLeader) {
    UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: 已跟随 %s，拒绝 %s 的招募"),
           *GetName(), *FollowTarget->GetName(), *NewLeader->GetName());
    return;
  }

  if (!bComponentsInitialized) {
    UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: 组件未初始化，延迟招募"),
           *GetName());
    FTimerHandle TempHandle;
    GetWorldTimerManager().SetTimer(
        TempHandle,
        [this, NewLeader, SlotIndex]() { OnRecruited(NewLeader, SlotIndex); },
        0.1f, false);
    return;
  }

  UE_LOG(LogXBSoldier, Log,
         TEXT("士兵 %s: 被将领 %s 招募，槽位: %d，当前状态: %d"), *GetName(),
         *NewLeader->GetName(), SlotIndex, static_cast<int32>(CurrentState));

  // ✨ 核心修改 - 立即设置招募状态
  bIsRecruited = true;
  FollowTarget = NewLeader;
  FormationSlotIndex = SlotIndex;

  // 退出休眠态（如果处于休眠）
  if (CurrentState == EXBSoldierState::Dormant) {
    ExitDormantState();
  }

  // 设置阵营
  AXBCharacterBase *LeaderChar = Cast<AXBCharacterBase>(NewLeader);
  if (LeaderChar) {
    Faction = LeaderChar->GetFaction();
  } else {
    UE_LOG(LogXBSoldier, Warning,
           TEXT("士兵 %s: 招募目标 %s 不是 AXBCharacterBase，无法同步阵营"),
           *GetName(), *NewLeader->GetName());
  }

  // 🔧 修改 - 面向将领
  FVector DirectionToLeader =
      (NewLeader->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
  if (!DirectionToLeader.IsNearlyZero()) {
    SetActorRotation(DirectionToLeader.Rotation());
  }

  // ✨ 核心修改 - 统一调用跟随配置入口，确保事件绑定/移动组件一致
  if (LeaderChar) {
    SetupFollowingAndStartMoving(LeaderChar, SlotIndex);
  } else {
    // 🔧 修改 - 兜底：无主将类型时保持原始跟随逻辑，避免空指针
    if (FollowComponent) {
      FollowComponent->SetComponentTickEnabled(true);
      FollowComponent->SetFollowTarget(NewLeader);
      FollowComponent->SetFormationSlotIndex(SlotIndex);
      FollowComponent->StartRecruitTransition();
    }
  }

  // 广播事件
  OnSoldierRecruited.Broadcast(this, NewLeader);

  UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: 招募完成，立即开始移动到槽位 %d"),
         *GetName(), SlotIndex);
}

// ==================== 跟随系统 ====================

void AXBSoldierCharacter::SetFollowTarget(AActor *NewLeader, int32 SlotIndex) {
  FollowTarget = NewLeader;
  FormationSlotIndex = SlotIndex;

  // 🔧 修改 - 跟随目标切换时同步编队事件绑定
  if (NewLeader) {
    BindLeaderFormationEvents(Cast<AXBCharacterBase>(NewLeader));
  } else {
    UnbindLeaderFormationEvents();
  }

  if (FollowComponent) {
    FollowComponent->SetFollowTarget(NewLeader);
    FollowComponent->SetFormationSlotIndex(SlotIndex);
    RequestRelocateToSlot(true);
  }

  if (AAIController *AICtrl = Cast<AAIController>(GetController())) {
    if (UBlackboardComponent *BBComp = AICtrl->GetBlackboardComponent()) {
      BBComp->SetValueAsObject(TEXT("Leader"), NewLeader);
      BBComp->SetValueAsInt(TEXT("FormationSlot"), SlotIndex);
    }
  }

  if (NewLeader) {
    SetSoldierState(EXBSoldierState::Following);
  } else {
    SetSoldierState(EXBSoldierState::Idle);
  }
}

void AXBSoldierCharacter::SetLeaderCharacter(AXBCharacterBase *NewLeader) {
  SetFollowTarget(NewLeader, INDEX_NONE);
}

AXBCharacterBase *AXBSoldierCharacter::GetLeaderCharacter() const {
  return Cast<AXBCharacterBase>(FollowTarget.Get());
}

void AXBSoldierCharacter::SetFormationSlotIndex(int32 NewIndex) {
  int32 OldIndex = FormationSlotIndex;
  FormationSlotIndex = NewIndex;

  if (FollowComponent) {
    FollowComponent->SetFormationSlotIndex(NewIndex);

    // 🔧 修改 - 槽位变更时使用招募过渡移动而非瞬移
    if (OldIndex != NewIndex && CurrentState == EXBSoldierState::Following) {
      RequestRelocateToSlot(true);
    }
  }

  if (AAIController *AICtrl = Cast<AAIController>(GetController())) {
    if (UBlackboardComponent *BBComp = AICtrl->GetBlackboardComponent()) {
      BBComp->SetValueAsInt(TEXT("FormationSlot"), NewIndex);
    }
  }
}

// ==================== 状态管理 ====================

void AXBSoldierCharacter::SetSoldierState(EXBSoldierState NewState) {
  if (CurrentState == NewState) {
    return;
  }

  EXBSoldierState OldState = CurrentState;
  CurrentState = NewState;

  // 🔧 修改 - 跟随状态下强制关闭行为树，避免非战斗逻辑运行
  if (NewState == EXBSoldierState::Following) {
    if (AXBSoldierAIController *SoldierAI =
            Cast<AXBSoldierAIController>(GetController())) {
      SoldierAI->StopBehaviorTreeLogic();
      UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 跟随状态停止行为树"), *GetName());
    }
  }

  if (AAIController *AICtrl = Cast<AAIController>(GetController())) {
    if (UBlackboardComponent *BBComp = AICtrl->GetBlackboardComponent()) {
      BBComp->SetValueAsInt(TEXT("SoldierState"), static_cast<int32>(NewState));
    }
  }

  // 🔧 修改 - RVO 避让系统仅在战斗状态下开启，避免跟随/入列时的队列抖动
  if (UCharacterMovementComponent *MoveComp = GetCharacterMovement()) {
    // 仅战斗状态启用 RVO
    const bool bShouldEnableRVO = (NewState == EXBSoldierState::Combat);

    // 如果避让系统被全局禁用(bEnableAvoidanceSystem=false)，则永远不开启
    // 但这里 bUseRVOAvoidance 是底层物理的开关，我们可以先控制它
    // 注意：bEnableAvoidanceSystem 是用来控制 CalculateAvoidanceDirection
    // 的自定义逻辑 CharacterMovement 的 RVOAvoidance 是另一套系统

    if (MoveComp->bUseRVOAvoidance != bShouldEnableRVO) {
      MoveComp->SetAvoidanceEnabled(bShouldEnableRVO);
      UE_LOG(LogXBSoldier, Verbose,
             TEXT("士兵 %s 切换RVO避让状态: %s (State=%s)"), *GetName(),
             bShouldEnableRVO ? TEXT("开启") : TEXT("关闭"),
             *UEnum::GetValueAsString(NewState));
    }
  }

  OnSoldierStateChanged.Broadcast(OldState, NewState);

  UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 状态变化: %d -> %d"), *GetName(),
         static_cast<int32>(OldState), static_cast<int32>(NewState));
}

// ==================== 战斗系统 ====================

void AXBSoldierCharacter::EnterCombat() {
  // 🔧 修改 - 主将在草丛中时，士兵强制保持跟随状态
  if (AXBCharacterBase *Leader = GetLeaderCharacter()) {
    if (Leader->IsHiddenInBush()) {
      ReturnToFormation();
      UE_LOG(LogXBCombat, Log, TEXT("士兵 %s 因主将草丛隐身，禁止进入战斗"),
             *GetName());
      return;
    }
  }

  if (CurrentState == EXBSoldierState::Dead ||
      CurrentState == EXBSoldierState::Dormant ||
      CurrentState == EXBSoldierState::Dropping) {
    return;
  }

  if (!bIsRecruited) {
    return;
  }

  // 🔧 修改 - 战斗状态启用行为树，跟随状态停用行为树
  if (AXBSoldierAIController *SoldierAI =
          Cast<AXBSoldierAIController>(GetController())) {
    if (BehaviorTreeAsset) {
      SoldierAI->StartBehaviorTree(BehaviorTreeAsset);
    }
  }

  // 🔧 修改 - RVO 避让系统统一由 SetSoldierState 管理，此处仅配置参数
  if (UCharacterMovementComponent *MoveComp = GetCharacterMovement()) {
    // 确保参数是最新的
    MoveComp->AvoidanceConsiderationRadius = GetAvoidanceRadius();
    MoveComp->AvoidanceWeight = GetAvoidanceWeight();
  }

  if (FollowComponent) {
    // 🔧 修改 - 战斗时关闭跟随组件，完全由行为树控制
    FollowComponent->SetComponentTickEnabled(false);
    FollowComponent->EnterCombatMode();
    UE_LOG(LogXBCombat, Log, TEXT("士兵 %s 战斗时关闭跟随组件Tick"),
           *GetName());
  }

  SetSoldierState(EXBSoldierState::Combat);

  UE_LOG(LogXBCombat, Log, TEXT("士兵 %s 进入战斗, 目标: %s"), *GetName(),
         CurrentAttackTarget.IsValid() ? *CurrentAttackTarget->GetName()
                                       : TEXT("无"));
}

void AXBSoldierCharacter::ExitCombat() {
  if (CurrentState == EXBSoldierState::Dead ||
      CurrentState == EXBSoldierState::Dormant ||
      CurrentState == EXBSoldierState::Dropping) {
    return;
  }

  // 🔧 修复 - 主将进入草丛时，士兵必须无条件脱战
  // 说明：移除原有的主将战斗状态检查，因为会阻止草丛脱战
  bool bLeaderInBush = false;
  if (AXBCharacterBase *Leader = GetLeaderCharacter()) {
    if (Leader->IsHiddenInBush()) {
      bLeaderInBush = true;
    } else {
      // 仅在主将未在草丛时，才检查主将战斗状态
      AXBCharacterBase *TargetLeader = Leader->GetLastAttackedEnemyLeader();
      if (TargetLeader && !TargetLeader->IsDead()) {
        UE_LOG(LogXBCombat, Verbose,
               TEXT("士兵 %s: 主将仍锁定目标主将 %s，保持战斗"), *GetName(),
               *TargetLeader->GetName());
        RequestNewTarget();
        return;
      }
    }
  }

  CurrentAttackTarget = nullptr;
  UnbindAssignedTargetEvents();

  // 🔧 修改 - RVO 避让已由 SetSoldierState(Following)
  // 自动关闭，此处无需重复处理

  if (AXBSoldierAIController *SoldierAI =
          Cast<AXBSoldierAIController>(GetController())) {
    SoldierAI->StopBehaviorTreeLogic();
  }

  if (FollowComponent) {
    // 🔧 修改 - 退出战斗时重新启用跟随组件
    FollowComponent->SetComponentTickEnabled(true);
    FollowComponent->ExitCombatMode();
    UE_LOG(LogXBCombat, Log, TEXT("士兵 %s 退出战斗时启用跟随组件Tick"),
           *GetName());
  }

  if (AAIController *AICtrl = Cast<AAIController>(GetController())) {
    AICtrl->StopMovement();
  }

  SetSoldierState(EXBSoldierState::Following);

  UE_LOG(LogXBCombat, Log, TEXT("士兵 %s 退出战斗"), *GetName());
}

float AXBSoldierCharacter::TakeSoldierDamage(float DamageAmount,
                                             AActor *DamageSource) {
  // ✨ 新增 - 友军伤害硬拦截（C++层级绝对防御）
  if (DamageSource) {
    AXBCharacterBase *MyLeader = GetLeaderCharacter();

    // 1. 绝对防御：主将不能伤害自己的士兵
    if (MyLeader && DamageSource == MyLeader) {
      UE_LOG(LogXBCombat, Verbose, TEXT("士兵 %s: 主将伤害被硬拦截"),
             *GetName());
      return 0.0f;
    }

    // 2. 绝对防御：同队士兵不能互相伤害
    if (AXBSoldierCharacter *SourceSoldier =
            Cast<AXBSoldierCharacter>(DamageSource)) {
      if (MyLeader && SourceSoldier->GetLeaderCharacter() == MyLeader) {
        UE_LOG(LogXBCombat, Verbose, TEXT("士兵 %s: 同队士兵伤害被硬拦截"),
               *GetName());
        return 0.0f;
      }
    }

    // 3. 绝对防御：主将的技能投射物也不能伤害自己的士兵
    // (DamageSource 可能是投射物，需要追溯其所有者)
    if (AActor *SourceOwner = DamageSource->GetOwner()) {
      if (MyLeader && SourceOwner == MyLeader) {
        UE_LOG(LogXBCombat, Verbose, TEXT("士兵 %s: 主将投射物伤害被硬拦截"),
               *GetName());
        return 0.0f;
      }
    }
  }

  if (bIsDead || CurrentState == EXBSoldierState::Dead) {
    return 0.0f;
  }

  if (DamageAmount <= 0.0f) {
    return 0.0f;
  }

  float ActualDamage = FMath::Min(DamageAmount, CurrentHealth);
  CurrentHealth -= ActualDamage;

  UE_LOG(LogXBCombat, Log, TEXT("士兵 %s 受到 %.1f 伤害, 剩余血量: %.1f"),
         *GetName(), ActualDamage, CurrentHealth);

  if (CurrentHealth <= 0.0f) {
    HandleDeath();
  }

  return ActualDamage;
}

// ✨ 新增 - 跟随/待机自动反击入口
/**
 * @brief 跟随/待机状态下自动进入战斗
 * @param DeltaTime 帧间隔
 * @note   详细流程分析: 校验主将战斗状态 -> 累计计时 -> 检查已分配目标 ->
 * 若目标有效则进入战斗 性能/架构注意事项:
 * 仅在跟随/待机且主将已命中敌方主将时执行，避免无意义索敌
 */
void AXBSoldierCharacter::TryAutoEngage(float DeltaTime) {
  // 未启用自动反击则直接返回
  if (!bEnableAutoEngage) {
    return;
  }

  // 仅在被招募且处于跟随/待机状态下执行
  if (!bIsRecruited || (CurrentState != EXBSoldierState::Following &&
                        CurrentState != EXBSoldierState::Idle)) {
    return;
  }

  // 🔧 修改 - 必须存在主将并且主将已命中敌方主将，士兵才允许自动进入战斗
  const AXBCharacterBase *Leader = GetLeaderCharacter();
  if (!Leader) {
    return;
  }

  // 草丛隐身主将时禁止自动战斗
  if (Leader->IsHiddenInBush()) {
    return;
  }

  // 主将未处于战斗或未命中敌方主将时，不允许士兵自动进入战斗
  if (!Leader->HasEnemiesInCombat() || !Leader->GetLastAttackedEnemyLeader()) {
    return;
  }

  // 计时器未到则不执行扫描
  AutoEngageCheckTimer -= DeltaTime;
  if (AutoEngageCheckTimer > 0.0f) {
    return;
  }
  AutoEngageCheckTimer = FMath::Max(0.05f, AutoEngageCheckInterval);

  AActor *AssignedTarget = CurrentAttackTarget.Get();
  if (AssignedTarget && BehaviorInterface &&
      BehaviorInterface->IsTargetValid(AssignedTarget)) {
    EnterCombat();
    BehaviorInterface->RecordEnemySeen();
    UE_LOG(LogXBCombat, Log, TEXT("士兵 %s 被动接收目标进入战斗: %s"),
           *GetName(), *AssignedTarget->GetName());
  }
}

bool AXBSoldierCharacter::PerformAttack(AActor *Target) {
  if (BehaviorInterface) {
    EXBBehaviorResult Result = BehaviorInterface->ExecuteAttack(Target);
    return Result == EXBBehaviorResult::Success;
  }
  return false;
}

/**
 * @brief  接收主将分配的目标
 * @param  AssignedTarget 分配的目标
 * @return 无
 * 功能说明: 缓存目标、绑定死亡回调并同步黑板
 * 详细流程: 校验状态 -> 解绑旧目标事件 -> 绑定新目标事件 -> 写入黑板 ->
 * 记录日志 注意事项: 死亡/休眠/掉落状态不接收目标
 */
void AXBSoldierCharacter::ReceiveAssignedTarget(AActor *AssignedTarget) {
  // 死亡/休眠/掉落态不处理
  if (CurrentState == EXBSoldierState::Dead ||
      CurrentState == EXBSoldierState::Dormant ||
      CurrentState == EXBSoldierState::Dropping) {
    return;
  }

  AXBCharacterBase *Leader = GetLeaderCharacter();
  AXBCharacterBase *TargetLeader =
      Leader ? Leader->GetLastAttackedEnemyLeader() : nullptr;
  if (!TargetLeader || TargetLeader->IsDead()) {
    return;
  }

  if (AssignedTarget == nullptr) {
    return;
  }

  bool bIsTargetLeader = (AssignedTarget == TargetLeader);
  bool bIsTargetLeaderSoldier = false;
  if (AXBSoldierCharacter *TargetSoldier =
          Cast<AXBSoldierCharacter>(AssignedTarget)) {
    bIsTargetLeaderSoldier =
        (TargetSoldier->GetLeaderCharacter() == TargetLeader &&
         TargetSoldier->GetSoldierState() != EXBSoldierState::Dead);
  }

  if (!bIsTargetLeader && !bIsTargetLeaderSoldier) {
    UE_LOG(LogXBCombat, Verbose,
           TEXT("士兵 %s: 目标不属于主将锁定的目标主将 %s，忽略分配"),
           *GetName(), *TargetLeader->GetName());
    return;
  }

  AActor *ExistingTarget = CurrentAttackTarget.Get();
  if (ExistingTarget && ExistingTarget != AssignedTarget) {
    bool bExistingTargetDead = false;
    if (AXBSoldierCharacter *ExistingSoldier =
            Cast<AXBSoldierCharacter>(ExistingTarget)) {
      bExistingTargetDead =
          ExistingSoldier->IsDead() ||
          ExistingSoldier->GetSoldierState() == EXBSoldierState::Dead;
    } else if (AXBCharacterBase *ExistingLeader =
                   Cast<AXBCharacterBase>(ExistingTarget)) {
      bExistingTargetDead = ExistingLeader->IsDead();
    }

    if (!bExistingTargetDead) {
      UE_LOG(LogXBCombat, Verbose,
             TEXT("士兵 %s 已锁定目标 %s，忽略新分配目标 %s"), *GetName(),
             *ExistingTarget->GetName(),
             AssignedTarget ? *AssignedTarget->GetName() : TEXT("无"));
      return;
    }
  }

  // 解绑旧目标事件
  UnbindAssignedTargetEvents();
  // 更新当前目标
  CurrentAttackTarget = AssignedTarget;
  // 绑定新目标事件
  BindAssignedTargetEvents(AssignedTarget);

  // 同步黑板数据
  if (AAIController *AICtrl = Cast<AAIController>(GetController())) {
    if (UBlackboardComponent *BBComp = AICtrl->GetBlackboardComponent()) {
      // 写入当前目标
      BBComp->SetValueAsObject(XBSoldierBBKeys::CurrentTarget, AssignedTarget);
      // 写入目标位置
      if (AssignedTarget) {
        BBComp->SetValueAsVector(XBSoldierBBKeys::TargetLocation,
                                 AssignedTarget->GetActorLocation());
      }
      // 写入目标标记
      BBComp->SetValueAsBool(XBSoldierBBKeys::HasTarget,
                             AssignedTarget != nullptr);
    }
  }

  // 打印日志
  UE_LOG(LogXBCombat, Verbose, TEXT("士兵 %s 接收目标: %s"), *GetName(),
         AssignedTarget ? *AssignedTarget->GetName() : TEXT("无"));
}

/**
 * @brief  向主将申请新目标
 * @param  无
 * @return 无
 * 功能说明: 在目标失效时延迟请求新目标，避免瞬时拥塞
 * 详细流程: 校验状态 -> 获取主将 -> 检查计时器 -> 计算随机延迟 -> 请求分配 ->
 * 接收目标 注意事项: 同一时间仅允许一个申请计时器
 */
void AXBSoldierCharacter::RequestNewTarget() {
  // 死亡/休眠/掉落态不处理
  if (CurrentState == EXBSoldierState::Dead ||
      CurrentState == EXBSoldierState::Dormant ||
      CurrentState == EXBSoldierState::Dropping) {
    return;
  }

  if (CurrentAttackTarget.IsValid()) {
    return;
  }

  // 获取主将
  AXBCharacterBase *Leader = GetLeaderCharacter();
  if (!Leader) {
    return;
  }

  AXBCharacterBase *TargetLeader = Leader->GetLastAttackedEnemyLeader();
  if (!TargetLeader || TargetLeader->IsDead()) {
    if (Cast<AXBPlayerCharacter>(Leader)) {
      ExitCombat();
      ReturnToFormation();
    }
    return;
  }

  // 已存在申请计时器则直接返回
  if (GetWorldTimerManager().IsTimerActive(TargetRequestTimerHandle)) {
    return;
  }

  // 计算随机延迟
  const float Delay =
      FMath::Max(0.0f, FMath::FRandRange(TargetRequestDelayRange.X,
                                         TargetRequestDelayRange.Y));

  // 设置延迟计时器并请求目标
  GetWorldTimerManager().SetTimer(
      TargetRequestTimerHandle,
      FTimerDelegate::CreateWeakLambda(
          this,
          [this, Leader]() {
            // 二次校验自身状态
            if (!IsValid(this) || CurrentState == EXBSoldierState::Dead) {
              return;
            }

            // 向主将申请新目标
            AActor *NewTarget = Leader->AssignTargetToSoldier(this);
            if (!NewTarget) {
              if (Cast<AXBPlayerCharacter>(Leader)) {
                ExitCombat();
                ReturnToFormation();
                return;
              }
              ExitCombat();
              return;
            }

            // 接收新目标
            ReceiveAssignedTarget(NewTarget);
          }),
      Delay, false);
}

void AXBSoldierCharacter::HandleTargetBlocked() {
  // 死亡/休眠/掉落态不处理
  if (CurrentState == EXBSoldierState::Dead ||
      CurrentState == EXBSoldierState::Dormant ||
      CurrentState == EXBSoldierState::Dropping) {
    return;
  }

  UnbindAssignedTargetEvents();
  CurrentAttackTarget = nullptr;
  RequestNewTarget();
}

bool AXBSoldierCharacter::PlayAttackMontage() {
  if (!IsDataAccessorValid()) {
    return false;
  }

  UAnimMontage *AttackMontage = DataAccessor->GetBasicAttackMontage();

  if (!AttackMontage) {
    return false;
  }

  if (UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance()) {
    return AnimInstance->Montage_Play(AttackMontage) > 0.0f;
  }

  return false;
}

bool AXBSoldierCharacter::CanAttack() const {
  // 🔧 修改 - 主将在草丛中时，士兵不可攻击
  if (const AXBCharacterBase *Leader = GetLeaderCharacter()) {
    if (Leader->IsHiddenInBush()) {
      return false;
    }
  }

  if (CurrentState == EXBSoldierState::Dead ||
      CurrentState == EXBSoldierState::Dormant ||
      CurrentState == EXBSoldierState::Dropping) {
    return false;
  }

  if (BehaviorInterface) {
    return BehaviorInterface->GetAttackCooldownRemaining() <= 0.0f;
  }
  return false;
}

// ==================== AI系统 ====================

bool AXBSoldierCharacter::HasEnemiesInRadius(float Radius) const {
  FXBDetectionResult Result;
  return UXBBlueprintFunctionLibrary::DetectEnemiesInRadius(
      this, GetActorLocation(), Radius, Faction, true, Result);
}

float AXBSoldierCharacter::GetDistanceToTarget(AActor *Target) const {
  if (!Target || !IsValid(Target)) {
    return MAX_FLT;
  }
  return FVector::Dist2D(GetActorLocation(), Target->GetActorLocation());
}

bool AXBSoldierCharacter::IsInAttackRange(AActor *Target) const {
  if (!Target || !IsValid(Target)) {
    return false;
  }

  const float AttackRangeVal = GetAttackRange();
  const float SelfRadius = GetSimpleCollisionRadius();
  const float TargetRadius = Target->GetSimpleCollisionRadius();
  return GetDistanceToTarget(Target) <=
         (AttackRangeVal + SelfRadius + TargetRadius);
}

void AXBSoldierCharacter::ReturnToFormation() {
  CurrentAttackTarget = nullptr;
  UnbindAssignedTargetEvents();

  // 🔧 修复 - 主将进入草丛时，士兵必须无条件回归编队
  // 说明：移除原有的主将战斗状态检查，因为会阻止草丛脱战后的回归
  bool bLeaderInBush = false;
  if (AXBCharacterBase *Leader = GetLeaderCharacter()) {
    if (Leader->IsHiddenInBush()) {
      bLeaderInBush = true;
    } else {
      // 仅在主将未在草丛时，才检查主将战斗状态
      AXBCharacterBase *TargetLeader = Leader->GetLastAttackedEnemyLeader();
      if (TargetLeader && !TargetLeader->IsDead()) {
        UE_LOG(LogXBCombat, Verbose,
               TEXT("士兵 %s: 主将仍锁定目标主将 %s，禁止回归编队"), *GetName(),
               *TargetLeader->GetName());
        RequestNewTarget();
        return;
      }
    }
  }

  if (FollowComponent) {
    // 🔧 修复 - 通过回归编队结束战斗时，确保跟随组件重新启用
    FollowComponent->SetComponentTickEnabled(true);
    FollowComponent->ExitCombatMode();
  }

  if (AAIController *AICtrl = Cast<AAIController>(GetController())) {
    AICtrl->StopMovement();
  }

  SetSoldierState(EXBSoldierState::Following);
}

FVector AXBSoldierCharacter::CalculateAvoidanceDirection(
    const FVector &DesiredDirection) {
  // ✨ 新增 - 避让系统总开关
  if (!bEnableAvoidanceSystem) {
    return DesiredDirection;
  }

  float AvoidanceRadiusVal = GetAvoidanceRadius();
  float AvoidanceWeightVal = GetAvoidanceWeight();

  if (AvoidanceRadiusVal <= 0.0f) {
    return DesiredDirection;
  }

  FVector AvoidanceForce = FVector::ZeroVector;
  FVector MyLocation = GetActorLocation();

  FXBDetectionResult AlliesResult;
  UXBBlueprintFunctionLibrary::DetectAlliesInRadius(
      this, MyLocation, AvoidanceRadiusVal, Faction, true, AlliesResult);

  int32 AvoidanceCount = 0;

  for (AActor *OtherActor : AlliesResult.DetectedActors) {
    if (OtherActor == this) {
      continue;
    }

    float Distance =
        FVector::Dist2D(MyLocation, OtherActor->GetActorLocation());
    if (Distance > KINDA_SMALL_NUMBER) {
      FVector AwayDirection =
          (MyLocation - OtherActor->GetActorLocation()).GetSafeNormal2D();
      float Strength = 1.0f - (Distance / AvoidanceRadiusVal);
      AvoidanceForce += AwayDirection * Strength;
      AvoidanceCount++;
    }
  }

  if (AvoidanceCount == 0) {
    return DesiredDirection;
  }

  AvoidanceForce.Normalize();

  FVector BlendedDirection = DesiredDirection * (1.0f - AvoidanceWeightVal) +
                             AvoidanceForce * AvoidanceWeightVal;

  return BlendedDirection.GetSafeNormal();
}

void AXBSoldierCharacter::MoveToFormationPosition() {
  if (FollowComponent) {
    FollowComponent->StartInterpolateToFormation();
  }
}

FVector AXBSoldierCharacter::GetFormationWorldPosition() const {
  if (!FollowTarget.IsValid()) {
    return GetActorLocation();
  }

  if (FollowComponent) {
    return FollowComponent->GetTargetPosition();
  }

  return FollowTarget->GetActorLocation();
}

FVector AXBSoldierCharacter::GetFormationWorldPositionSafe() const {
  if (!FollowTarget.IsValid()) {
    return FVector::ZeroVector;
  }

  AActor *Target = FollowTarget.Get();
  if (!Target || !IsValid(Target)) {
    return FVector::ZeroVector;
  }

  if (!FollowComponent) {
    return Target->GetActorLocation();
  }

  FVector TargetPos = FollowComponent->GetTargetPosition();
  if (!TargetPos.IsZero() && !TargetPos.ContainsNaN()) {
    return TargetPos;
  }

  return Target->GetActorLocation();
}

bool AXBSoldierCharacter::IsAtFormationPosition() const {
  if (FollowComponent) {
    return FollowComponent->IsAtFormationPosition();
  }

  FVector TargetPos = GetFormationWorldPosition();
  float ArrivalThresholdVal = GetArrivalThreshold();
  return FVector::Dist2D(GetActorLocation(), TargetPos) <= ArrivalThresholdVal;
}

bool AXBSoldierCharacter::IsAtFormationPositionSafe() const {
  if (!FollowTarget.IsValid() || FormationSlotIndex == INDEX_NONE) {
    return true;
  }

  if (FollowComponent) {
    return FollowComponent->IsAtFormationPosition();
  }

  return true;
}

// ==================== 逃跑系统 ====================

void AXBSoldierCharacter::SetEscaping(bool bEscaping) {
  bIsEscaping = bEscaping;

  if (bEscaping) {
    if (FollowComponent) {
      FollowComponent->SetCombatState(false);

      if (CurrentState == EXBSoldierState::Combat) {
        CurrentAttackTarget = nullptr;
        UnbindAssignedTargetEvents();
        SetSoldierState(EXBSoldierState::Following);
      }

      FollowComponent->StartInterpolateToFormation();
    }

    if (AAIController *AICtrl = Cast<AAIController>(GetController())) {
      AICtrl->StopMovement();
    }
  }

  float BaseSpeed = GetMoveSpeed();
  float SprintMultiplier = GetSprintSpeedMultiplier();

  float NewSpeed = bEscaping ? BaseSpeed * SprintMultiplier : BaseSpeed;

  if (UCharacterMovementComponent *MovementComp = GetCharacterMovement()) {
    MovementComp->MaxWalkSpeed = NewSpeed;
  }
}

// ==================== 对象池支持 ====================

void AXBSoldierCharacter::ResetForPooling() {
  EnterDormantState(EXBDormantType::Hidden);

  FollowTarget = nullptr;
  FormationSlotIndex = INDEX_NONE;

  bIsRecruited = false;
  bIsDead = false;
  bIsEscaping = false;

  CurrentHealth = 100.0f;

  CurrentAttackTarget = nullptr;
  UnbindAssignedTargetEvents();

  AttackCooldownTimer = 0.0f;
  TargetSearchTimer = 0.0f;

  // 重置掉落状态
  DropElapsedTime = 0.0f;
  DropTargetLeader = nullptr;
  bAutoRecruitOnLanding = true;

  GetWorldTimerManager().ClearTimer(DelayedAIStartTimerHandle);

  UE_LOG(LogXBSoldier, Verbose, TEXT("士兵 %s 状态已重置，进入池化休眠"),
         *GetName());
}

/**
 * @brief  设置草丛隐身状态
 * @param  bHidden 是否隐身
 * @note   详细流程分析: 设置半透明 -> 调整碰撞通道
 *         性能/架构注意事项: 仅在状态变化时执行，避免材质频繁更新
 */
void AXBSoldierCharacter::SetHiddenInBush(bool bEnableHidden) {
  if (bIsHiddenInBush == bEnableHidden) {
    return;
  }

  bIsHiddenInBush = bEnableHidden;

  if (USkeletalMeshComponent *MeshComp = GetMesh()) {
    if (!CachedOverlayMaterial) {
      CachedOverlayMaterial = MeshComp->GetOverlayMaterial();
    }

    if (bEnableHidden) {
      if (BushOverlayMaterial) {
        MeshComp->SetOverlayMaterial(BushOverlayMaterial);
      }
      // 🔧 修改 - 草丛中对非友军不可见，仅对本地玩家做可见性过滤
      bool bShouldHideForLocal = false;
      if (APawn *LocalPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)) {
        if (const AXBCharacterBase *LocalLeader =
                Cast<AXBCharacterBase>(LocalPawn)) {
          bShouldHideForLocal = (LocalLeader->GetFaction() != Faction);
        }
      }
      MeshComp->SetVisibility(!bShouldHideForLocal, true);
    } else {
      // 🔧 修改 - 离开草丛时清理覆层材质
      MeshComp->SetOverlayMaterial(nullptr);
      CachedOverlayMaterial = nullptr;
      MeshComp->SetVisibility(true, true);
    }
  }

  if (UCapsuleComponent *Capsule = GetCapsuleComponent()) {
    if (!bCachedBushCollisionResponse) {
      CachedLeaderCollisionResponse =
          Capsule->GetCollisionResponseToChannel(XBCollision::Leader);
      CachedSoldierCollisionResponse =
          Capsule->GetCollisionResponseToChannel(XBCollision::Soldier);
      bCachedBushCollisionResponse = true;
    }

    Capsule->SetCollisionResponseToChannel(
        XBCollision::Leader,
        bEnableHidden ? ECR_Ignore : CachedLeaderCollisionResponse.GetValue());
    Capsule->SetCollisionResponseToChannel(
        XBCollision::Soldier,
        bEnableHidden ? ECR_Ignore : CachedSoldierCollisionResponse.GetValue());
  }

  UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 草丛隐身状态=%s"), *GetName(),
         bEnableHidden ? TEXT("开启") : TEXT("关闭"));
}

/**
 * @brief  刷新跟随状态（供主将初始化后调用）
 * @param  Leader 主将
 * @param  SlotIndex 槽位索引
 * @return 无
 * @note   详细流程分析: 统一走内部跟随配置入口，保证跟随/编队/AI状态一致
 *         性能/架构注意事项: 仅在主将初始化后调用一次，避免重复触发
 */
void AXBSoldierCharacter::RefreshFollowingAfterLeaderInit(
    AXBCharacterBase *Leader, int32 SlotIndex) {
  if (!Leader) {
    UE_LOG(LogXBSoldier, Warning, TEXT("士兵 %s: 刷新跟随失败，Leader为空"),
           *GetName());
    return;
  }

  // 🔧 修改 - 主将必须是当前跟随目标，避免误刷新到其它队伍
  if (GetLeaderCharacter() != Leader) {
    UE_LOG(LogXBSoldier, Warning,
           TEXT("士兵 %s: 刷新跟随失败，Leader不匹配: %s"), *GetName(),
           *Leader->GetName());
    return;
  }

  // 🔧 修改 - 仅处理已招募士兵，避免休眠/掉落态误触发
  if (!bIsRecruited) {
    UE_LOG(LogXBSoldier, Verbose, TEXT("士兵 %s: 未招募，跳过跟随刷新"),
           *GetName());
    return;
  }

  // 🔧 修改 - 统一调用内部跟随入口，保证组件与AI配置一致
  SetupFollowingAndStartMoving(Leader, SlotIndex);
}

// ==================== 死亡系统 ====================

void AXBSoldierCharacter::HandleDeath() {
  if (bIsDead) {
    return;
  }

  GetWorldTimerManager().ClearTimer(DelayedAIStartTimerHandle);
  GetWorldTimerManager().ClearTimer(TargetRequestTimerHandle);
  UnbindAssignedTargetEvents();

  bIsDead = true;

  if (FollowComponent) {
    FollowComponent->SetFollowMode(EXBFollowMode::Free);
    FollowComponent->SetComponentTickEnabled(false);
  }

  SetSoldierState(EXBSoldierState::Dead);

  OnSoldierDied.Broadcast(this);

  if (AXBCharacterBase *LeaderCharacter = GetLeaderCharacter()) {
    LeaderCharacter->OnSoldierDied(this);
  }

  if (UCapsuleComponent *Capsule = GetCapsuleComponent()) {
    Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  }

  if (AAIController *AICtrl = Cast<AAIController>(GetController())) {
    AICtrl->StopMovement();
  }

  if (UCharacterMovementComponent *MoveComp = GetCharacterMovement()) {
    MoveComp->StopMovementImmediately();
    MoveComp->DisableMovement();
    MoveComp->SetComponentTickEnabled(false);
  }

  bool bMontageStarted = false;
  float DeathAnimDuration = 1.5f;

  if (IsDataAccessorValid()) {
    UAnimMontage *DeathMontage = DataAccessor->GetDeathMontage();
    if (DeathMontage) {
      if (UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance()) {
        float Duration = AnimInstance->Montage_Play(DeathMontage);
        if (Duration > 0.0f) {
          bMontageStarted = true;
          DeathAnimDuration = Duration;
        }
      }
    }
  }

  // 🔧 修改 - 根据死亡动画时长安排回收
  FTimerHandle RecycleTimerHandle;
  GetWorldTimerManager().SetTimer(
      RecycleTimerHandle,
      [this]() {
        if (!IsValid(this)) {
          return;
        }

        if (UWorld *World = GetWorld()) {
          if (UXBSoldierPoolSubsystem *PoolSubsystem =
                  World->GetSubsystem<UXBSoldierPoolSubsystem>()) {
            PoolSubsystem->ReleaseSoldier(this);
            UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 已回收到对象池"),
                   *GetName());
          } else {
            ResetForPooling();
            UE_LOG(LogXBSoldier, Log,
                   TEXT("士兵 %s 已重置为休眠态（无对象池）"), *GetName());
          }
        }
      },
      DeathAnimDuration + 0.5f, false);

  UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s 死亡，%.1f秒后回收"), *GetName(),
         DeathAnimDuration + 0.5f);
}

// ==================== 编队事件绑定 ====================

/**
 * @brief 绑定将领编队事件
 * @param Leader 将领指针
 * @note 🔧 确保队形更新时士兵使用插值方式补位
 */
void AXBSoldierCharacter::BindLeaderFormationEvents(AXBCharacterBase *Leader) {
  if (!Leader) {
    return;
  }

  UXBFormationComponent *FormationComp = Leader->GetFormationComponent();
  if (!FormationComp) {
    return;
  }

  // 如果已绑定到相同组件则无需重复绑定
  if (CachedLeaderFormation.Get() == FormationComp) {
    return;
  }

  UnbindLeaderFormationEvents();

  CachedLeaderFormation = FormationComp;

  FormationComp->OnFormationUpdated.AddUniqueDynamic(
      this, &AXBSoldierCharacter::HandleFormationUpdated);

  UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: 绑定编队事件到 %s"), *GetName(),
         *FormationComp->GetName());
}

/**
 * @brief 解除编队事件绑定
 * @note 🔧 防止更换将领或销毁时遗留委托
 */
void AXBSoldierCharacter::UnbindLeaderFormationEvents() {
  if (CachedLeaderFormation.IsValid()) {
    CachedLeaderFormation->OnFormationUpdated.RemoveDynamic(
        this, &AXBSoldierCharacter::HandleFormationUpdated);
    UE_LOG(LogXBSoldier, Verbose, TEXT("士兵 %s: 解除编队事件绑定"),
           *GetName());
  }

  CachedLeaderFormation.Reset();

  if (FormationRealignTimerHandle.IsValid()) {
    GetWorldTimerManager().ClearTimer(FormationRealignTimerHandle);
  }
}

/**
 * @brief 编队更新时触发平滑补位
 * @note ✨ 通过槽位序号叠加延迟，实现“蛇尾”式旋转/扩列补位
 */
void AXBSoldierCharacter::HandleFormationUpdated() {
  if (!FollowComponent || !FollowTarget.IsValid()) {
    return;
  }

  if (CurrentState == EXBSoldierState::Dormant ||
      CurrentState == EXBSoldierState::Dropping) {
    return;
  }

  if (CurrentState == EXBSoldierState::Combat) // ← 增加这一行
  {
    return; // 战斗中不响应编队更新
  }
  const int32 SafeSlotIndex = FMath::Max(FormationSlotIndex, 0);
  const float Delay = bEnableFormationTailDelay
                          ? FormationTailDelayPerSlot * SafeSlotIndex
                          : 0.0f;

  if (FormationRealignTimerHandle.IsValid()) {
    GetWorldTimerManager().ClearTimer(FormationRealignTimerHandle);
  }

  if (Delay <= KINDA_SMALL_NUMBER) {
    FollowComponent->StartInterpolateToFormation();
    UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: 编队更新立即补位，槽位 %d"),
           *GetName(), FormationSlotIndex);
    return;
  }

  // ✨ 槽位越靠后延迟越久，形成蛇尾效果
  FTimerDelegate DelayDelegate;
  DelayDelegate.BindLambda([this]() {
    if (FollowComponent && FollowTarget.IsValid()) {
      FollowComponent->StartInterpolateToFormation();
      UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: 编队更新延迟补位完成"),
             *GetName());
    }
  });

  GetWorldTimerManager().SetTimer(FormationRealignTimerHandle, DelayDelegate,
                                  Delay, false);

  UE_LOG(LogXBSoldier, Log, TEXT("士兵 %s: 编队更新排队补位，延迟 %.2fs"),
         *GetName(), Delay);
}

/**
 * @brief  处理已分配目标士兵死亡
 * @param  DeadSoldier 死亡士兵
 * @return 无
 * 功能说明: 当目标死亡时触发重新申请
 * 详细流程: 校验对象 -> 比对当前目标 -> 清理绑定 -> 申请新目标
 * 注意事项: 仅处理当前目标
 */
void AXBSoldierCharacter::HandleAssignedTargetSoldierDied(
    AXBSoldierCharacter *DeadSoldier) {
  // 校验参数
  if (!DeadSoldier) {
    return;
  }

  // 仅处理当前目标
  if (CurrentAttackTarget.Get() == DeadSoldier) {
    // 解绑旧目标事件
    UnbindAssignedTargetEvents();
    // 清空当前目标
    CurrentAttackTarget = nullptr;
    // 申请新目标
    RequestNewTarget();
  }
}

/**
 * @brief  处理已分配目标主将死亡
 * @param  DeadLeader 死亡主将
 * @return 无
 * 功能说明: 当目标主将死亡时触发重新申请
 * 详细流程: 校验对象 -> 比对当前目标 -> 清理绑定 -> 申请新目标
 * 注意事项: 仅处理当前目标
 */
void AXBSoldierCharacter::HandleAssignedTargetLeaderDied(
    AXBCharacterBase *DeadLeader) {
  // 校验参数
  if (!DeadLeader) {
    return;
  }

  // 仅处理当前目标
  if (CurrentAttackTarget.Get() == DeadLeader) {
    // 解绑旧目标事件
    UnbindAssignedTargetEvents();
    // 清空当前目标
    CurrentAttackTarget = nullptr;
    // 申请新目标
    RequestNewTarget();
  }
}

/**
 * @brief  绑定目标死亡事件
 * @param  AssignedTarget 分配的目标
 * @return 无
 * 功能说明: 根据目标类型绑定对应死亡委托
 * 详细流程: 校验目标 -> 判断士兵/主将 -> 绑定死亡委托
 * 注意事项: 未传目标直接返回
 */
void AXBSoldierCharacter::BindAssignedTargetEvents(AActor *AssignedTarget) {
  // 无目标直接返回
  if (!AssignedTarget || !IsValid(AssignedTarget)) {
    return;
  }

  // 绑定士兵死亡事件
  if (AXBSoldierCharacter *TargetSoldier =
          Cast<AXBSoldierCharacter>(AssignedTarget)) {
    TargetSoldier->OnSoldierDied.AddDynamic(
        this, &AXBSoldierCharacter::HandleAssignedTargetSoldierDied);
    return;
  }

  // 绑定主将死亡事件
  if (AXBCharacterBase *TargetLeader = Cast<AXBCharacterBase>(AssignedTarget)) {
    TargetLeader->OnCharacterDeath.AddDynamic(
        this, &AXBSoldierCharacter::HandleAssignedTargetLeaderDied);
  }
}

/**
 * @brief  解绑目标死亡事件
 * @param  无
 * @return 无
 * 功能说明: 根据当前目标类型解除死亡委托
 * 详细流程: 获取当前目标 -> 判断士兵/主将 -> 移除死亡委托
 * 注意事项: 无目标直接返回
 */
void AXBSoldierCharacter::UnbindAssignedTargetEvents() {
  // 获取当前目标
  AActor *AssignedTarget = CurrentAttackTarget.Get();
  if (!AssignedTarget) {
    return;
  }

  // 解绑士兵死亡事件
  if (AXBSoldierCharacter *TargetSoldier =
          Cast<AXBSoldierCharacter>(AssignedTarget)) {
    TargetSoldier->OnSoldierDied.RemoveDynamic(
        this, &AXBSoldierCharacter::HandleAssignedTargetSoldierDied);
    return;
  }

  // 解绑主将死亡事件
  if (AXBCharacterBase *TargetLeader = Cast<AXBCharacterBase>(AssignedTarget)) {
    TargetLeader->OnCharacterDeath.RemoveDynamic(
        this, &AXBSoldierCharacter::HandleAssignedTargetLeaderDied);
  }
}

void AXBSoldierCharacter::FaceTarget(AActor *Target, float DeltaTime) {
  if (!Target || !IsValid(Target)) {
    return;
  }

  FVector Direction =
      (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
  if (!Direction.IsNearlyZero()) {
    FRotator TargetRotation = Direction.Rotation();
    FRotator CurrentRotation = GetActorRotation();

    float RotationSpeedVal = GetRotationSpeed();
    FRotator NewRotation = FMath::RInterpTo(
        CurrentRotation, TargetRotation, DeltaTime, RotationSpeedVal / 90.0f);
    SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
  }
}

// ==================== AI控制器初始化 ====================

void AXBSoldierCharacter::SpawnAndPossessAIController() {
  if (!IsValid(this) || IsPendingKillPending()) {
    return;
  }

  if (CurrentState == EXBSoldierState::Dormant ||
      CurrentState == EXBSoldierState::Dropping) {
    return;
  }

  UCapsuleComponent *Capsule = GetCapsuleComponent();
  UCharacterMovementComponent *MoveComp = GetCharacterMovement();

  if (!Capsule || !MoveComp) {
    UE_LOG(LogXBSoldier, Error, TEXT("士兵 %s: 组件无效，无法启动AI"),
           *GetName());
    return;
  }

  FTransform CapsuleTransform = Capsule->GetComponentTransform();
  if (!CapsuleTransform.IsValid()) {
    GetWorldTimerManager().SetTimer(
        DelayedAIStartTimerHandle, this,
        &AXBSoldierCharacter::SpawnAndPossessAIController, 0.1f, false);
    return;
  }

  if (GetController()) {
    InitializeAI();
    return;
  }

  UWorld *World = GetWorld();
  if (!World) {
    return;
  }

  UClass *ControllerClassToUse = nullptr;
  if (SoldierAIControllerClass) {
    ControllerClassToUse = SoldierAIControllerClass.Get();
  } else {
    ControllerClassToUse = AXBSoldierAIController::StaticClass();
  }

  if (!ControllerClassToUse) {
    return;
  }

  FActorSpawnParameters SpawnParams;
  SpawnParams.SpawnCollisionHandlingOverride =
      ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

  AAIController *NewController =
      World->SpawnActor<AAIController>(ControllerClassToUse, GetActorLocation(),
                                       GetActorRotation(), SpawnParams);

  if (NewController) {
    NewController->Possess(this);
    InitializeAI();
  }
}

void AXBSoldierCharacter::InitializeAI() {
  AAIController *AICtrl = Cast<AAIController>(GetController());
  if (!AICtrl) {
    return;
  }

  if (BehaviorTreeAsset) {
    // 🔧 修改 - 行为树仅在战斗状态时启动
    if (CurrentState == EXBSoldierState::Combat) {
      if (AXBSoldierAIController *SoldierAI =
              Cast<AXBSoldierAIController>(AICtrl)) {
        SoldierAI->StartBehaviorTree(BehaviorTreeAsset);
      }
    }

    if (UBlackboardComponent *BBComp = AICtrl->GetBlackboardComponent()) {
      BBComp->SetValueAsObject(TEXT("Self"), this);
      BBComp->SetValueAsObject(TEXT("Leader"), FollowTarget.Get());
      BBComp->SetValueAsInt(TEXT("SoldierState"),
                            static_cast<int32>(CurrentState));
      BBComp->SetValueAsInt(TEXT("FormationSlot"), FormationSlotIndex);
      BBComp->SetValueAsFloat(TEXT("AttackRange"), GetAttackRange());
      BBComp->SetValueAsFloat(TEXT("VisionRange"), GetVisionRange());
      BBComp->SetValueAsFloat(TEXT("DetectionRange"), GetVisionRange());
      BBComp->SetValueAsBool(TEXT("IsAtFormation"), true);
      BBComp->SetValueAsBool(TEXT("CanAttack"), true);
    }
  }
}
