/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTTask_XBDummyMoveToTarget.cpp

/**
 * @file BTTask_XBDummyMoveToTarget.cpp
 * @brief 假人主将智能移动任务实现
 */

#include "AI/BehaviorTree/BTTask_XBDummyMoveToTarget.h"
#include "AI/XBDummyAIType.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/XBDummyCharacter.h"
#include "Character/XBCharacterBase.h"
#include "Character/Components/XBCombatComponent.h"
#include "Character/Components/XBMagnetFieldComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Components/CapsuleComponent.h"
#include "Utils/XBLogCategories.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"

/**
 * @brief 构造函数
 */
UBTTask_XBDummyMoveToTarget::UBTTask_XBDummyMoveToTarget()
{
	// 设置任务名称
	NodeName = TEXT("假人主将智能移动");
	
	// 开启Tick更新
	bNotifyTick = true;
	bNotifyTaskFinished = true;
	
	// 配置默认目标键
	TargetKey.SelectedKeyName = TEXT("TargetLeader");
	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBDummyMoveToTarget, TargetKey), AActor::StaticClass());

	// ✨ 新增 - 配置能力类型键，用于决定移动到哪个攻击范围
	AbilityTypeKey.SelectedKeyName = TEXT("SelectedAbilityType");
	AbilityTypeKey.AddIntFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBDummyMoveToTarget, AbilityTypeKey));
}

/**
 * @brief 检查目标是否在攻击范围内（球体碰撞检测）
 * @param Dummy 假人AI
 * @param AttackRange 攻击范围（技能或普攻的范围值）
 * @param TargetActor 目标Actor用于调试日志输出
 * @return 是否检测到Pawn类型的目标
 * @note 球体半径会根据AI的缩放系数自动调整
 */
static bool CheckTargetInMoveRange(AActor* Dummy, float AttackRange, AActor* TargetActor)
{
	if (!Dummy || !Dummy->GetWorld())
	{
		return false;
	}

	// 🔧 获取AI的缩放系数（仅用于日志显示，不再参与计算）
	const FVector Scale3D = Dummy->GetActorScale3D();
	const float ScaleFactor = Scale3D.X;

	// 🔧 关键修改 - 攻击范围不再在此处进行二次缩放，直接使用传入值
	const float ScaledAttackRadius = AttackRange;

	// 🔧 球体中心为AI的中心位置
	const FVector SphereCenter = Dummy->GetActorLocation();

	// 🔧 配置碰撞查询参数
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Dummy); // 忽略自己
	QueryParams.bTraceComplex = false;  // 使用简单碰撞

	// 🔧 修改 - 添加Leader和Soldier通道，确保主将也能被检测到
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);  // Pawn通道
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel4);  // Leader通道（XBCollision::Leader）
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel3);  // Soldier通道（XBCollision::Soldier）

	// 🔧 执行球体碰撞检测
	TArray<FHitResult> HitResults;
	const bool bHit = Dummy->GetWorld()->SweepMultiByObjectType(
		HitResults,
		SphereCenter,
		SphereCenter, // 起点和终点相同，只做overlap检测
		FQuat::Identity,
		ObjectParams,  // 使用多通道检测
		FCollisionShape::MakeSphere(ScaledAttackRadius),
		QueryParams
	);

	// 🔧 遍历命中结果，检查是否有Pawn类型的目标
	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			// 🔧 过滤1：忽略磁场组件的碰撞
			if (UXBMagnetFieldComponent* MagnetComp = Cast<UXBMagnetFieldComponent>(Hit.GetComponent()))
			{
				UE_LOG(LogXBAI, VeryVerbose, TEXT("球体碰撞检测：忽略磁场组件 %s"), *MagnetComp->GetName());
				continue; // 跳过磁场组件
			}

			// 🔧 过滤2：检查是否是Pawn类型
			APawn* HitPawn = Cast<APawn>(Hit.GetActor());
			if (!HitPawn)
			{
				continue; // 不是Pawn，跳过
			}

			// 🔧 过滤3：检查是否是我们要找的目标
			if (TargetActor && HitPawn == TargetActor)
			{
				
				UE_LOG(LogXBAI, Log, TEXT("✅ 球体碰撞检测成功：在攻击范围内找到目标 %s (范围=%.1f, 缩放=%.2f, 缩放后半径=%.1f)"),
					*HitPawn->GetName(), AttackRange, ScaleFactor, ScaledAttackRadius);
				return true;
			}
		}
	}
	

	// 没有检测到目标
	UE_LOG(LogXBAI, Warning, TEXT("❌ 球体碰撞检测失败：未在攻击范围内找到目标 (范围=%.1f, 缩放=%.2f, 缩放后半径=%.1f)"),
		AttackRange, ScaleFactor, ScaledAttackRadius);
	return false;
}

/**
 * @brief 执行任务
 */
