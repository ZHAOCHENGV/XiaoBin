/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Animation/ANS_XBMeleeDetection.h

/**
 * @file ANS_XBMeleeDetection.h
 * @brief 近战检测动画通知状态 - 支持攻击范围缩放
 * 
 * @note 🔧 修改记录:
 *       1. 新增角色缩放倍率支持
 *       2. 胶囊体检测范围随角色体型动态缩放
 *       3. 🔧 修复士兵检测问题 - 添加自定义碰撞通道支持
 *       4. ✨ 新增阵营过滤功能
 *       5. ❌ 删除 BaseDamage（现在从战斗组件获取）
 */

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "Army/XBSoldierTypes.h"
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

    /** @brief 是否检测士兵碰撞通道（XBCollision::Soldier） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置", meta = (DisplayName = "检测士兵通道"))
    bool bDetectSoldierChannel = true;

    /** @brief 是否检测将领碰撞通道（XBCollision::Leader） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置", meta = (DisplayName = "检测将领通道"))
    bool bDetectLeaderChannel = true;

    /** @brief 是否启用攻击范围缩放 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置", meta = (DisplayName = "启用范围缩放"))
    bool bEnableRangeScaling = true;

    /** @brief 缩放倍率（相对于角色缩放）*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置", meta = (DisplayName = "缩放倍率", ClampMin = "0.1", EditCondition = "bEnableRangeScaling"))
    float ScaleMultiplier = 1.0f;

    /** @brief 是否启用阵营过滤（只伤害敌对阵营） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置", meta = (DisplayName = "启用阵营过滤"))
    bool bEnableFactionFilter = true;

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

    // ❌ 删除 - BaseDamage（现在从战斗组件获取）

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "伤害配置", meta = (DisplayName = "伤害效果类"))
    TSubclassOf<class UGameplayEffect> DamageEffectClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "伤害配置", meta = (DisplayName = "伤害Tag"))
    FGameplayTag DamageTag;

    /** 命中音效标签（命中敌人时播放） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "音效配置", meta = (DisplayName = "命中音效", Categories = "Sound"))
    FGameplayTag HitSoundTag;

    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

    virtual FString GetNotifyName_Implementation() const override { return TEXT("XB近战检测"); }

protected:
    TArray<FHitResult> PerformCapsuleTrace(USkeletalMeshComponent* MeshComp);
    void ApplyDamageToTargets(const TArray<FHitResult>& HitResults, AActor* OwnerActor);

    /**
     * @brief 获取角色的当前缩放倍率
     * @param OwnerActor 角色Actor
     * @return 缩放倍率（如 1.5 表示放大到 150%）
     */
    float GetOwnerScale(AActor* OwnerActor) const;

    /**
     * @brief 获取攻击者的阵营
     * @param OwnerActor 攻击者Actor
     * @return 阵营枚举
     */
    EXBFaction GetOwnerFaction(AActor* OwnerActor) const;

    /**
     * @brief 检查目标是否应该受到伤害（阵营过滤）
     * @param OwnerActor 攻击者
     * @param TargetActor 目标
     * @return 是否应该造成伤害
     */
    bool ShouldDamageTarget(AActor* OwnerActor, AActor* TargetActor) const;

    // ✨ 新增 - 从战斗组件获取伤害值
    /**
     * @brief 获取当前攻击的伤害值
     * @param OwnerActor 攻击者
     * @return 伤害值（已应用倍率）
     */
    float GetAttackDamage(AActor* OwnerActor) const;

private:
    UPROPERTY()
    TSet<AActor*> HitActors;
};
