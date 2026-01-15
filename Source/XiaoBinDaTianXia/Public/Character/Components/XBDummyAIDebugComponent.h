/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Character/Components/XBDummyAIDebugComponent.h

/**
 * @file XBDummyAIDebugComponent.h
 * @brief 假人AI调试组件 - 可视化视野和攻击范围
 *
 * @note ✨ 新增 - 用于调试假人AI的战斗逻辑
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "XBDummyAIDebugComponent.generated.h"

class AXBDummyCharacter;

/**
 * @brief 假人AI调试组件
 * @note 在编辑器中可视化视野范围、普攻范围、技能范围
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent, DisplayName = "XB假人AI调试"))
class XIAOBINDATIANXIA_API UXBDummyAIDebugComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UXBDummyAIDebugComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============ 调试开关 ============

	// 启用视野范围绘制
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|视野", meta = (DisplayName = "绘制视野范围"))
	bool bDrawVisionRange = true;

	// 启用移动范围绘制
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|移动", meta = (DisplayName = "绘制移动范围"))
	bool bDrawMoveRange = true;

	// 启用随机移动半径绘制
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|移动", meta = (DisplayName = "绘制随机移动半径"))
	bool bDrawWanderRadius = false;

	// 启用站立回位半径绘制
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|移动", meta = (DisplayName = "绘制站立回位半径"))
	bool bDrawStandReturnRadius = false;

	// 启用普攻范围绘制
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|攻击", meta = (DisplayName = "绘制普攻范围"))
	bool bDrawBasicAttackRange = true;

	// 启用技能范围绘制
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|攻击", meta = (DisplayName = "绘制技能范围"))
	bool bDrawSkillRange = true;

	// ============ 颜色配置 ============

	// 视野范围颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|颜色", meta = (DisplayName = "视野范围颜色"))
	FColor VisionRangeColor = FColor::Green;

	// 移动范围颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|颜色", meta = (DisplayName = "移动范围颜色"))
	FColor MoveRangeColor = FColor(0, 200, 255);

	// 随机移动半径颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|颜色", meta = (DisplayName = "随机移动半径颜色"))
	FColor WanderRadiusColor = FColor(0, 120, 255);

	// 站立回位半径颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|颜色", meta = (DisplayName = "站立回位半径颜色"))
	FColor StandReturnRadiusColor = FColor(255, 200, 0);

	// 普攻范围颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|颜色", meta = (DisplayName = "普攻范围颜色"))
	FColor BasicAttackRangeColor = FColor::Red;

	// 技能范围颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|颜色", meta = (DisplayName = "技能范围颜色"))
	FColor SkillRangeColor = FColor::Blue;

	// ============ 绘制配置 ============

	// 调试圆圈分段数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|配置", meta = (DisplayName = "圆圈分段数", ClampMin = "8", ClampMax = "64"))
	int32 CircleSegments = 32;

	// 调试绘制高度偏移
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|配置", meta = (DisplayName = "高度偏移"))
	float HeightOffset = 10.0f;

protected:
	virtual void BeginPlay() override;

private:
	// 绘制调试圆圈
	void DrawDebugRangeCircle(float Radius, const FColor& Color) const;

	// 缓存的假人主将
	UPROPERTY()
	TWeakObjectPtr<AXBDummyCharacter> CachedDummy;
};
