// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "XBSoldierTypes.generated.h"

/**
 * @brief 士兵兵种枚举
 * * 定义了游戏中所有可用的兵种类型，用于逻辑判断和资源加载。
 */
UENUM(BlueprintType)
enum class EXBSoldierType : uint8
{
    None        UMETA(DisplayName = "无"),
    Infantry    UMETA(DisplayName = "步兵"),
    Cavalry     UMETA(DisplayName = "骑兵"),
    Archer      UMETA(DisplayName = "弓手")
};

/**
 * @brief 士兵行为状态枚举
 * * 定义了单个士兵（Agent）在状态机中的当前状态。
 */
UENUM(BlueprintType)
enum class EXBSoldierState : uint8
{
    Idle,           // 待机（等待加入队列）
    Following,      // 队列跟随
    Engaging,       // 战斗追踪
    Seeking,        // 寻找新目标
    Returning,      // 返回队列
    InBuilding,     // 在建筑内
    Dead            // 死亡
};

/**
 * @brief 村民状态枚举
 * * 用于定义未被招募的村民的行为。
 */
UENUM(BlueprintType)
enum class EXBVillagerState : uint8
{
    Sleeping,       // 睡眠（头顶Zzz）
    Idle            // 待机
};

/**
 * @brief 阵营枚举
 * * 用于区分敌我关系，决定战斗逻辑的目标选择。
 */
UENUM(BlueprintType)
enum class EXBFaction : uint8
{
    Neutral,        // 中立（村民）
    Player,         // 玩家
    Enemy1,         // 敌方1
    Enemy2,         // 敌方2
    Enemy3          // 敌方3
};

/**
 * @brief 士兵包算术类型枚举
 * * 决定士兵包对士兵数量的影响方式（加减乘除）。
 */
UENUM(BlueprintType)
enum class EXBSoldierPackType : uint8
{
    Add,            // 加法（黄色）
    Subtract,       // 减法（红色）
    Multiply,       // 乘法（蓝色）
    Divide          // 除法（紫色）
};

/**
 * @brief 编队槽位数据结构
 * * 描述阵列中某一个具体位置的状态。
 * * @details 用于 FormationComponent 计算每个士兵应该站的位置。
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBFormationSlot
{
    GENERATED_BODY()

    
    /** 槽位在数组中的唯一索引 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "槽位索引"))
    int32 SlotIndex = INDEX_NONE;

    
    /** 该槽位相对于主将（Leader）的本地坐标偏移量 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "本地位置偏移"))
    FVector LocalOffset = FVector::ZeroVector;

    
    /** 标记该槽位当前是否已有士兵占据 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "是否被占用"))
    bool bIsOccupied = false;

    
    /** 占据该槽位的士兵的全局唯一 ID (SoldierId) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "占用者ID"))
    int32 OccupyingSoldierId = INDEX_NONE;

    /** 默认构造函数 */
    FXBFormationSlot() = default;
    
    /** 带参构造函数 */
    FXBFormationSlot(int32 InIndex, const FVector& InOffset)
        : SlotIndex(InIndex)
        , LocalOffset(InOffset)
        , bIsOccupied(false)
        , OccupyingSoldierId(INDEX_NONE)
    {}
};

