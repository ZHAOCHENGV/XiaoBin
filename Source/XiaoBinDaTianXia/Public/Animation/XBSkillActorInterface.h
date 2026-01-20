/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Animation/XBSkillActorInterface.h

/**
 * @file XBSkillActorInterface.h
 * @brief 技能Actor接口 - 用于统一技能Actor的初始化和伤害传递
 * 
 * @note ✨ 新增文件
 *       所有由动画通知生成的技能Actor都应实现此接口
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "XBSkillActorInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class UXBSkillActorInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * @brief 技能Actor接口
 * @note 实现此接口的Actor可被 AN_XBSpawnSkillActor 动画通知生成并初始化
 */
class XIAOBINDATIANXIA_API IXBSkillActorInterface
{
    GENERATED_BODY()

public:
    /**
     * @brief 初始化技能Actor
     * @param Instigator 施法者（主将或士兵）
     * @param Damage 伤害值（已应用倍率）
     * @param SpawnDirection 生成方向（施法者朝向或指向目标）
     * @param Target 目标Actor（可选）
     * @note 由动画通知在生成后调用
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Skill")
    virtual void InitializeSkillActor(AActor* Instigator, float Damage, FVector SpawnDirection, AActor* Target = nullptr) = 0;

    /**
     * @brief 获取技能伤害值
     * @return 当前技能伤害
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Skill")
    virtual float GetSkillDamage() const = 0;

    /**
     * @brief 获取施法者
     * @return 施法者Actor
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Skill")
    virtual AActor* GetSkillInstigator() const = 0;
};
