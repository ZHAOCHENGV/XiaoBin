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

/**
 * @brief  初始化假人AI配置缓存
 * @param  Dummy 假人主将
 * @return 无
 * @note   详细流程分析: 读取主将AI配置 -> 缓存到控制器
 *         性能/架构注意事项: 仅在初始化阶段调用，避免运行期频繁写入
 */
void AXBDummyAIController::InitializeLeaderAIConfig(const AXBDummyCharacter* Dummy)
{
	// 🔧 修改 - Dummy 无效时直接返回，避免空指针访问
	if (!Dummy)
	{
		return;
	}

	// 🔧 修改 - 缓存配置，避免每帧读取数据表
	CachedAIConfig = Dummy->GetLeaderAIConfig();
	bLeaderAIInitialized = true;
}

void AXBDummyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 🔧 修改 - 初始化假人主将AI配置
	if (AXBDummyCharacter* Dummy = Cast<AXBDummyCharacter>(InPawn))
	{
		// 🔧 修改 - 统一走初始化接口，确保可复用
		InitializeLeaderAIConfig(Dummy);
	}

	// 🔧 修改 - 行为树由玩家主将生成后统一启动，避免过早启动
	bBehaviorTreeStarted = false;
}

/**
 * @brief  玩家主将生成后启动行为树
 * @return 无
 * @note   详细流程分析: 校验配置 -> 加载行为树 -> 初始化黑板
 *         性能/架构注意事项: 避免在主将未生成时启动
 */
void AXBDummyAIController::StartBehaviorTreeAfterPlayerSpawn()
{
	// 🔧 修改 - 已启动则直接返回，避免重复启动
	if (bBehaviorTreeStarted)
	{
		return;
	}

	// 🔧 修改 - 若配置未初始化，尝试从当前Pawn补全
	if (!bLeaderAIInitialized)
	{
		if (AXBDummyCharacter* Dummy = Cast<AXBDummyCharacter>(GetPawn()))
		{
			CachedAIConfig = Dummy->GetLeaderAIConfig();
			bLeaderAIInitialized = true;
		}
	}

	if (!bLeaderAIInitialized)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人AI控制器未初始化配置，无法启动行为树"));
		return;
	}

	if (!TryStartBehaviorTree())
	{
		return;
	}

	// 🔧 修改 - 初始化黑板默认值，确保受击响应键可用
	if (UBlackboardComponent* BlackboardComp = GetBlackboardComponent())
	{
		BlackboardComp->SetValueAsBool(GetDamageResponseKey(), false);
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
 * @note   详细流程分析: 统一返回默认键名
 *         性能/架构注意事项: 仅轻量字符串读取，可频繁调用
 */
FName AXBDummyAIController::GetDamageResponseKey() const
{
	// 🔧 修改 - 黑板键使用默认固定名称，避免数据表配置差异
	static const FName DamageResponseKey(TEXT("DamageResponseReady"));
	return DamageResponseKey;
}

/**
 * @brief  尝试启动行为树
 * @return 是否成功启动
 * @note   详细流程分析: 校验配置 -> 加载行为树 -> 运行行为树
 *         性能/架构注意事项: 避免重复启动
 */
bool AXBDummyAIController::TryStartBehaviorTree()
{
	if (CachedAIConfig.BehaviorTree.IsNull())
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人AI控制器未配置行为树资源"));
		return false;
	}

	UBehaviorTree* LoadedBehaviorTree = CachedAIConfig.BehaviorTree.LoadSynchronous();
	if (!LoadedBehaviorTree)
	{
		UE_LOG(LogXBAI, Warning, TEXT("假人AI控制器行为树资源加载失败"));
		return false;
	}

	RunBehaviorTree(LoadedBehaviorTree);
	bBehaviorTreeStarted = true;
	UE_LOG(LogXBAI, Log, TEXT("假人AI控制器在玩家主将生成后启动行为树: %s"), *LoadedBehaviorTree->GetName());
	return true;
}
