/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Animation/AN_XBMeleeHit.h

/**
 * @file AN_XBMeleeHit.h
 * @brief 近战命中动画通知 - 单次伤害判定
 *
 * @note ✨ 新增文件 - 基于 ANS_XBMeleeDetection 改造
 *       用于在动画的特定帧执行一次伤害检测，而非持续检测
 */

#pragma once


#include "Animation/AnimNotifies/AnimNotify.h"
#include "Army/XBSoldierTypes.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class UNiagaraSystem;
#include "AN_XBMeleeHit.generated.h"

/**
 * @brief 近战命中检测配置
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBMeleeHitConfig {
  GENERATED_BODY()

  /** @brief 起始骨骼插槽名称 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置",
            meta = (DisplayName = "起始骨骼插槽"))
  FName StartSocketName = FName("weapon_start");

  /** @brief 结束骨骼插槽名称 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置",
            meta = (DisplayName = "结束骨骼插槽"))
  FName EndSocketName = FName("weapon_end");

  /** @brief 起始位置偏移 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置",
            meta = (DisplayName = "起始位置偏移"))
  FVector StartLocationOffset = FVector::ZeroVector;

  /** @brief 结束位置偏移 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置",
            meta = (DisplayName = "结束位置偏移"))
  FVector EndLocationOffset = FVector::ZeroVector;

  /** @brief 起始旋转偏移 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置",
            meta = (DisplayName = "起始旋转偏移"))
  FRotator StartRotationOffset = FRotator::ZeroRotator;

  /** @brief 结束旋转偏移 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置",
            meta = (DisplayName = "结束旋转偏移"))
  FRotator EndRotationOffset = FRotator::ZeroRotator;

  /** @brief 是否根据角色朝向旋转检测区域 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置",
            meta = (DisplayName = "随角色朝向旋转"))
  bool bRotateWithCharacter = true;

  /** @brief 检测半径 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置",
            meta = (DisplayName = "检测半径", ClampMin = "1.0"))
  float DetectionRadius = 50.0f;

  /** @brief 是否检测士兵碰撞通道 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置",
            meta = (DisplayName = "检测士兵通道"))
  bool bDetectSoldierChannel = true;

  /** @brief 是否检测将领碰撞通道 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置",
            meta = (DisplayName = "检测将领通道"))
  bool bDetectLeaderChannel = true;

  /** @brief 是否启用攻击范围缩放 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置",
            meta = (DisplayName = "启用范围缩放"))
  bool bEnableRangeScaling = true;

  /** @brief 缩放倍率 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置",
            meta = (DisplayName = "缩放倍率", ClampMin = "0.1",
                    EditCondition = "bEnableRangeScaling"))
  float ScaleMultiplier = 1.0f;

  /** @brief 是否启用阵营过滤 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置",
            meta = (DisplayName = "启用阵营过滤"))
  bool bEnableFactionFilter = true;

  /** @brief 是否启用调试绘制 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试",
            meta = (DisplayName = "启用调试绘制"))
  bool bEnableDebugDraw = false;

  /** @brief 调试绘制持续时间 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试",
            meta = (DisplayName = "调试绘制时间", ClampMin = "0.0",
                    EditCondition = "bEnableDebugDraw"))
  float DebugDrawDuration = 1.0f;

  FXBMeleeHitConfig() {}
};

/**
 * @brief 近战命中动画通知 - 单次伤害判定
 * @note 在动画的特定帧触发一次伤害检测，适用于需要精准打击时机的攻击
 */
UCLASS(meta = (DisplayName = "XB主将近战命中"))
class XIAOBINDATIANXIA_API UAN_XBMeleeHit : public UAnimNotify {
  GENERATED_BODY()

public:
  UAN_XBMeleeHit();

  /** @brief 检测配置 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "检测配置",
            meta = (DisplayName = "检测配置"))
  FXBMeleeHitConfig HitConfig;

  /** @brief 伤害效果类 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "伤害配置",
            meta = (DisplayName = "伤害效果类"))
  TSubclassOf<class UGameplayEffect> DamageEffectClass;

  /** @brief 伤害Tag */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "伤害配置",
            meta = (DisplayName = "伤害Tag"))
  FGameplayTag DamageTag;

  /** @brief 命中音效标签 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "音效配置",
            meta = (DisplayName = "命中音效", Categories = "Sound"))
  FGameplayTag HitSoundTag;

  /** @brief 命中 Niagara 特效 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "特效配置",
            meta = (DisplayName = "命中Niagara特效"))
  TObjectPtr<UNiagaraSystem> HitNiagaraEffect;

  // UAnimNotify 接口
  virtual void Notify(USkeletalMeshComponent *MeshComp,
                      UAnimSequenceBase *Animation,
                      const FAnimNotifyEventReference &EventReference) override;
  virtual FString GetNotifyName_Implementation() const override {
    return TEXT("XB近战命中");
  }

protected:
  /**
   * @brief 执行伤害检测
   * @param MeshComp 骨骼网格组件
   * @return 命中结果列表
   */
  TArray<FHitResult> PerformHitDetection(USkeletalMeshComponent *MeshComp);

  /**
   * @brief 对命中目标应用伤害
   * @param HitResults 命中结果
   * @param OwnerActor 攻击者
   */
  void ApplyDamageToTargets(const TArray<FHitResult> &HitResults,
                            AActor *OwnerActor);

  /**
   * @brief 获取攻击者的缩放倍率
   */
  float GetOwnerScale(AActor *OwnerActor) const;

  /**
   * @brief 获取攻击者阵营
   */
  EXBFaction GetOwnerFaction(AActor *OwnerActor) const;

  /**
   * @brief 检查目标是否应该受到伤害
   */
  bool ShouldDamageTarget(AActor *OwnerActor, AActor *TargetActor) const;

  /**
   * @brief 获取攻击伤害值
   */
  float GetAttackDamage(AActor *OwnerActor) const;
};
