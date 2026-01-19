/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTTask_XBMoveToTarget.cpp

#include "AI/BehaviorTree/BTTask_XBMoveToTarget.h"
#include "AI/XBSoldierAIController.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/XBCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Utils/XBLogCategories.h"


// 🔧 修改 - 按要求补充构造函数头部注释与逐行注释
/**
 * @brief 构造移动任务并初始化键过滤器
 * @param 无
 * @return 无
 * 功能说明: 设置节点名称并启用Tick更新
 * 详细流程: 设置显示名称 -> 开启Tick通知 -> 配置目标/范围键过滤
 * 注意事项: 目标键必须为对象类型
 */
UBTTask_XBMoveToTarget::UBTTask_XBMoveToTarget() {
  // 设置任务在行为树中的显示名称
  NodeName = TEXT("移动到目标");

  // 开启Tick更新
  bNotifyTick = true;
  // 开启任务结束通知
  bNotifyTaskFinished = true;

  // 配置目标键对象过滤
  TargetKey.AddObjectFilter(
      this, GET_MEMBER_NAME_CHECKED(UBTTask_XBMoveToTarget, TargetKey),
      AActor::StaticClass());
  // 配置攻击范围键浮点过滤
  AttackRangeKey.AddFloatFilter(
      this, GET_MEMBER_NAME_CHECKED(UBTTask_XBMoveToTarget, AttackRangeKey));
}

// 🔧 修改 - 按要求补充执行函数头部注释与逐行注释
/**
 * @brief 执行移动到目标任务
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @return 行为树执行结果
 * 功能说明: 通过寻路移动到目标攻击范围内
 * 详细流程: 获取控制器与目标 -> 校验目标有效性 -> 计算停止距离 -> 下发移动请求
 * 注意事项: 目标死亡时会清空黑板并失败
 */
EBTNodeResult::Type
UBTTask_XBMoveToTarget::ExecuteTask(UBehaviorTreeComponent &OwnerComp,
                                    uint8 *NodeMemory) {
  // 获取 AI 控制器
  AAIController *AIController = OwnerComp.GetAIOwner();
  if (!AIController)
    return EBTNodeResult::Failed;

  // 获取受控士兵
  AXBSoldierCharacter *Soldier =
      Cast<AXBSoldierCharacter>(AIController->GetPawn());
  if (!Soldier)
    return EBTNodeResult::Failed;

  // 获取黑板组件
  UBlackboardComponent *BlackboardComp = OwnerComp.GetBlackboardComponent();
  if (!BlackboardComp)
    return EBTNodeResult::Failed;

  // 从黑板读取当前目标
  AActor *CurrentTarget =
      Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
  if (!CurrentTarget)
    return EBTNodeResult::Failed;

  // 🔧 修改: 增加目标死亡检查 (Fail Fast)
  bool bTargetIsDead = false;
  if (AXBSoldierCharacter *TS = Cast<AXBSoldierCharacter>(CurrentTarget)) {
    if (TS->IsDead() || TS->GetSoldierState() == EXBSoldierState::Dead)
      bTargetIsDead = true;
  } else if (AXBCharacterBase *TL = Cast<AXBCharacterBase>(CurrentTarget)) {
    if (TL->IsDead())
      bTargetIsDead = true;
  }

  if (bTargetIsDead) {
    // 目标已死，立即清理数据并失败
    BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, nullptr);
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
    Soldier->CurrentAttackTarget = nullptr;
    return EBTNodeResult::Failed;
  }

  // 设置移动时的视觉焦点
  AIController->SetFocus(CurrentTarget);

  // 🔧 修改: 核心距离计算逻辑
  // 获取半径
  const float SoldierRadius = Soldier->GetSimpleCollisionRadius();
  const float TargetRadius = CurrentTarget->GetSimpleCollisionRadius();
  const float AttackRange = Soldier->GetAttackRange();

  // 1. 绝对停止距离 (用于判断成功)：100% 攻击范围 + 接触半径
  // 只要在这个距离内，就算到达，可以攻击
  const float AbsoluteStopDistance = AttackRange + SoldierRadius + TargetRadius;

  // 2. 移动目标距离 (用于 MoveTo)：90% 攻击范围 + 接触半径
  // 让士兵试图走得更近一点，留出误差缓冲 (Hysteresis)
  // 解决 "离目标121但范围是120" 的死锁问题
  const float MoveToDistance =
      (AttackRange * StopDistanceScale) + SoldierRadius + TargetRadius;

  // 计算当前距离
  float CurrentDistance = FVector::Dist2D(Soldier->GetActorLocation(),
                                          CurrentTarget->GetActorLocation());

  // 🔧 修改: 使用"绝对距离"判断是否已到达 (条件宽松)
  if (CurrentDistance <= AbsoluteStopDistance) {
    AIController->ClearFocus(EAIFocusPriority::Gameplay);
    // 如果真的很近，就不移动了，直接成功
    return EBTNodeResult::Succeeded;
  }

  // 🔧 修改: 使用"移动目标距离"下发请求 (条件严格)
  // 减去 5.0f 是为了保险，确保 NavMesh 寻路不会刚好停在边界外
  EPathFollowingRequestResult::Type MoveResult = AIController->MoveToActor(
      CurrentTarget, FMath::Max(0.0f, MoveToDistance - 5.0f),
      true, // StopOnOverlap
      true, // UsePathfinding
      true, // CanStrafe
      nullptr,
      true // AllowPartialPath
  );

  if (MoveResult == EPathFollowingRequestResult::RequestSuccessful) {
    // 🔧 修改 - 随机化初始计时器，避免所有士兵同一帧刷新寻路
    TargetUpdateTimer = FMath::RandRange(0.0f, TargetUpdateInterval * 0.5f);
    StuckTimer = 0.0f;
    return EBTNodeResult::InProgress;
  } else if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal) {
    return EBTNodeResult::Succeeded;
  }

  // 🔧 修改: 无法寻路到目标时清理目标，触发后续自动寻敌
  BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, nullptr);
  BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
  Soldier->CurrentAttackTarget = nullptr;

  return EBTNodeResult::Failed;
}

