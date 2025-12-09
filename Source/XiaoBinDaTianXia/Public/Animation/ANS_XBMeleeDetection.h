/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Animation/ANS_XBMeleeDetection.h

/**
 * @file ANS_XBMeleeDetection.h
 * @brief 近战检测动画通知状态 - 支持双骨骼插槽的胶囊体检测
 * @note 在动画蒙太奇中配置检测参数，实现可视化的近战碰撞检测
 */

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "ANS_XBMeleeDetection.generated.h"

/**
 * @brief 近战检测配置结构体
 * @note 在动画通知中直接配置，不依赖数据表
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBMeleeDetectionConfig
{
    GENERATED_BODY()

    /** @brief 起始位置骨骼插槽名称 */
    // ✨ 新增 - 起始插槽配置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置", meta = (DisplayName = "起始骨骼插槽"))
    FName StartSocketName = FName("weapon_start");

    /** @brief 结束位置骨骼插槽名称 */
    // ✨ 新增 - 结束插槽配置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置", meta = (DisplayName = "结束骨骼插槽"))
    FName EndSocketName = FName("weapon_end");

    /** @brief 起始位置偏移 (相对于起始插槽) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置", meta = (DisplayName = "起始位置偏移"))
    FVector StartLocationOffset = FVector::ZeroVector;

    /** @brief 结束位置偏移 (相对于结束插槽) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置", meta = (DisplayName = "结束位置偏移"))
    FVector EndLocationOffset = FVector::ZeroVector;

    /** @brief 起始旋转偏移 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置", meta = (DisplayName = "起始旋转偏移"))
    FRotator StartRotationOffset = FRotator::ZeroRotator;

    /** @brief 结束旋转偏移 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置", meta = (DisplayName = "结束旋转偏移"))
    FRotator EndRotationOffset = FRotator::ZeroRotator;

    /** @brief 胶囊体半径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置", meta = (DisplayName = "胶囊体半径", ClampMin = "1.0"))
    float CapsuleRadius = 30.0f;

    /** @brief 胶囊体半高 (设为0则自动计算为两点间距离的一半) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置", meta = (DisplayName = "胶囊体半高", ClampMin = "0.0"))
    float CapsuleHalfHeight = 0.0f;

    /** @brief 检测的对象类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置", meta = (DisplayName = "检测对象类型"))
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;

    /** @brief 要忽略的Actor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置", meta = (DisplayName = "忽略的Actor"))
    TArray<AActor*> ActorsToIgnore;

    /** @brief 是否启用调试绘制 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试", meta = (DisplayName = "启用调试绘制"))
    bool bEnableDebugDraw = false;

    /** @brief 调试绘制持续时间 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试", meta = (DisplayName = "调试绘制时间", ClampMin = "0.0", EditCondition = "bEnableDebugDraw"))
    float DebugDrawDuration = 0.1f;

    FXBMeleeDetectionConfig()
    {
        // 默认检测Pawn类型
        ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
    }
};

/**
 * @brief 近战检测动画通知状态
 * @note 在蒙太奇播放期间持续进行胶囊体碰撞检测
 */
UCLASS(meta = (DisplayName = "XB近战检测"))
class XIAOBINDATIANXIA_API UANS_XBMeleeDetection : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    UANS_XBMeleeDetection();

    /** @brief 近战检测配置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置", meta = (DisplayName = "检测配置"))
    FXBMeleeDetectionConfig DetectionConfig;

    /** @brief 造成的伤害值 (通过SetByCaller传递) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "伤害配置", meta = (DisplayName = "基础伤害值"))
    float BaseDamage = 10.0f;

    /** @brief 伤害GameplayEffect类 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "伤害配置", meta = (DisplayName = "伤害效果类"))
    TSubclassOf<class UGameplayEffect> DamageEffectClass;

    /** @brief 伤害Tag (用于SetByCaller) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "伤害配置", meta = (DisplayName = "伤害Tag"))
    FGameplayTag DamageTag;

    // AnimNotifyState 接口
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

    virtual FString GetNotifyName_Implementation() const override { return TEXT("XB近战检测"); }

protected:
    /**
     * @brief 执行胶囊体检测
     * @param MeshComp 骨骼网格体组件
     * @return 检测到的所有命中结果
     */
    TArray<FHitResult> PerformCapsuleTrace(USkeletalMeshComponent* MeshComp);

    /**
     * @brief 对命中目标应用伤害
     * @param HitResults 命中结果数组
     * @param OwnerActor 攻击者
     */
    void ApplyDamageToTargets(const TArray<FHitResult>& HitResults, AActor* OwnerActor);

private:
    /** @brief 已命中的Actor集合 (防止重复伤害) */
    UPROPERTY()
    TSet<AActor*> HitActors;
};
