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

  // 计算基础生成位置和旋转
  FVector BaseSpawnLocation;
  FRotator BaseSpawnRotation;
  if (!CalculateSpawnTransform(MeshComp, BaseSpawnLocation, BaseSpawnRotation)) {
    UE_LOG(LogXBCombat, Warning,
           TEXT("AN_XBSpawnSkillActor: 计算生成位置失败"));
    return;
  }

  // 配置生成参数
  FActorSpawnParameters SpawnParams;
  SpawnParams.Owner = OwnerActor;
  SpawnParams.Instigator = Cast<APawn>(OwnerActor);
  SpawnParams.SpawnCollisionHandlingOverride =
      ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

  // 获取施法者的缩放（用于应用到生成的Actor）
  float OwnerScaleFactor = 1.0f;
  if (AXBCharacterBase *Character = Cast<AXBCharacterBase>(OwnerActor)) {
    OwnerScaleFactor = Character->GetActorScale3D().X;
  }

  // 计算伤害值
  float Damage = GetDamage(OwnerActor);

  // 获取当前目标
  AActor *Target = GetCurrentTarget(OwnerActor);

  // 获取生成数量（至少为1）
  const int32 ActualSpawnCount = FMath::Max(1, SpawnConfig.SpawnCount);
  
  // 计算角度分布参数
  // 如果只生成1个，则在正前方；多个时在Yaw角度范围内均匀分布
  const float HalfSpreadAngle = SpawnConfig.SpreadAngle * 0.5f;
  
  // 获取角色前方向量和右方向量（用于扇形位置计算）
  const FVector OwnerForward = OwnerActor->GetActorForwardVector();
  const FVector OwnerRight = OwnerActor->GetActorRightVector();
  const FVector OwnerLocation = OwnerActor->GetActorLocation();
  
  // 循环生成多个 Actor
  for (int32 i = 0; i < ActualSpawnCount; ++i) {
    // 初始化为基础位置和旋转
    FVector FinalSpawnLocation = BaseSpawnLocation;
    FRotator FinalSpawnRotation = BaseSpawnRotation;
    FVector SpawnDirection = BaseSpawnRotation.Vector();
    
    // 如果生成数量大于1，计算扇形分布的位置和旋转
    if (ActualSpawnCount > 1) {
      // 计算当前索引对应的角度偏移
      // 角度从 -HalfSpreadAngle 到 +HalfSpreadAngle 均匀分布
      float AngleRatio = static_cast<float>(i) / static_cast<float>(ActualSpawnCount - 1);
      const float CurrentAngle = -HalfSpreadAngle + (SpawnConfig.SpreadAngle * AngleRatio);
      
      // 将角度转换为弧度
      const float AngleRad = FMath::DegreesToRadians(CurrentAngle);
      
      // 计算偏移方向（基于角色前方和右方的2D旋转）
      // 新方向 = Forward * cos(angle) + Right * sin(angle)
      const FVector OffsetDirection = (OwnerForward * FMath::Cos(AngleRad) + OwnerRight * FMath::Sin(AngleRad)).GetSafeNormal();
      
      // 计算扇形位置：从角色位置沿偏移方向移动 SpreadRadius 距离
      const float ScaledRadius = SpawnConfig.SpreadRadius * OwnerScaleFactor;
      FinalSpawnLocation = OwnerLocation + OffsetDirection * ScaledRadius;
      
      // 保持Z轴高度（使用基础生成位置的高度）
      FinalSpawnLocation.Z = BaseSpawnLocation.Z;
      
      // 旋转朝向偏移方向
      FinalSpawnRotation = OffsetDirection.Rotation();
      
      // 发射方向与偏移方向一致
      SpawnDirection = OffsetDirection;
    }

    // 生成Actor
    AActor *SpawnedActor = World->SpawnActor<AActor>(
        SpawnConfig.ActorClass, FinalSpawnLocation, FinalSpawnRotation, SpawnParams);

    if (!SpawnedActor) {
      UE_LOG(LogXBCombat, Error, TEXT("AN_XBSpawnSkillActor: 生成 %s 失败 (索引=%d)"),
             *SpawnConfig.ActorClass->GetName(), i);
      continue;
    }

    // 如果需要附着到插槽（仅第一个Actor附着）
    if (i == 0 && SpawnConfig.bAttachToSocket &&
        SpawnConfig.SpawnMode == EXBSkillSpawnMode::Socket) {
      SpawnedActor->AttachToComponent(
          MeshComp, FAttachmentTransformRules::KeepWorldTransform,
          SpawnConfig.SocketName);
    }

    // 应用施法者缩放到生成的 Actor
    if (OwnerScaleFactor != 1.0f) {
      SpawnedActor->SetActorScale3D(FVector(OwnerScaleFactor));
      UE_LOG(LogXBCombat, Verbose,
             TEXT("AN_XBSpawnSkillActor: 生成的 %s 应用缩放 %.2f"),
             *SpawnedActor->GetName(), OwnerScaleFactor);
    }

    // 如果使用目标方向且只有单个生成，则重新计算发射方向
    if (ActualSpawnCount == 1 || SpawnConfig.bUseTargetDirection) {
      FVector TargetBasedDirection = CalculateSpawnDirection(OwnerActor, FinalSpawnLocation);
      if (ActualSpawnCount == 1) {
        SpawnDirection = TargetBasedDirection;
      }
      // 多生成时如果启用目标方向，也使用目标方向
      else if (SpawnConfig.bUseTargetDirection && Target) {
        SpawnDirection = TargetBasedDirection;
      }
    }

    // 如果Actor实现了技能接口，则初始化
    if (IXBSkillActorInterface *SkillInterface =
            Cast<IXBSkillActorInterface>(SpawnedActor)) {
      SkillInterface->InitializeSkillActor(OwnerActor, Damage, SpawnDirection,
                                           Target);
      UE_LOG(LogXBCombat, Log,
             TEXT("AN_XBSpawnSkillActor: 生成 %s (索引=%d)，伤害=%.1f，方向=%s"),
             *SpawnedActor->GetName(), i, Damage, *SpawnDirection.ToString());
    }
    // 添加对 AXBProjectile 的直接初始化支持
    else if (AXBProjectile *Projectile = Cast<AXBProjectile>(SpawnedActor)) {
      // 先禁用碰撞，避免在初始化前触发
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

      // 初始化完成后重新启用碰撞
      Projectile->SetActorEnableCollision(true);

      UE_LOG(
          LogXBCombat, Log,
          TEXT("AN_XBSpawnSkillActor: 生成投射物 %s (索引=%d)，来源=%s，伤害=%.1f，方向=%s"),
          *SpawnedActor->GetName(), i,
          OwnerActor ? *OwnerActor->GetName() : TEXT("无"), Damage,
          *SpawnDirection.ToString());
    } else {
      UE_LOG(LogXBCombat, Warning,
             TEXT("AN_XBSpawnSkillActor: 生成 %s，但不支持该类型的自动初始化"),
             *SpawnedActor->GetName());
    }

    // 调试绘制 - 位置和旋转同步显示
    if (SpawnConfig.bEnableDebugDraw) {
      // 使用不同颜色区分不同索引（红到绿渐变）
      const FColor SphereColor = FColor::MakeRedToGreenColorFromScalar(
          static_cast<float>(i) / FMath::Max(1.0f, static_cast<float>(ActualSpawnCount - 1)));
      
      // 绘制生成位置球体
      DrawDebugSphere(World, FinalSpawnLocation, 20.0f, 12, SphereColor, false,
                      SpawnConfig.DebugDrawDuration);
      
      // 绘制发射方向箭头（与Actor旋转一致）
      DrawDebugDirectionalArrow(
          World, FinalSpawnLocation, FinalSpawnLocation + SpawnDirection * 150.0f, 25.0f,
          FColor::Cyan, false, SpawnConfig.DebugDrawDuration, 0, 3.0f);
      
      // 绘制从角色中心到生成点的连线（显示扇形结构）
      if (ActualSpawnCount > 1) {
        DrawDebugLine(World, OwnerLocation, FinalSpawnLocation, FColor::Yellow, 
                      false, SpawnConfig.DebugDrawDuration, 0, 1.5f);
      }
      
      // 显示索引和角度信息
      const float DisplayAngle = (ActualSpawnCount > 1) 
          ? (-HalfSpreadAngle + SpawnConfig.SpreadAngle * (static_cast<float>(i) / static_cast<float>(ActualSpawnCount - 1)))
          : 0.0f;
      DrawDebugString(World, FinalSpawnLocation + FVector(0, 0, 40.0f),
                      FString::Printf(TEXT("[%d] 角度:%.1f°"), i, DisplayAngle), nullptr,
                      FColor::White, SpawnConfig.DebugDrawDuration);
    }
  }
  
  UE_LOG(LogXBCombat, Log,
         TEXT("AN_XBSpawnSkillActor: 共生成 %d 个 %s，分布角度=%.1f°"),
         ActualSpawnCount, *SpawnConfig.ActorClass->GetName(), SpawnConfig.SpreadAngle);
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
