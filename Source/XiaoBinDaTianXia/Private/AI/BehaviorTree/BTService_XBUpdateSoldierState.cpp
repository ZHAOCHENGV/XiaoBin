/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTService_XBUpdateSoldierState.cpp

/**
 * @file BTService_XBUpdateSoldierState.cpp
 * @brief 行为树服务 - 更新士兵状态
 *
 * @note 🔧 重构 - 使用感知系统与行为接口
 */

#include "AI/BehaviorTree/BTService_XBUpdateSoldierState.h"
#include "AI/XBSoldierAIController.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/XBCharacterBase.h"
#include "Soldier/Component/XBSoldierBehaviorInterface.h" // ✅新增
#include "Soldier/XBSoldierCharacter.h"
#include "Utils/XBLogCategories.h"

// 🔧 修改 - 按要求补充构造函数头部注释与逐行注释
/**
 * @brief 构造状态更新服务并初始化黑板键过滤器
 * @param 无
 * @return 无
 * 功能说明: 设置服务名称与更新间隔，配置目标/主将键过滤
 * 详细流程: 设置名称 -> 设置Interval/RandomDeviation -> 配置键过滤器
 * 注意事项: 键过滤器类型必须匹配黑板
 */
UBTService_XBUpdateSoldierState::UBTService_XBUpdateSoldierState() {
  // 设置服务在行为树中的显示名称
  NodeName = TEXT("更新士兵状态");
  // 设置更新间隔
  Interval = 0.2f;
  // 设置随机偏差
  RandomDeviation = 0.05f;

  // 配置目标键对象过滤
  TargetKey.AddObjectFilter(
      this, GET_MEMBER_NAME_CHECKED(UBTService_XBUpdateSoldierState, TargetKey),
      AActor::StaticClass());
  // 配置主将键对象过滤
  LeaderKey.AddObjectFilter(
      this, GET_MEMBER_NAME_CHECKED(UBTService_XBUpdateSoldierState, LeaderKey),
      AActor::StaticClass());
}

// 🔧 修改 - 按要求补充激活函数头部注释与逐行注释
/**
 * @brief 服务激活时触发一次更新
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @return 无
 * 功能说明: 激活时立即执行一次状态更新
 * 详细流程: 调用父类 -> 手动触发一次Tick
 * 注意事项: DeltaSeconds 置为0以避免异常时间差
 */
void UBTService_XBUpdateSoldierState::OnBecomeRelevant(
    UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) {
  // 调用父类逻辑
  Super::OnBecomeRelevant(OwnerComp, NodeMemory);
  // 触发一次更新
  TickNode(OwnerComp, NodeMemory, 0.0f);
}

// 🔧 修改 - 按要求补充Tick函数头部注释与逐行注释
/**
 * @brief 定期更新黑板与士兵状态
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @param DeltaSeconds 帧间隔
 * @return 无
 * 功能说明: 同步目标、距离、战斗状态与撤退标记
 * 详细流程: 获取控制器/士兵/黑板 -> 处理目标 -> 处理主将 -> 更新战斗与自动寻敌
 * 注意事项: 关键对象为空时直接返回
 */
