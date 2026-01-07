/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Character/XBConfigCameraPawn.h

/**
 * @file XBConfigCameraPawn.h
 * @brief 配置阶段浮空相机Pawn
 * 
 * @note ✨ 新增 - 配置阶段先行操控
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DefaultPawn.h"
#include "XBConfigCameraPawn.generated.h"



/**
 * @brief 配置阶段浮空相机Pawn
 * @note 继承 DefaultPawn，复用其移动与旋转逻辑
 */
UCLASS()
class XIAOBINDATIANXIA_API AXBConfigCameraPawn : public ADefaultPawn
{
	GENERATED_BODY()

public:
};
