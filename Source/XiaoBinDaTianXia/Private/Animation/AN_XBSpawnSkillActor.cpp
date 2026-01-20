/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Animation/AN_XBSpawnSkillActor.cpp

/**
 * @file AN_XBSpawnSkillActor.cpp
 * @brief 通用技能生成动画通知实现
 *
 * @note ✨ 新增文件
 */

#include "Animation/AN_XBSpawnSkillActor.h"
#include "Animation/XBSkillActorInterface.h"
#include "Character/Components/XBCombatComponent.h"
#include "Character/XBCharacterBase.h"
#include "Combat/XBProjectile.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Utils/XBLogCategories.h"

UAN_XBSpawnSkillActor::UAN_XBSpawnSkillActor() {}

/**
 * @brief 动画通知触发时执行
 * @param MeshComp 骨骼网格组件
 * @param Animation 动画资产
 * @param EventReference 事件引用
 * 功能说明: 在指定位置生成技能Actor并初始化
 * 详细流程: 校验配置 -> 计算位置 -> 生成Actor -> 初始化接口
 */
void UAN_XBSpawnSkillActor::Notify(
    USkeletalMeshComponent *MeshComp, UAnimSequenceBase *Animation,
    const FAnimNotifyEventReference &EventReference) {
  Super::Notify(MeshComp, Animation, EventReference);

  // 校验网格组件
  if (!MeshComp) {
    UE_LOG(LogXBCombat, Warning, TEXT("AN_XBSpawnSkillActor: MeshComp 为空"));
    return;
  }

  // 校验Actor类
  if (!SpawnConfig.ActorClass) {
    UE_LOG(LogXBCombat, Warning,
           TEXT("AN_XBSpawnSkillActor: 未配置 ActorClass"));
    return;
  }

  // 获取施法者
  AActor *OwnerActor = MeshComp->GetOwner();
  if (!OwnerActor) {
    UE_LOG(LogXBCombat, Warning, TEXT("AN_XBSpawnSkillActor: 无法获取施法者"));
    return;
  }

  // 获取世界
  UWorld *World = MeshComp->GetWorld();
  if (!World) {
    return;
  }

  // 计算生成位置和旋转
  FVector SpawnLocation;
  FRotator SpawnRotation;
  if (!CalculateSpawnTransform(MeshComp, SpawnLocation, SpawnRotation)) {
    UE_LOG(LogXBCombat, Warning,
           TEXT("AN_XBSpawnSkillActor: 计算生成位置失败"));
    return;
  }

  // 生成Actor
  FActorSpawnParameters SpawnParams;
  SpawnParams.Owner = OwnerActor;
  SpawnParams.Instigator = Cast<APawn>(OwnerActor);
  SpawnParams.SpawnCollisionHandlingOverride =
      ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

  AActor *SpawnedActor = World->SpawnActor<AActor>(
      SpawnConfig.ActorClass, SpawnLocation, SpawnRotation, SpawnParams);

  if (!SpawnedActor) {
    UE_LOG(LogXBCombat, Error, TEXT("AN_XBSpawnSkillActor: 生成 %s 失败"),
           *SpawnConfig.ActorClass->GetName());
    return;
  }

  // 如果需要附着到插槽
  if (SpawnConfig.bAttachToSocket &&
      SpawnConfig.SpawnMode == EXBSkillSpawnMode::Socket) {
    SpawnedActor->AttachToComponent(
        MeshComp, FAttachmentTransformRules::KeepWorldTransform,
        SpawnConfig.SocketName);
  }

  // 🔧 修复 - 如果施法者是 XBCharacterBase，应用其缩放到生成的 Actor
  if (AXBCharacterBase *Character = Cast<AXBCharacterBase>(OwnerActor)) {
    const FVector OwnerScale = Character->GetActorScale3D();
    SpawnedActor->SetActorScale3D(OwnerScale);

    UE_LOG(LogXBCombat, Verbose,
           TEXT("AN_XBSpawnSkillActor: 生成的 %s 应用缩放 %.2f"),
           *SpawnedActor->GetName(), OwnerScale.X);
  }

  // 计算伤害值
  float Damage = GetDamage(OwnerActor);

  // 计算生成方向
  FVector SpawnDirection = CalculateSpawnDirection(OwnerActor, SpawnLocation);

  // 获取当前目标
  AActor *Target = GetCurrentTarget(OwnerActor);

  // 如果Actor实现了技能接口，则初始化
  if (IXBSkillActorInterface *SkillInterface =
          Cast<IXBSkillActorInterface>(SpawnedActor)) {
    SkillInterface->InitializeSkillActor(OwnerActor, Damage, SpawnDirection,
                                         Target);
    UE_LOG(LogXBCombat, Log,
           TEXT("AN_XBSpawnSkillActor: 生成 %s，伤害=%.1f，方向=%s"),
           *SpawnedActor->GetName(), Damage, *SpawnDirection.ToString());
  }
  // 🔧 修复 - 添加对 AXBProjectile 的直接初始化支持
  else if (AXBProjectile *Projectile = Cast<AXBProjectile>(SpawnedActor)) {
    // 🔧 修复时序问题 - 先禁用碰撞，避免在初始化前触发
    Projectile->SetActorEnableCollision(false);

    // 计算目标位置用于抛射轨迹
    FVector TargetLocation = FVector::ZeroVector;
    if (Target) {
      TargetLocation = Target->GetActorLocation();
    }

    // 直接调用投射物的初始化方法
    Projectile->InitializeProjectileWithTarget(
        OwnerActor,              // 来源Actor（士兵或主将）
        Damage,                  // 伤害值
        SpawnDirection,          // 发射方向
        Projectile->LinearSpeed, // 使用投射物自身配置的速度
        Projectile->bUseArc,     // 使用投射物自身配置的抛射模式
        TargetLocation           // 目标位置
    );

    // 🔧 修复时序问题 - 初始化完成后重新启用碰撞
    Projectile->SetActorEnableCollision(true);

    UE_LOG(
        LogXBCombat, Log,
        TEXT(
            "AN_XBSpawnSkillActor: 生成投射物 %s，来源=%s，伤害=%.1f，方向=%s"),
        *SpawnedActor->GetName(),
        OwnerActor ? *OwnerActor->GetName() : TEXT("无"), Damage,
        *SpawnDirection.ToString());
  } else {
    UE_LOG(LogXBCombat, Warning,
           TEXT("AN_XBSpawnSkillActor: 生成 %s，但不支持该类型的自动初始化"),
           *SpawnedActor->GetName());
  }

  // 调试绘制
  if (SpawnConfig.bEnableDebugDraw) {
    DrawDebugSphere(World, SpawnLocation, 20.0f, 12, FColor::Yellow, false,
                    SpawnConfig.DebugDrawDuration);
    DrawDebugDirectionalArrow(
        World, SpawnLocation, SpawnLocation + SpawnDirection * 100.0f, 20.0f,
        FColor::Red, false, SpawnConfig.DebugDrawDuration, 0, 2.0f);
    DrawDebugString(World, SpawnLocation + FVector(0, 0, 30.0f),
                    FString::Printf(TEXT("伤害: %.1f"), Damage), nullptr,
                    FColor::White, SpawnConfig.DebugDrawDuration);
  }
}

