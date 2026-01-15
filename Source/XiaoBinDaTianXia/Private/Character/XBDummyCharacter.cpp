// Fill out your copyright notice in the Description page of Project Settings.


#include "Public/Character/XBDummyCharacter.h"
#include "AI/XBDummyAIController.h"
#include "Character/Components/XBCombatComponent.h"
#include "Character/Components/XBDummyAIDebugComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Utils/XBBlueprintFunctionLibrary.h"
#include "Utils/XBLogCategories.h"
#include "TimerManager.h"
#include "Components/SplineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"


AXBDummyCharacter::AXBDummyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 🔧 修改 - 绑定假人专用AI控制器，确保行为树可运行
	AIControllerClass = AXBDummyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// ?? 修改 - 默认假人主将阵营为“各自为战”
	Faction = EXBFaction::FreeForAll;

	// ✨ 新增 - 启用控制器旋转，以支持AI的SetFocus转向
	// 这样AI控制器调用SetFocus时，角色会平滑转向目标
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// 禁用移动方向旋转，避免与控制器旋转冲突
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;
		GetCharacterMovement()->bUseControllerDesiredRotation = true; // 使用控制器期望的旋转
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f); // 设置旋转速度
	}

	// ✨ 新增 - 创建AI调试组件
	AIDebugComponent = CreateDefaultSubobject<UXBDummyAIDebugComponent>(TEXT("AIDebugComponent"));
}


/**
 * @brief  假人初始化入口
 * @return 无
 * @note   详细流程分析: 复用父类通用初始化逻辑
 */
void AXBDummyCharacter::BeginPlay()
{
	Super::BeginPlay();
}


void AXBDummyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

/**
 * @brief  处理假人受击逻辑
 * @param  DamageSource 伤害来源
 * @param  DamageAmount 伤害数值
 * @return 无
 * @note   详细流程分析: 校验伤害 -> 计算随机延迟 -> 设置定时器 -> 触发行为树攻击
 *         性能/架构注意事项: 连续受击只保留最新一次响应，避免堆积定时器
 */
void AXBDummyCharacter::HandleDamageReceived(AActor* DamageSource, float DamageAmount)
{
	// 调用父类（保留通用处理）
	Super::HandleDamageReceived(DamageSource, DamageAmount);

	// 🔧 修改 - 只在真实伤害时响应
	if (DamageAmount <= 0.0f)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 🔧 修改 - 修正延迟范围，避免配置错误导致负值
	const float MinDelay = FMath::Max(0.0f, DamageResponseDelayMin);
	const float MaxDelay = FMath::Max(MinDelay, DamageResponseDelayMax);
	const float RandomDelay = FMath::FRandRange(MinDelay, MaxDelay);

	// 🔧 修改 - 记录伤害来源主将，用于中立/无敌人时反击
	if (DamageSource)
	{
		if (AXBCharacterBase* DamageLeader = Cast<AXBCharacterBase>(DamageSource))
		{
			LastDamageLeader = DamageLeader;
		}
		else if (AXBSoldierCharacter* DamageSoldier = Cast<AXBSoldierCharacter>(DamageSource))
		{
			LastDamageLeader = DamageSoldier->GetLeaderCharacter();
		}
	}

	// 🔧 修改 - 受击时刷新定时器，确保只响应最新一次受击
	World->GetTimerManager().ClearTimer(DamageResponseTimerHandle);
	World->GetTimerManager().SetTimer(
		DamageResponseTimerHandle,
		this,
		&AXBDummyCharacter::TriggerDamageResponse,
		RandomDelay,
		false
	);

	UE_LOG(LogXBCombat, Log, TEXT("假人 %s 受击触发延迟响应，延迟: %.2fs"),
		*GetName(), RandomDelay);
}

/**
 * @brief  初始化主将数据（假人）
 * @return 无
 * @note   详细流程分析: 默认从 Actor 内部配置与数据表初始化
 */
void AXBDummyCharacter::InitializeLeaderData()
{
	// 🔧 修改 - 假人仅使用父类通用初始化
	Super::InitializeLeaderData();

	// ?? 修改 - 覆盖假人移动方式，避免依赖数据表配置
	CachedLeaderData.AIConfig.MoveMode = DummyMoveMode;

	// 🔧 修改 - 初始化假人AI配置缓存，确保行为树读取到数据表AI配置
	if (AXBDummyAIController* DummyAI = Cast<AXBDummyAIController>(GetController()))
	{
		// 🔧 修改 - 复用控制器初始化接口，统一AI配置缓存逻辑
		DummyAI->InitializeLeaderAIConfig(this);
	}
	else
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人 %s 无法获取AI控制器，AI配置缓存初始化失败"), *GetName());
	}
}

