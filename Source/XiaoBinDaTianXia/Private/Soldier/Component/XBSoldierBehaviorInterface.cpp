/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Soldier/Component/XBSoldierBehaviorInterface.cpp

/**
 * @file XBSoldierBehaviorInterface.cpp
 * @brief 士兵行为接口组件实现
 * 
 * @note ✨ 新增文件
 */

#include "Soldier/Component/XBSoldierBehaviorInterface.h"
#include "Utils/XBLogCategories.h"
#include "AI/XBSoldierPerceptionSubsystem.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Soldier/Component/XBSoldierFollowComponent.h"
#include "Data/XBSoldierDataAccessor.h"
#include "Character/XBCharacterBase.h"
#include "Utils/XBBlueprintFunctionLibrary.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"  // ✨ 新增 - 包含枚举定义
#include "NavigationSystem.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"  // ✨ 新增 - 世界遍历支持

UXBSoldierBehaviorInterface::UXBSoldierBehaviorInterface()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UXBSoldierBehaviorInterface::BeginPlay()
{
    Super::BeginPlay();

    // 缓存士兵引用
    CachedSoldier = Cast<AXBSoldierCharacter>(GetOwner());

    // 缓存感知子系统
    if (UWorld* World = GetWorld())
    {
        CachedPerceptionSubsystem = World->GetSubsystem<UXBSoldierPerceptionSubsystem>();

        // 注册到感知子系统
        if (CachedPerceptionSubsystem.IsValid() && CachedSoldier.IsValid())
        {
            CachedPerceptionSubsystem->RegisterActor(
                CachedSoldier.Get(),
                CachedSoldier->GetFaction()
            );
        }
    }

    UE_LOG(LogXBAI, Log, TEXT("士兵行为接口组件初始化: %s"), 
        CachedSoldier.IsValid() ? *CachedSoldier->GetName() : TEXT("无效"));
}

void UXBSoldierBehaviorInterface::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 更新攻击冷却
    UpdateAttackCooldown(DeltaTime);
}

// ==================== 内部辅助方法 ====================

AXBSoldierCharacter* UXBSoldierBehaviorInterface::GetOwnerSoldier() const
{
    return CachedSoldier.Get();
}

UXBSoldierPerceptionSubsystem* UXBSoldierBehaviorInterface::GetPerceptionSubsystem() const
{
    return CachedPerceptionSubsystem.Get();
}

// ✨ 新增 - 统一阵营解析入口，避免跨主将误伤
/**
 * @brief 解析目标阵营信息（优先使用士兵所属主将阵营）
 * @param Target 目标Actor
 * @param OutFaction 输出阵营
 * @param OutLeaderOwner 输出所属主将
 * @return 是否为可识别的战斗单位
 * @note   详细流程分析: 判定目标类型 -> 若为士兵则优先取主将阵营 -> 输出阵营/主将
 *         性能/架构注意事项: 该方法仅做轻量级类型判断，避免重复逻辑散落
 */
bool UXBSoldierBehaviorInterface::ResolveTargetFaction(AActor* Target, EXBFaction& OutFaction, AXBCharacterBase*& OutLeaderOwner) const
{
    // 初始化输出，避免上层使用脏数据
    OutFaction = EXBFaction::Neutral;
    OutLeaderOwner = nullptr;

    // 目标为空直接返回
    if (!Target)
    {
        return false;
    }

    // 若为士兵，优先使用其所属主将阵营
    if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(Target))
    {
        OutLeaderOwner = TargetSoldier->GetLeaderCharacter();
        OutFaction = OutLeaderOwner ? OutLeaderOwner->GetFaction() : TargetSoldier->GetFaction();
        return true;
    }

    // 若为主将，直接读取主将阵营
    if (AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Target))
    {
        OutLeaderOwner = TargetLeader;
        OutFaction = TargetLeader->GetFaction();
        return true;
    }

    return false;
}

void UXBSoldierBehaviorInterface::UpdateAttackCooldown(float DeltaTime)
{
    if (AttackCooldownTimer > 0.0f)
    {
        AttackCooldownTimer -= DeltaTime;
        if (AttackCooldownTimer < 0.0f)
        {
            AttackCooldownTimer = 0.0f;
        }
    }
}

// ==================== 感知行为实现 ====================

/**
 * @brief  搜索并获取最近的有效敌方目标
 * @param  OutEnemy [输出] 找到的敌方 Actor 指针（未找到置空）
 * @return bool 是否成功找到有效目标
 * @note   详细流程分析:
 * 1. 上下文获取：获取士兵自身与所属主将，确认当前阵营信息。
 * 2. 优先权判定：若主将刚攻击敌人，则记录敌方阵营作为“优先阵营”。
 * 3. 缓存利用：若缓存未过期，直接从缓存列表中选择优先目标。
 * 4. 直接扫描：不使用感知子系统，直接遍历世界中的敌方单位（士兵/主将），按优先阵营与距离挑选。
 * 5. 拥挤规避：统计友军对目标的集中度，对拥挤目标施加惩罚，减少扎堆卡位。
 * 6. 目标回退：优先阵营无可用目标时，回退到任意敌对阵营，避免目标长期为空。
 * 7. 最终择优：优先士兵、再主将；更新缓存与“最后看见敌人时间”。
 * * 架构注意事项:
 * 1. 战斗态优先使用直接扫描以保证实时性，避免队列延迟导致“断目标”。
 * 2. 通过优先阵营约束缩小扫描范围，降低遍历成本。
 * 3. 严格的过滤逻辑（Self/Leader/Hostile/Dead）是防止 AI 逻辑死锁的关键防线。
 * 4. 拥挤惩罚使用轻量级统计，避免昂贵的空间查询。
 */
