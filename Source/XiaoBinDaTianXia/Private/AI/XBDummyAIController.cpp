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
#include "Character/XBDummyCharacter.h"
#include "Utils/XBLogCategories.h"

AXBDummyAIController::AXBDummyAIController()
{
	bAttachToPawn = true;
	// 🔧 修改 - 行为树驱动不依赖Tick，关闭Tick降低开销
	PrimaryActorTick.bCanEverTick = false;
}

void AXBDummyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 🔧 修改 - 初始化假人主将AI配置
	if (AXBDummyCharacter* Dummy = Cast<AXBDummyCharacter>(InPawn))
	{
		// 🔧 修改 - 缓存配置，避免每帧读取数据表
		CachedAIConfig = Dummy->GetLeaderAIConfig();
		bLeaderAIInitialized = true;
	}

	// 🔧 修改 - 启动行为树，确保假人AI逻辑可运行
	if (bLeaderAIInitialized && !CachedAIConfig.BehaviorTree.IsNull())
	{
		if (UBehaviorTree* LoadedBehaviorTree = CachedAIConfig.BehaviorTree.LoadSynchronous())
		{
			RunBehaviorTree(LoadedBehaviorTree);
			UE_LOG(LogXBAI, Log, TEXT("假人AI控制器启动行为树: %s"), *LoadedBehaviorTree->GetName());
		}
		else
		{
			UE_LOG(LogXBAI, Warning, TEXT("假人AI控制器行为树资源加载失败"));
		}
	}
	else if (bLeaderAIInitialized && CachedAIConfig.BehaviorTree.IsNull())
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人AI控制器未配置行为树资源"));
	}

	// ✨ 新增 - 初始化黑板默认值，确保行为树键可用
	if (UBlackboardComponent* BlackboardComp = GetBlackboardComponent())
	{
		const FName DamageResponseKey = GetDamageResponseKey();
		BlackboardComp->SetValueAsBool(DamageResponseKey, false);
	}
}

void AXBDummyAIController::SetDamageResponseReady(bool bReady)
{
	// 🔧 修改 - 使用黑板同步受击响应状态
	if (UBlackboardComponent* BlackboardComp = GetBlackboardComponent())
	{
		BlackboardComp->SetValueAsBool(GetDamageResponseKey(), bReady);
		UE_LOG(LogXBAI, Log, TEXT("假人AI黑板受击响应标记=%s"), bReady ? TEXT("true") : TEXT("false"));
	}
	else
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人AI控制器黑板无效，无法写入受击响应标记"));
	}
}

/**
 * @brief  获取受击响应黑板键
 * @return 黑板键名
 * @note   详细流程分析: 优先读取数据表配置，否则使用默认键
 *         性能/架构注意事项: 仅轻量字符串读取，可频繁调用
 */
FName AXBDummyAIController::GetDamageResponseKey() const
{
	if (CachedAIConfig.DamageResponseKey.IsNone())
	{
		return TEXT("DamageResponseReady");
	}

	return CachedAIConfig.DamageResponseKey;
}