// 🔧 修改 - 按要求补充Tick函数头部注释与逐行注释
/**
 * @brief Tick 更新移动过程
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @param DeltaSeconds 帧间隔
 * @return 无
 * 功能说明: 持续检查目标有效性与距离，必要时更新移动请求
 * 详细流程: 获取控制器/士兵/黑板 -> 校验目标 -> 更新焦点 -> 判断距离 ->
 * 定期更新移动 注意事项: 目标死亡或丢失会终止任务
 */
void UBTTask_XBMoveToTarget::TickTask(UBehaviorTreeComponent &OwnerComp,
                                      uint8 *NodeMemory, float DeltaSeconds) {
  // 获取 AI 控制器
  AAIController *AIController = OwnerComp.GetAIOwner();
  if (!AIController) {
    FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
    return;
  }

  AXBSoldierCharacter *Soldier =
      Cast<AXBSoldierCharacter>(AIController->GetPawn());
  if (!Soldier) {
    FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
    return;
  }

  UBlackboardComponent *BlackboardComp = OwnerComp.GetBlackboardComponent();
  if (!BlackboardComp) {
    FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
    return;
  }

  AActor *Target =
      Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));

  // 1. 基础有效性检查
  if (!Target) {
    AIController->StopMovement();
    AIController->ClearFocus(EAIFocusPriority::Gameplay);
    FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
    return;
  }

  // 🔧 修改: 2. 增强的目标死亡检查 (关键修复: 目标死后立即停止移动)
  bool bTargetIsDead = false;
  if (AXBSoldierCharacter *TS = Cast<AXBSoldierCharacter>(Target)) {
    if (TS->IsDead() || TS->GetSoldierState() == EXBSoldierState::Dead)
      bTargetIsDead = true;
  } else if (AXBCharacterBase *TL = Cast<AXBCharacterBase>(Target)) {
    if (TL->IsDead())
      bTargetIsDead = true;
  }

  if (bTargetIsDead) {
    // 目标已死，清理黑板
    BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, nullptr);
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
    Soldier->CurrentAttackTarget = nullptr;

    // 停止移动并返回失败
    AIController->StopMovement();
    AIController->ClearFocus(EAIFocusPriority::Gameplay);
    FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
    return;
  }

  // 保持焦点
  AIController->SetFocus(Target);

  // 🔧 修改: 3. 距离判定逻辑 (同 ExecuteTask)
  const float SoldierRadius = Soldier->GetSimpleCollisionRadius();
  const float TargetRadius = Target->GetSimpleCollisionRadius();
  const float AttackRange = Soldier->GetAttackRange();

  // 宽松的判定距离 (100% Range)
  const float AbsoluteStopDistance = AttackRange + SoldierRadius + TargetRadius;
  // 严格的移动距离 (90% Range)
  const float MoveToDistance =
      (AttackRange * StopDistanceScale) + SoldierRadius + TargetRadius;

  float CurrentDistance =
      FVector::Dist2D(Soldier->GetActorLocation(), Target->GetActorLocation());

  // 如果在宽松距离内，视为成功
  if (CurrentDistance <= AbsoluteStopDistance) {
    AIController->StopMovement();
    AIController->ClearFocus(EAIFocusPriority::Gameplay);
    StuckTimer = 0.0f;
    FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    return;
  }

  // 🔧 修改 - 卡住检测：速度过低且距离未达标，触发目标切换
  if (UCharacterMovementComponent *MoveComp = Soldier->GetCharacterMovement()) {
    const float CurrentSpeed = MoveComp->Velocity.Size2D();
    if (CurrentSpeed <= MinMoveSpeed) {
      StuckTimer += DeltaSeconds;
    } else {
      StuckTimer = 0.0f;
    }
  }

  if (StuckTimer >= StuckTimeThreshold) {
    UE_LOG(LogXBAI, Warning, TEXT("移动任务卡住(%.1fs)，暂时放弃本次移动: %s"),
           StuckTimeThreshold, *Soldier->GetName());

    // 🔧 修改 - 卡住时不清除目标，而是返回 Failed 让行为树决定重试或切换
    // BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, nullptr);
    // BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
    // Soldier->CurrentAttackTarget = nullptr;

    AIController->StopMovement();
    AIController->ClearFocus(EAIFocusPriority::Gameplay);
    StuckTimer = 0.0f;
    FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
    return;
  }

  // 定期更新移动请求
  TargetUpdateTimer += DeltaSeconds;
  if (TargetUpdateTimer >= TargetUpdateInterval) {
    // 🔧 修改 - 随机化重置时间，防止固定频率脉冲
    TargetUpdateTimer = FMath::RandRange(0.0f, TargetUpdateInterval * 0.2f);

    // 使用严格距离继续逼近
    AIController->MoveToActor(Target, FMath::Max(0.0f, MoveToDistance - 5.0f),
                              true, true, true, nullptr, true);
  }
}

