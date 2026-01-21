/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Animation/AN_XBSpawnSkillActor.h

/**
 * @file AN_XBSpawnSkillActor.h
 * @brief 通用技能生成动画通知 - 在动画指定帧生成技能Actor
 *
 * @note ✨ 新增文件
 *       支持多种生成位置模式：插槽、世界偏移、前方偏移、目标位置
 *       自动从施法者获取伤害并传递给技能Actor
 */

#pragma once


#include "Animation/AnimNotifies/AnimNotify.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AN_XBSpawnSkillActor.generated.h"

class UGameplayAbility;
class UGameplayEffect;

/**
 * @brief 技能生成位置模式
 */
UENUM(BlueprintType)
enum class EXBSkillSpawnMode : uint8 {
  /** 从骨骼插槽位置生成 */
  Socket UMETA(DisplayName = "插槽位置"),
  /** 从角色位置+世界坐标偏移生成 */
  WorldOffset UMETA(DisplayName = "世界偏移"),
  /** 从角色前方偏移生成（相对于角色朝向） */
  ForwardOffset UMETA(DisplayName = "前方偏移"),
  /** 从当前目标位置生成 */
  TargetBased UMETA(DisplayName = "目标位置")
};

/**
 * @brief 技能生成配置结构体
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBSkillSpawnConfig {
  GENERATED_BODY()

  /** 要生成的技能Actor类 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "生成配置",
            meta = (DisplayName = "技能Actor类"))
  TSubclassOf<AActor> ActorClass;

  /** 生成位置模式 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "生成配置",
            meta = (DisplayName = "生成模式"))
  EXBSkillSpawnMode SpawnMode = EXBSkillSpawnMode::Socket;

  /** 骨骼插槽名称（Socket模式使用） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置",
            meta = (DisplayName = "插槽名称",
                    EditCondition = "SpawnMode == EXBSkillSpawnMode::Socket"))
  FName SocketName = FName("weapon_muzzle");

  /** 位置偏移（相对于生成点） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置",
            meta = (DisplayName = "位置偏移"))
  FVector LocationOffset = FVector::ZeroVector;

  /** 旋转偏移 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置",
            meta = (DisplayName = "旋转偏移"))
  FRotator RotationOffset = FRotator::ZeroRotator;

  /** 是否继承施法者旋转 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置",
            meta = (DisplayName = "继承施法者旋转"))
  bool bInheritOwnerRotation = true;

  /** 是否附着到插槽（仅Socket模式有效） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "位置配置",
            meta = (DisplayName = "附着到插槽",
                    EditCondition = "SpawnMode == EXBSkillSpawnMode::Socket"))
  bool bAttachToSocket = false;

  /** 伤害倍率（基于施法者攻击力） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "伤害配置",
            meta = (DisplayName = "伤害倍率", ClampMin = "0.0"))
  float DamageMultiplier = 1.0f;

  /** 是否使用当前攻击的伤害（从战斗组件获取） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "伤害配置",
            meta = (DisplayName = "使用当前攻击伤害"))
  bool bUseCurrentAttackDamage = true;

  /** 固定伤害值（当不使用当前攻击伤害时） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "伤害配置",
            meta = (DisplayName = "固定伤害",
                    EditCondition = "!bUseCurrentAttackDamage",
                    ClampMin = "0.0"))
  float FixedDamage = 10.0f;

  /** 是否使用目标方向（指向当前目标） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "方向配置",
            meta = (DisplayName = "使用目标方向"))
  bool bUseTargetDirection = true;

  /** 生成Actor的数量 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "多生成配置",
            meta = (DisplayName = "生成数量", ClampMin = "1", ClampMax = "20"))
  int32 SpawnCount = 1;

  /** 分布角度（所有生成点在角色前方该角度范围内均匀分布） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "多生成配置",
            meta = (DisplayName = "分布角度", ClampMin = "0.0", ClampMax = "360.0",
                    EditCondition = "SpawnCount > 1"))
  float SpreadAngle = 45.0f;

  /** GAS - 触发的 Gameplay Ability 类（可选） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS配置",
            meta = (DisplayName = "触发的GA"))
  TSubclassOf<UGameplayAbility> TriggerAbilityClass;

  /** GAS - 应用的 Gameplay Effect 类（可选） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS配置",
            meta = (DisplayName = "应用的GE"))
  TSubclassOf<UGameplayEffect> ApplyEffectClass;

  /** GAS - 触发事件的 Gameplay Tag（可选） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS配置",
            meta = (DisplayName = "事件Tag"))
  FGameplayTag EventTag;

  /** 是否启用调试绘制 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试",
            meta = (DisplayName = "启用调试绘制"))
  bool bEnableDebugDraw = false;

  /** 调试绘制持续时间 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试",
            meta = (DisplayName = "调试绘制时间",
                    EditCondition = "bEnableDebugDraw", ClampMin = "0.0"))
  float DebugDrawDuration = 2.0f;

  FXBSkillSpawnConfig() {}
};

/**
 * @brief 通用技能生成动画通知
 * @note 在动画指定帧生成技能Actor，自动传递伤害和方向信息
 */
UCLASS(meta = (DisplayName = "XB生成技能Actor"))
class XIAOBINDATIANXIA_API UAN_XBSpawnSkillActor : public UAnimNotify {
  GENERATED_BODY()

public:
  UAN_XBSpawnSkillActor();

  /** 技能生成配置 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "技能配置",
            meta = (DisplayName = "生成配置"))
  FXBSkillSpawnConfig SpawnConfig;

  virtual void Notify(USkeletalMeshComponent *MeshComp,
                      UAnimSequenceBase *Animation,
                      const FAnimNotifyEventReference &EventReference) override;

  virtual FString GetNotifyName_Implementation() const override {
    return TEXT("XB生成技能Actor");
  }

protected:
  /**
   * @brief 计算生成位置和旋转
   * @param MeshComp 骨骼网格组件
   * @param OutLocation 输出位置
   * @param OutRotation 输出旋转
   * @return 是否成功计算
   */
  bool CalculateSpawnTransform(USkeletalMeshComponent *MeshComp,
                               FVector &OutLocation,
                               FRotator &OutRotation) const;

  /**
   * @brief 获取施法者伤害值
   * @param OwnerActor 施法者
   * @return 伤害值（已应用倍率）
   */
  float GetDamage(AActor *OwnerActor) const;

  /**
   * @brief 获取施法者当前目标
   * @param OwnerActor 施法者
   * @return 目标Actor（可能为空）
   */
  AActor *GetCurrentTarget(AActor *OwnerActor) const;

  /**
   * @brief 计算生成方向
   * @param OwnerActor 施法者
   * @param SpawnLocation 生成位置
   * @return 方向向量
   */
  FVector CalculateSpawnDirection(AActor *OwnerActor,
                                  const FVector &SpawnLocation) const;
};
