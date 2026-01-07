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
#include "XBDummyAIController.generated.h"

class UBehaviorTree;
class UBlackboardComponent;

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

private:
	// ✨ 新增 - 行为树资源
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "行为树资源", AllowPrivateAccess = "true"))
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	// ✨ 新增 - 黑板键名
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (DisplayName = "受击响应键名", AllowPrivateAccess = "true"))
	FName DamageResponseKey = TEXT("DamageResponseReady");
};