/**
 * @brief  执行假人受击后的攻击逻辑
 * @return 是否成功执行攻击
 * @note   详细流程分析: 检查战斗组件 -> 判断冷却 -> 优先释放技能或普攻
 *         性能/架构注意事项: 仅负责释放，不处理目标选择
 */
bool AXBDummyCharacter::ExecuteDamageResponseAttack()
{
	// 🔧 修改 - 必须存在战斗组件才能释放技能/普攻
	if (!CombatComponent)
	{
		UE_LOG(LogXBCombat, Warning, TEXT("假人 %s 无战斗组件，无法释放技能/普攻"), *GetName());
		return false;
	}

	// 🔧 修改 - 读取冷却状态，按优先规则选择释放
	const bool bSkillOnCooldown = CombatComponent->IsSkillOnCooldown();
	const bool bBasicOnCooldown = CombatComponent->IsBasicAttackOnCooldown();

	// 🔧 修改 - 技能冷却则释放普攻，普攻冷却则释放技能
	if (bSkillOnCooldown && !bBasicOnCooldown)
	{
		return CombatComponent->PerformBasicAttack();
	}

	if (bBasicOnCooldown && !bSkillOnCooldown)
	{
		return CombatComponent->PerformSpecialSkill();
	}

	// 🔧 修改 - 两者都可用时优先释放技能
	if (!bSkillOnCooldown)
	{
		return CombatComponent->PerformSpecialSkill();
	}

	// 🔧 修改 - 两者都在冷却则不释放
	UE_LOG(LogXBCombat, Log, TEXT("假人 %s 技能与普攻均在冷却中"), *GetName());
	return false;
}

/**
 * @brief  获取最近造成伤害的主将
 * @return 主将指针（可能为空）
 * @note   详细流程分析: 返回缓存的伤害来源主将
 *         性能/架构注意事项: 仅用于AI反击目标判断
 */
AXBCharacterBase* AXBDummyCharacter::GetLastDamageLeader() const
{
	return LastDamageLeader.Get();
}

/**
 * @brief  清理最近伤害来源主将记录
 * @return 无
 * @note   详细流程分析: 反击成功锁定后清理，避免重复锁定
 *         性能/架构注意事项: 仅在AI服务中调用
 */
void AXBDummyCharacter::ClearLastDamageLeader()
{
	LastDamageLeader = nullptr;
}

/**
 * @brief  获取巡逻路线样条组件
 * @return 样条组件指针（可能为空）
 * @note   详细流程分析: 读取路线Actor -> 查找第一个Spline组件
 *         性能/架构注意事项: 仅在AI初始化时调用，避免每帧查找
 */
USplineComponent* AXBDummyCharacter::GetPatrolSplineComponent() const
{
	// 🔧 修改 - 未配置路线Actor时直接返回
	if (!PatrolSplineActor)
	{
		return nullptr;
	}

	// 🔧 修改 - 仅查找第一个样条组件
	return PatrolSplineActor->FindComponentByClass<USplineComponent>();
}

// ✨ 新增 - 延迟后触发攻击请求
/**
 * @brief 延迟回调：通知AI执行攻击
 * @return 无
 * @note   详细流程分析: 获取AI控制器 -> 写入黑板触发攻击任务
 *         性能/架构注意事项: 若AI控制器缺失则直接返回
 */
void AXBDummyCharacter::TriggerDamageResponse()
{
	// 🔧 修改 - 通过AI控制器写入黑板，触发行为空间逻辑
	if (AXBDummyAIController* DummyAI = Cast<AXBDummyAIController>(GetController()))
	{
		DummyAI->SetDamageResponseReady(true);
		UE_LOG(LogXBCombat, Log, TEXT("假人 %s 受击响应就绪，通知行为树执行攻击"), *GetName());
	}
	else
	{
		UE_LOG(LogXBCombat, Warning, TEXT("假人 %s 无法获取AI控制器，响应失败"), *GetName());
	}
}