// 🔧 修改 - 按要求补充中止函数头部注释与逐行注释
/**
 * @brief 中止任务并清理移动状态
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @return 中止结果
 * 功能说明: 停止移动并清理焦点
 * 详细流程: 获取控制器 -> 停止移动 -> 清理焦点 -> 返回中止
 * 注意事项: 仅在任务被打断时调用
 */
EBTNodeResult::Type
UBTTask_XBMoveToTarget::AbortTask(UBehaviorTreeComponent &OwnerComp,
                                  uint8 *NodeMemory) {
  // 获取 AI 控制器并停止移动
  if (AAIController *AIController = OwnerComp.GetAIOwner()) {
    // 停止移动
    AIController->StopMovement();
    // 清理焦点
    AIController->ClearFocus(EAIFocusPriority::Gameplay);
  }

  // 返回中止结果
  return EBTNodeResult::Aborted;
}

// 🔧 修改 - 按要求补充描述函数头部注释与逐行注释
/**
 * @brief 获取任务静态描述
 * @param 无
 * @return 描述字符串
 * 功能说明: 展示目标键与默认停止距离
 * 详细流程: 拼接固定文本与参数
 * 注意事项: 仅用于编辑器显示
 */
FString UBTTask_XBMoveToTarget::GetStaticDescription() const {
  // 返回描述字符串
  return FString::Printf(TEXT("移动到目标\n目标键: %s\n停止距离: %.1f"),
                         *TargetKey.SelectedKeyName.ToString(),
                         DefaultStopDistance);
}
