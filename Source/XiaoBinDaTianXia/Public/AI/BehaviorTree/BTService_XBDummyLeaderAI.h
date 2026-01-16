/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/BehaviorTree/BTService_XBDummyLeaderAI.h

/**
 * @file BTService_XBDummyLeaderAI.h
 * @brief 行为树服务 - 假人主将AI状态更新
 *
 * @note ✨ 新增 - 统一在行为树服务中完成目标搜索/回归/移动意图更新
 */

#pragma once

#include "CoreMinimal.h"
#include "AI/XBDummyAIType.h"
#include "BehaviorTree/BTService.h"
#include "BTService_XBDummyLeaderAI.generated.h"

class AXBDummyCharacter;
class AXBCharacterBase;
class UBlackboardComponent;
class USplineComponent;

/**
 * @brief 假人主将AI状态更新服务
 * @note 负责目标管理、行为中心维护和行为目的地更新
 */
UCLASS()
class XIAOBINDATIANXIA_API UBTService_XBDummyLeaderAI : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_XBDummyLeaderAI();

protected:
	/**
	 * @brief 服务进入时初始化
	 * @param OwnerComp 行为树组件
	 * @param NodeMemory 节点内存
	 * @return 无
	 * @note   详细流程分析: 初始化时间与黑板默认值
	 *         性能/架构注意事项: 仅在进入时执行一次
	 */
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/**
	 * @brief 服务Tick更新
	 * @param OwnerComp 行为树组件
	 * @param NodeMemory 节点内存
	 * @param DeltaSeconds 帧间隔
	 * @return 无
	 * @note   详细流程分析: 目标搜索 -> 战斗状态 -> 行为目的地
	 *         性能/架构注意事项: 通过间隔控制感知与随机移动频率
	 */
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	/**
	 * @brief  初始化黑板基础数据
	 * @param  Dummy 假人主将
	 * @param  Blackboard 黑板组件
	 * @return 无
	 * @note   详细流程分析: 写入初始位置/行为中心/行为模式
	 *         性能/架构注意事项: 仅在服务激活时执行
	 */
	void InitializeBlackboard(AXBDummyCharacter* Dummy, UBlackboardComponent* Blackboard);

	/**
	 * @brief  搜索视野内敌方主将
	 * @param  Dummy 假人主将
	 * @param  OutLeader 输出主将
	 * @return 是否找到
	 * @note   详细流程分析: 感知查询 -> 过滤无效目标 -> 选择最近主将
	 *         性能/架构注意事项: 只遍历感知结果
	 */
	bool FindEnemyLeader(AXBDummyCharacter* Dummy, AXBCharacterBase*& OutLeader) const;

	/**
	 * @brief  判断敌方主将与士兵是否已全部阵亡
	 * @param  Leader 目标主将
	 * @return 是否全灭
	 * @note   详细流程分析: 主将死亡 -> 遍历士兵状态
	 *         性能/架构注意事项: 士兵数量通常有限，可接受遍历
	 */
	bool IsLeaderArmyEliminated(AXBCharacterBase* Leader) const;

	/**
	 * @brief  处理目标丢失后的回归逻辑
	 * @param  Dummy 假人主将
	 * @param  Blackboard 黑板组件
	 * @param  bForwardMoveAfterLost 是否进入“丢失目标后正前方行走”流程
	 * @return 无
	 * @note   详细流程分析: 重置行为中心 -> 可选正前方行走 -> 校正路线索引
	 *         性能/架构注意事项: 仅在目标切换时执行
	 */
	void HandleTargetLost(AXBDummyCharacter* Dummy, UBlackboardComponent* Blackboard, bool bForwardMoveAfterLost);

	/**
	 * @brief  更新非战斗状态下的行为目的地
	 * @param  Dummy 假人主将
	 * @param  Blackboard 黑板组件
	 * @return 无
	 * @note   详细流程分析: 根据移动方式设置目的地
	 *         性能/架构注意事项: 随机移动采用时间间隔控制
	 */
	void UpdateBehaviorDestination(AXBDummyCharacter* Dummy, UBlackboardComponent* Blackboard);

	/**
	 * @brief  在无需 Observe Blackboard 的情况下持续更新移动请求
	 * @param  Dummy 假人主将
	 * @param  Destination 目标位置
	 * @return 无
	 * @note   详细流程分析: 行为树未观察黑板值时，通过服务主动下发 MoveTo
	 *         性能/架构注意事项: 仅在巡逻路线模式使用，避免与战斗靠近冲突
	 */
	void RequestContinuousMove(AXBDummyCharacter* Dummy, const FVector& Destination) const;

	/**
	 * @brief  将路线索引重置到最近样条点
	 * @param  Dummy 假人主将
	 * @param  Blackboard 黑板组件
	 * @param  SplineComp 巡逻样条
	 * @return 无
	 * @note   详细流程分析: 通过最近点重建路线索引
	 *         性能/架构注意事项: 仅在回归时执行
	 */
	void ResetRouteIndexToNearest(AXBDummyCharacter* Dummy, UBlackboardComponent* Blackboard, USplineComponent* SplineComp);

	/**
	 * @brief  选择当前战斗阶段的能力
	 * @param  Dummy 假人主将
	 * @param  Blackboard 黑板组件
	 * @param  Target 当前目标
	 * @return 选择的能力类型
	 * @note   详细流程分析: 校验当前选择 -> 冷却判断 -> 选择可用能力写入黑板
	 *         性能/架构注意事项: 仅在有目标时调用，避免频繁无效写入
	 */
	EXBDummyLeaderAbilityType SelectCombatAbility(AXBDummyCharacter* Dummy, UBlackboardComponent* Blackboard, AXBCharacterBase* Target);

	/** @brief 下一次搜索时间 */
	float NextSearchTime = 0.0f;

	/** @brief 下一次随机移动时间 */
	float NextWanderTime = 0.0f;

	/** @brief 是否曾拥有目标 */
	bool bHadCombatTarget = false;

	/** @brief 是否已提示缺失样条 */
	bool bLoggedMissingSpline = false;

	/** @brief 是否处于“丢失目标后正前方移动”阶段 */
	bool bForwardMoveAfterLost = false;

	/** @brief 丢失目标后的前进结束时间 */
	float ForwardMoveEndTime = 0.0f;
};