bool UXBSoldierBehaviorInterface::SearchForEnemy(AActor*& OutEnemy)
{
    // 初始化输出参数，防止调用方使用未初始化的指针
    OutEnemy = nullptr;

    // 获取关键组件引用，若基础组件缺失则无法执行逻辑
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    UWorld* World = GetWorld();

    if (!Soldier || !World)
    {
        return false;
    }

    // 缓存主将引用与自身阵营，用于后续筛选器中的高频访问
    AXBCharacterBase* MyLeader = Soldier->GetLeaderCharacter();
    EXBFaction MyFaction = Soldier->GetFaction();

    // 初始化优先攻击阵营/主将数据，默认无优先目标
    EXBFaction PreferredFaction = EXBFaction::Neutral;
    bool bHasPreferredFaction = false;
    AXBCharacterBase* PreferredEnemyLeader = nullptr;
    bool bHasPreferredLeader = false;
    
    // 实现"集火"逻辑：若跟随主将，则尝试同步主将的攻击目标
    if (MyLeader)
    {
        // 尝试获取主将最近攻击的敌方主将/阵营，实现小队协同攻击
        EXBFaction LeaderEnemyFaction = EXBFaction::Neutral;
        
        // 优先策略 A：若主将攻击了具体敌方主将，则只锁定该主将及其士兵
        if (AXBCharacterBase* EnemyLeader = MyLeader->GetLastAttackedEnemyLeader())
        {
            if (!EnemyLeader->IsDead())
            {
                PreferredEnemyLeader = EnemyLeader;
                bHasPreferredLeader = true;
            }
        }

        // 优先策略 B：若无主将目标，则回退到阵营锁定
        if (!bHasPreferredLeader && MyLeader->GetLastAttackedEnemyFaction(LeaderEnemyFaction))
        {
            PreferredFaction = LeaderEnemyFaction;
            bHasPreferredFaction = true;
        }
    }

    // 定义目标筛选逻辑 (Lambda)，封装为独立函数以便在缓存检查和新查询中复用
    // 逻辑目标：在一次遍历中找出四种优先级的最佳候选，避免多次排序带来的性能开销 (O(N))
    auto SelectPriorityTarget = [&](const FXBPerceptionResult& Result) -> AActor*
    {
        if (!Soldier) return nullptr;

        // 维护四个最佳候选槽位，分别对应不同优先级
        AActor* NearestPreferredSoldier = nullptr;
        float NearestPreferredSoldierDistSq = MAX_FLT;

        AActor* NearestPreferredLeader = nullptr;
        float NearestPreferredLeaderDistSq = MAX_FLT;

        AActor* NearestSoldier = nullptr;
        float NearestSoldierDistSq = MAX_FLT;

        AActor* NearestLeader = nullptr;
        float NearestLeaderDistSq = MAX_FLT;

        const FVector SoldierLocation = Soldier->GetActorLocation();

        // 遍历感知到的所有潜在目标
        for (AActor* Candidate : Result.DetectedEnemies)
        {
            // 基础指针校验，过滤无效对象
            if (!Candidate || !IsValid(Candidate)) continue;

            // 🔧 关键修复 1: 绝对过滤自身
            // 防止距离计算为 0 导致 AI 锁定自己为敌人的逻辑死循环
            if (Candidate == Soldier) continue;

            // 🔧 关键修复 2: 绝对过滤自己跟随的主将
            // 防止感知系统误将友方主将纳入列表，导致"叛变"行为
            if (MyLeader && Candidate == MyLeader) continue;

            EXBFaction CandidateFaction = EXBFaction::Neutral;
            bool bIsSoldier = false;
            bool bIsLeader = false;
            AXBCharacterBase* CandidateLeaderOwner = nullptr;

            // 根据目标类型（士兵/武将）提取阵营并标记类型
            if (AXBSoldierCharacter* EnemySoldier = Cast<AXBSoldierCharacter>(Candidate))
            {
                // 忽略已死亡单位，防止鞭尸
                if (EnemySoldier->GetSoldierState() == EXBSoldierState::Dead) continue;
                // 🔧 修改 - 草丛隐身单位不可锁定
                if (EnemySoldier->IsHiddenInBush()) continue;
                // 🔧 修改 - 优先使用士兵所属主将阵营，避免跨主将误判
                ResolveTargetFaction(EnemySoldier, CandidateFaction, CandidateLeaderOwner);
                bIsSoldier = true;
            }
            else if (AXBCharacterBase* EnemyLeader = Cast<AXBCharacterBase>(Candidate))
            {
                if (EnemyLeader->IsDead()) continue;
                // 🔧 修改 - 草丛隐身主将不可锁定
                if (EnemyLeader->IsHiddenInBush()) continue;
                ResolveTargetFaction(EnemyLeader, CandidateFaction, CandidateLeaderOwner);
                bIsLeader = true;
            }
            else
            {
                // 忽略非角色类型的 Actor（如可破坏物等，视项目需求而定）
                continue;
            }

            // 🔧 修改 - 若主将明确锁定敌方主将，则只选择该主将及其士兵
            if (bHasPreferredLeader)
            {
                if (CandidateLeaderOwner != PreferredEnemyLeader)
                {
                    continue;
                }
            }

            // 🔧 关键修复 3: 核心敌对关系检查
            // 感知系统可能返回范围内所有单位，此处必须严格校验敌对关系
            if (!UXBBlueprintFunctionLibrary::AreFactionsHostile(MyFaction, CandidateFaction))
            {
                continue;
            }

            // --- 执行距离计算与择优更新 ---
            
            // 使用距离平方比较，避免开方运算带来的性能损耗
            const float DistSq = FVector::DistSquared(SoldierLocation, Candidate->GetActorLocation());
            // 判断是否属于主将正在攻击的"优先阵营"
            const bool bPreferred = bHasPreferredLeader
                ? (CandidateLeaderOwner == PreferredEnemyLeader)
                : (bHasPreferredFaction && CandidateFaction == PreferredFaction);

            // 根据单位类型和优先权更新对应的最近候选者
            if (bIsSoldier)
            {
                if (bPreferred)
                {
                    if (DistSq < NearestPreferredSoldierDistSq)
                    {
                        NearestPreferredSoldierDistSq = DistSq;
                        NearestPreferredSoldier = Candidate;
                    }
                }
                else
                {
                    if (DistSq < NearestSoldierDistSq)
                    {
                        NearestSoldierDistSq = DistSq;
                        NearestSoldier = Candidate;
                    }
                }
            }
            else if (bIsLeader)
            {
                if (bPreferred)
                {
                    if (DistSq < NearestPreferredLeaderDistSq)
                    {
                        NearestPreferredLeaderDistSq = DistSq;
                        NearestPreferredLeader = Candidate;
                    }
                }
                else
                {
                    if (DistSq < NearestLeaderDistSq)
                    {
                        NearestLeaderDistSq = DistSq;
                        NearestLeader = Candidate;
                    }
                }
            }
        }

        // 按照战略优先级返回结果：
        // 1. 优先阵营士兵 (集火清理杂兵)
        // 2. 普通敌方士兵 (就近原则 - 始终优先于主将)
        // 3. 优先阵营主将 (集火敌方核心)
        // 4. 普通敌方主将 (最后选择)
        if (NearestPreferredSoldier) return NearestPreferredSoldier;
        if (NearestSoldier) return NearestSoldier;
        if (NearestPreferredLeader) return NearestPreferredLeader;
        return NearestLeader;
    };

    // 性能优化：检查本地感知缓存是否在有效期内
    // 避免每一帧都执行昂贵的空间查询 (QuadTree/Octree 查询)
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - PerceptionCacheTime < PerceptionCacheValidity)
    {
        AActor* CachedTarget = SelectPriorityTarget(CachedPerceptionResult);
        // 只有当缓存中确实找到了符合严格筛选条件的目标时才返回
        // 如果缓存里全是已死单位或友军，则视为缓存无效，需强制刷新
        if (CachedTarget)
        {
            OutEnemy = CachedTarget;
            RecordEnemySeen();
            return true;
        }
    }

    // 缓存未命中或失效，准备发起新的直接扫描
    // 🔧 修改 - 不使用感知子系统，直接从世界中扫描敌方单位（仅战斗时使用效果最佳）
    CachedPerceptionResult = FXBPerceptionResult();
    CachedPerceptionResult.ResultTime = CurrentTime;
    CachedPerceptionResult.bIsValid = true;

    const float VisionRange = Soldier->GetVisionRange();
    const float VisionRangeSq = VisionRange * VisionRange;
    const FVector SoldierLocation = Soldier->GetActorLocation();

    // 🔧 修改 - 按阵营与兵种维护优先目标，支持“优先士兵”规则
    AActor* NearestPreferredSoldier = nullptr;
    float NearestPreferredSoldierDistSq = MAX_FLT;
    AActor* NearestPreferredLeader = nullptr;
    float NearestPreferredLeaderDistSq = MAX_FLT;
    AActor* NearestSoldier = nullptr;
    float NearestSoldierDistSq = MAX_FLT;
    AActor* NearestLeader = nullptr;
    float NearestLeaderDistSq = MAX_FLT;

    auto UpdateBestCandidate = [&](AActor* Candidate, bool bPreferred, bool bIsSoldier, float DistSq)
    {
        // 🔧 修改 - 优先阵营 + 士兵优先规则
        if (bIsSoldier)
        {
            if (bPreferred)
            {
                if (DistSq < NearestPreferredSoldierDistSq)
                {
                    NearestPreferredSoldierDistSq = DistSq;
                    NearestPreferredSoldier = Candidate;
                }
            }
            else
            {
                if (DistSq < NearestSoldierDistSq)
                {
                    NearestSoldierDistSq = DistSq;
                    NearestSoldier = Candidate;
                }
            }
        }
        else
        {
            if (bPreferred)
            {
                if (DistSq < NearestPreferredLeaderDistSq)
                {
                    NearestPreferredLeaderDistSq = DistSq;
                    NearestPreferredLeader = Candidate;
                }
            }
            else
            {
                if (DistSq < NearestLeaderDistSq)
                {
                    NearestLeaderDistSq = DistSq;
                    NearestLeader = Candidate;
                }
            }
        }
    };

    // 🔧 修改 - 使用 AActor 指针进行比较，避免不同类型指针直接比较导致编译报错
    AActor* SoldierActor = Soldier;
    AActor* LeaderActor = MyLeader;

    // 🔧 修改 - 仅在战斗态启用“拥挤规避”统计，减少非战斗时的开销
    const bool bEnableCrowdAvoidance = (Soldier->GetSoldierState() == EXBSoldierState::Combat);
    TMap<AActor*, int32> TargetAttackers;
    if (bEnableCrowdAvoidance)
    {
        // 🔧 修改 - 统计友军正在攻击的目标数量，降低目标拥挤度
        for (TActorIterator<AXBSoldierCharacter> It(World); It; ++It)
        {
            AXBSoldierCharacter* Friendly = *It;
            if (!Friendly || Friendly->GetSoldierState() == EXBSoldierState::Dead)
            {
                continue;
            }

            if (Friendly->GetFaction() != MyFaction)
            {
                continue;
            }

            AActor* FriendlyTarget = Friendly->CurrentAttackTarget.Get();
            if (!FriendlyTarget || !IsValid(FriendlyTarget))
            {
                continue;
            }

            // 🔧 修改 - 仅统计敌方目标，避免把友军聚集当作拥挤
            EXBFaction TargetFaction = EXBFaction::Neutral;
            AXBCharacterBase* TargetLeaderOwner = nullptr;
            if (!ResolveTargetFaction(FriendlyTarget, TargetFaction, TargetLeaderOwner))
            {
                continue;
            }

            // 🔧 修改 - 锁定主将时，仅统计该主将目标，避免错误拥挤惩罚
            if (bHasPreferredLeader && TargetLeaderOwner != PreferredEnemyLeader)
            {
                continue;
            }

            if (!UXBBlueprintFunctionLibrary::AreFactionsHostile(MyFaction, TargetFaction))
            {
                continue;
            }

            TargetAttackers.FindOrAdd(FriendlyTarget) += 1;
        }
    }

    auto GetCrowdPenalty = [&](AActor* Candidate) -> float
    {
        if (!bEnableCrowdAvoidance || !Candidate)
        {
            return 0.0f;
        }

        const int32* AttackerCount = TargetAttackers.Find(Candidate);
        if (!AttackerCount || *AttackerCount <= 0)
        {
            return 0.0f;
        }

        // 🔧 修改 - 以士兵半径为尺度进行惩罚，避免大量士兵挤到同一目标
        const float AvoidanceRadius = Soldier->GetSimpleCollisionRadius();
        const float CrowdPenaltyWeight = FMath::Max(200.0f, AvoidanceRadius * AvoidanceRadius);
        return static_cast<float>(*AttackerCount) * CrowdPenaltyWeight;
    };

    // 🔧 修改 - 先扫描士兵列表，确保“优先士兵”原则
    for (TActorIterator<AXBSoldierCharacter> It(World); It; ++It)
    {
        AXBSoldierCharacter* Candidate = *It;
        if (!Candidate || Candidate == SoldierActor)
        {
            continue;
        }

        if (LeaderActor && Candidate == LeaderActor)
        {
            continue;
        }

        if (Candidate->GetSoldierState() == EXBSoldierState::Dead)
        {
            continue;
        }
        // 🔧 修改 - 草丛隐身士兵不可锁定
        if (Candidate->IsHiddenInBush())
        {
            continue;
        }


        if (bHasPreferredLeader && Candidate->GetLeaderCharacter() != PreferredEnemyLeader)
        {
            continue;
        }

        // 🔧 新增 - 严重错误修复：士兵不能攻击未被招募的士兵（没有主将的士兵）
        // 士兵只能对有主将且不同阵营的士兵发动攻击
        AXBCharacterBase* CandidateLeader = Candidate->GetLeaderCharacter();
        if (!CandidateLeader)
        {
            // 未招募的士兵（没有主将）不能被攻击
            continue;
        }

        // 🔧 修改 - 优先读取所属主将阵营，避免跨主将误伤
        EXBFaction CandidateFaction = CandidateLeader->GetFaction();
        if (!UXBBlueprintFunctionLibrary::AreFactionsHostile(MyFaction, CandidateFaction))
        {
            continue;
        }

        const bool bPreferred = bHasPreferredLeader
            ? (Candidate->GetLeaderCharacter() == PreferredEnemyLeader)
            : (bHasPreferredFaction && CandidateFaction == PreferredFaction);
        if (!bPreferred && (bHasPreferredLeader || bHasPreferredFaction))
        {
            continue;
        }

        // 🔧 修改 - 先用真实距离过滤，再叠加拥挤惩罚用于排序
        const float DistSq = FVector::DistSquared(SoldierLocation, Candidate->GetActorLocation());
        if (DistSq > VisionRangeSq)
        {
            continue;
        }

        CachedPerceptionResult.DetectedEnemies.Add(Candidate);
        UpdateBestCandidate(Candidate, bPreferred, true, DistSq + GetCrowdPenalty(Candidate));
    }

    // 🔧 修改 - 再扫描主将列表，作为次级目标
    for (TActorIterator<AXBCharacterBase> It(World); It; ++It)
    {
        AXBCharacterBase* Candidate = *It;
        if (!Candidate || Candidate == SoldierActor)
        {
            continue;
        }

        if (LeaderActor && Candidate == LeaderActor)
        {
            continue;
        }

        if (Candidate->IsDead())
        {
            continue;
        }
        // 🔧 修改 - 草丛隐身主将不可锁定
        if (Candidate->IsHiddenInBush())
        {
            continue;
        }

        if (bHasPreferredLeader && Candidate != PreferredEnemyLeader)
        {
            continue;
        }

        // 🔧 修改 - 使用统一阵营解析，避免在主将类型上误调用接口
        EXBFaction CandidateFaction = EXBFaction::Neutral;
        AXBCharacterBase* CandidateLeaderOwner = nullptr;
        if (!ResolveTargetFaction(Candidate, CandidateFaction, CandidateLeaderOwner))
        {
            continue;
        }
        if (!UXBBlueprintFunctionLibrary::AreFactionsHostile(MyFaction, CandidateFaction))
        {
            continue;
        }

        const bool bPreferred = bHasPreferredLeader
            ? (Candidate == PreferredEnemyLeader)
            : (bHasPreferredFaction && CandidateFaction == PreferredFaction);
        if (!bPreferred && (bHasPreferredLeader || bHasPreferredFaction))
        {
            continue;
        }

        // 🔧 修改 - 先用真实距离过滤，再叠加拥挤惩罚用于排序
        const float DistSq = FVector::DistSquared(SoldierLocation, Candidate->GetActorLocation());
        if (DistSq > VisionRangeSq)
        {
            continue;
        }

        CachedPerceptionResult.DetectedEnemies.Add(Candidate);
        UpdateBestCandidate(Candidate, bPreferred, false, DistSq + GetCrowdPenalty(Candidate));
    }

    // 🔧 修改 - 当优先阵营没有任何可用目标时，允许回退到任意敌对阵营
    if (bHasPreferredFaction && !bHasPreferredLeader &&
        !NearestPreferredSoldier && !NearestPreferredLeader)
    {
        // 🔧 修改 - 回退扫描仅补充“其他阵营”候选，避免打断优先规则
        for (TActorIterator<AXBSoldierCharacter> It(World); It; ++It)
        {
            AXBSoldierCharacter* Candidate = *It;
            if (!Candidate || Candidate == SoldierActor)
            {
                continue;
            }

            if (LeaderActor && Candidate == LeaderActor)
            {
                continue;
            }

            if (Candidate->GetSoldierState() == EXBSoldierState::Dead)
            {
                continue;
            }

            // 🔧 修改 - 优先读取所属主将阵营，避免跨主将误伤
            // 🔧 修改 - 使用统一阵营解析，避免在主将类型上误调用接口
            EXBFaction CandidateFaction = EXBFaction::Neutral;
            AXBCharacterBase* CandidateLeaderOwner = nullptr;
            if (!ResolveTargetFaction(Candidate, CandidateFaction, CandidateLeaderOwner))
            {
                continue;
            }
            if (!UXBBlueprintFunctionLibrary::AreFactionsHostile(MyFaction, CandidateFaction))
            {
                continue;
            }

            // 🔧 修改 - 先用真实距离过滤，再叠加拥挤惩罚用于排序
            const float DistSq = FVector::DistSquared(SoldierLocation, Candidate->GetActorLocation());
            if (DistSq > VisionRangeSq)
            {
                continue;
            }

            CachedPerceptionResult.DetectedEnemies.Add(Candidate);
            UpdateBestCandidate(Candidate, false, true, DistSq + GetCrowdPenalty(Candidate));
        }

        for (TActorIterator<AXBCharacterBase> It(World); It; ++It)
        {
            AXBCharacterBase* Candidate = *It;
            if (!Candidate || Candidate == SoldierActor)
            {
                continue;
            }

            if (LeaderActor && Candidate == LeaderActor)
            {
                continue;
            }

            if (Candidate->IsDead())
            {
                continue;
            }

            // 🔧 修改 - 优先读取所属主将阵营，避免跨主将误伤
            // 🔧 修改 - 使用统一阵营解析，避免在主将类型上误调用接口
            EXBFaction CandidateFaction = EXBFaction::Neutral;
            AXBCharacterBase* CandidateLeaderOwner = nullptr;
            if (!ResolveTargetFaction(Candidate, CandidateFaction, CandidateLeaderOwner))
            {
                continue;
            }
            if (!UXBBlueprintFunctionLibrary::AreFactionsHostile(MyFaction, CandidateFaction))
            {
                continue;
            }

            // 🔧 修改 - 先用真实距离过滤，再叠加拥挤惩罚用于排序
            const float DistSq = FVector::DistSquared(SoldierLocation, Candidate->GetActorLocation());
            if (DistSq > VisionRangeSq)
            {
                continue;
            }

            CachedPerceptionResult.DetectedEnemies.Add(Candidate);
            UpdateBestCandidate(Candidate, false, false, DistSq + GetCrowdPenalty(Candidate));
        }
    }

    // 更新缓存时间戳
    PerceptionCacheTime = CurrentTime;

    // 对新的查询结果应用筛选逻辑
    AActor* PriorityTarget = nullptr;
    if (NearestPreferredSoldier)
    {
        PriorityTarget = NearestPreferredSoldier;
    }
    else if (NearestSoldier)
    {
        PriorityTarget = NearestSoldier;
    }
    else if (NearestPreferredLeader)
    {
        PriorityTarget = NearestPreferredLeader;
    }
    else
    {
        PriorityTarget = NearestLeader;
    }

    if (PriorityTarget)
    {
        CachedPerceptionResult.NearestEnemy = PriorityTarget;
        CachedPerceptionResult.DistanceToNearest = FVector::Dist(SoldierLocation, PriorityTarget->GetActorLocation());
        OutEnemy = PriorityTarget;
        RecordEnemySeen(); // 更新"最后看见敌人时间"，用于脱战判断
        return true;
    }

    return false;
}

