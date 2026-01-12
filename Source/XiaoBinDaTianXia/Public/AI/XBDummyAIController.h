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
	 * @brief  获取受击响应黑板键
	 * @return 黑板键名
	 * @note   详细流程分析: 统一返回默认键名
	 *         性能/架构注意事项: 仅轻量字符串读取，可频繁调用
	 */
	FName GetDamageResponseKey() const;

	/**
	 * @brief  玩家主将生成后启动行为树
	 * @return 无
	 * @note   详细流程分析: 校验配置 -> 加载行为树 -> 初始化黑板
	 *         性能/架构注意事项: 仅在主将生成后调用，避免过早启动
	 */
	void StartBehaviorTreeAfterPlayerSpawn();

	/**
	 * @brief 设置受击响应标记
	 * @param bReady 是否准备执行攻击
	 * @return 无
	 * @note   详细流程分析: 获取黑板 -> 写入标记
	 *         性能/架构注意事项: 黑板不存在时仅记录日志
	 */
	void SetDamageResponseReady(bool bReady);

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	/**
	 * @brief  尝试启动行为树
	 * @return 是否成功启动
	 * @note   详细流程分析: 校验配置 -> 加载行为树 -> 运行行为树
	 *         性能/架构注意事项: 避免重复启动
	 */
	bool TryStartBehaviorTree();

	// ✨ 新增 - AI配置缓存
	FXBLeaderAIConfig CachedAIConfig;

	// ✨ 新增 - 是否已初始化配置
	bool bLeaderAIInitialized = false;

	// ✨ 新增 - 是否已启动行为树
	bool bBehaviorTreeStarted = false;
};
