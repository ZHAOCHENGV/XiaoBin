/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Animation/ANS_XBMeleeDetection.h

/**
 * @file ANS_XBMeleeDetection.h
 * @brief 近战检测动画通知状态 - 支持攻击范围缩放
 * 
 * @note 🔧 修改记录:
 *       1. 新增角色缩放倍率支持
 *       2. 胶囊体检测范围随角色体型动态缩放
 */

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "ANS_XBMeleeDetection.generated.h"

/**
 * @brief 近战检测配置结构体
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBMeleeDetectionConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置", meta = (DisplayName = "起始骨骼插槽"))
    FName StartSocketName = FName("weapon_start");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置", meta = (DisplayName = "结束骨骼插槽"))
    FName EndSocketName = FName("weapon_end");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置", meta = (DisplayName = "起始位置偏移"))
    FVector StartLocationOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置", meta = (DisplayName = "结束位置偏移"))
    FVector EndLocationOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置", meta = (DisplayName = "起始旋转偏移"))
    FRotator StartRotationOffset = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置", meta = (DisplayName = "结束旋转偏移"))
    FRotator EndRotationOffset = FRotator::ZeroRotator;

    // 🔧 修改 - 这些值是基础值，实际使用时会乘以角色缩放倍率
    /** @brief 基础胶囊体半径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置", meta = (DisplayName = "基础胶囊体半径", ClampMin = "1.0"))
    float CapsuleRadius = 30.0f;

    /** @brief 基础胶囊体半高 (设为0则自动计算为两点间距离的一半) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置", meta = (DisplayName = "基础胶囊体半高", ClampMin = "0.0"))
    float CapsuleHalfHeight = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置", meta = (DisplayName = "检测对象类型"))
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置", meta = (DisplayName = "忽略的Actor"))
    TArray<AActor*> ActorsToIgnore;

    // ✨ 新增 - 缩放配置
    /** @brief 是否启用攻击范围缩放 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置", meta = (DisplayName = "启用范围缩放"))
    bool bEnableRangeScaling = true;

    /** @brief 缩放倍率（相对于角色缩放）*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置", meta = (DisplayName = "缩放倍率", ClampMin = "0.1", EditCondition = "bEnableRangeScaling"))
    float ScaleMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试", meta = (DisplayName = "启用调试绘制"))
    bool bEnableDebugDraw = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试", meta = (DisplayName = "调试绘制时间", ClampMin = "0.0", EditCondition = "bEnableDebugDraw"))
    float DebugDrawDuration = 0.1f;

    FXBMeleeDetectionConfig()
    {
        ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
    }
};

UCLASS(meta = (DisplayName = "XB近战检测"))
class XIAOBINDATIANXIA_API UANS_XBMeleeDetection : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    UANS_XBMeleeDetection();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置", meta = (DisplayName = "检测配置"))
    FXBMeleeDetectionConfig DetectionConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "伤害配置", meta = (DisplayName = "基础伤害值"))
    float BaseDamage = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "伤害配置", meta = (DisplayName = "伤害效果类"))
    TSubclassOf<class UGameplayEffect> DamageEffectClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "伤害配置", meta = (DisplayName = "伤害Tag"))
    FGameplayTag DamageTag;

    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

    virtual FString GetNotifyName_Implementation() const override { return TEXT("XB近战检测"); }

protected:
    TArray<FHitResult> PerformCapsuleTrace(USkeletalMeshComponent* MeshComp);
    void ApplyDamageToTargets(const TArray<FHitResult>& HitResults, AActor* OwnerActor);

    // ✨ 新增 - 获取角色的实际缩放倍率
    /**
     * @brief 获取角色的当前缩放倍率
     * @param OwnerActor 角色Actor
     * @return 缩放倍率（如 1.5 表示放大到 150%）
     */
    float GetOwnerScale(AActor* OwnerActor) const;

private:
    UPROPERTY()
    TSet<AActor*> HitActors;
};