bool UXBSoldierBehaviorInterface::HasEnemyInSight() const
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - PerceptionCacheTime < PerceptionCacheValidity)
    {
        // 🔧 修改 - 清理后检查数量
        // 注意：这里需要 const_cast 或者将方法改为非 const
        return CachedPerceptionResult.DetectedEnemies.Num() > 0;
    }

    // 缓存过期，执行新查询（复用搜索逻辑）
    AActor* TempEnemy = nullptr;
    // 🔧 修改 - 使用 const_cast 复用搜索逻辑，避免重复实现
    return const_cast<UXBSoldierBehaviorInterface*>(this)->SearchForEnemy(TempEnemy);
}

bool UXBSoldierBehaviorInterface::IsTargetValid(AActor* Target) const
{
    if (!Target || !IsValid(Target))
    {
        return false;
    }

    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return false;
    }

    // 🔧 安全增强 [建议添加]：攻击目标绝不能是自己
    if (Target == Soldier)
    {
        return false;
    }

    // 检查是否是士兵且已死亡
    if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(Target))
    {
        if (TargetSoldier->GetSoldierState() == EXBSoldierState::Dead)
        {
            return false;
        }
        // 🔧 修改 - 使用所属主将阵营作为有效阵营，避免跨主将误伤
        EXBFaction TargetFaction = TargetSoldier->GetFaction();
        if (AXBCharacterBase* TargetLeader = TargetSoldier->GetLeaderCharacter())
        {
            TargetFaction = TargetLeader->GetFaction();
        }
        return UXBBlueprintFunctionLibrary::AreFactionsHostile(Soldier->GetFaction(), TargetFaction);
    }

    // 检查是否是将领且已死亡
    if (AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Target))
    {
        if (TargetLeader->IsDead())
        {
            return false;
        }
        return UXBBlueprintFunctionLibrary::AreFactionsHostile(Soldier->GetFaction(), TargetLeader->GetFaction());
    }

    return true;
}

