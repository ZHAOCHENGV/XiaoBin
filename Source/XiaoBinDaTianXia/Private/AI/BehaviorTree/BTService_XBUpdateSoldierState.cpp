/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTService_XBUpdateSoldierState.cpp

/**
 * @file BTService_XBUpdateSoldierState.cpp
 * @brief 行为树服务 - 更新士兵状态
 *
 * @note 🔧 重构 - 使用感知系统与行为接口
 */

#include "AI/BehaviorTree/BTService_XBUpdateSoldierState.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Soldier/Component/XBSoldierBehaviorInterface.h"  // ✅新增
#include "Character/XBCharacterBase.h"
#include "AI/XBSoldierAIController.h"

// 🔧 修改 - 按要求补充构造函数头部注释与逐行注释
/**
 * @brief 构造状态更新服务并初始化黑板键过滤器
 * @param 无
 * @return 无
 * 功能说明: 设置服务名称与更新间隔，配置目标/主将键过滤
 * 详细流程: 设置名称 -> 设置Interval/RandomDeviation -> 配置键过滤器
 * 注意事项: 键过滤器类型必须匹配黑板
 */
UBTService_XBUpdateSoldierState::UBTService_XBUpdateSoldierState()
{
    // 设置服务在行为树中的显示名称
    NodeName = TEXT("更新士兵状态");
    // 设置更新间隔
    Interval = 0.2f;
    // 设置随机偏差
    RandomDeviation = 0.05f;
    
    // 配置目标键对象过滤
    TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_XBUpdateSoldierState, TargetKey), AActor::StaticClass());
    // 配置主将键对象过滤
    LeaderKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_XBUpdateSoldierState, LeaderKey), AActor::StaticClass());
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
void UBTService_XBUpdateSoldierState::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
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
void UBTService_XBUpdateSoldierState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    // 调用父类Tick逻辑
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
    
    // 获取 AI 控制器
    AAIController* AIController = OwnerComp.GetAIOwner();
    // 控制器为空则不更新
    if (!AIController)
    {
        // 直接返回
        return;
    }
    
    // 获取受控士兵
    AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(AIController->GetPawn());
    // 士兵为空则不更新
    if (!Soldier)
    {
        // 直接返回
        return;
    }
    
    // 获取黑板组件
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    // 黑板为空则不更新
    if (!BlackboardComp)
    {
        // 直接返回
        return;
    }
    
    // 获取行为接口（可为空）
    UXBSoldierBehaviorInterface* BehaviorInterface = Soldier->GetBehaviorInterface();
    
    // 缓存士兵位置
    FVector SoldierLocation = Soldier->GetActorLocation();
    
    // ==================== 更新目标状态 ====================
    
    // 定义当前目标指针
    AActor* CurrentTarget = nullptr;
    // 从黑板读取目标
    if (TargetKey.SelectedKeyName != NAME_None)
    {
        // 读取目标对象
        CurrentTarget = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
    }
    
   // 🔧 修改: 增加对 IsDead 的强校验
    bool bTargetIsDead = false;
    if (CurrentTarget)
    {
        if (AXBSoldierCharacter* TS = Cast<AXBSoldierCharacter>(CurrentTarget))
        {
            if (TS->IsDead() || TS->GetSoldierState() == EXBSoldierState::Dead) bTargetIsDead = true;
        }
        else if (AXBCharacterBase* TL = Cast<AXBCharacterBase>(CurrentTarget))
        {
            if (TL->IsDead()) bTargetIsDead = true;
        }
    }

    bool bTargetValid = false;
    bool bTargetBecameInvalid = false;
    
    // 如果目标已死，强制视为无效
    if (bTargetIsDead)
    {
        bTargetValid = false;
        bTargetBecameInvalid = true; // 标记失效，触发下方寻敌逻辑
    }
    else if (bCheckTargetValidity && CurrentTarget && BehaviorInterface)
    {
        // 只有没死的时候才跑常规校验 (距离/视野等)
        bTargetValid = BehaviorInterface->IsTargetValid(CurrentTarget);
        if (!bTargetValid) bTargetBecameInvalid = true;
    }
    
    // 处理目标失效 (死亡或超出范围)
    if (!bTargetValid && (bTargetIsDead || bCheckTargetValidity))
    {
        // 只有当前有目标时才执行清理，避免重复日志
        if (CurrentTarget != nullptr)
        {
            BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, nullptr);
            CurrentTarget = nullptr;
            Soldier->CurrentAttackTarget = nullptr;
            UE_LOG(LogTemp, Log, TEXT("Service: 士兵 %s 的目标已失效(死亡或丢失)"), *Soldier->GetName());
        }
    }
    
    // 写入是否有目标标记
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, CurrentTarget != nullptr);
    
    // 若有目标则更新距离与位置
    if (CurrentTarget)
    {
        Soldier->CurrentAttackTarget = CurrentTarget;
        const float SelfRadius = Soldier->GetSimpleCollisionRadius();
        const float TargetRadius = CurrentTarget->GetSimpleCollisionRadius();
        float DistToTarget = FVector::Dist2D(SoldierLocation, CurrentTarget->GetActorLocation());
        // 计算边缘距离
        DistToTarget = FMath::Max(0.0f, DistToTarget - (SelfRadius + TargetRadius));
        
        BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToTarget, DistToTarget);
        BlackboardComp->SetValueAsVector(XBSoldierBBKeys::TargetLocation, CurrentTarget->GetActorLocation());

        if (BehaviorInterface) BehaviorInterface->RecordEnemySeen();
    }
    else
    {
        BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToTarget, MAX_FLT);
    }
    // ==================== 更新主将状态 ====================
    
    // 定义主将指针
    AActor* Leader = nullptr;
    // 从黑板读取主将
    if (LeaderKey.SelectedKeyName != NAME_None)
    {
        // 读取主将对象
        Leader = Cast<AActor>(BlackboardComp->GetValueAsObject(LeaderKey.SelectedKeyName));
    }
    
    // 黑板无主将则从跟随目标获取
    if (!Leader)
    {
        // 获取跟随目标作为主将
        Leader = Soldier->GetFollowTarget();
        // 若获取到主将则写回黑板
        if (Leader && LeaderKey.SelectedKeyName != NAME_None)
        {
            // 写回主将到黑板
            BlackboardComp->SetValueAsObject(LeaderKey.SelectedKeyName, Leader);
        }
    }
    
    // 若有主将则更新距离与撤退标记
    if (Leader)
    {
        // 计算到主将距离
        float DistToLeader = FVector::Dist(SoldierLocation, Leader->GetActorLocation());
        // 写入主将距离
        BlackboardComp->SetValueAsFloat(XBSoldierBBKeys::DistanceToLeader, DistToLeader);
        
        // 使用数据表脱离距离
        float DisengageDistanceValue = Soldier->GetDisengageDistance();
        // 初始化撤退标记
        bool bShouldRetreat = false;
        // 若有行为接口则用接口判断
        if (BehaviorInterface)
        {
            // 判断是否应脱离战斗
            bShouldRetreat = BehaviorInterface->ShouldDisengage();
            // 叠加距离阈值
            bShouldRetreat = bShouldRetreat && (DistToLeader >= DisengageDistanceValue);
        }
        else
        {
            // 仅用距离阈值判断
            bShouldRetreat = (DistToLeader >= DisengageDistanceValue);
        }
        // 写入撤退标记
        BlackboardComp->SetValueAsBool(XBSoldierBBKeys::ShouldRetreat, bShouldRetreat);
        
        // 若需要撤退且正在战斗则退出战斗
        if (bShouldRetreat && Soldier->GetSoldierState() == EXBSoldierState::Combat)
        {
            // 🔧 修改 - 仅处理超距回队，脱战延迟由将领统一调度
            if (DistToLeader >= DisengageDistanceValue)
            {
                Soldier->ExitCombat();
                Soldier->ReturnToFormation();
                UE_LOG(LogXBAI, Log, TEXT("士兵 %s 超距回队列"), *Soldier->GetName());
            }
        }
    }
    
    // ==================== 更新攻击状态 ====================
    
    // 写入是否可攻击
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::CanAttack, Soldier->CanAttack());
    // 写入士兵状态枚举
    BlackboardComp->SetValueAsEnum(XBSoldierBBKeys::SoldierState, static_cast<uint8>(Soldier->GetSoldierState()));
    
    // 计算是否处于战斗
    bool bInCombat = (Soldier->GetSoldierState() == EXBSoldierState::Combat);
    // 写入战斗标记
    BlackboardComp->SetValueAsBool(XBSoldierBBKeys::IsInCombat, bInCombat);
    
    // ==================== 自动寻找目标 ====================
    
    // 只有在战斗中，或者目标刚刚失效(比如刚打死一个)时，才自动寻找新目标
    if (bAutoFindTarget && !CurrentTarget && BehaviorInterface && (bInCombat || bTargetBecameInvalid))
    {
        AActor* NewTarget = nullptr;
        if (BehaviorInterface->SearchForEnemy(NewTarget))
        {
            // 🔧 核心修复：防止 Service 自动搜到自己
            if (NewTarget == Soldier)
            {
                NewTarget = nullptr;
            }
            
            if (NewTarget && TargetKey.SelectedKeyName != NAME_None)
            {
                BlackboardComp->SetValueAsObject(TargetKey.SelectedKeyName, NewTarget);
                BlackboardComp->SetValueAsBool(XBSoldierBBKeys::HasTarget, true);
                
                UE_LOG(LogTemp, Log, TEXT("士兵 %s 自动补位新目标 %s"),
                    *Soldier->GetName(), *NewTarget->GetName());
            }
        }
    }

    // 🔧 修改 - 若战斗中没有敌人，通知主将延迟脱战
    if (bInCombat && BehaviorInterface && !BehaviorInterface->HasEnemyInSight())
    {
        if (AXBCharacterBase* LeaderCharacter = Soldier->GetLeaderCharacter())
        {
            if (LeaderCharacter->HasEnemiesInCombat())
            {
                LeaderCharacter->SetHasEnemiesInCombat(false);
                LeaderCharacter->ScheduleNoEnemyDisengage();
                UE_LOG(LogXBAI, Log, TEXT("士兵 %s 通知主将 %s：无敌人，开始脱战计时"), 
                    *Soldier->GetName(), *LeaderCharacter->GetName());
            }
        }
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
FString UBTService_XBUpdateSoldierState::GetStaticDescription() const
{
    // 返回描述字符串
    return FString::Printf(TEXT("更新士兵状态（使用感知系统）\n目标键: %s\n主将键: %s"),
        *TargetKey.SelectedKeyName.ToString(),
        *LeaderKey.SelectedKeyName.ToString());
}