void UBTService_XBUpdateSoldierState::TickNode(
    UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory, float DeltaSeconds) {
  // 调用父类Tick逻辑
  Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

  // 获取 AI 控制器
  AAIController *AIController = OwnerComp.GetAIOwner();
  // 控制器为空则不更新
  if (!AIController) {
    // 直接返回
    return;
  }

  // 获取受控士兵
  AXBSoldierCharacter *Soldier =
      Cast<AXBSoldierCharacter>(AIController->GetPawn());
  // 士兵为空则不更新
  if (!Soldier) {
    // 直接返回
    return;
  }

  // 获取黑板组件
  UBlackboardComponent *BlackboardComp = OwnerComp.GetBlackboardComponent();
  // 黑板为空则不更新
  if (!BlackboardComp) {
    // 直接返回
    return;
  }

  // 获取行为接口（可为空）
  UXBSoldierBehaviorInterface *BehaviorInterface =
      Soldier->GetBehaviorInterface();

  // 记录上一次战斗状态，避免目标死亡时误判退回跟随
  const bool bWasInCombat =
      BlackboardComp->GetValueAsBool(XBSoldierBBKeys::IsInCombat);

  // 缓存士兵位置
  FVector SoldierLocation = Soldier->GetActorLocation();

  // ==================== 更新目标状态 ====================

  // 定义当前目标指针
  AActor *CurrentTarget = nullptr;
  // 从黑板读取目标
  if (TargetKey.SelectedKeyName != NAME_None) {
    // 读取目标对象
    CurrentTarget = Cast<AActor>(
        BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
  }

  // 🔧 修改: 增加对 IsDead 的强校验
  bool bTargetIsDead = false;
  bool bTargetIsFriendly = false; // ✨ 新增 - 同主将友军标记
  bool bTargetCrossLeader =
      false; // ✨ 新增 - 跨主将标记（目标不属于主将锁敌目标）
  if (CurrentTarget) {
    // ✨ 新增 - 同主将友军判定
    AXBCharacterBase *MyLeader = Soldier->GetLeaderCharacter();
    AXBCharacterBase *TargetLeader = nullptr;

    if (AXBSoldierCharacter *TS = Cast<AXBSoldierCharacter>(CurrentTarget)) {
      if (TS->IsDead() || TS->GetSoldierState() == EXBSoldierState::Dead)
        bTargetIsDead = true;
      TargetLeader = TS->GetLeaderCharacter();
    } else if (AXBCharacterBase *TL = Cast<AXBCharacterBase>(CurrentTarget)) {
      if (TL->IsDead())
        bTargetIsDead = true;
      TargetLeader = TL;
    }

    // 同主将 = 同队友军，严禁攻击
    if (MyLeader && TargetLeader && MyLeader == TargetLeader) {
      bTargetIsFriendly = true;
      UE_LOG(LogXBAI, Verbose,
             TEXT("士兵 %s 的目标与自己同属一个主将，视为友军"),
             *Soldier->GetName());
    }

    // 🔧 修复 - 跨主将校验：目标必须属于主将锁定的敌方主将或其士兵
    // 说明：避免士兵锁定经过的主将C或主将C的士兵，仅允许锁定与主将交战的目标
    if (MyLeader && !bTargetIsFriendly) {
      AXBCharacterBase *EnemyLeader = MyLeader->GetLastAttackedEnemyLeader();
      if (!EnemyLeader || EnemyLeader->IsDead()) {
        // 主将未锁敌，所有目标都无效
        bTargetCrossLeader = true;
      } else if (TargetLeader && TargetLeader != EnemyLeader) {
        // 目标属于其他主将（非交战主将），视为跨主将锁敌
        bTargetCrossLeader = true;
        UE_LOG(LogXBAI, Verbose,
               TEXT("士兵 %s 的目标属于主将 %s，但主将交战目标是 "
                    "%s，视为跨主将锁敌"),
               *Soldier->GetName(), *TargetLeader->GetName(),
               *EnemyLeader->GetName());
      }
    }
  }

  bool bTargetValid = (CurrentTarget != nullptr);
  bool bTargetBecameInvalid = false;

  // 🔧 修复 - 目标死亡/友军/跨主将时清理目标
  // 说明：跨主将锁敌视为无效目标，避免士兵攻击非交战主将的单位
  if (bTargetIsDead || bTargetIsFriendly || bTargetCrossLeader) {
    bTargetValid = false;
    bTargetBecameInvalid = true;
  }

  // 处理目标失效 (死亡或超出范围)
  if (!bTargetValid && (bTargetIsDead || bCheckTargetValidity)) {
    // 只有当前有目标时才执行清理，避免重复日志
    if (CurrentTarget != nullptr) {
      BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, nullptr);
      CurrentTarget = nullptr;
      Soldier->CurrentAttackTarget = nullptr;
      UE_LOG(LogTemp, Log, TEXT("Service: 士兵 %s 的目标已失效(死亡或丢失)"),
             *Soldier->GetName());
    }
  }

  // 写入是否有目标标记
  BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget,
                                 CurrentTarget != nullptr);

  // 若有目标则更新距离与位置
  if (CurrentTarget) {
    Soldier->CurrentAttackTarget = CurrentTarget;
    const float SelfRadius = Soldier->GetSimpleCollisionRadius();
    const float TargetRadius = CurrentTarget->GetSimpleCollisionRadius();
    float DistToTarget =
        FVector::Dist2D(SoldierLocation, CurrentTarget->GetActorLocation());
    // 计算边缘距离
    DistToTarget = FMath::Max(0.0f, DistToTarget - (SelfRadius + TargetRadius));

    BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToTarget,
                                    DistToTarget);
    BlackboardComp->SetValueAsVector(XBSoldierBBKeys::TargetLocation,
                                     CurrentTarget->GetActorLocation());

    if (BehaviorInterface)
      BehaviorInterface->RecordEnemySeen();
  } else {
    BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToTarget, MAX_FLT);
  }
  // ==================== 更新主将状态 ====================

  // 定义主将指针
  AActor *Leader = nullptr;
  // 从黑板读取主将
  if (LeaderKey.SelectedKeyName != NAME_None) {
    // 读取主将对象
    Leader = Cast<AActor>(
        BlackboardComp->GetValueAsObject(LeaderKey.SelectedKeyName));
  }

  // 黑板无主将则从跟随目标获取
  if (!Leader) {
    // 获取跟随目标作为主将
    Leader = Soldier->GetFollowTarget();
    // 若获取到主将则写回黑板
    if (Leader && LeaderKey.SelectedKeyName != NAME_None) {
      // 写回主将到黑板
      BlackboardComp->SetValueAsObject(LeaderKey.SelectedKeyName, Leader);
    }
  }

  // 若有主将则更新距离与撤退标记
  if (Leader) {
    // 计算到主将距离
    float DistToLeader =
        FVector::Dist(SoldierLocation, Leader->GetActorLocation());
    // 写入主将距离
    BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToLeader,
                                    DistToLeader);

    // 🔧 修改 - 使用数据表追击距离作为脱战阈值
    float DisengageDistanceValue = Soldier->GetDisengageDistance();
    // 初始化撤退标记
    bool bShouldRetreat = false;
    // 若有行为接口则用接口判断
    if (BehaviorInterface) {
      // 判断是否应脱离战斗
      bShouldRetreat = BehaviorInterface->ShouldDisengage();
      // 🔧 修复 - 改为 OR 逻辑：接口判定脱战 OR 距离超限都应脱战
      // 说明：之前的 && 逻辑导致主将进入草丛时，如果距离不够远，士兵不会脱战
      if (!bShouldRetreat) {
        bShouldRetreat = (DistToLeader >= DisengageDistanceValue);
      }
    } else {
      // 仅用距离阈值判断
      bShouldRetreat = (DistToLeader >= DisengageDistanceValue);
    }
    // 写入撤退标记
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::ShouldRetreat,
                                   bShouldRetreat);

    // 🔧 修复 - 脱战时立即执行状态切换，确保士兵返回跟随
    if (bShouldRetreat &&
        Soldier->GetSoldierState() == EXBSoldierState::Combat) {
      // 检查是否因主将草丛而脱战（最高优先级）
      bool bLeaderInBush = false;
      if (AXBCharacterBase *LeaderCharacter = Soldier->GetLeaderCharacter()) {
        if (LeaderCharacter->IsHiddenInBush()) {
          bLeaderInBush = true;
        }
      }

      // 主将在草丛中：立即脱战并跟随
      if (bLeaderInBush) {
        Soldier->ExitCombat();
        Soldier->ReturnToFormation();

        // 🔧 修复 - 清理黑板状态，确保与实际状态一致
        BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, nullptr);
        BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, false);
        BlackboardComp->SetValueAsBool(XBSoldierBBKeys::IsInCombat, false);
        BlackboardComp->SetValueAsBool(XBSoldierBBKeys::ShouldRetreat, false);
        Soldier->CurrentAttackTarget = nullptr;

        UE_LOG(LogXBAI, Log, TEXT("士兵 %s 因主将进入草丛而脱战，返回跟随"),
               *Soldier->GetName());
        return;
      }

      // ✨ 核心修复 - 战斗状态锁定
      // 战斗中且有有效目标时，强制 ShouldRetreat = false
      // 除非距离超过极远阈值（3000+），否则战斗优先级高于跟随优先级
      const float MaxTeleportDistance = 3000.0f;

      if (CurrentTarget && DistToLeader < MaxTeleportDistance) {
        // 战斗状态锁定：有目标且未追太远，绝对不允许回队
        BlackboardComp->SetValueAsBool(XBSoldierBBKeys::ShouldRetreat, false);
        goto SkipRetreatLogic;
      }

      // 极远距离时强制脱战（超过3000码）
      if (DistToLeader >= MaxTeleportDistance) {
        Soldier->ExitCombat();
        Soldier->ReturnToFormation();
        UE_LOG(LogXBAI, Log, TEXT("士兵 %s 极远距离强制脱战回队列 (距离=%.0f)"),
               *Soldier->GetName(), DistToLeader);
        return;
      }
      // 主将已脱战时跟随脱战
      if (AXBCharacterBase *LeaderCharacter = Soldier->GetLeaderCharacter()) {
        if (!LeaderCharacter->IsInCombat()) {
          Soldier->ExitCombat();
          Soldier->ReturnToFormation();
          UE_LOG(LogXBAI, Log, TEXT("士兵 %s 跟随主将脱战回队列"),
                 *Soldier->GetName());
        }
      }
    }
  }

  // 🔧 修复 - 目标死亡不应导致其他士兵脱战
  // 说明：当目标失效但主将仍在战斗，且士兵上一帧已处于战斗，则保持战斗状态并寻找新目标
  if (bTargetBecameInvalid && bWasInCombat &&
      Soldier->GetSoldierState() != EXBSoldierState::Combat) {
    if (const AXBCharacterBase *LeaderCharacter =
            Soldier->GetLeaderCharacter()) {
      if (LeaderCharacter->IsInCombat()) {
        Soldier->EnterCombat();
      }
    }
  }