// ==================== 战斗行为实现 ====================

/**
 * @brief 执行攻击
 * @note 包含完整的攻击逻辑：冷却检查、距离检查、动画播放、伤害应用
 */
EXBBehaviorResult UXBSoldierBehaviorInterface::ExecuteAttack(AActor* Target)
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return EXBBehaviorResult::Failed;
    }

    // 🔧 修改 - 主将在草丛中时禁止攻击，保持跟随
    if (AXBCharacterBase* Leader = Soldier->GetLeaderCharacter())
    {
        if (Leader->IsHiddenInBush())
        {
            Soldier->ReturnToFormation();
            return EXBBehaviorResult::Failed;
        }
    }

    // 检查是否可以攻击
    if (!CanAttack(Target))
    {
        // 如果只是冷却中，返回进行中
        if (AttackCooldownTimer > 0.0f && IsInAttackRange(Target))
        {
            // 🔧 修改 - 冷却中也保持朝向目标并停止移动，避免在攻击范围内乱跑
            FaceTarget(Target, GetWorld()->GetDeltaSeconds());
            if (AAIController* AIController = Cast<AAIController>(Soldier->GetController()))
            {
                AIController->StopMovement();
            }
            return EXBBehaviorResult::InProgress;
        }
        return EXBBehaviorResult::Failed;
    }

    // 🔧 修改 - 进入攻击时停止移动并面向目标，保证攻击稳定触发
    if (AAIController* AIController = Cast<AAIController>(Soldier->GetController()))
    {
        AIController->StopMovement();
    }
    FaceTarget(Target, GetWorld()->GetDeltaSeconds());

    // 播放攻击蒙太奇（必须成功）
    if (!PlayAttackMontage())
    {
        UE_LOG(LogXBCombat, Warning, TEXT("士兵 %s 攻击失败：未能播放攻击蒙太奇"), *Soldier->GetName());
        return EXBBehaviorResult::Failed;
    }

    // 设置攻击冷却
    float AttackInterval = Soldier->GetAttackInterval();
    AttackCooldownTimer = AttackInterval;

    // 🔧 修改 - 近战伤害由蒙太奇Tag触发GA处理，避免提前结算
    // 弓手不使用该Tag，伤害应由投射物命中时处理
    if (Soldier->GetSoldierType() == EXBSoldierType::Archer)
    {
        UE_LOG(LogXBCombat, Verbose, TEXT("弓手攻击不走近战Tag: %s"), *Soldier->GetName());
    }

    // 🔧 修改 - 记录看见敌人，避免战斗状态被过早清理
    RecordEnemySeen();

    UE_LOG(LogXBCombat, Verbose, TEXT("士兵 %s 攻击 %s，等待近战Tag结算"),
        *Soldier->GetName(), *Target->GetName());

    // 广播行为完成
    OnBehaviorCompleted.Broadcast(FName("Attack"), EXBBehaviorResult::Success);

    return EXBBehaviorResult::Success;
}