/**
 * @brief 计算生成位置和旋转
 */
bool UAN_XBSpawnSkillActor::CalculateSpawnTransform(
    USkeletalMeshComponent *MeshComp, FVector &OutLocation,
    FRotator &OutRotation) const {
  AActor *OwnerActor = MeshComp->GetOwner();
  if (!OwnerActor) {
    return false;
  }

  FVector BaseLocation = OwnerActor->GetActorLocation();
  FRotator BaseRotation = OwnerActor->GetActorRotation();

  switch (SpawnConfig.SpawnMode) {
  case EXBSkillSpawnMode::Socket: {
    // 从插槽获取位置
    if (MeshComp->DoesSocketExist(SpawnConfig.SocketName)) {
      FTransform SocketTransform = MeshComp->GetSocketTransform(
          SpawnConfig.SocketName, ERelativeTransformSpace::RTS_World);
      BaseLocation = SocketTransform.GetLocation();
      if (SpawnConfig.bInheritOwnerRotation) {
        BaseRotation = OwnerActor->GetActorRotation();
      } else {
        BaseRotation = SocketTransform.Rotator();
      }
    } else {
      UE_LOG(LogXBCombat, Warning, TEXT("AN_XBSpawnSkillActor: 插槽 %s 不存在"),
             *SpawnConfig.SocketName.ToString());
    }
  } break;

  case EXBSkillSpawnMode::WorldOffset: {
    // 世界坐标偏移（不随角色旋转）
    BaseLocation = OwnerActor->GetActorLocation();
  } break;

  case EXBSkillSpawnMode::ForwardOffset: {
    // 相对于角色朝向的前方偏移
    BaseLocation = OwnerActor->GetActorLocation();
    // LocationOffset.X 作为前方距离
    FVector ForwardOffset =
        OwnerActor->GetActorForwardVector() * SpawnConfig.LocationOffset.X;
    ForwardOffset +=
        OwnerActor->GetActorRightVector() * SpawnConfig.LocationOffset.Y;
    ForwardOffset.Z += SpawnConfig.LocationOffset.Z;
    OutLocation = BaseLocation + ForwardOffset;
    OutRotation = SpawnConfig.bInheritOwnerRotation
                      ? BaseRotation + SpawnConfig.RotationOffset
                      : SpawnConfig.RotationOffset;
    return true;
  }

  case EXBSkillSpawnMode::TargetBased: {
    // 从目标位置生成
    AActor *Target = GetCurrentTarget(OwnerActor);
    if (Target) {
      BaseLocation = Target->GetActorLocation();
      // 方向从施法者指向目标
      FVector DirectionToTarget =
          (Target->GetActorLocation() - OwnerActor->GetActorLocation())
              .GetSafeNormal();
      BaseRotation = DirectionToTarget.Rotation();
    } else {
      UE_LOG(LogXBCombat, Warning,
             TEXT("AN_XBSpawnSkillActor: TargetBased "
                  "模式但无目标，使用施法者位置"));
      BaseLocation = OwnerActor->GetActorLocation();
    }
  }
  }

  // 🔧 修复 - 如果施法者是 XBCharacterBase，根据其缩放调整位置偏移
  FVector ScaledLocationOffset = SpawnConfig.LocationOffset;
  if (AXBCharacterBase *Character = Cast<AXBCharacterBase>(OwnerActor)) {
    const FVector OwnerScale = Character->GetActorScale3D();
    // 使用 X 轴缩放（假设均匀缩放）来调整位置偏移
    const float ScaleFactor = OwnerScale.X;
    ScaledLocationOffset *= ScaleFactor;

    UE_LOG(LogXBCombat, Verbose,
           TEXT("AN_XBSpawnSkillActor: 施法者 %s 缩放=%.2f，偏移已调整"),
           *OwnerActor->GetName(), ScaleFactor);
  }

  // 应用位置偏移
  OutLocation = BaseLocation + BaseRotation.RotateVector(ScaledLocationOffset);
  // 应用旋转偏移
  OutRotation = SpawnConfig.bInheritOwnerRotation
                    ? BaseRotation + SpawnConfig.RotationOffset
                    : SpawnConfig.RotationOffset;

  return true;
}