EBTNodeResult::Type UBTTask_XBDummyMoveToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 获取AI控制器
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人移动任务失败：AI控制器无效"));
		return EBTNodeResult::Failed;
	}

	// 获取假人主将
	AXBDummyCharacter* Dummy = Cast<AXBDummyCharacter>(AIController->GetPawn());
	if (!Dummy)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人移动任务失败：Pawn不是假人主将"));
		return EBTNodeResult::Failed;
	}

	// 获取黑板组件
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人移动任务失败：黑板无效"));
		return EBTNodeResult::Failed;
	}

	// 获取目标
	AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(TargetKey.SelectedKeyName));
	if (!Target)
	{
		return EBTNodeResult::Failed;
	}

	// 🔧 修改 - 获取当前选择的能力类型，确保移动范围与能力一致
	static const FName DefaultAbilityTypeKey(TEXT("SelectedAbilityType"));
	const FName AbilityTypeKeyName = AbilityTypeKey.SelectedKeyName.IsNone()
		? DefaultAbilityTypeKey
		: AbilityTypeKey.SelectedKeyName;
	const EXBDummyLeaderAbilityType SelectedAbilityType =
		static_cast<EXBDummyLeaderAbilityType>(Blackboard->GetValueAsInt(AbilityTypeKeyName));

	// 检查目标有效性
	if (AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Target))
	{
		if (TargetLeader->IsDead() || TargetLeader->IsHiddenInBush())
		{
			UE_LOG(LogXBAI, Verbose, TEXT("假人移动任务：目标无效（死亡/草丛）"));
			return EBTNodeResult::Failed;
		}
	}

	// 获取战斗组件
	UXBCombatComponent* CombatComp = Dummy->GetCombatComponent();
	if (!CombatComp)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人移动任务失败：战斗组件无效"));
		return EBTNodeResult::Failed;
	}

	// 设置焦点
	AIController->SetFocus(Target);

	// 🔧 修改 - 计算攻击范围（技能或普攻的范围值）
	const float AttackRange = CalculateOptimalStopDistance(CombatComp, Dummy, Target, SelectedAbilityType);
	
	// 🔧 修改 - 使用球体碰撞检测判断目标是否在攻击范围内
	if (CheckTargetInMoveRange(Dummy, AttackRange, Target))
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		UE_LOG(LogXBAI, Log, TEXT("假人 %s 已在攻击范围内（球体碰撞检测成功，范围=%.1f）"), 
			*Dummy->GetName(), AttackRange);
		return EBTNodeResult::Succeeded;
	}
	
	// 🔧 修改 - 发起移动请求，移动到攻击范围的80%位置（留一些裕度）
	EPathFollowingRequestResult::Type MoveResult = AIController->MoveToActor(
		Target,
		AttackRange * 0.8f,  // 移动到攻击范围的80%，避免寻路精度问题
		true,  // StopOnOverlap
		true,  // UsePathfinding
		true,  // CanStrafe
		nullptr,
		true   // AllowPartialPath
	);

	if (MoveResult == EPathFollowingRequestResult::RequestSuccessful)
	{
		TargetUpdateTimer = 0.0f;
		// 🔧 新增 - 日志中添加能力类型信息
		const FString AbilityTypeName = (SelectedAbilityType == EXBDummyLeaderAbilityType::SpecialSkill) ? TEXT("技能") :
			(SelectedAbilityType == EXBDummyLeaderAbilityType::BasicAttack) ? TEXT("普攻") : TEXT("无");
		UE_LOG(LogXBAI, Log, TEXT("假人 %s 开始移动到目标，选择=%s, 攻击范围=%.1f"), 
			*Dummy->GetName(), *AbilityTypeName, AttackRange);
		return EBTNodeResult::InProgress;
	}
	else if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		// 🔧 修复 - AlreadyAtGoal不代表真的在攻击范围内，继续用Tick检测
		UE_LOG(LogXBAI, Log, TEXT("假人 %s AlreadyAtGoal，将在TickTask中继续检测球体碰撞"), 
			*Dummy->GetName());
		return EBTNodeResult::InProgress;
	}

	// 无法寻路
	UE_LOG(LogXBAI, Warning, TEXT("假人 %s 无法寻路到目标"), *Dummy->GetName());
	return EBTNodeResult::Failed;
}

/**
 * @brief Tick更新任务
 */