bool UXBSoldierBehaviorInterface::CanAttack(AActor* Target) const
{
    if (!Target || !IsValid(Target))
    {
        return false;
    }

    // 冷却检查
    if (AttackCooldownTimer > 0.0f)
    {
        return false;
    }

    // 目标有效性检查
    if (!IsTargetValid(Target))
    {
        return false;
    }

    // 距离检查
    if (!IsInAttackRange(Target))
    {
        return false;
    }

    return true;
}

bool UXBSoldierBehaviorInterface::IsInAttackRange(AActor* Target) const
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier || !Target)
    {
        return false;
    }

    const float AttackRange = Soldier->GetAttackRange();
    const float SelfRadius = Soldier->GetSimpleCollisionRadius();
    const float TargetRadius = Target->GetSimpleCollisionRadius();
    const float CenterDistance = FVector::Dist2D(Soldier->GetActorLocation(), Target->GetActorLocation());
    const float EdgeDistance = CenterDistance - SelfRadius - TargetRadius;

    return EdgeDistance <= AttackRange;
}

bool UXBSoldierBehaviorInterface::PlayAttackMontage()
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return false;
    }

    UXBSoldierDataAccessor* DataAccessor = Soldier->GetDataAccessor();
    if (!DataAccessor || !DataAccessor->IsInitialized())
    {
        return false;
    }

    UAnimMontage* AttackMontage = DataAccessor->GetBasicAttackMontage();
    if (!AttackMontage)
    {
        return false;
    }

    USkeletalMeshComponent* Mesh = Soldier->GetMesh();
    if (!Mesh)
    {
        return false;
    }

    UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
    if (!AnimInstance)
    {
        return false;
    }

    return AnimInstance->Montage_Play(AttackMontage) > 0.0f;
}

