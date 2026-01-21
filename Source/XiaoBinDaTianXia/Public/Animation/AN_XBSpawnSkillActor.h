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
  TargetBased UMETA(DisplayName = "目标位置"),
  /** 指定范围内生成（在指定区域内延迟生成多个投射物，如箭雨） */
  DesignatedArea UMETA(DisplayName = "指定范围")
};

/**
 * @brief 指定范围目标位置来源
 */
UENUM(BlueprintType)
enum class EXBDesignatedAreaTarget : uint8 {
  /** 以当前锁定的敌方目标位置为中心（无目标时回退到前方偏移） */
  EnemyTarget UMETA(DisplayName = "敌方目标"),
  /** 以施法者前方偏移位置为中心 */
  ForwardOffset UMETA(DisplayName = "前方偏移"),
  /** 以施法者自身位置为中心 */
  Self UMETA(DisplayName = "自身位置")
};

/**
 * @brief 指定范围形状
 */
UENUM(BlueprintType)
enum class EXBDesignatedAreaShape : uint8 {
  /** 圆形区域 */
  Circle UMETA(DisplayName = "圆形"),
  /** 正方形区域 */
  Square UMETA(DisplayName = "正方形")
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
            meta = (DisplayName = "生成数量", ClampMin = "1", ClampMax = "999"))
  int32 SpawnCount = 1;

  /** 分布角度（所有生成点在角色前方该角度范围内均匀分布） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "多生成配置",
            meta = (DisplayName = "分布角度", ClampMin = "0.0", ClampMax = "360.0",
                    EditCondition = "SpawnCount > 1"))
  float SpreadAngle = 45.0f;

  /** 生成距离（扇形生成时，距离角色中心的距离） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "多生成配置",
            meta = (DisplayName = "生成距离", ClampMin = "0.0",
                    EditCondition = "SpawnCount > 1"))
  float SpreadRadius = 100.0f;

  // ========== 指定范围配置 ==========

  /** 指定范围目标位置来源 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "指定范围配置",
            meta = (DisplayName = "区域中心",
                    EditCondition = "SpawnMode == EXBSkillSpawnMode::DesignatedArea",
                    EditConditionHides))
  EXBDesignatedAreaTarget DesignatedAreaTarget = EXBDesignatedAreaTarget::EnemyTarget;

  /** 指定范围形状 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "指定范围配置",
            meta = (DisplayName = "区域形状",
                    EditCondition = "SpawnMode == EXBSkillSpawnMode::DesignatedArea",
                    EditConditionHides))
  EXBDesignatedAreaShape DesignatedAreaShape = EXBDesignatedAreaShape::Circle;

  /** 区域半径（圆形时为半径，正方形时为边长的一半） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "指定范围配置",
            meta = (DisplayName = "区域大小", ClampMin = "50.0", ClampMax = "1000.0",
                    EditCondition = "SpawnMode == EXBSkillSpawnMode::DesignatedArea",
                    EditConditionHides))
  float AreaRadius = 300.0f;

  /** 生成高度（距离地面的高度，0表示不使用高度偏移） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "指定范围配置",
            meta = (DisplayName = "生成高度", ClampMin = "0.0", ClampMax = "2000.0",
                    EditCondition = "SpawnMode == EXBSkillSpawnMode::DesignatedArea",
                    EditConditionHides))
  float SpawnHeight = 500.0f;

  /** 生成间隔（秒），控制延迟生成的速度 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "指定范围配置",
            meta = (DisplayName = "生成间隔", ClampMin = "0.01", ClampMax = "0.5",
                    EditCondition = "SpawnMode == EXBSkillSpawnMode::DesignatedArea",
                    EditConditionHides))
  float SpawnInterval = 0.02f;

  /** 投射物俯仰角范围（负值表示向下，0表示水平） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "指定范围配置",
            meta = (DisplayName = "俯仰角范围",
                    EditCondition = "SpawnMode == EXBSkillSpawnMode::DesignatedArea",
                    EditConditionHides))
  FVector2D ArrowPitchRange = FVector2D(-85.0f, -95.0f);

  /** 前方偏移距离（当区域中心为前方偏移或自身位置时使用） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "指定范围配置",
            meta = (DisplayName = "前方偏移距离", ClampMin = "0.0",
                    EditCondition = "SpawnMode == EXBSkillSpawnMode::DesignatedArea && DesignatedAreaTarget != EXBDesignatedAreaTarget::EnemyTarget",
                    EditConditionHides))
  float AreaForwardDistance = 500.0f;

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

  /**
   * @brief 计算指定范围区域中心位置
   * @param OwnerActor 施法者
   * @return 区域中心位置
   */
  FVector CalculateDesignatedAreaCenter(AActor *OwnerActor) const;

  /**
   * @brief 启动指定范围延迟生成
   * @param World 世界对象
   * @param OwnerActor 施法者
   * @param AreaCenter 区域中心
   * @param Damage 伤害值
   * @param Target 目标Actor
   */
  void StartDesignatedAreaSpawn(UWorld *World, AActor *OwnerActor,
                                const FVector &AreaCenter, float Damage,
                                AActor *Target) const;

  /**
   * @brief 在指定范围内生成单个投射物
   * @param World 世界对象
   * @param OwnerActor 施法者
   * @param AreaCenter 区域中心
   * @param Damage 伤害值
   * @param Target 目标Actor
   * @param Index 当前索引（用于调试）
   */
  void SpawnDesignatedAreaProjectile(UWorld *World, AActor *OwnerActor,
                                     const FVector &AreaCenter, float Damage,
                                     AActor *Target, int32 Index) const;
};
