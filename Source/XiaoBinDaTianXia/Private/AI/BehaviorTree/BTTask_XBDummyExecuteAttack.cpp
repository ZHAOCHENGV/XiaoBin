/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/BehaviorTree/BTTask_XBDummyExecuteAttack.cpp

/**
 * @file BTTask_XBDummyExecuteAttack.cpp
 * @brief 行为树任务 - 假人执行受击反击
 * 
 * @note ✨ 新增 - 由黑板触发释放技能/普攻
 */

#include "AI/BehaviorTree/BTTask_XBDummyExecuteAttack.h"
#include "AI/XBDummyAIController.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/XBCharacterBase.h"
#include "Character/XBDummyCharacter.h"
#include "Utils/XBLogCategories.h"

namespace
{
	// 重置受击反击黑板标记，避免任务失败后重复触发
	void ResetDamageResponseKey(UBehaviorTreeComponent& OwnerComp, const AAIController* AIController)
	{
		if (!AIController)
		{
			return;
		}

		if (UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent())
		{
			if (const AXBDummyAIController* DummyAI = Cast<AXBDummyAIController>(AIController))
			{
				BlackboardComp->SetValueAsBool(DummyAI->GetDamageResponseKey(), false);
			}
		}
	}
}

UBTTask_XBDummyExecuteAttack::UBTTask_XBDummyExecuteAttack()
{
	NodeName = TEXT("假人执行受击反击");
	TargetKey.SelectedKeyName = TEXT("TargetLeader");
	bNotifyTick = true;
	bNotifyTaskFinished = true;
}

/**
 * @brief 执行任务
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @return 任务执行结果
 * @note   详细流程分析: 获取控制器/假人 -> 设置焦点转向 -> Tick等待朝向 -> 执行攻击 -> 重置黑板标记
 *         性能/架构注意事项: 任务仅负责一次攻击，不循环
 */
EBTNodeResult::Type UBTTask_XBDummyExecuteAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 获取AI控制器与黑板组件
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!AIController || !Blackboard)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人受击反击任务失败：AI控制器或黑板无效"));
		ResetDamageResponseKey(OwnerComp, AIController);
		return EBTNodeResult::Failed;
	}

	// 获取假人实例
	AXBDummyCharacter* Dummy = Cast<AXBDummyCharacter>(AIController->GetPawn());
	if (!Dummy)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人受击反击任务失败：Pawn不是假人"));
		ResetDamageResponseKey(OwnerComp, AIController);
		return EBTNodeResult::Failed;
	}

	// ?? 修改 - 使用默认黑板键名，避免依赖数据表配置
	static const FName DefaultTargetLeaderKey(TEXT("TargetLeader"));
	const FName TargetLeaderKey = TargetKey.SelectedKeyName.IsNone()
		? DefaultTargetLeaderKey
		: TargetKey.SelectedKeyName;

	// ?? 修改 - 受击反击也需要先朝向目标再出手
	AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Blackboard->GetValueAsObject(TargetLeaderKey));
	if (!TargetLeader || TargetLeader->IsDead() || TargetLeader->IsHiddenInBush())
	{
		UE_LOG(LogXBAI, Verbose, TEXT("假人受击反击目标无效，取消执行"));
		ResetDamageResponseKey(OwnerComp, AIController);
		return EBTNodeResult::Failed;
	}

	// ?? 修改 - 设置焦点触发平滑转向
	AIController->SetFocus(TargetLeader);
	RotationTimer = 0.0f;
	UE_LOG(LogXBAI, Log, TEXT("假人 %s 受击反击开始转向目标"), *Dummy->GetName());

	return EBTNodeResult::InProgress;
}

/**
 * @brief Tick更新任务
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @param DeltaSeconds 帧间隔
 * @return 无
 * @note   详细流程分析: 检查转向是否完成 -> 完成后执行受击反击
 *         性能/架构注意事项: 超时未转向则放弃本次攻击，避免背对出手
 */
void UBTTask_XBDummyExecuteAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	// 获取AI控制器与黑板组件
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!AIController || !Blackboard)
	{
		ResetDamageResponseKey(OwnerComp, AIController);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 获取假人实例
	AXBDummyCharacter* Dummy = Cast<AXBDummyCharacter>(AIController->GetPawn());
	if (!Dummy)
	{
		ResetDamageResponseKey(OwnerComp, AIController);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// ?? 修改 - 使用默认黑板键名，避免依赖数据表配置
	static const FName DefaultTargetLeaderKey(TEXT("TargetLeader"));
	const FName TargetLeaderKey = TargetKey.SelectedKeyName.IsNone()
		? DefaultTargetLeaderKey
		: TargetKey.SelectedKeyName;

	// ?? 修改 - 目标无效时直接失败并清理焦点
	AXBCharacterBase* TargetLeader = Cast<AXBCharacterBase>(Blackboard->GetValueAsObject(TargetLeaderKey));
	if (!TargetLeader || TargetLeader->IsDead() || TargetLeader->IsHiddenInBush())
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		ResetDamageResponseKey(OwnerComp, AIController);
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

	// 判断是否已朝向目标
	const bool bIsFacingTarget = AngleDegrees <= FacingAngleThreshold;
	const bool bTimeout = RotationTimer >= MaxRotationWaitTime;
	if (!bIsFacingTarget)
	{
		if (bTimeout)
		{
			// 超时仍未转向则放弃本次反击，避免背对出手
			AIController->ClearFocus(EAIFocusPriority::Gameplay);
			UE_LOG(LogXBAI, Verbose, TEXT("假人 %s 转向超时，取消受击反击"), *Dummy->GetName());
			ResetDamageResponseKey(OwnerComp, AIController);
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		}

		return;
	}

	// ?? 修改 - 转向完成后执行攻击
	AIController->ClearFocus(EAIFocusPriority::Gameplay);
	const bool bExecuted = Dummy->ExecuteDamageResponseAttack();
	ResetDamageResponseKey(OwnerComp, AIController);
	FinishLatentTask(OwnerComp, bExecuted ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
}

/**
 * @brief 中止任务
 * @param OwnerComp 行为树组件
 * @param NodeMemory 节点内存
 * @return 中止结果
 * @note   详细流程分析: 清理焦点并重置黑板标记
 */
EBTNodeResult::Type UBTTask_XBDummyExecuteAttack::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		ResetDamageResponseKey(OwnerComp, AIController);
	}

	return EBTNodeResult::Aborted;
}