void UXBSoldierBehaviorInterface::ApplyDamageToTarget(AActor* Target, float Damage)
{
    if (!Target)
    {
        return;
    }

    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return;
    }

    // 对士兵应用伤害
    if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(Target))
    {
        // 🔧 修改 - 使用所属主将阵营作为有效阵营，避免跨主将误伤
        EXBFaction TargetFaction = TargetSoldier->GetFaction();
        if (AXBCharacterBase* TargetLeader = TargetSoldier->GetLeaderCharacter())
        {
            TargetFaction = TargetLeader->GetFaction();
        }
        if (!UXBBlueprintFunctionLibrary::AreFactionsHostile(Soldier->GetFaction(), TargetFaction))
        {
            return;
        }
        TargetSoldier->TakeSoldierDamage(Damage, GetOwner());
    }
    // 对将领应用伤害（通过 GAS）
    else if (AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Target))
    {
        if (!UXBBlueprintFunctionLibrary::AreFactionsHostile(Soldier->GetFaction(), TargetLeader->GetFaction()))
        {
            return;
        }
        // TODO: 通过 GAS 应用伤害
        UE_LOG(LogXBCombat, Verbose, TEXT("士兵攻击将领，伤害待 GAS 处理"));
    }
}

// ==================== 移动行为实现 ====================

