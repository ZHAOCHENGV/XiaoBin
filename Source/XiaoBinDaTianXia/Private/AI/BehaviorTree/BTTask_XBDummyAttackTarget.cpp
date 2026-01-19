/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTTask_XBDummyAttackTarget.cpp

/**
 * @file BTTask_XBDummyAttackTarget.cpp
 * @brief 行为树任务 - 假人主将攻击目标
 *
 * @note ✨ 新增 - 将假人主将战斗逻辑迁移到行为树任务
 */

#include "AI/BehaviorTree/BTTask_XBDummyAttackTarget.h"
#include "AI/XBDummyAIType.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/Components/XBCombatComponent.h"
#include "Character/Components/XBMagnetFieldComponent.h"
#include "Character/XBDummyCharacter.h"
#include "Character/XBCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Utils/XBLogCategories.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"

UBTTask_XBDummyAttackTarget::UBTTask_XBDummyAttackTarget()
{
	NodeName = TEXT("假人主将攻击目标");
	TargetKey.SelectedKeyName = TEXT("TargetLeader");
	// ✨ 新增 - 绑定能力类型黑板键，保证攻击与选择一致
	AbilityTypeKey.SelectedKeyName = TEXT("SelectedAbilityType");
	AbilityTypeKey.AddIntFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_XBDummyAttackTarget, AbilityTypeKey));
	
	// 启用Tick更新，以便等待转向完成
	bNotifyTick = true;
	bNotifyTaskFinished = true;
}

/**
 * @brief 检查目标是否在攻击范围内（球体碰撞检测）
 * @param Dummy 假人AI
 * @param AttackRange 攻击范围（技能或普攻的范围值）
 * @param TargetActor 目标Actor
 * @return 是否检测到目标
 * @note 球体半径会根据AI的缩放系数自动调整，并过滤磁场组件
 */
static bool CheckTargetInAttackRange(AActor* Dummy, float AttackRange, AActor* TargetActor)
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

	// 🔧 执行球体碰撞检测
	TArray<FHitResult> HitResults;
	const bool bHit = Dummy->GetWorld()->SweepMultiByProfile(
		HitResults,
		SphereCenter,
		SphereCenter, // 起点和终点相同，只做overlap检测
		FQuat::Identity,
		"Pawn", // 只检测Pawn通道
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
				UE_LOG(LogXBAI, VeryVerbose, TEXT("球体碰撞检测(攻击)：忽略磁场组件 %s"), *MagnetComp->GetName());
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
				UE_LOG(LogXBAI, Verbose, TEXT("球体碰撞检测(攻击)：在范围内找到目标Pawn %s (范围=%.1f, 缩放=%.2f, 缩放后半径=%.1f)"),
					*HitPawn->GetName(), AttackRange, ScaleFactor, ScaledAttackRadius);
				return true;
			}
		}
	}

	// 没有检测到目标
	UE_LOG(LogXBAI, Verbose, TEXT("球体碰撞检测(攻击)：未在范围内找到目标 (范围=%.1f, 缩放=%.2f, 缩放后半径=%.1f)"),
		AttackRange, ScaleFactor, ScaledAttackRadius);
	return false;
}

/**
 * @brief 执行任务
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @return 任务执行结果
 * @note   详细流程分析: 获取目标 -> 设置焦点开始转向 -> 返回InProgress等待Tick
 *         性能/架构注意事项: 使用Tick等待转向完成
 */