/**
 * @brief 编队形状配置参数
 * * 控制士兵排列的紧密程度和跟随手感。
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBFormationConfig
{
    GENERATED_BODY()

    
    /** 队列中士兵之间的水平（左右）间隔距离 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "50.0", DisplayName = "横向间距"))
    float HorizontalSpacing = 100.0f;

    
    /** 队列中士兵之间的垂直（前后）间隔距离 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "50.0", DisplayName = "纵向间距"))
    float VerticalSpacing = 100.0f;

    
    /** 第一排士兵距离主将的最近距离，防止穿模 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "100.0", DisplayName = "距主将最小距离"))
    float MinDistanceToLeader = 150.0f;

    
    /** 士兵跟随目标位置时的插值速度（VInterpTo Speed），值越大响应越快 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1.0", DisplayName = "跟随平滑速度"))
    float FollowInterpSpeed = 5.0f;
};

/**
 * @brief 士兵基础数值配置
 * * 定义不同兵种的基础属性，用于生成 Agent 数据。
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBSoldierConfig
{
    GENERATED_BODY()

    
    /** 该配置对应的兵种类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "兵种类型"))
    EXBSoldierType SoldierType = EXBSoldierType::Infantry;

    
    /** 士兵的基础最大血量 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1.0", DisplayName = "基础血量"))
    float BaseHealth = 100.0f;

    
    /** 士兵单次攻击的基础伤害值 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", DisplayName = "基础伤害"))
    float BaseDamage = 10.0f;

    
    /** 士兵移动的最大速度（通常需要略高于主将以免掉队） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "100.0", DisplayName = "移动速度"))
    float MoveSpeed = 400.0f;

    
    /** 士兵的攻击判定半径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "50.0", DisplayName = "攻击范围"))
    float AttackRange = 150.0f;

    
    /** 两次攻击之间的时间间隔（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1", DisplayName = "攻击间隔"))
    float AttackInterval = 1.0f;

    
    /** 士兵追击敌人的最大距离，超过此距离将强制返回队列 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "500.0", DisplayName = "最大追击距离"))
    float MaxChaseDistance = 1500.0f;
};

/**
 * @brief 将领成长配置参数
 * * 定义主将如何随着携带士兵数量的增加而变强。
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBLeaderGrowthConfig
{
    GENERATED_BODY()

    
    /** 获得一个士兵时，主将模型缩放增加的数值（加法计算） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", DisplayName = "单兵缩放增量"))
    float ScalePerSoldier = 0.05f;

    
    /** 获得一个士兵时，主将血量上限增加的数值 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", DisplayName = "单兵血量增量"))
    float HealthPerSoldier = 50.0f;

    
    /** 主将模型缩放的最大上限倍率，防止过大穿模 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1.0", DisplayName = "最大缩放倍率"))
    float MaxScale = 3.0f;

    
    /** 磁场（吸附圈）的基础半径大小 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "100.0", DisplayName = "磁场基础半径"))
    float BaseMagnetRadius = 300.0f;
};

/**
 * @brief 轻量级士兵代理数据结构
 * * @details 这是群集 AI 的核心数据单元，用于替代笨重的 AActor。
 * * 包含士兵的所有运行时状态、位置、属性和逻辑标记。
 */