EXBBehaviorResult UXBSoldierBehaviorInterface::MoveToLocation(const FVector& TargetLocation, float AcceptanceRadius)
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return EXBBehaviorResult::Failed;
    }

    AAIController* AIController = Cast<AAIController>(Soldier->GetController());
    if (!AIController)
    {
        return EXBBehaviorResult::Failed;
    }

    // 检查是否已到达
    float Distance = FVector::Dist(Soldier->GetActorLocation(), TargetLocation);
    if (Distance <= AcceptanceRadius)
    {
        return EXBBehaviorResult::Success;
    }

    // 发起移动请求
    EPathFollowingRequestResult::Type Result = AIController->MoveToLocation(
        TargetLocation,
        AcceptanceRadius,
        true,
        true,
        true,
        true
    );

    // 🔧 修改 - 使用 if-else 替代 switch（避免枚举不完整问题）
    if (Result == EPathFollowingRequestResult::Type::RequestSuccessful)
    {
        return EXBBehaviorResult::InProgress;
    }
    else if (Result == EPathFollowingRequestResult::Type::AlreadyAtGoal)
    {
        return EXBBehaviorResult::Success;
    }
    else
    {
        return EXBBehaviorResult::Failed;
    }
}

EXBBehaviorResult UXBSoldierBehaviorInterface::MoveToActor(AActor* Target, float AcceptanceRadius)
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier || !Target)
    {
        return EXBBehaviorResult::Failed;
    }

    if (AcceptanceRadius < 0.0f)
    {
        AcceptanceRadius = Soldier->GetAttackRange() * 0.9f;
    }

    AAIController* AIController = Cast<AAIController>(Soldier->GetController());
    if (!AIController)
    {
        return EXBBehaviorResult::Failed;
    }

    float Distance = FVector::Dist(Soldier->GetActorLocation(), Target->GetActorLocation());
    if (Distance <= AcceptanceRadius)
    {
        return EXBBehaviorResult::Success;
    }

    EPathFollowingRequestResult::Type Result = AIController->MoveToActor(
        Target,
        AcceptanceRadius,
        true,
        true
    );

    // 🔧 修改 - 使用 if-else 替代 switch
    if (Result == EPathFollowingRequestResult::Type::RequestSuccessful)
    {
        return EXBBehaviorResult::InProgress;
    }
    else if (Result == EPathFollowingRequestResult::Type::AlreadyAtGoal)
    {
        return EXBBehaviorResult::Success;
    }
    else
    {
        return EXBBehaviorResult::Failed;
    }
}

EXBBehaviorResult UXBSoldierBehaviorInterface::ReturnToFormation()
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return EXBBehaviorResult::Failed;
    }

    // 检查是否已在编队位置
    if (IsAtFormationPosition())
    {
        return EXBBehaviorResult::Success;
    }

    // 获取编队位置
    FVector FormationPosition = Soldier->GetFormationWorldPositionSafe();
    if (FormationPosition.IsZero())
    {
        return EXBBehaviorResult::Failed;
    }

    // 移动到编队位置
    return MoveToLocation(FormationPosition, Soldier->GetArrivalThreshold());
}