/**
 * @brief 获取施法者伤害值
 */
float UAN_XBSpawnSkillActor::GetDamage(AActor *OwnerActor) const {
  float BaseDamage = SpawnConfig.FixedDamage;

  if (SpawnConfig.bUseCurrentAttackDamage) {
    // 从将领战斗组件获取伤害
    if (AXBCharacterBase *Character = Cast<AXBCharacterBase>(OwnerActor)) {
      if (UXBCombatComponent *CombatComp = Character->GetCombatComponent()) {
        BaseDamage = CombatComp->GetCurrentAttackFinalDamage();
        if (BaseDamage <= 0.0f) {
          // 回退到当前攻击伤害
          BaseDamage = CombatComp->GetCurrentAttackDamage();
        }
      }
    }
    // 从士兵获取伤害
    else if (AXBSoldierCharacter *Soldier =
                 Cast<AXBSoldierCharacter>(OwnerActor)) {
      BaseDamage = Soldier->GetBaseDamage();
    }
  }

  // 应用伤害倍率
  return BaseDamage * SpawnConfig.DamageMultiplier;
}

/**
 * @brief 获取施法者当前目标
 */
AActor *UAN_XBSpawnSkillActor::GetCurrentTarget(AActor *OwnerActor) const {
  // 从将领获取锁定的敌方目标
  if (AXBCharacterBase *Character = Cast<AXBCharacterBase>(OwnerActor)) {
    // 优先返回锁定的敌方主将
    if (AXBCharacterBase *EnemyLeader =
            Character->GetLastAttackedEnemyLeader()) {
      return EnemyLeader;
    }
  }
  // 从士兵获取目标
  else if (AXBSoldierCharacter *Soldier =
               Cast<AXBSoldierCharacter>(OwnerActor)) {
    return Soldier->CurrentAttackTarget.Get();
  }

  return nullptr;
}

/**
 * @brief 计算生成方向
 */
FVector UAN_XBSpawnSkillActor::CalculateSpawnDirection(
    AActor *OwnerActor, const FVector &SpawnLocation) const {
  if (!OwnerActor) {
    return FVector::ForwardVector;
  }

  // 如果使用目标方向且有目标
  if (SpawnConfig.bUseTargetDirection) {
    AActor *Target = GetCurrentTarget(OwnerActor);
    if (Target) {
      FVector Direction =
          (Target->GetActorLocation() - SpawnLocation).GetSafeNormal();
      if (!Direction.IsNearlyZero()) {
        return Direction;
      }
    }
  }

  // 默认使用施法者朝向
  return OwnerActor->GetActorForwardVector();
}
