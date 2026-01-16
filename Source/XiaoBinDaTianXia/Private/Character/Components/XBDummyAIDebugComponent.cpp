/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Character/Components/XBDummyAIDebugComponent.cpp

/**
 * @file XBDummyAIDebugComponent.cpp
 * @brief 假人AI调试组件实现
 *
 * @note ✨ 新增 - 使用 DrawDebugCircle 绘制可视化范围
 */

#include "Character/Components/XBDummyAIDebugComponent.h"
#include "Character/XBDummyCharacter.h"
#include "Character/Components/XBCombatComponent.h"
#include "Data/XBLeaderDataTable.h"
#include "DrawDebugHelpers.h"

UXBDummyAIDebugComponent::UXBDummyAIDebugComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UXBDummyAIDebugComponent::BeginPlay()
{
	Super::BeginPlay();

	// 缓存假人主将
	CachedDummy = Cast<AXBDummyCharacter>(GetOwner());

	if (!CachedDummy.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("XBDummyAIDebugComponent: 挂载对象不是假人主将，组件将不工作"));
	}
}

/**
 * @brief Tick更新绘制调试范围
 * @param DeltaTime 帧间隔
 * @param TickType Tick类型
 * @param ThisTickFunction Tick函数
 */
void UXBDummyAIDebugComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 仅在编辑器或开发版本中绘制
#if !UE_BUILD_SHIPPING
	if (!CachedDummy.IsValid())
	{
		return;
	}

	AXBDummyCharacter* Dummy = CachedDummy.Get();

	// 绘制视野范围
	if (bDrawVisionRange)
	{
		const FXBLeaderAIConfig& AIConfig = Dummy->GetLeaderAIConfig();
		DrawDebugRangeCircle(AIConfig.VisionRange, VisionRangeColor);
	}

	// 绘制攻击范围
	UXBCombatComponent* CombatComp = Dummy->GetCombatComponent();
	if (CombatComp)
	{
		// 绘制普攻范围
		if (bDrawBasicAttackRange)
		{
			DrawDebugRangeCircle(CombatComp->GetBasicAttackRange(), BasicAttackRangeColor);
		}

		// 绘制技能范围
		if (bDrawSkillRange)
		{
			DrawDebugRangeCircle(CombatComp->GetSkillAttackRange(), SkillRangeColor);
		}
	}

	// ✨ 新增 - 绘制巡逻/随机移动范围
	if (bDrawPatrolRange)
	{
		const FXBLeaderAIConfig& AIConfig = Dummy->GetLeaderAIConfig();
		// 🔧 修改 - 统一使用 WanderRadius（原 PatrolRadius 已删除）
		DrawDebugRangeCircle(AIConfig.WanderRadius, PatrolRangeColor);
	}
#endif
}

/**
 * @brief 绘制调试圆圈
 * @param Radius 半径
 * @param Color 颜色
 */
void UXBDummyAIDebugComponent::DrawDebugRangeCircle(float Radius, const FColor& Color) const
{
	if (!CachedDummy.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Center = CachedDummy->GetActorLocation() + FVector(0.0f, 0.0f, HeightOffset);

	DrawDebugCircle(
		World,
		Center,
		Radius,
		CircleSegments,
		Color,
		false,
		-1.0f,
		0,
		2.0f,
		FVector::RightVector,
		FVector::ForwardVector,
		false
	);
}
