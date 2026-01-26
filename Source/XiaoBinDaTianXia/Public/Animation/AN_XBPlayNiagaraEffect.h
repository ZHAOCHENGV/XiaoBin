// Source/XiaoBinDaTianXia/Public/Animation/AN_XBPlayNiagaraEffect.h

/**
 * @file AN_XBPlayNiagaraEffect.h
 * @brief 自定义Niagara特效动画通知 - 支持角色缩放
 *
 * @note ✨ 新增文件 - 基于 UAnimNotify_PlayNiagaraEffect 改造
 *       在设置的Scale基础上，根据拥有角色大小自动变更特效大小
 */

#pragma once

#include "AN_XBPlayNiagaraEffect.generated.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "CoreMinimal.h"


class UNiagaraSystem;
class UFXSystemComponent;

/**
 * @brief 自定义Niagara特效动画通知
 * @note 在设置的Scale基础上，根据角色大小自动调整特效缩放
 */
UCLASS(const, hidecategories = Object, collapsecategories,
       meta = (DisplayName = "XB播放Niagara特效"))
class XIAOBINDATIANXIA_API UAN_XBPlayNiagaraEffect : public UAnimNotify {
  GENERATED_BODY()

public:
  UAN_XBPlayNiagaraEffect();

  // UObject 接口
  virtual void PostLoad() override;
#if WITH_EDITOR
  virtual void PostEditChangeProperty(
      struct FPropertyChangedEvent &PropertyChangedEvent) override;
#endif

  // UAnimNotify 接口
  virtual FString GetNotifyName_Implementation() const override;
  virtual void Notify(USkeletalMeshComponent *MeshComp,
                      UAnimSequenceBase *Animation,
                      const FAnimNotifyEventReference &EventReference) override;
#if WITH_EDITOR
  virtual void ValidateAssociatedAssets() override;
#endif

  /** @brief 要生成的Niagara系统 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "特效配置",
            meta = (DisplayName = "Niagara系统"))
  TObjectPtr<UNiagaraSystem> Template;

  /** @brief 相对于插槽的位置偏移 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "特效配置",
            meta = (DisplayName = "位置偏移"))
  FVector LocationOffset;

  /** @brief 相对于插槽的旋转偏移 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "特效配置",
            meta = (DisplayName = "旋转偏移"))
  FRotator RotationOffset;

  /** @brief 基础缩放 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "特效配置",
            meta = (DisplayName = "基础缩放"))
  FVector Scale;

  /** @brief 是否使用绝对缩放模式 */
  UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "特效配置",
            meta = (DisplayName = "绝对缩放模式"))
  bool bAbsoluteScale;

  /** @brief 是否根据角色大小自动缩放特效 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "缩放配置",
            meta = (DisplayName = "随角色缩放"))
  bool bScaleWithCharacter;

  /** @brief 角色缩放倍率（在角色缩放基础上的额外倍率） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "缩放配置",
            meta = (DisplayName = "缩放倍率", ClampMin = "0.1",
                    EditCondition = "bScaleWithCharacter"))
  float CharacterScaleMultiplier;

  /** @brief 位置偏移是否随角色大小自动调整 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "缩放配置",
            meta = (DisplayName = "位置偏移随角色缩放",
                    EditCondition = "bScaleWithCharacter"))
  bool bScaleLocationOffset;

  /** @brief 是否附着到骨骼/插槽 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "附着配置",
            meta = (DisplayName = "附着到插槽"))
  uint32 Attached : 1;

  /** @brief 要附着的插槽名称 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "附着配置",
            meta = (DisplayName = "插槽名称", AnimNotifyBoneName = "true",
                    EditCondition = "Attached"))
  FName SocketName;

  /** @brief 是否在地面生成特效（向下检测地面位置） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "地面配置",
            meta = (DisplayName = "在地面释放", EditCondition = "!Attached"))
  bool bSpawnOnGround;

  /** @brief 地面检测类型（用于向下射线检测） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "地面配置",
            meta = (DisplayName = "地面检测类型",
                    EditCondition = "bSpawnOnGround"))
  TArray<TEnumAsByte<EObjectTypeQuery>> GroundTraceTypes;

  /** @brief 地面检测距离（从角色位置向下检测的最大距离） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "地面配置",
            meta = (DisplayName = "地面检测距离", ClampMin = "10.0",
                    EditCondition = "bSpawnOnGround"))
  float GroundTraceDistance;

  /** @brief 获取生成的特效组件 */
  UFUNCTION(BlueprintCallable, Category = "AnimNotify")
  UFXSystemComponent *GetSpawnedEffect();

protected:
  /** @brief 生成的特效组件指针 */
  UPROPERTY()
  TObjectPtr<UFXSystemComponent> SpawnedEffect;

  /** @brief 缓存的旋转偏移四元数 */
  FQuat RotationOffsetQuat;

  /**
   * @brief 生成特效
   * @param MeshComp 骨骼网格组件
   * @param Animation 动画序列
   * @return 生成的特效组件
   */
  virtual UFXSystemComponent *SpawnEffect(USkeletalMeshComponent *MeshComp,
                                          UAnimSequenceBase *Animation);

  /**
   * @brief 获取角色的当前缩放倍率
   * @param OwnerActor 角色Actor
   * @return 缩放倍率
   */
  float GetOwnerScale(AActor *OwnerActor) const;
};
