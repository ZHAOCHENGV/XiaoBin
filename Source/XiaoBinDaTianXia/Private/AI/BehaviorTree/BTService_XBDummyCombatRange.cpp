/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTService_XBDummyCombatRange.cpp

/**
 * @file BTService_XBDummyCombatRange.cpp
 * @brief 行为树服务 - 假人主将战斗范围检测实现
 *
 * @note ✨ 新增 - 专用于选择攻击能力并检测目标是否在攻击范围内
 */

#include "AI/BehaviorTree/BTService_XBDummyCombatRange.h"
#include "AI/XBDummyAIType.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/XBDummyCharacter.h"
#include "Character/XBCharacterBase.h"
#include "Character/Components/XBCombatComponent.h"
#include "Character/Components/XBMagnetFieldComponent.h"
#include "Utils/XBLogCategories.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"

UBTService_XBDummyCombatRange::UBTService_XBDummyCombatRange()
{
	NodeName = TEXT("假人战斗范围检测");
	
	// 设置Tick间隔
	Interval = 0.1f;
	RandomDeviation = 0.0f;
	
	// 配置默认黑板键
	TargetKey.SelectedKeyName = TEXT("TargetLeader");
	AbilityTypeKey.SelectedKeyName = TEXT("SelectedAbilityType");
	AbilityTypeKey.AddIntFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_XBDummyCombatRange, AbilityTypeKey));
	IsInAttackRangeKey.SelectedKeyName = TEXT("IsInAttackRange");
	IsInAttackRangeKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_XBDummyCombatRange, IsInAttackRangeKey));
}

/**
 * @brief 服务Tick更新
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @param DeltaSeconds 帧间隔
 * @return 无
 * @note   详细流程分析: 选择能力 -> 计算范围 -> 球体碰撞检测 -> 更新黑板
 */
void UBTService_XBDummyCombatRange::TickNode(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	// 获取AI控制器
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return;
	}

	// 获取假人主将
	AXBDummyCharacter* Dummy = Cast<AXBDummyCharacter>(AIController->GetPawn());
	if (!Dummy || Dummy->IsDead())
	{
		return;
	}

	// 获取黑板组件
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return;
	}

	// 获取目标
	static const FName DefaultTargetKey(TEXT("TargetLeader"));
	const FName TargetKeyName = TargetKey.SelectedKeyName.IsNone()
		? DefaultTargetKey
		: TargetKey.SelectedKeyName;
	AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(TargetKeyName));
	
	// 无目标时设置为不在范围内
	if (!Target)
	{
		static const FName DefaultIsInAttackRangeKey(TEXT("IsInAttackRange"));
		const FName IsInAttackRangeKeyName = IsInAttackRangeKey.SelectedKeyName.IsNone()
			? DefaultIsInAttackRangeKey
			: IsInAttackRangeKey.SelectedKeyName;
		Blackboard->SetValueAsBool(IsInAttackRangeKeyName, false);
		return;
	}

	// 检查目标有效性
	if (AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Target))
	{
		if (TargetLeader->IsDead() || TargetLeader->IsHiddenInBush())
		{
			static const FName DefaultIsInAttackRangeKey(TEXT("IsInAttackRange"));
			const FName IsInAttackRangeKeyName = IsInAttackRangeKey.SelectedKeyName.IsNone()
				? DefaultIsInAttackRangeKey
				: IsInAttackRangeKey.SelectedKeyName;
			Blackboard->SetValueAsBool(IsInAttackRangeKeyName, false);
			return;
		}
	}

	// 获取战斗组件
	UXBCombatComponent* CombatComp = Dummy->GetCombatComponent();
	if (!CombatComp)
	{
		return;
	}

	// 计算当前优先攻击范围
	const float AttackRange = CalculateCurrentAttackRange(CombatComp);
	
	// 球体碰撞检测
	const bool bInRange = CheckTargetInAttackRange(Dummy, AttackRange, Target);
	
	// 更新黑板变量
	static const FName DefaultIsInAttackRangeKey(TEXT("IsInAttackRange"));
	const FName IsInAttackRangeKeyName = IsInAttackRangeKey.SelectedKeyName.IsNone()
		? DefaultIsInAttackRangeKey
		: IsInAttackRangeKey.SelectedKeyName;
	
	// 仅在值改变时更新，减少性能开销
	const bool bCurrentValue = Blackboard->GetValueAsBool(IsInAttackRangeKeyName);
	if (bInRange != bCurrentValue)
	{
		Blackboard->SetValueAsBool(IsInAttackRangeKeyName, bInRange);
		UE_LOG(LogXBAI, Log, TEXT("假人 %s 范围检测服务：IsInAttackRange=%s (范围=%.1f)"), 
			*Dummy->GetName(), bInRange ? TEXT("true") : TEXT("false"), AttackRange);
	}
}