void UBTTask_XBDummyMoveToTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	// 获取AI控制器
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 获取假人主将
	AXBDummyCharacter* Dummy = Cast<AXBDummyCharacter>(AIController->GetPawn());
	if (!Dummy)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 获取黑板
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 获取目标
	AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(TargetKey.SelectedKeyName));
	if (!Target)
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 检查目标有效性
	if (AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Target))
	{
		if (TargetLeader->IsDead() || TargetLeader->IsHiddenInBush())
		{
			AIController->StopMovement();
			AIController->ClearFocus(EAIFocusPriority::Gameplay);
			UE_LOG(LogXBAI, Log, TEXT("假人移动任务中止：目标丢失"));
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}
	}

	// 保持焦点
	AIController->SetFocus(Target);

	// 🔧 修改 - 获取战斗组件并计算攻击范围
	UXBCombatComponent* CombatComp = Dummy->GetCombatComponent();
	if (!CombatComp)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 计算攻击范围
	static const FName DefaultAbilityTypeKey(TEXT("SelectedAbilityType"));
	const FName AbilityTypeKeyName = AbilityTypeKey.SelectedKeyName.IsNone()
		? DefaultAbilityTypeKey
		: AbilityTypeKey.SelectedKeyName;
	const EXBDummyLeaderAbilityType SelectedAbilityType =
		static_cast<EXBDummyLeaderAbilityType>(Blackboard->GetValueAsInt(AbilityTypeKeyName));
	const float AttackRange = CalculateOptimalStopDistance(CombatComp, Dummy, Target, SelectedAbilityType);
	
	// 🔧 修改 - 使用球体碰撞检测判断是否到达攻击范围
	if (CheckTargetInMoveRange(Dummy, AttackRange, Target))
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		UE_LOG(LogXBAI, Log, TEXT("假人 %s 到达攻击范围（球体碰撞检测成功，范围=%.1f），停止移动"), 
			*Dummy->GetName(), AttackRange);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// 定期更新移动请求（目标可能移动）
	TargetUpdateTimer += DeltaSeconds;
	if (TargetUpdateTimer >= TargetUpdateInterval)
	{
		TargetUpdateTimer = 0.0f;
		// 🔧 简化 - 直接使用攻击范围的70%作为移动距离，继续靠近目标
		AIController->MoveToActor(Target, AttackRange * 0.7f, true, true, true, nullptr, true);
	
	}
}

/**
 * @brief 中止任务
 */
EBTNodeResult::Type UBTTask_XBDummyMoveToTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
	
	return EBTNodeResult::Aborted;
}

/**
 * @brief 获取节点描述
 */
FString UBTTask_XBDummyMoveToTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("智能移动到目标\n目标键: %s"),
		*TargetKey.SelectedKeyName.ToString());
}

/**
 * @brief 计算最优停止距离
 * @note 优先级：已选择能力 > 技能就绪（用技能范围）> 普攻就绪（用普攻范围）> 都冷却（用最大范围等待）
 */
float UBTTask_XBDummyMoveToTarget::CalculateOptimalStopDistance(
	UXBCombatComponent* CombatComp,
	AActor* Dummy,
	AActor* Target,
	EXBDummyLeaderAbilityType SelectedAbilityType) const
{
	if (!CombatComp || !Dummy || !Target)
	{
		return 100.0f; // 默认值
	}

	// 🔧 关键修复 - 不再加碰撞半径，只使用纯攻击范围
// 说明：OptimalStopDistance 是边缘距离，不再叠加碰撞半径

	// 获取技能和普攻的范围与冷却状态
	const float SkillRange = CombatComp->GetSkillAttackRange();
	const float BasicRange = CombatComp->GetBasicAttackRange();
	const bool bSkillOnCooldown = CombatComp->IsSkillOnCooldown();
	const bool bBasicOnCooldown = CombatComp->IsBasicAttackOnCooldown();

	UE_LOG(LogXBAI, Verbose, TEXT("假人AI技能范围=%.1f, 普攻范围=%.1f"), SkillRange, BasicRange);

	// 🔧 修改 - 直接使用攻击范围作为停止距离，不加碰撞半径
	if (SelectedAbilityType == EXBDummyLeaderAbilityType::SpecialSkill && !bSkillOnCooldown)
	{
		UE_LOG(LogXBAI, Verbose, TEXT("假人移动：已选择技能，停止距离=%.1f"), SkillRange);
		return SkillRange;
	}

	if (SelectedAbilityType == EXBDummyLeaderAbilityType::BasicAttack && !bBasicOnCooldown)
	{
		UE_LOG(LogXBAI, Verbose, TEXT("假人移动：已选择普攻，停止距离=%.1f"), BasicRange);
		return BasicRange;
	}

	// 优先级1：技能就绪，使用技能范围
	if (!bSkillOnCooldown)
	{
		UE_LOG(LogXBAI, Verbose, TEXT("假人移动：技能就绪，停止距离=%.1f"), SkillRange);
		return SkillRange;
	}

	// 优先级2：普攻就绪，使用普攻范围
	if (!bBasicOnCooldown)
	{
		UE_LOG(LogXBAI, Verbose, TEXT("假人移动：普攻就绪，停止距离=%.1f"), BasicRange);
		return BasicRange;
	}

	// 优先级3：都在冷却，使用最小范围等待
	const float MinRange = FMath::Min(SkillRange, BasicRange);
	UE_LOG(LogXBAI, Verbose, TEXT("假人移动：都在冷却，停止距离=%.1f"), MinRange);
	return MinRange;
}