EBTNodeResult::Type UBTTask_XBDummyAttackTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!AIController || !Blackboard)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人攻击任务失败：AI控制器或黑板无效"));
		return EBTNodeResult::Failed;
	}

	AXBDummyCharacter* Dummy = Cast<AXBDummyCharacter>(AIController->GetPawn());
	if (!Dummy)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人攻击任务失败：Pawn不是假人"));
		return EBTNodeResult::Failed;
	}

	// 🔧 修改 - 黑板键使用默认固定名称，避免依赖数据表配置
	static const FName DefaultTargetLeaderKey(TEXT("TargetLeader"));
	const FName TargetLeaderKey = TargetKey.SelectedKeyName.IsNone()
		? DefaultTargetLeaderKey
		: TargetKey.SelectedKeyName;

	AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Blackboard->GetValueAsObject(TargetLeaderKey));
	if (!TargetLeader || TargetLeader->IsDead() || TargetLeader->IsHiddenInBush())
	{
		UE_LOG(LogXBAI, Verbose, TEXT("假人攻击任务中目标无效，取消执行"));
		return EBTNodeResult::Failed;
	}

	// 🔧 修改 - 读取当前选择的能力类型，确保攻击与移动使用同一能力
	static const FName DefaultAbilityTypeKey(TEXT("SelectedAbilityType"));
	const FName AbilityTypeKeyName = AbilityTypeKey.SelectedKeyName.IsNone()
		? DefaultAbilityTypeKey
		: AbilityTypeKey.SelectedKeyName;
	const EXBDummyLeaderAbilityType SelectedAbilityType =
		static_cast<EXBDummyLeaderAbilityType>(Blackboard->GetValueAsInt(AbilityTypeKeyName));

	if (SelectedAbilityType == EXBDummyLeaderAbilityType::None)
	{
		UE_LOG(LogXBAI, Verbose, TEXT("假人攻击任务失败：未选择可用能力，等待重新选择"));
		return EBTNodeResult::Failed;
	}

	UXBCombatComponent* CombatComp = Dummy->GetCombatComponent();
	if (!CombatComp)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人攻击任务失败：战斗组件无效"));
		return EBTNodeResult::Failed;
	}

	// 检查攻击范围和冷却状态
	const bool bSkillOnCooldown = CombatComp->IsSkillOnCooldown();
	const bool bBasicOnCooldown = CombatComp->IsBasicAttackOnCooldown();
	const float SkillRange = CombatComp->GetSkillAttackRange();
	const float BasicRange = CombatComp->GetBasicAttackRange();

	// 🔧 修改 - 使用球体碰撞检测替代距离计算
	const bool bInSkillRange = CheckTargetInAttackRange(Dummy, SkillRange, TargetLeader);
	const bool bInBasicRange = CheckTargetInAttackRange(Dummy, BasicRange, TargetLeader);

	// 🔧 新增 - 详细调试日志
	const FString AbilityTypeName = (SelectedAbilityType == EXBDummyLeaderAbilityType::SpecialSkill) ? TEXT("技能") :
		(SelectedAbilityType == EXBDummyLeaderAbilityType::BasicAttack) ? TEXT("普攻") : TEXT("无");
	UE_LOG(LogXBAI, Log, TEXT("假人 %s 攻击检查: 选择=%s, 技能范围=%.1f(在范围=%s,CD=%s), 普攻范围=%.1f(在范围=%s,CD=%s)"),
		*Dummy->GetName(), *AbilityTypeName,
		SkillRange, bInSkillRange ? TEXT("是") : TEXT("否"), bSkillOnCooldown ? TEXT("是") : TEXT("否"),
		BasicRange, bInBasicRange ? TEXT("是") : TEXT("否"), bBasicOnCooldown ? TEXT("是") : TEXT("否"));

	// 🔧 修改 - 按已选择能力判断范围与冷却，避免未到对应范围就停下
	const bool bSelectedSkillReady = SelectedAbilityType == EXBDummyLeaderAbilityType::SpecialSkill && !bSkillOnCooldown && bInSkillRange;
	const bool bSelectedBasicReady = SelectedAbilityType == EXBDummyLeaderAbilityType::BasicAttack && !bBasicOnCooldown && bInBasicRange;
	if (!bSelectedSkillReady && !bSelectedBasicReady)
	{
		UE_LOG(LogXBAI, Log, TEXT("假人 %s 攻击条件不满足: 技能就绪=%s, 普攻就绪=%s"),
			*Dummy->GetName(), bSelectedSkillReady ? TEXT("是") : TEXT("否"), bSelectedBasicReady ? TEXT("是") : TEXT("否"));
		return EBTNodeResult::Failed;
	}

	// 🔧 关键修改 - 使用SetFocus开始平滑转向
	AIController->SetFocus(TargetLeader);
	
	// 重置转向计时器
	RotationTimer = 0.0f;
	
	UE_LOG(LogXBAI, Log, TEXT("假人 %s 开始转向目标准备攻击(%s)"), *Dummy->GetName(), *AbilityTypeName);
	
	// 返回InProgress，让Tick检查转向并执行攻击
	return EBTNodeResult::InProgress;
}

/**
 * @brief Tick更新任务 - 检查转向完成后执行攻击
 */