/**
 * @brief 计算当前优先攻击范围
 * @param CombatComp 战斗组件
 * @return 当前应使用的攻击范围（技能优先、普攻其次）
 * @note   优先级: 技能未冷却用技能范围 > 普攻未冷却用普攻范围 > 都冷却用最小范围
 */
float UBTService_XBDummyCombatRange::CalculateCurrentAttackRange(UXBCombatComponent* CombatComp) const
{
	if (!CombatComp)
	{
		return 100.0f; // 默认值
	}

	// 获取技能和普攻的范围与冷却状态
	const float SkillRange = CombatComp->GetSkillAttackRange();
	const float BasicRange = CombatComp->GetBasicAttackRange();
	const bool bSkillOnCooldown = CombatComp->IsSkillOnCooldown();
	const bool bBasicOnCooldown = CombatComp->IsBasicAttackOnCooldown();

	// 优先级1：技能未冷却，使用技能范围
	if (!bSkillOnCooldown)
	{
		return SkillRange;
	}

	// 优先级2：普攻未冷却，使用普攻范围
	if (!bBasicOnCooldown)
	{
		return BasicRange;
	}

	// 优先级3：都在冷却，使用最小范围
	return FMath::Min(SkillRange, BasicRange);
}

/**
 * @brief 检查目标是否在攻击范围内（球体碰撞检测）
 * @param Dummy 假人AI
 * @param AttackRange 攻击范围
 * @param TargetActor 目标Actor
 * @return 是否在范围内
 * @note   使用球体碰撞检测，过滤磁场组件
 */
bool UBTService_XBDummyCombatRange::CheckTargetInAttackRange(
	AActor* Dummy, float AttackRange, AActor* TargetActor) const
{
	if (!Dummy || !Dummy->GetWorld())
	{
		return false;
	}

	// 球体中心为AI的中心位置
	const FVector SphereCenter = Dummy->GetActorLocation();

	// 配置碰撞查询参数
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Dummy);
	QueryParams.bTraceComplex = false;

	// 配置碰撞对象类型
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel4); // Leader通道
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel3); // Soldier通道

	// 执行球体碰撞检测（支持可配置的调试绘制）
	TArray<FHitResult> HitResults;
	const bool bHit = Dummy->GetWorld()->SweepMultiByObjectType(
		HitResults,
		SphereCenter,
		SphereCenter,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(AttackRange),
		QueryParams
	);

	// 调试绘制球体范围（根据配置的枚举值）
	if (DebugDrawType != EDrawDebugTrace::None)
	{
		const FColor DebugColor = bHit ? FColor::Green : FColor::Red;
		const float DebugLifeTime = (DebugDrawType == EDrawDebugTrace::ForDuration) ? 0.5f : -1.0f;
		DrawDebugSphere(
			Dummy->GetWorld(),
			SphereCenter,
			AttackRange,
			32, // 球体段数
			DebugColor,
			DebugDrawType == EDrawDebugTrace::Persistent, // 是否持久绘制
			DebugLifeTime
		);
	}

	// 遍历命中结果
	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			// 过滤磁场组件
			if (UXBMagnetFieldComponent* MagnetComp = Cast<UXBMagnetFieldComponent>(Hit.GetComponent()))
			{
				continue;
			}

			// 检查是否是目标
			ACharacter* HitPawn = Cast<ACharacter>(Hit.GetActor());
			if (HitPawn && HitPawn == TargetActor)
			{
				return true;
			}
		}
	}

	return false;
}