// 🔧 修改 - 重命名结构体，防止名字冲突
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBSoldierAgent
{
    GENERATED_BODY()

    // 🔧 修改 - 添加 BlueprintReadOnly 和 DisplayName
    /** 全局唯一ID，用于索引和查找 */
    UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "士兵ID"))
    int32 SoldierId = INDEX_NONE;

    // 🔧 修改 - 添加 BlueprintReadOnly 和 DisplayName
    /** 士兵所属阵营 */
    UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "所属阵营"))
    EXBFaction Faction = EXBFaction::Neutral;

    // 🔧 修改 - 添加 BlueprintReadOnly 和 DisplayName
    /** 士兵兵种类型 */
    UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "兵种类型"))
    EXBSoldierType SoldierType = EXBSoldierType::Infantry;

    // 🔧 修改 - 添加 BlueprintReadOnly 和 DisplayName
    /** 士兵当前行为状态 */
    UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "当前状态"))
    EXBSoldierState State = EXBSoldierState::Idle;

    // 🔧 修改 - 添加 BlueprintReadOnly 和 DisplayName
    /** 所属的主将 Actor 引用 */
    UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "所属主将"))
    TWeakObjectPtr<AActor> OwnerLeader;

    // 🔧 修改 - 添加 BlueprintReadOnly 和 DisplayName
    /** 在 FormationComponent 中的槽位索引 */
    UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "编队槽位索引"))
    int32 FormationSlotIndex = INDEX_NONE;

    // 🔧 修改 - 添加 BlueprintReadOnly 和 DisplayName
    /** 当前世界空间位置 */
    UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "当前位置"))
    FVector Position = FVector::ZeroVector;

    // 🔧 修改 - 添加 BlueprintReadOnly 和 DisplayName
    /** 当前世界空间旋转 */
    UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "当前旋转"))
    FRotator Rotation = FRotator::ZeroRotator;

    // 🔧 修改 - 添加 BlueprintReadOnly 和 DisplayName
    /** 移动逻辑的目标位置 */
    UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "目标位置"))
    FVector TargetPosition = FVector::ZeroVector;

    // 🔧 修改 - 添加 BlueprintReadOnly 和 DisplayName
    /** 当前锁定的敌人 ID */
    UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "目标敌人ID"))
    int32 TargetEnemyId = INDEX_NONE;

    // 🔧 修改 - 添加 BlueprintReadOnly 和 DisplayName
    /** 当前剩余血量 */
    UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "当前血量"))
    float CurrentHealth = 100.0f;

    // 🔧 修改 - 添加 BlueprintReadOnly 和 DisplayName
    /** 最大血量上限 */
    UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "最大血量"))
    float MaxHealth = 100.0f;

    // 🔧 修改 - 添加 BlueprintReadOnly 和 DisplayName
    /** 攻击冷却倒计时 */
    UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "攻击冷却"))
    float AttackCooldown = 0.0f;

    // 🔧 修改 - 添加 BlueprintReadOnly 和 DisplayName
    /** 当前速度向量，用于计算移动和惯性 */
    UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "速度向量"))
    FVector Velocity = FVector::ZeroVector;

    // 🔧 修改 - 添加 BlueprintReadOnly 和 DisplayName
    /** 对应的 Instanced Static Mesh (ISM) 实例索引，用于渲染 */
    UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "ISM实例索引"))
    int32 InstanceIndex = INDEX_NONE;

    // 🔧 修改 - 添加 BlueprintReadOnly 和 DisplayName
    /** Vertex Animation Texture (VAT) 的动画播放时间 */
    UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "动画时间"))
    float AnimationTime = 0.0f;

    // 🔧 修改 - 添加 BlueprintReadOnly 和 DisplayName
    /** 当前播放的 VAT 动画 ID */
    UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "当前动画ID"))
    int32 CurrentAnimationId = 0;

    // 🔧 修改 - 添加 BlueprintReadOnly 和 DisplayName
    /** 标记：是否在草丛中处于隐身状态 */
    UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "是否隐身"))
    bool bIsHidden = false;

    // 🔧 修改 - 添加 BlueprintReadOnly 和 DisplayName
    /** 标记：是否启用了物理碰撞（通常只在战斗中启用） */
    UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "是否启用碰撞"))
    bool bCollisionEnabled = false;

    // ---- 辅助方法 ----

    /** 检查数据是否有效 */
    bool IsValid() const { return SoldierId != INDEX_NONE; }
    /** 检查士兵是否存活 */
    bool IsAlive() const { return CurrentHealth > 0.0f && State != EXBSoldierState::Dead; }
    /** 检查士兵是否处于战斗相关的状态 */
    bool IsInCombat() const { return State == EXBSoldierState::Engaging || State == EXBSoldierState::Seeking; }
    /** 检查是否属于指定主将 */
    bool BelongsTo(const AActor* Leader) const { return OwnerLeader.IsValid() && OwnerLeader.Get() == Leader; }

    /** 重置数据为默认状态 */
    void Reset()
    {
        SoldierId = INDEX_NONE;
        Faction = EXBFaction::Neutral;
        SoldierType = EXBSoldierType::Infantry;
        State = EXBSoldierState::Idle;
        OwnerLeader.Reset();
        FormationSlotIndex = INDEX_NONE;
        Position = FVector::ZeroVector;
        Rotation = FRotator::ZeroRotator;
        TargetPosition = FVector::ZeroVector;
        TargetEnemyId = INDEX_NONE;
        CurrentHealth = 100.0f;
        MaxHealth = 100.0f;
        AttackCooldown = 0.0f;
        Velocity = FVector::ZeroVector;
        InstanceIndex = INDEX_NONE;
        AnimationTime = 0.0f;
        CurrentAnimationId = 0;
        bIsHidden = false;
        bCollisionEnabled = false;
    }
};

/**
 * 士兵状态变更委托
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FXBOnSoldierStateChanged, int32, SoldierId, EXBSoldierState, NewState);
/**
 * 士兵受伤委托
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FXBOnSoldierDamaged, int32, SoldierId, float, DamageAmount);
/**
 * 士兵死亡委托
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FXBOnSoldierDied, int32, SoldierId);