void UBTTask_XBDummyAttackTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!AIController || !Blackboard)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	AXBDummyCharacter* Dummy = Cast<AXBDummyCharacter>(AIController->GetPawn());
	if (!Dummy)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	static const FName DefaultTargetLeaderKey(TEXT("TargetLeader"));
	const FName TargetLeaderKey = TargetKey.SelectedKeyName.IsNone()
		? DefaultTargetLeaderKey
		: TargetKey.SelectedKeyName;

	AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Blackboard->GetValueAsObject(TargetLeaderKey));
	if (!TargetLeader || TargetLeader->IsDead() || TargetLeader->IsHiddenInBush())
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	static const FName DefaultAbilityTypeKey(TEXT("SelectedAbilityType"));
	const FName AbilityTypeKeyName = AbilityTypeKey.SelectedKeyName.IsNone()
		? DefaultAbilityTypeKey
		: AbilityTypeKey.SelectedKeyName;
	const EXBDummyLeaderAbilityType SelectedAbilityType =
		static_cast<EXBDummyLeaderAbilityType>(Blackboard->GetValueAsInt(AbilityTypeKeyName));
	if (SelectedAbilityType == EXBDummyLeaderAbilityType::None)
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// ?? 修改 - 仅使用平面方向计算朝向，避免高度差导致角度无法收敛
	FVector ToTarget = TargetLeader->GetActorLocation() - Dummy->GetActorLocation();
	ToTarget.Z = 0.0f;
	if (!ToTarget.IsNearlyZero())
	{
		// ?? 修改 - 使用匀速插值转向，保证未移动时也能持续转头
		// 公式思路: 以固定角速度逼近目标Yaw，避免移动组件不更新导致转向停滞
		const FRotator DesiredRotation = ToTarget.Rotation();
		const FRotator CurrentRotation = Dummy->GetActorRotation();
		const FRotator TargetRotation(0.0f, DesiredRotation.Yaw, 0.0f);
		const FRotator NewRotation = FMath::RInterpConstantTo(CurrentRotation, TargetRotation, DeltaSeconds, FacingRotationSpeed);
		Dummy->SetActorRotation(NewRotation);
		AIController->SetControlRotation(NewRotation);
	}

	FVector Forward = Dummy->GetActorForwardVector();
	Forward.Z = 0.0f;
	Forward = Forward.GetSafeNormal();
	const FVector ToTargetDir = ToTarget.IsNearlyZero() ? Forward : ToTarget.GetSafeNormal();
	// 点积公式: cosθ = (A·B)/(|A||B|)，用于计算朝向夹角
	const float DotProduct = FVector::DotProduct(Forward, ToTargetDir);
	// 避免数值误差导致Acos无效
	const float ClampedDotProduct = FMath::Clamp(DotProduct, -1.0f, 1.0f);
	const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(ClampedDotProduct));

	// 更新计时器
	RotationTimer += DeltaSeconds;

	// 判断是否已转向目标（角度足够小）
	const bool bIsFacingTarget = AngleDegrees <= FacingAngleThreshold;
	const bool bTimeout = RotationTimer >= MaxRotationWaitTime;

	if (bIsFacingTarget)
	{
		// 转向完成，执行攻击
		UXBCombatComponent* CombatComp = Dummy->GetCombatComponent();
		if (!CombatComp)
		{
			AIController->ClearFocus(EAIFocusPriority::Gameplay);
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}

		const bool bSkillOnCooldown = CombatComp->IsSkillOnCooldown();
		const bool bBasicOnCooldown = CombatComp->IsBasicAttackOnCooldown();
		const float SkillRange = CombatComp->GetSkillAttackRange();
		const float BasicRange = CombatComp->GetBasicAttackRange();

		// 🔧 修改 - 使用球体碰撞检测替代距离计算
		const bool bInSkillRange = CheckTargetInAttackRange(Dummy, SkillRange, TargetLeader);
		const bool bInBasicRange = CheckTargetInAttackRange(Dummy, BasicRange, TargetLeader);

		// 清除焦点
		AIController->ClearFocus(EAIFocusPriority::Gameplay);

		// 🔧 修改 - 仅释放已选择的能力，确保与移动范围一致
		if (SelectedAbilityType == EXBDummyLeaderAbilityType::SpecialSkill && bInSkillRange && !bSkillOnCooldown)
		{
			CombatComp->PerformSpecialSkill();
			UE_LOG(LogXBAI, Log, TEXT("假人 %s 转向完成(%.1f度)，释放技能"), *Dummy->GetName(), AngleDegrees);
			Blackboard->SetValueAsInt(AbilityTypeKeyName, static_cast<int32>(EXBDummyLeaderAbilityType::None));
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}

		if (SelectedAbilityType == EXBDummyLeaderAbilityType::BasicAttack && bInBasicRange && !bBasicOnCooldown)
		{
			CombatComp->PerformBasicAttack();
			UE_LOG(LogXBAI, Log, TEXT("假人 %s 转向完成(%.1f度)，释放普攻"), *Dummy->GetName(), AngleDegrees);
			Blackboard->SetValueAsInt(AbilityTypeKeyName, static_cast<int32>(EXBDummyLeaderAbilityType::None));
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}

		// 🔧 修改 - 如果释放失败（蒙太奇正在播放等原因），清空能力选择让AI重新评估
		UE_LOG(LogXBAI, Verbose, TEXT("假人 %s 转向完成但无法攻击，清空能力选择"), *Dummy->GetName());
		Blackboard->SetValueAsInt(AbilityTypeKeyName, static_cast<int32>(EXBDummyLeaderAbilityType::None));
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (bTimeout)
	{
		// 超时仍未转向则放弃本次攻击，避免背对出手
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		UE_LOG(LogXBAI, Verbose, TEXT("假人 %s 转向超时，取消本次攻击"), *Dummy->GetName());
		Blackboard->SetValueAsInt(AbilityTypeKeyName, static_cast<int32>(EXBDummyLeaderAbilityType::None));
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	// 否则继续等待转向
}

/**
 * @brief 中止任务
 */
EBTNodeResult::Type UBTTask_XBDummyAttackTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
	
	return EBTNodeResult::Aborted;
}
