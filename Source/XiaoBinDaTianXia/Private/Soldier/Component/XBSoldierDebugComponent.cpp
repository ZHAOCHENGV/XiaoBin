/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Soldier/Component/XBSoldierDebugComponent.cpp

/**
 * @file XBSoldierDebugComponent.cpp
 * @brief 士兵状态可视化调试组件实现
 *
 * @note ✨ 新增文件
 * @note 🔧 修改 - 使用独立布尔变量替代位掩码
 *              - 移除 DrawDebugString 的阴影效果，避免黑色区域
 */

#include "Soldier/Component/XBSoldierDebugComponent.h"
#include "AI/XBSoldierAIController.h" // ✨ 新增
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h" // ✨ 新增
#include "Character/Components/XBFormationComponent.h"
#include "Character/XBCharacterBase.h"
#include "Data/XBSoldierDataAccessor.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Soldier/Component/XBSoldierFollowComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Utils/XBLogCategories.h"


// ✨ 新增 - 静态变量初始化
bool UXBSoldierDebugComponent::bGlobalDebugEnabled = false;

UXBSoldierDebugComponent::UXBSoldierDebugComponent() {
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UXBSoldierDebugComponent::BeginPlay() {
  Super::BeginPlay();

  // 缓存士兵引用
  CachedSoldier = Cast<AXBSoldierCharacter>(GetOwner());

  if (!CachedSoldier.IsValid()) {
    UE_LOG(LogXBSoldier, Warning,
           TEXT("调试组件 %s: Owner 不是 AXBSoldierCharacter"), *GetName());
  } else {
    UE_LOG(LogXBSoldier, Log, TEXT("士兵调试组件初始化: %s, 调试状态: %s"),
           *CachedSoldier->GetName(),
           bEnableDebug ? TEXT("启用") : TEXT("禁用"));
  }
}

void UXBSoldierDebugComponent::TickComponent(
    float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  // 🔧 修改 - 检查是否应该绘制
  if (!ShouldDrawDebug()) {
    return;
  }

  // 检查士兵引用有效性
  if (!CachedSoldier.IsValid()) {
    return;
  }

  // 🔧 修改 - 检查是否有任何选项被启用
  if (!HasAnyOptionEnabled()) {
    return;
  }

  // 🔧 修改 - 根据独立的布尔变量绘制各类调试信息
  if (bShowState) {
    DrawStateText();
  }

  if (bShowHealth) {
    DrawHealthInfo();
  }

  if (bShowTarget) {
    DrawTargetInfo();
  }

  if (bShowFormation) {
    DrawFormationPosition();
  }

  if (bShowAttackRange) {
    DrawAttackRange();
  }

  if (bShowVisionRange) {
    DrawVisionRange();
  }

  if (bShowVelocity) {
    DrawVelocity();
  }

  if (bShowLeaderLine) {
    DrawLeaderLine();
  }

  if (bShowAIInfo) {
    DrawAIInfo();
  }
}

// ==================== 内部辅助方法 ====================

/**
 * @brief 检查是否应该显示调试信息
 * @return 是否应该绘制
 */
bool UXBSoldierDebugComponent::ShouldDrawDebug() const {
  // 本地开关启用 或 全局开关启用
  return bEnableDebug || bGlobalDebugEnabled;
}

/**
 * @brief 检查是否有任何显示选项被启用
 * @return 至少有一个选项启用时返回 true
 */
bool UXBSoldierDebugComponent::HasAnyOptionEnabled() const {
  return bShowState || bShowHealth || bShowTarget || bShowFormation ||
         bShowAttackRange || bShowVisionRange || bShowVelocity ||
         bShowVisionRange || bShowVelocity || bShowLeaderLine || bShowAIInfo;
}

// ==================== 调试控制实现 ====================

void UXBSoldierDebugComponent::SetDebugEnabled(bool bEnable) {
  bEnableDebug = bEnable;

  if (GetOwner()) {
    UE_LOG(LogXBSoldier, Log, TEXT("士兵调试组件 %s: 调试显示 %s"),
           *GetOwner()->GetName(), bEnable ? TEXT("启用") : TEXT("禁用"));
  }
}

void UXBSoldierDebugComponent::EnableAllOptions() {
  bShowState = true;
  bShowHealth = true;
  bShowTarget = true;
  bShowFormation = true;
  bShowAttackRange = true;
  bShowVisionRange = true;
  bShowVelocity = true;
  bShowLeaderLine = true;
  bShowAIInfo = true;

  UE_LOG(LogXBSoldier, Log, TEXT("士兵调试组件: 启用所有显示选项"));
}

void UXBSoldierDebugComponent::DisableAllOptions() {
  bShowState = false;
  bShowHealth = false;
  bShowTarget = false;
  bShowFormation = false;
  bShowAttackRange = false;
  bShowVisionRange = false;
  bShowVelocity = false;
  bShowLeaderLine = false;
  bShowAIInfo = false;

  UE_LOG(LogXBSoldier, Log, TEXT("士兵调试组件: 禁用所有显示选项"));
}

// ==================== 静态全局控制 ====================

void UXBSoldierDebugComponent::SetGlobalDebugEnabled(bool bEnable) {
  bGlobalDebugEnabled = bEnable;

  UE_LOG(LogXBSoldier, Warning, TEXT("===== 士兵调试全局开关: %s ====="),
         bEnable ? TEXT("启用") : TEXT("禁用"));
}

// ==================== 绘制方法实现 ====================

/**
 * @brief 绘制状态文字
 * @note 显示：状态、阵营、类型、槽位索引
 * @note 🔧 修改 - 禁用阴影效果，避免黑色区域
 */
void UXBSoldierDebugComponent::DrawStateText() {
  AXBSoldierCharacter *Soldier = CachedSoldier.Get();
  if (!Soldier) {
    return;
  }

  UWorld *World = GetWorld();
  if (!World) {
    return;
  }

  FVector Location = Soldier->GetActorLocation();
  FVector TextLocation = Location + FVector(0.0f, 0.0f, TextHeightOffset);

  // 🔧 修复 - 移除 GetSoldierConfig() 调用
  FString SoldierIdText =
      Soldier->GetDataAccessor() && Soldier->GetDataAccessor()->IsInitialized()
          ? Soldier->GetDataAccessor()
                ->GetRawData()
                .SoldierTags.ToStringSimple()
          : TEXT("未初始化");

  FString StateInfo = FString::Printf(
      TEXT("[%s]\n阵营:%s | 类型:%s\n槽位:%d | 招募:%s\n标签:%s"),
      *GetStateName(Soldier->GetSoldierState()),
      *GetFactionName(Soldier->GetFaction()),
      *GetSoldierTypeName(Soldier->GetSoldierType()),
      Soldier->GetFormationSlotIndex(),
      Soldier->IsRecruited() ? TEXT("是") : TEXT("否"), *SoldierIdText);

  DrawDebugString(World, TextLocation, StateInfo, nullptr, StateTextColor, 0.0f,
                  false, TextScale);
}

/**
 * @brief 绘制血量信息
 * @note 显示血量条和数值
 * @note 🔧 修改 - 禁用阴影效果
 */
void UXBSoldierDebugComponent::DrawHealthInfo() {
  AXBSoldierCharacter *Soldier = CachedSoldier.Get();
  if (!Soldier) {
    return;
  }

  UWorld *World = GetWorld();
  if (!World) {
    return;
  }

  float CurrentHealth = Soldier->GetCurrentHealth();
  float MaxHealth = Soldier->GetMaxHealth();
  float HealthPercent = MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;

  // 使用 FLinearColor 进行插值
  FLinearColor LowColorLinear = FLinearColor(HealthLowColor);
  FLinearColor FullColorLinear = FLinearColor(HealthFullColor);
  FLinearColor InterpolatedColor =
      FMath::Lerp(LowColorLinear, FullColorLinear, HealthPercent);
  FColor HealthColor = InterpolatedColor.ToFColor(true);

  FVector Location = Soldier->GetActorLocation();

  // 🔧 修改 - 调整血量文字位置，避免与状态信息重叠
  FVector HealthTextLocation =
      Location + FVector(0.0f, 0.0f, TextHeightOffset - 30.0f);

  // 绘制血量文字
  FString HealthText =
      FString::Printf(TEXT("HP: %.0f/%.0f"), CurrentHealth, MaxHealth);

  // 🔧 修改 - bDrawShadow 改为 false
  DrawDebugString(World, HealthTextLocation, HealthText, nullptr, HealthColor,
                  0.0f,
                  false, // 🔧 禁用阴影
                  TextScale * 0.9f);

  // 绘制血量条
  float BarWidth = 60.0f;
  FVector BarCenter = Location + FVector(0.0f, 0.0f, TextHeightOffset - 45.0f);
  FVector BarStart = BarCenter + FVector(-BarWidth * 0.5f, 0.0f, 0.0f);
  FVector BarEnd = BarStart + FVector(BarWidth, 0.0f, 0.0f);
  FVector HealthBarEnd =
      BarStart + FVector(BarWidth * HealthPercent, 0.0f, 0.0f);

  // 背景条（深灰色）
  DrawDebugLine(World, BarStart, BarEnd, FColor(80, 80, 80), false, 0.0f, 0,
                5.0f);

  // 血量条
  if (HealthPercent > 0.01f) {
    DrawDebugLine(World, BarStart, HealthBarEnd, HealthColor, false, 0.0f, 0,
                  3.0f);
  }
}

/**
 * @brief 绘制目标信息和连线
 * @note 显示当前攻击目标及到目标的距离
 */
void UXBSoldierDebugComponent::DrawTargetInfo() {
  AXBSoldierCharacter *Soldier = CachedSoldier.Get();
  if (!Soldier) {
    return;
  }

  UWorld *World = GetWorld();
  if (!World) {
    return;
  }

  AActor *Target = Soldier->CurrentAttackTarget.Get();
  if (!Target || !IsValid(Target)) {
    return;
  }

  FVector SoldierLocation = Soldier->GetActorLocation();
  FVector TargetLocation = Target->GetActorLocation();
  float Distance = FVector::Dist(SoldierLocation, TargetLocation);

  // 绘制到目标的连线
  DrawDebugLine(World, SoldierLocation + FVector(0.0f, 0.0f, 50.0f),
                TargetLocation + FVector(0.0f, 0.0f, 50.0f), TargetLineColor,
                false, 0.0f, 0, 2.0f);

  // 在连线中点显示距离
  FVector MidPoint =
      (SoldierLocation + TargetLocation) * 0.5f + FVector(0.0f, 0.0f, 70.0f);
  FString DistanceText = FString::Printf(TEXT("%.0f"), Distance);

  DrawDebugString(World, MidPoint, DistanceText, nullptr, TargetLineColor, 0.0f,
                  false, TextScale * 0.8f);

  // 在目标位置绘制标记
  DrawDebugSphere(World, TargetLocation + FVector(0.0f, 0.0f, 100.0f), 15.0f, 8,
                  TargetLineColor, false, 0.0f, 0, 1.5f);
}

/**
 * @brief 绘制编队位置
 * @note 显示士兵应该在的编队位置
 */
void UXBSoldierDebugComponent::DrawFormationPosition() {
  AXBSoldierCharacter *Soldier = CachedSoldier.Get();
  if (!Soldier) {
    return;
  }

  UWorld *World = GetWorld();
  if (!World) {
    return;
  }

  // 获取编队位置
  FVector FormationPos = Soldier->GetFormationWorldPositionSafe();
  if (FormationPos.IsZero()) {
    return;
  }

  FVector SoldierLocation = Soldier->GetActorLocation();
  float Distance = FVector::Dist2D(SoldierLocation, FormationPos);

  // 绘制编队位置圆圈
  DrawDebugCircle(World, FormationPos + FVector(0, 0, 5), 25.0f, 16,
                  FormationColor, false, 0.0f, 0, 2.0f, FVector(1, 0, 0),
                  FVector(0, 1, 0));

  // 如果距离编队位置较远，绘制连线
  if (Distance > 30.0f) {
    DrawDebugLine(World, SoldierLocation + FVector(0, 0, 10),
                  FormationPos + FVector(0, 0, 10), FormationColor, false, 0.0f,
                  0, 1.0f);

    // 显示距离
    FVector TextPos = FormationPos + FVector(0.0f, 0.0f, 40.0f);
    FString DistText = FString::Printf(TEXT("%.0f"), Distance);
    DrawDebugString(World, TextPos, DistText, nullptr, FormationColor, 0.0f,
                    false, TextScale * 0.7f);
  }
}

/**
 * @brief 绘制攻击范围
 * @note 在士兵位置绘制攻击范围圆圈
 */
void UXBSoldierDebugComponent::DrawAttackRange() {
  AXBSoldierCharacter *Soldier = CachedSoldier.Get();
  if (!Soldier) {
    return;
  }

  UWorld *World = GetWorld();
  if (!World) {
    return;
  }

  // 🔧 修复 - 直接调用 GetAttackRange()
  float AttackRange = Soldier->GetAttackRange();

  if (AttackRange <= 0.0f) {
    return;
  }

  FVector Location = Soldier->GetActorLocation();

  DrawDebugCircle(World, Location + FVector(0, 0, 10), AttackRange,
                  CircleSegments, AttackRangeColor, false, 0.0f, 0,
                  CircleThickness, FVector(1, 0, 0), FVector(0, 1, 0));
}

/**
 * @brief 绘制视野范围
 * @note 在士兵位置绘制视野/检测范围圆圈
 */
void UXBSoldierDebugComponent::DrawVisionRange() {
  AXBSoldierCharacter *Soldier = CachedSoldier.Get();
  if (!Soldier) {
    return;
  }

  UWorld *World = GetWorld();
  if (!World) {
    return;
  }

  // 获取视野范围
  float VisionRange = Soldier->GetVisionRange();

  if (VisionRange <= 0.0f) {
    return;
  }

  FVector Location = Soldier->GetActorLocation();

  // 绘制视野范围圆圈（使用虚线效果 - 较少段数）
  DrawDebugCircle(World, Location + FVector(0, 0, 15), VisionRange,
                  CircleSegments / 2, VisionRangeColor, false, 0.0f, 0,
                  CircleThickness * 0.6f, FVector(1, 0, 0), FVector(0, 1, 0));
}

/**
 * @brief 绘制速度向量
 * @note 显示士兵当前的移动方向和速度
 */
void UXBSoldierDebugComponent::DrawVelocity() {
  AXBSoldierCharacter *Soldier = CachedSoldier.Get();
  if (!Soldier) {
    return;
  }

  UWorld *World = GetWorld();
  if (!World) {
    return;
  }

  UCharacterMovementComponent *MoveComp = Soldier->GetCharacterMovement();
  if (!MoveComp) {
    return;
  }

  FVector Velocity = MoveComp->Velocity;
  Velocity.Z = 0.0f;
  float Speed = Velocity.Size();

  // 只有在移动时才绘制
  if (Speed < 10.0f) {
    return;
  }

  FVector Location = Soldier->GetActorLocation();
  float ArrowLength = FMath::Clamp(Speed * 0.3f, 30.0f, 150.0f);
  FVector VelocityEnd = Location + Velocity.GetSafeNormal() * ArrowLength;

  // 绘制速度箭头
  DrawDebugDirectionalArrow(World, Location + FVector(0, 0, 40),
                            VelocityEnd + FVector(0, 0, 40), 30.0f,
                            VelocityColor, false, 0.0f, 0, 2.0f);

  // 显示速度数值
  FString SpeedText = FString::Printf(TEXT("%.0f"), Speed);
  DrawDebugString(World, VelocityEnd + FVector(0.0f, 0.0f, 60.0f), SpeedText,
                  nullptr, VelocityColor, 0.0f, false, TextScale * 0.7f);
}

/**
 * @brief 绘制将领连线
 * @note 显示士兵到将领的连线
 */
void UXBSoldierDebugComponent::DrawLeaderLine() {
  AXBSoldierCharacter *Soldier = CachedSoldier.Get();
  if (!Soldier) {
    return;
  }

  UWorld *World = GetWorld();
  if (!World) {
    return;
  }

  AActor *Leader = Soldier->GetFollowTarget();
  if (!Leader || !IsValid(Leader)) {
    return;
  }

  FVector SoldierLocation = Soldier->GetActorLocation();
  FVector LeaderLocation = Leader->GetActorLocation();

  // 绘制到将领的连线
  DrawDebugLine(World, SoldierLocation + FVector(0.0f, 0.0f, 30.0f),
                LeaderLocation + FVector(0.0f, 0.0f, 30.0f), LeaderLineColor,
                false, 0.0f, 0, 1.0f);
}

/**
 * @brief 绘制AI信息
 * @note 显示AI控制器类名和当前行为树任务
 */
// 🔧 修改 - 添加必要的头文件
#include "AI/XBSoldierAIController.h"
#include "BehaviorTree/BlackboardComponent.h"

// ... (previous headers are fine, but ensure these are present. Since I am
// replacing DrawAIInfo block, I will actually use a separate replace call for
// headers or just put everything in one if possible. Wait, I can only replace
// one contiguous block. The includes are at the top. DrawAIInfo is at the
// bottom. I should first add the includes, then replace DrawAIInfo. ACTUALLY, I
// will use multi_replace to do both in one go.

void UXBSoldierDebugComponent::DrawAIInfo() {
  AXBSoldierCharacter *Soldier = CachedSoldier.Get();
  if (!Soldier) {
    return;
  }

  AController *Controller = Soldier->GetController();
  if (!Controller) {
    return;
  }

  // 1. 基础信息：控制器名称
  FString AIInfoText =
      FString::Printf(TEXT("🎮 Ctrl: %s"), *Controller->GetName());

  // 尝试获取行为树组件和黑板
  UBehaviorTreeComponent *BTComp = nullptr;
  UBlackboardComponent *Blackboard = nullptr;

  if (AAIController *AIController = Cast<AAIController>(Controller)) {
    AIInfoText += TEXT("\n"); // 换行
    BTComp = Cast<UBehaviorTreeComponent>(AIController->GetBrainComponent());
    Blackboard = AIController->GetBlackboardComponent();
  }

  // 2. 行为树状态 & 当前任务
  if (BTComp) {
    FString TaskName = TEXT("None");
    FString ActiveState = TEXT("Idle");

    // 解析当前 Task 名称
    FString DebugInfo = BTComp->GetDebugInfoString();
    int32 TaskIndex = DebugInfo.Find(TEXT("Task:"));
    if (TaskIndex != INDEX_NONE) {
      FString SubString = DebugInfo.Mid(TaskIndex + 5).TrimStartAndEnd();
      int32 LineBreak;
      if (SubString.FindChar('\n', LineBreak)) {
        TaskName = SubString.Left(LineBreak);
      } else {
        TaskName = SubString;
      }
    }

    // 简化 Task 名称 (移除前缀)
    if (TaskName.Contains(TEXT("BTTask_"))) {
      TaskName = TaskName.Replace(TEXT("BTTask_"), TEXT(""));
    }

    AIInfoText += FString::Printf(
        TEXT("🌲 BT: %s | Task: %s"),
        BTComp->IsPaused()
            ? TEXT("⏸️Paused")
            : (BTComp->IsRunning() ? TEXT("▶️Running") : TEXT("⏹️Stopped")),
        *TaskName);
  } else {
    AIInfoText += TEXT("⚠️ No BT Component");
  }

  // 3. 🛡️ 状态校对 (State Verification)
  if (Blackboard) {
    AIInfoText += TEXT("\n--------------\n📡 State Sync Check:");
    bool bAllSync = true;

    // [State] 状态校对
    int32 BB_State = Blackboard->GetValueAsInt(XBSoldierBBKeys::SoldierState);
    int32 Actor_State = (int32)Soldier->GetSoldierState();

    if (BB_State == Actor_State) {
      // AIInfoText += FString::Printf(TEXT("\n✅ State: %s"),
      // *GetStateName((EXBSoldierState)Actor_State));
    } else {
      bAllSync = false;
      AIInfoText +=
          FString::Printf(TEXT("\n🔴 State MISMATCH!\n   Actor: %s | BB: %s"),
                          *GetStateName((EXBSoldierState)Actor_State),
                          *GetStateName((EXBSoldierState)BB_State));
    }

    // [Target] 目标校对
    AActor *BB_Target = Cast<AActor>(
        Blackboard->GetValueAsObject(XBSoldierBBKeys::CurrentTarget));
    AActor *Actor_Target = Soldier->CurrentAttackTarget.Get();

    if (BB_Target == Actor_Target) {
      // 匹配时如果是 battle 状态显示一下目标
      if (Actor_Target) {
        AIInfoText +=
            FString::Printf(TEXT("\n✅ Target: %s"), *Actor_Target->GetName());
      }
    } else {
      bAllSync = false;
      AIInfoText += FString::Printf(
          TEXT("\n🔴 Target MISMATCH!\n   Actor: %s | BB: %s"),
          Actor_Target ? *Actor_Target->GetName() : TEXT("null"),
          BB_Target ? *BB_Target->GetName() : TEXT("null"));
    }

    // [Leader] 将领校对
    AActor *BB_Leader =
        Cast<AActor>(Blackboard->GetValueAsObject(XBSoldierBBKeys::Leader));
    AActor *Actor_Leader = Soldier->GetFollowTarget();

    if (BB_Leader != Actor_Leader) {
      bAllSync = false;
      AIInfoText += FString::Printf(
          TEXT("\n🔴 Leader MISMATCH!\n   Actor: %s | BB: %s"),
          Actor_Leader ? *Actor_Leader->GetName() : TEXT("null"),
          BB_Leader ? *BB_Leader->GetName() : TEXT("null"));
    }

    if (bAllSync) {
      AIInfoText += TEXT(" ✅ All Synced");
    }
  }

  UWorld *World = GetWorld();
  if (World) {
    FVector Location = Soldier->GetActorLocation();
    // 放在更下方，字体稍微小一点
    FVector TextLocation =
        Location + FVector(0.0f, 0.0f, TextHeightOffset - 80.0f);

    DrawDebugString(World, TextLocation, AIInfoText, nullptr, AIInfoColor, 0.0f,
                    false, TextScale * 0.65f);
  }
}

// ==================== 辅助方法实现 ====================

FString UXBSoldierDebugComponent::GetStateName(EXBSoldierState State) const {
  switch (State) {
  case EXBSoldierState::Dormant:
    return TEXT("休眠");
  case EXBSoldierState::Dropping:
    return TEXT("掉落"); // ✨ 新增
  case EXBSoldierState::Idle:
    return TEXT("待机");
  case EXBSoldierState::Following:
    return TEXT("跟随");
  case EXBSoldierState::Combat:
    return TEXT("战斗");
  case EXBSoldierState::Seeking:
    return TEXT("搜索");
  case EXBSoldierState::Returning:
    return TEXT("返回");
  case EXBSoldierState::Dead:
    return TEXT("死亡");
  default:
    return TEXT("未知");
  }
}

FString UXBSoldierDebugComponent::GetFactionName(EXBFaction Faction) const {
  switch (Faction) {
  case EXBFaction::Neutral:
    return TEXT("中立");
  case EXBFaction::Player:
    return TEXT("玩家");
  case EXBFaction::Enemy:
    return TEXT("敌人");
  case EXBFaction::Ally:
    return TEXT("友军");
  case EXBFaction::FreeForAll:
    return TEXT("各自为战");
  default:
    return TEXT("未知");
  }
}

FString
UXBSoldierDebugComponent::GetSoldierTypeName(EXBSoldierType Type) const {
  switch (Type) {
  case EXBSoldierType::None:
    return TEXT("无");
  case EXBSoldierType::Infantry:
    return TEXT("步兵");
  case EXBSoldierType::Archer:
    return TEXT("弓手");
  case EXBSoldierType::Cavalry:
    return TEXT("骑兵");
  default:
    return TEXT("未知");
  }
}
