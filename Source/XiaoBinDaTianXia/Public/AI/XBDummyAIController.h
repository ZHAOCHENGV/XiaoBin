/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/XBDummyAIController.h

/**
 * @file XBDummyAIController.h
 * @brief 假人AI控制器 - 简单行为树驱动
 * 
 * @note ✨ 新增 - 受击后延迟触发攻击逻辑
 */

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Data/XBLeaderDataTable.h"
#include "XBDummyAIController.generated.h"

class UBehaviorTree;
class UBlackboardComponent;
class AXBDummyCharacter;
class AXBCharacterBase;
class USplineComponent;

/**
 * @brief 假人AI控制器
 * @note 负责运行行为树并写入受击响应黑板
 */
UCLASS()
class XIAOBINDATIANXIA_API AXBDummyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AXBDummyAIController();

	/**
	 * @brief 设置受击响应标记
	 * @param bReady 是否准备执行攻击
	 * @return 无
	 * @note   详细流程分析: 获取黑板 -> 写入标记
	 *         性能/架构注意事项: 黑板不存在时仅记录日志
	 */
	void SetDamageResponseReady(bool bReady);

	/** @brief 获取黑板键名 */
	FName GetDamageResponseKey() const { return DamageResponseKey; }

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * @brief  初始化假人主将AI配置
	 * @param  Dummy 假人主将
	 * @return 无
	 * @note   详细流程分析: 缓存配置 -> 初始化状态 -> 记录初始位置
	 *         性能/架构注意事项: 仅在首次Possess时调用
	 */
	void InitializeLeaderAI(AXBDummyCharacter* Dummy);

	/**
	 * @brief  更新假人主将AI主循环
	 * @param  DeltaSeconds 帧间隔
	 * @return 无
	 * @note   详细流程分析: 目标侦测 -> 战斗逻辑 -> 行为模式
	 *         性能/架构注意事项: 使用时间间隔降低感知开销
	 */
	void TickLeaderAI(float DeltaSeconds);

	/**
	 * @brief  侦测并维护目标主将
	 * @return 无
	 * @note   详细流程分析: 目标有效性检测 -> 搜索敌方主将 -> 状态切换
	 *         性能/架构注意事项: 仅按间隔查询感知子系统
	 */
	void UpdateCombatTarget();

	/**
	 * @brief  尝试获取视野内敌方主将
	 * @param  OutLeader 输出主将
	 * @return 是否找到
	 * @note   详细流程分析: 感知查询 -> 过滤阵营/死亡/草丛 -> 选择最近主将
	 *         性能/架构注意事项: 只遍历查询结果而非全局列表
	 */
	bool FindEnemyLeaderInRange(AXBCharacterBase*& OutLeader);

	/**
	 * @brief  判断敌方主将与士兵是否已全部阵亡
	 * @param  Leader 目标主将
	 * @return 是否全灭
	 * @note   详细流程分析: 主将死亡 -> 遍历士兵状态
	 *         性能/架构注意事项: 士兵数量通常有限，可接受遍历
	 */
	bool IsLeaderArmyEliminated(AXBCharacterBase* Leader) const;

	/**
	 * @brief  进入战斗状态
	 * @param  TargetLeader 目标主将
	 * @return 无
	 * @note   详细流程分析: 缓存目标 -> 触发EnterCombat
	 *         性能/架构注意事项: 仅在目标有效时调用
	 */
	void EnterCombatState(AXBCharacterBase* TargetLeader);

	/**
	 * @brief  退出战斗状态并回归行为
	 * @return 无
	 * @note   详细流程分析: 清理目标 -> 触发ExitCombat -> 重置行为中心
	 *         性能/架构注意事项: 退出时停止移动，防止路径残留
	 */
	void ExitCombatState();

	/**
	 * @brief  更新行为模式移动
	 * @param  DeltaSeconds 帧间隔
	 * @return 无
	 * @note   详细流程分析: 根据配置选择站立/随机/路线
	 *         性能/架构注意事项: 通过间隔控制随机移动频率
	 */
	void UpdateBehaviorMovement(float DeltaSeconds);

	/**
	 * @brief  更新原地站立行为
	 * @return 无
	 * @note   详细流程分析: 回到初始位置 -> 到位后停止移动
	 *         性能/架构注意事项: 使用接受半径避免频繁微调
	 */
	void UpdateStandBehavior();

	/**
	 * @brief  更新范围内随机移动行为
	 * @return 无
	 * @note   详细流程分析: 按间隔取随机点 -> MoveToLocation
	 *         性能/架构注意事项: 仅在间隔到达时计算随机点
	 */
	void UpdateWanderBehavior();

	/**
	 * @brief  更新路线巡逻行为
	 * @return 无
	 * @note   详细流程分析: 移动至路线点 -> 到达后切换下一个点
	 *         性能/架构注意事项: 路线为空时回退为站立
	 */
	void UpdateRouteBehavior();

	/**
	 * @brief  获取假人主将
	 * @return 假人主将指针
	 * @note   详细流程分析: 从控制器Pawn安全转换
	 *         性能/架构注意事项: 每帧访问可接受
	 */
	AXBDummyCharacter* GetDummyPawn() const;

private:
	// ✨ 新增 - 主将AI状态
	UENUM()
	enum class EXBDummyLeaderAIState : uint8
	{
		Behavior,
		Combat
	};

	// ✨ 新增 - 行为树资源
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "行为树资源", AllowPrivateAccess = "true"))
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	// ✨ 新增 - 黑板键名
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "受击响应键名", AllowPrivateAccess = "true"))
	FName DamageResponseKey = TEXT("DamageResponseReady");

	// ✨ 新增 - AI配置缓存
	FXBLeaderAIConfig CachedAIConfig;

	// ✨ 新增 - 状态缓存
	EXBDummyLeaderAIState CurrentState = EXBDummyLeaderAIState::Behavior;

	// ✨ 新增 - 初始位置
	FVector HomeLocation = FVector::ZeroVector;

	// ✨ 新增 - 行为中心位置（随机移动基准）
	FVector BehaviorCenterLocation = FVector::ZeroVector;

	// ✨ 新增 - 目标主将缓存
	TWeakObjectPtr<AXBCharacterBase> CurrentTargetLeader;

	// ✨ 新增 - 路线样条缓存
	TWeakObjectPtr<USplineComponent> CachedPatrolSpline;

	// ✨ 新增 - 路线索引缓存
	int32 CurrentRoutePointIndex = 0;

	// ✨ 新增 - 时间控制
	float NextSearchTime = 0.0f;
	float NextWanderTime = 0.0f;
	bool bLeaderAIInitialized = false;
};
