// Source/XiaoBinDaTianXia/Public/Animation/AN_XBTriggerMeleeHit.h

/**
 * @file AN_XBTriggerMeleeHit.h
 * @brief 近战命中AnimNotify - 在蒙太奇Tag点触发GA事件
 */

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AN_XBTriggerMeleeHit.generated.h"

/**
 * @brief 近战命中AnimNotify
 * @note 通过GameplayEvent触发GA，由GA对当前目标结算伤害
 */
UCLASS(meta = (DisplayName = "XB 近战命中Tag"))
class XIAOBINDATIANXIA_API UAN_XBTriggerMeleeHit : public UAnimNotify
{
    GENERATED_BODY()

public:
    UAN_XBTriggerMeleeHit();

    /**
     * @brief 触发通知
     * @param MeshComp 骨骼网格
     * @param Animation 动画序列
     * @param EventReference 事件引用
     * @note 详细流程分析: 校验Owner -> 过滤弓手 -> 发送GameplayEvent
     *       性能/架构注意事项: 仅在蒙太奇Tag点触发，不额外Tick
     */
    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;

    virtual FString GetNotifyName_Implementation() const override;

protected:
    /** @brief 触发的事件Tag */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "战斗", meta = (DisplayName = "事件Tag"))
    FGameplayTag EventTag;

    /** 命中音效标签（命中敌人时播放） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "音效", meta = (DisplayName = "命中音效", Categories = "Sound"))
    FGameplayTag HitSoundTag;
};
