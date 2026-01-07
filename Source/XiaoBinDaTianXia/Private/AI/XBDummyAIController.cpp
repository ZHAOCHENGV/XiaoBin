/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/AI/XBDummyAIController.cpp

/**
 * @file XBDummyAIController.cpp
 * @brief 假人AI控制器实现
 * 
 * @note ✨ 新增 - 行为树启动与受击响应黑板写入
 */

#include "AI/XBDummyAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Utils/XBLogCategories.h"

AXBDummyAIController::AXBDummyAIController()
{
	bAttachToPawn = true;
}

void AXBDummyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 🔧 修改 - 启动行为树，确保假人AI逻辑可运行
	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
		UE_LOG(LogXBAI, Log, TEXT("假人AI控制器启动行为树: %s"), *BehaviorTreeAsset->GetName());
	}
	else
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人AI控制器未配置行为树资源"));
	}
}

void AXBDummyAIController::SetDamageResponseReady(bool bReady)
{
	// 🔧 修改 - 使用黑板同步受击响应状态
	if (UBlackboardComponent* BlackboardComp = GetBlackboardComponent())
	{
		BlackboardComp->SetValueAsBool(DamageResponseKey, bReady);
		UE_LOG(LogXBAI, Log, TEXT("假人AI黑板受击响应标记=%s"), bReady ? TEXT("true") : TEXT("false"));
	}
	else
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人AI控制器黑板无效，无法写入受击响应标记"));
	}
}
