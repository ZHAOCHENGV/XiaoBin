// Source/XiaoBinDaTianXia/Public/Animation/AN_XBSpawnAbility.h
/* --- 完整文件代码 --- */

// Copyright XiaoBing Project. All Rights Reserved.

/**
 * @file AN_XBSpawnAbility.h
 * @brief 技能释放动画通知
 * 
 * 功能说明：
 * - 在动画特定帧触发技能效果
 * - 支持生成投射物、AOE效果等
 * - 可配置触发的GameplayEvent
 */

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AN_XBSpawnAbility.generated.h"

class UNiagaraSystem;

/**
 * @brief 技能释放动画通知
 * @note 在动画播放到此通知时，发送GameplayEvent触发技能效果
 */
UCLASS(DisplayName = "XB: 释放技能")
class XIAOBINDATIANXIA_API UAN_XBSpawnAbility : public UAnimNotify
{
    GENERATED_BODY()

public:
    UAN_XBSpawnAbility();

    /**
     * @brief 获取通知名称（编辑器显示）
     */
    virtual FString GetNotifyName_Implementation() const override;

    /**
     * @brief 通知触发
     * @param MeshComp 触发通知的骨骼网格体组件
     * @param Animation 当前播放的动画序列
     * @param EventReference 事件引用
     */
    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, 
        const FAnimNotifyEventReference& EventReference) override;

protected:
    // ==================== 配置 ====================

    /** 触发的GameplayEvent标签 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "技能", meta = (DisplayName = "事件标签"))
    FGameplayTag EventTag;

    /** 技能生成位置使用的骨骼插槽 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "技能", meta = (DisplayName = "生成插槽"))
    FName SpawnSocketName = NAME_None;

    /** 位置偏移 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "技能", meta = (DisplayName = "位置偏移"))
    FVector LocationOffset = FVector::ZeroVector;

    /** 是否使用角色朝向而非插槽朝向 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "技能", meta = (DisplayName = "使用角色朝向"))
    bool bUseActorRotation = false;

    /** 触发时播放的特效 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "视觉", meta = (DisplayName = "触发特效"))
    TSoftObjectPtr<UNiagaraSystem> SpawnVFX;

    /** 触发时播放的音效 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "音效", meta = (DisplayName = "触发音效"))
    TSoftObjectPtr<USoundBase> SpawnSound;

    /** 事件附带的额外数据（可选） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "技能", meta = (DisplayName = "事件数值"))
    float EventMagnitude = 0.0f;
};
