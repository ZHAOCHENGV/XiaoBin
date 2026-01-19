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
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
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

	// 获取当前选择的能力类型
	static const FName DefaultAbilityTypeKey(TEXT("SelectedAbilityType"));
	const FName AbilityTypeKeyName = AbilityTypeKey.SelectedKeyName.IsNone()
		? DefaultAbilityTypeKey
		: AbilityTypeKey.SelectedKeyName;
	const EXBDummyLeaderAbilityType SelectedAbilityType =
		static_cast<EXBDummyLeaderAbilityType>(Blackboard->GetValueAsInt(AbilityTypeKeyName));

	// 根据选择的能力类型计算攻击范围
	const float AttackRange = CalculateCurrentAttackRange(CombatComp, SelectedAbilityType);
	
	// 球体重叠检测（而非扫描）
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
 * @brief 根据选择的能力类型计算攻击范围
 * @param CombatComp 战斗组件
 * @param SelectedAbilityType 当前选择的能力类型（从黑板读取）
 * @return 对应能力的攻击范围
 * @note   直接根据黑板中的能力选择返回范围，不再依赖冷却状态
 */
float UBTService_XBDummyCombatRange::CalculateCurrentAttackRange(
	UXBCombatComponent* CombatComp, EXBDummyLeaderAbilityType SelectedAbilityType) const
{
	if (!CombatComp)
	{
		return 100.0f; // 默认值
	}

	// 获取技能和普攻的范围
	const float SkillRange = CombatComp->GetSkillAttackRange();
	const float BasicRange = CombatComp->GetBasicAttackRange();

	// 根据选择的能力类型返回对应范围
	switch (SelectedAbilityType)
	{
	case EXBDummyLeaderAbilityType::SpecialSkill:
		return SkillRange;
		
	case EXBDummyLeaderAbilityType::BasicAttack:
		return BasicRange;
		
	case EXBDummyLeaderAbilityType::None:
	default:
		// 如果没有选择能力，返回最小范围
		return FMath::Min(SkillRange, BasicRange);
	}
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
		UE_LOG(LogXBAI, Warning, TEXT("❌ 范围检测失败：Dummy 或 World 无效"));
		return false;
	}

	if (!TargetActor)
	{
		UE_LOG(LogXBAI, Warning, TEXT("❌ 范围检测失败：TargetActor 无效"));
		return false;
	}

	// 球体中心为AI的中心位置
	const FVector SphereCenter = Dummy->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	
	// 🔍 调试信息1：计算实际距离
	const float ActualDistance = FVector::Dist(SphereCenter, TargetLocation);
	UE_LOG(LogXBAI, Log, TEXT("🔍 范围检测开始：Dummy=%s, Target=%s, 检测范围=%.1f, 实际距离=%.1f"),
		*Dummy->GetName(), *TargetActor->GetName(), AttackRange, ActualDistance);

	// 配置碰撞查询参数
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Dummy); // 必须忽略自己，否则只会检测到自己
	QueryParams.bTraceComplex = false;
	
	// 🔍 调试信息2：检查目标的碰撞设置
	if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
	{
		if (UCapsuleComponent* CapsuleComp = TargetCharacter->GetCapsuleComponent())
		{
			const ECollisionEnabled::Type CollisionEnabled = CapsuleComp->GetCollisionEnabled();
			const ECollisionResponse CollisionResponse = CapsuleComp->GetCollisionResponseToChannel(ECC_Pawn);
			const ECollisionChannel ObjectType = CapsuleComp->GetCollisionObjectType();
			
			UE_LOG(LogXBAI, Log, TEXT("🔍 目标碰撞设置：CollisionEnabled=%d, ResponseToPawn=%d, ObjectType=%d"),
				static_cast<int32>(CollisionEnabled),
				static_cast<int32>(CollisionResponse),
				static_cast<int32>(ObjectType));
		}
	}

	// 配置碰撞对象类型
	// 🔧 修复：添加 Channel17 以支持 ObjectType=17 的检测
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);                    // Pawn通道 (3)
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel4);       // Leader通道 (7)
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel3);       // Soldier通道 (6)
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel17);      // 自定义通道17

	// 🔧 关键修复 - 使用 OverlapMultiByObjectType 而非 SweepMultiByObjectType
	// 原因：Sweep在起点和终点相同时可能无法检测到已经在球体内的物体
	TArray<FOverlapResult> OverlapResults;
	const bool bHit = Dummy->GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		SphereCenter,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(AttackRange),
		QueryParams
	);

	// 🔍 调试信息3：输出所有检测到的物体
	UE_LOG(LogXBAI, Log, TEXT("🔍 Overlap检测结果：检测到 %d 个物体"), OverlapResults.Num());
	for (int32 i = 0; i < OverlapResults.Num(); ++i)
	{
		const FOverlapResult& Overlap = OverlapResults[i];
		AActor* HitActor = Overlap.GetActor();
		UPrimitiveComponent* HitComp = Overlap.GetComponent();
		
		if (HitActor && HitComp)
		{
			const float Distance = FVector::Dist(SphereCenter, HitActor->GetActorLocation());
			UE_LOG(LogXBAI, Log, TEXT("   [%d] Actor=%s, Component=%s, Distance=%.1f, ObjectType=%d"),
				i, *HitActor->GetName(), *HitComp->GetName(), Distance,
				static_cast<int32>(HitComp->GetCollisionObjectType()));
		}
	}

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
		
		// 绘制到目标的连线
		DrawDebugLine(
			Dummy->GetWorld(),
			SphereCenter,
			TargetLocation,
			ActualDistance <= AttackRange ? FColor::Green : FColor::Red,
			DebugDrawType == EDrawDebugTrace::Persistent,
			DebugLifeTime,
			0,
			2.0f
		);
	}

	// 遍历重叠结果
	if (bHit)
	{
		for (const FOverlapResult& Overlap : OverlapResults)
		{
			// 过滤磁场组件
			if (UXBMagnetFieldComponent* MagnetComp = Cast<UXBMagnetFieldComponent>(Overlap.GetComponent()))
			{
				UE_LOG(LogXBAI, VeryVerbose, TEXT("   ⏭️ 跳过磁场组件: %s"), *MagnetComp->GetName());
				continue;
			}

			// 检查是否是目标
			ACharacter* HitCharacter = Cast<ACharacter>(Overlap.GetActor());
			if (HitCharacter && HitCharacter == TargetActor)
			{
				UE_LOG(LogXBAI, Log, TEXT("✅ 范围检测成功：目标 %s 在攻击范围内 (范围=%.1f, 实际距离=%.1f)"),
					*HitCharacter->GetName(), AttackRange, ActualDistance);
				return true;
			}
		}
		
		// 检测到了其他物体，但不是目标
		UE_LOG(LogXBAI, Warning, TEXT("⚠️ 范围检测失败：检测到%d个物体，但目标 %s 不在其中"),
			OverlapResults.Num(), *TargetActor->GetName());
	}
	else
	{
		// 完全没有检测到任何物体
		UE_LOG(LogXBAI, Warning, TEXT("❌ 范围检测失败：未检测到任何物体 (范围=%.1f, 实际距离=%.1f, 目标=%s)"),
			AttackRange, ActualDistance, *TargetActor->GetName());
	}

	return false;
}