SkipRetreatLogic:
  // ==================== 更新攻击状态 ====================

  // 写入是否可攻击
  BlackboardComp->SetValueAsBool(XBSoldierBBKeys::CanAttack,
                                 Soldier->CanAttack());
  // 写入士兵状态枚举
  BlackboardComp->SetValueAsEnum(
      XBSoldierBBKeys::SoldierState,
      static_cast<uint8>(Soldier->GetSoldierState()));

  // 计算是否处于战斗
  bool bInCombat = (Soldier->GetSoldierState() == EXBSoldierState::Combat);
  // 写入战斗标记
  BlackboardComp->SetValueAsBool(XBSoldierBBKeys::IsInCombat, bInCombat);

  // ==================== 被动申请目标 ====================

  // 仅在目标死亡/友军失效时触发申请，不主动索敌
  if (bAutoFindTarget && bTargetBecameInvalid) {
    Soldier->RequestNewTarget();
  }
}

// 🔧 修改 - 按要求补充描述函数头部注释与逐行注释
/**
 * @brief 获取服务静态描述
 * @param 无
 * @return 描述字符串
 * 功能说明: 展示目标键与主将键
 * 详细流程: 拼接固定文本与键名
 * 注意事项: 仅用于编辑器显示
 */
FString UBTService_XBUpdateSoldierState::GetStaticDescription() const {
  // 返回描述字符串
  return FString::Printf(
      TEXT("更新士兵状态（被动目标）\n目标键: %s\n主将键: %s"),
      *TargetKey.SelectedKeyName.ToString(),
      *LeaderKey.SelectedKeyName.ToString());
}