void UXBSoldierBehaviorInterface::StopMovement()
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return;
    }

    if (AAIController* AIController = Cast<AAIController>(Soldier->GetController()))
    {
        AIController->StopMovement();
    }
}

bool UXBSoldierBehaviorInterface::IsAtFormationPosition() const
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return true;
    }

    FVector FormationPosition = Soldier->GetFormationWorldPositionSafe();
    if (FormationPosition.IsZero())
    {
        return true;
    }

    float Distance = FVector::Dist2D(Soldier->GetActorLocation(), FormationPosition);
    return Distance <= Soldier->GetArrivalThreshold();
}

// ==================== 决策辅助实现 ====================

/**
 * @brief  判断是否需要脱离战斗
 * @param  无
 * @return 是否脱离战斗
 * @note   详细流程分析: 先处理“目标为跟随态”超距脱战 -> 距离超限强制脱战 -> 目标非战斗时仅保留距离限制 -> 否则按无敌人时间判定
 *         性能/架构注意事项: 通过快速距离判断避免频繁感知查询，降低每帧开销
 */
bool UXBSoldierBehaviorInterface::ShouldDisengage() const
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return false;
    }

    // ✨ 新增 - 预先缓存追击距离与主将距离，减少重复计算
    float DisengageDistance = Soldier->GetDisengageDistance();
    float DistToLeader = GetDistanceToLeader();

    // ✨ 新增 - 目标状态判定：用于处理目标脱离战斗后的追击逻辑
    // 说明：当目标不处于战斗时，士兵允许追击，但必须受“追击距离”上限约束
    bool bIsTargetInCombat = true;
    bool bIsTargetFollowing = false;
    if (AActor* CurrentTarget = Soldier->CurrentAttackTarget.Get())
    {
        // 说明：目标类型不同，对应的战斗状态来源不同，必须区分读取以避免误判
        // 目标是士兵：检查其战斗状态
        if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(CurrentTarget))
        {
            bIsTargetInCombat = (TargetSoldier->GetSoldierState() == EXBSoldierState::Combat);
            bIsTargetFollowing = (TargetSoldier->GetSoldierState() == EXBSoldierState::Following);
        }
        // 目标是将领：检查其战斗状态
        else if (AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(CurrentTarget))
        {
            bIsTargetInCombat = TargetLeader->IsInCombat();
        }
    }

    // 🔧 修改 - 目标为跟随态时允许脱战：追击过远会导致队列散开
    if (bIsTargetFollowing && DistToLeader >= DisengageDistance)
    {
        UE_LOG(LogXBAI, Log, TEXT("士兵 %s 追击跟随目标超距: %.0f >= %.0f"),
            *Soldier->GetName(), DistToLeader, DisengageDistance);
        return true;
    }

    // 🔧 修改 - 距离超限时允许强制脱战，即使主将仍处于战斗
    if (DistToLeader >= DisengageDistance)
    {
        UE_LOG(LogXBAI, Verbose, TEXT("士兵 %s 距离将领过远: %.0f >= %.0f"),
            *Soldier->GetName(), DistToLeader, DisengageDistance);
        return true;
    }

    // 🔧 修改 - 目标脱离战斗时，优先进入追击模式，仅按追击距离判定是否脱战
    // 说明：此处直接返回 false 是为了维持追击，直到超过追击距离由上方条件触发脱战
    if (!bIsTargetInCombat)
    {
        return false;
    }

    // 条件2：长时间无敌人
    float ReturnDelay = Soldier->GetReturnDelay();
    float CurrentTime = GetWorld()->GetTimeSeconds();
    float TimeSinceLastEnemy = CurrentTime - LastEnemySeenTime;

    if (!HasEnemyInSight() && TimeSinceLastEnemy > ReturnDelay)
    {
        UE_LOG(LogXBAI, Verbose, TEXT("士兵 %s 长时间无敌人: %.1f > %.1f"),
            *Soldier->GetName(), TimeSinceLastEnemy, ReturnDelay);
        return true;
    }

    return false;
}

float UXBSoldierBehaviorInterface::GetDistanceToLeader() const
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier)
    {
        return MAX_FLT;
    }

    AActor* Leader = Soldier->GetFollowTarget();
    if (!Leader || !IsValid(Leader))
    {
        return MAX_FLT;
    }

    return FVector::Dist(Soldier->GetActorLocation(), Leader->GetActorLocation());
}

void UXBSoldierBehaviorInterface::RecordEnemySeen()
{
    LastEnemySeenTime = GetWorld()->GetTimeSeconds();
}

void UXBSoldierBehaviorInterface::FaceTarget(AActor* Target, float DeltaTime)
{
    AXBSoldierCharacter* Soldier = GetOwnerSoldier();
    if (!Soldier || !Target)
    {
        return;
    }

    FVector Direction = (Target->GetActorLocation() - Soldier->GetActorLocation()).GetSafeNormal2D();
    if (!Direction.IsNearlyZero())
    {
        FRotator TargetRotation = Direction.Rotation();
        FRotator CurrentRotation = Soldier->GetActorRotation();

        float RotationSpeed = Soldier->GetRotationSpeed();
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotationSpeed / 90.0f);
        Soldier->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
    }
}
