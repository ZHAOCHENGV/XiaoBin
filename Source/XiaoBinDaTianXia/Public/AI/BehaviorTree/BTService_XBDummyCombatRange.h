/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/BehaviorTree/BTService_XBDummyCombatRange.h

/**
 * @file BTService_XBDummyCombatRange.h
 * @brief 行为树服务 - 假人主将战斗范围检测
 *
 * @note ✨ 新增 - 专用于选择攻击能力并检测目标是否在攻击范围内
 *       核心功能：
 *       1. 选择可用能力（技能优先、普攻其次）
 *       2. 检测目标是否在对应攻击范围内
 *       3. 持续更新 IsInAttackRange 黑板变量
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "BTService_XBDummyCombatRange.generated.h"

class UXBCombatComponent;
enum class EXBDummyLeaderAbilityType : uint8;


/**
 * @brief 假人主将战斗范围检测服务
 * @note 持续检测目标是否在当前可用能力的攻击范围内，并更新黑板变量
 */
UCLASS()
class XIAOBINDATIANXIA_API UBTService_XBDummyCombatRange : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_XBDummyCombatRange();

protected:
	/**
	 * @brief 服务Tick更新
	 * @param OwnerComp 行为树组件
	 * @param NodeMemory 节点内存
	 * @param DeltaSeconds 帧间隔
	 * @return 无
	 * @note   详细流程分析: 选择能力 -> 计算范围 -> 球体碰撞检测 -> 更新黑板
	 */
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** @brief 目标黑板键 */
	UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "目标键"))
	FBlackboardKeySelector TargetKey;

	/** @brief 选择的能力类型黑板键 */
	UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "能力类型键"))
	FBlackboardKeySelector AbilityTypeKey;

	/** @brief 是否在攻击范围内黑板键（输出） */
	UPROPERTY(EditAnywhere, Category = "黑板", meta = (DisplayName = "攻击范围内键"))
	FBlackboardKeySelector IsInAttackRangeKey;

	/** @brief 球体追踪调试绘制类型 */
	UPROPERTY(EditAnywhere, Category = "调试", meta = (DisplayName = "调试绘制类型"))
	TEnumAsByte<EDrawDebugTrace::Type> DebugDrawType = EDrawDebugTrace::None;

private:
	/**
	 * @brief 检查目标是否在攻击范围内（球体碰撞检测）
	 * @param Dummy 假人AI
	 * @param AttackRange 攻击范围
	 * @param TargetActor 目标Actor
	 * @return 是否在范围内
	 */
	bool CheckTargetInAttackRange(AActor* Dummy, float AttackRange, AActor* TargetActor) const;

	/**
	 * @brief 根据选择的能力类型计算攻击范围
	 * @param CombatComp 战斗组件
	 * @param SelectedAbilityType 当前选择的能力类型
	 * @return 对应能力的攻击范围
	 */
	float CalculateCurrentAttackRange(UXBCombatComponent* CombatComp, EXBDummyLeaderAbilityType SelectedAbilityType) const;
};
