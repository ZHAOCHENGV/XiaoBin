/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Core/XBBlueprintFunctionLibrary.cpp

/**
 * @file XBBlueprintFunctionLibrary.cpp
 * @brief 项目通用蓝图函数库实现
 *
 * @note ✨ 新增文件
 */

#include "Utils/XBBlueprintFunctionLibrary.h"
#include "Character/XBCharacterBase.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Game/XBGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Utils/XBLogCategories.h"

// ==================== 阵营关系判断实现 ====================

bool UXBBlueprintFunctionLibrary::AreFactionsHostile(EXBFaction FactionA,
                                                     EXBFaction FactionB) {
  // ?? 修改 - 各自为战与任何阵营敌对（包括自身）
  if (FactionA == EXBFaction::FreeForAll ||
      FactionB == EXBFaction::FreeForAll) {
    return true;
  }

  // 相同阵营不敌对
  if (FactionA == FactionB) {
    return false;
  }

  // 🔧 修改 - 移除中立阵营保护，允许所有阵营攻击中立
  // 中立阵营可被所有阵营攻击（单向敌对）
  // 中立阵营本身不主动搜索，只通过受击反击

  // 玩家和友军互不敌对
  if ((FactionA == EXBFaction::Player && FactionB == EXBFaction::Ally) ||
      (FactionA == EXBFaction::Ally && FactionB == EXBFaction::Player)) {
    return false;
  }

  // 其他所有情况都是敌对
  // 包括：玩家 vs 敌人、玩家 vs 中立、友军 vs 敌人、友军 vs 中立、中立 vs
  // 任何阵营
  return true;
}

bool UXBBlueprintFunctionLibrary::AreFactionsFriendly(EXBFaction FactionA,
                                                      EXBFaction FactionB) {
  // ?? 修改 - 各自为战不与任何阵营友好（包括自身）
  if (FactionA == EXBFaction::FreeForAll ||
      FactionB == EXBFaction::FreeForAll) {
    return false;
  }

  // 相同阵营为友好
  if (FactionA == FactionB) {
    return true;
  }

  // 玩家和友军互为友好
  if ((FactionA == EXBFaction::Player && FactionB == EXBFaction::Ally) ||
      (FactionA == EXBFaction::Ally && FactionB == EXBFaction::Player)) {
    return true;
  }

  return false;
}

bool UXBBlueprintFunctionLibrary::AreActorsHostile(const AActor *ActorA,
                                                   const AActor *ActorB) {
  if (!ActorA || !ActorB) {
    return false;
  }

  // 自身不敌对
  if (ActorA == ActorB) {
    return false;
  }

  // ✨ 核心修复：同一军队内部不敌对（防止内讧）
  if (AreSameArmy(ActorA, ActorB)) {
    return false;
  }

  EXBFaction FactionA = GetActorFaction(ActorA);
  EXBFaction FactionB = GetActorFaction(ActorB);

  return AreFactionsHostile(FactionA, FactionB);
}

/**
 * @brief  判断两个单位是否属于同一军队（同一主将麾下）
 */
bool UXBBlueprintFunctionLibrary::AreSameArmy(const AActor *ActorA,
                                              const AActor *ActorB) {
  // 空指针或自身视为同一军队
  if (!ActorA || !ActorB || ActorA == ActorB) {
    return true;
  }

  AXBCharacterBase *LeaderA = nullptr;
  AXBCharacterBase *LeaderB = nullptr;

  // 获取 A 的主将
  if (const AXBSoldierCharacter *SoldierA = Cast<AXBSoldierCharacter>(ActorA)) {
    LeaderA = SoldierA->GetLeaderCharacter();
  } else if (const AXBCharacterBase *CharA = Cast<AXBCharacterBase>(ActorA)) {
    LeaderA = const_cast<AXBCharacterBase *>(CharA);
  }

  // 获取 B 的主将
  if (const AXBSoldierCharacter *SoldierB = Cast<AXBSoldierCharacter>(ActorB)) {
    LeaderB = SoldierB->GetLeaderCharacter();

    // 🔧 修复 - 如果士兵没有主将（未招募），检查是否与来源主将同阵营
    // 这处理主将攻击未设置主将引用的自己士兵的情况
    if (!LeaderB && LeaderA) {
      // 如果来源是主将，目标士兵没有主将，但同阵营，视为同军
      if (SoldierB->GetFaction() == LeaderA->GetFaction() &&
          SoldierB->GetFaction() != EXBFaction::Neutral &&
          SoldierB->GetFaction() != EXBFaction::FreeForAll) {
        return true;
      }
    }
  } else if (const AXBCharacterBase *CharB = Cast<AXBCharacterBase>(ActorB)) {
    LeaderB = const_cast<AXBCharacterBase *>(CharB);
  }

  // 同一主将视为同一军队
  return LeaderA && LeaderB && LeaderA == LeaderB;
}

EXBFaction UXBBlueprintFunctionLibrary::GetActorFaction(const AActor *Actor) {
  if (!Actor) {
    return EXBFaction::Neutral;
  }

  // 优先检查角色基类
  if (const AXBCharacterBase *CharBase = Cast<AXBCharacterBase>(Actor)) {
    return CharBase->GetFaction();
  }

  // 检查士兵类
  if (const AXBSoldierCharacter *Soldier = Cast<AXBSoldierCharacter>(Actor)) {
    return Soldier->GetFaction();
  }

  // 默认中立
  return EXBFaction::Neutral;
}

bool UXBBlueprintFunctionLibrary::IsActorAlive(const AActor *Actor) {
  if (!Actor || !IsValid(Actor)) {
    return false;
  }

  // 检查角色基类
  if (const AXBCharacterBase *CharBase = Cast<AXBCharacterBase>(Actor)) {
    return !CharBase->IsDead();
  }

  // 检查士兵
  if (const AXBSoldierCharacter *Soldier = Cast<AXBSoldierCharacter>(Actor)) {
    return Soldier->GetSoldierState() != EXBSoldierState::Dead;
  }

  // 其他Actor默认存活
  return true;
}

/**
 * @brief 判断目标是否为友军（统一友军判定）
 */
bool UXBBlueprintFunctionLibrary::IsFriendlyTarget(const AActor *SourceActor,
                                                   const AActor *TargetActor) {
  // 空指针检查
  if (!SourceActor || !TargetActor) {
    return false;
  }

  // 1. 自己永远是友军
  if (SourceActor == TargetActor) {
    return true;
  }

  // 2. 检查是否为未招募的士兵（无敌状态）
  // 🔧 修复 - 未招募或未初始化的士兵都视为友军（投射物穿透）
  if (const AXBSoldierCharacter *TargetSoldier =
          Cast<AXBSoldierCharacter>(TargetActor)) {
    // 未招募的士兵无敌（包括休眠、掉落、待机等状态）
    if (!TargetSoldier->IsRecruited()) {
      return true; // 未招募士兵视为友军（穿透）
    }
  }

  // 3. 检查是否同一军队（同主将）
  if (AreSameArmy(SourceActor, TargetActor)) {
    return true;
  }

  // 4. 检查是否同阵营（除Neutral外）
  EXBFaction SourceFaction = GetActorFaction(SourceActor);
  EXBFaction TargetFaction = GetActorFaction(TargetActor);

  // Neutral阵营不参与友军判定（各自为英）
  if (SourceFaction != EXBFaction::Neutral &&
      TargetFaction != EXBFaction::Neutral &&
      AreFactionsFriendly(SourceFaction, TargetFaction)) {
    return true;
  }

  // 其他情况为敌对
  return false;
}

FXBGameConfigData
UXBBlueprintFunctionLibrary::GetGameConfigData(const UObject *WorldContext) {
  if (!WorldContext) {
    UE_LOG(LogXBConfig, Warning, TEXT("获取游戏配置失败：WorldContext 无效"));
    return FXBGameConfigData();
  }

  // 🔧 修改 - 统一通过 GameInstance 读取配置，避免角色类重复逻辑
  if (const UWorld *World = WorldContext->GetWorld()) {
    if (const UXBGameInstance *GameInstance =
            World->GetGameInstance<UXBGameInstance>()) {
      return GameInstance->GetGameConfig();
    }
  }

  UE_LOG(LogXBConfig, Warning, TEXT("获取游戏配置失败：GameInstance 无效"));
  return FXBGameConfigData();
}

bool UXBBlueprintFunctionLibrary::SetGameConfigData(
    const UObject *WorldContext, const FXBGameConfigData &NewConfig,
    bool bSaveToDisk) {
  if (!WorldContext) {
    UE_LOG(LogXBConfig, Warning, TEXT("设置游戏配置失败：WorldContext 无效"));
    return false;
  }

  // 🔧 修改 - 统一通过 GameInstance 写入配置，便于后续敌人/系统复用
  if (UWorld *World = WorldContext->GetWorld()) {
    if (UXBGameInstance *GameInstance =
            World->GetGameInstance<UXBGameInstance>()) {
      GameInstance->SetGameConfig(NewConfig, bSaveToDisk);
      return true;
    }
  }

  UE_LOG(LogXBConfig, Warning, TEXT("设置游戏配置失败：GameInstance 无效"));
  return false;
}

// ==================== 范围检测实现 ====================

bool UXBBlueprintFunctionLibrary::PerformSphereOverlap(
    UWorld *World, const FVector &Origin, float Radius,
    TArray<FOverlapResult> &OutHits) {
  if (!World) {
    return false;
  }

  // 配置碰撞查询参数
  FCollisionQueryParams QueryParams;
  QueryParams.bTraceComplex = false;
  QueryParams.bReturnPhysicalMaterial = false;

  // 配置碰撞对象类型（只检测Pawn）
  FCollisionObjectQueryParams ObjectParams;
  ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

  // 执行球形重叠检测
  return World->OverlapMultiByObjectType(
      OutHits, Origin, FQuat::Identity, ObjectParams,
      FCollisionShape::MakeSphere(Radius), QueryParams);
}

bool UXBBlueprintFunctionLibrary::DetectEnemiesInRadius(
    const UObject *WorldContext, const FVector &Origin, float Radius,
    EXBFaction SourceFaction, bool bIgnoreDead, FXBDetectionResult &OutResult) {
  // 重置结果
  OutResult = FXBDetectionResult();

  if (!WorldContext) {
    return false;
  }

  UWorld *World = WorldContext->GetWorld();
  if (!World) {
    return false;
  }

  // 执行球形检测
  TArray<FOverlapResult> OverlapResults;
  if (!PerformSphereOverlap(World, Origin, Radius, OverlapResults)) {
    return false;
  }

  // 过滤结果
  for (const FOverlapResult &Result : OverlapResults) {
    AActor *HitActor = Result.GetActor();
    if (!HitActor || !IsValid(HitActor)) {
      continue;
    }

    // 检查是否存活
    if (bIgnoreDead && !IsActorAlive(HitActor)) {
      continue;
    }

    // 检查是否敌对
    EXBFaction TargetFaction = GetActorFaction(HitActor);
    if (!AreFactionsHostile(SourceFaction, TargetFaction)) {
      continue;
    }

    // 添加到结果
    OutResult.DetectedActors.Add(HitActor);

    // 计算距离，更新最近目标
    float Distance = FVector::Dist(Origin, HitActor->GetActorLocation());
    if (Distance < OutResult.NearestDistance) {
      OutResult.NearestDistance = Distance;
      OutResult.NearestActor = HitActor;
    }
  }

  OutResult.Count = OutResult.DetectedActors.Num();
  return OutResult.Count > 0;
}

bool UXBBlueprintFunctionLibrary::DetectAlliesInRadius(
    const UObject *WorldContext, const FVector &Origin, float Radius,
    EXBFaction SourceFaction, bool bIgnoreDead, FXBDetectionResult &OutResult) {
  // 重置结果
  OutResult = FXBDetectionResult();

  if (!WorldContext) {
    return false;
  }

  UWorld *World = WorldContext->GetWorld();
  if (!World) {
    return false;
  }

  // 执行球形检测
  TArray<FOverlapResult> OverlapResults;
  if (!PerformSphereOverlap(World, Origin, Radius, OverlapResults)) {
    return false;
  }

  // 过滤结果
  for (const FOverlapResult &Result : OverlapResults) {
    AActor *HitActor = Result.GetActor();
    if (!HitActor || !IsValid(HitActor)) {
      continue;
    }

    // 检查是否存活
    if (bIgnoreDead && !IsActorAlive(HitActor)) {
      continue;
    }

    // 检查是否友好
    EXBFaction TargetFaction = GetActorFaction(HitActor);
    if (!AreFactionsFriendly(SourceFaction, TargetFaction)) {
      continue;
    }

    // 添加到结果
    OutResult.DetectedActors.Add(HitActor);

    // 计算距离，更新最近目标
    float Distance = FVector::Dist(Origin, HitActor->GetActorLocation());
    if (Distance < OutResult.NearestDistance) {
      OutResult.NearestDistance = Distance;
      OutResult.NearestActor = HitActor;
    }
  }

  OutResult.Count = OutResult.DetectedActors.Num();
  return OutResult.Count > 0;
}

bool UXBBlueprintFunctionLibrary::DetectAllUnitsInRadius(
    const UObject *WorldContext, const FVector &Origin, float Radius,
    bool bIgnoreDead, FXBDetectionResult &OutResult) {
  // 重置结果
  OutResult = FXBDetectionResult();

  if (!WorldContext) {
    return false;
  }

  UWorld *World = WorldContext->GetWorld();
  if (!World) {
    return false;
  }

  // 执行球形检测
  TArray<FOverlapResult> OverlapResults;
  if (!PerformSphereOverlap(World, Origin, Radius, OverlapResults)) {
    return false;
  }

  // 过滤结果
  for (const FOverlapResult &Result : OverlapResults) {
    AActor *HitActor = Result.GetActor();
    if (!HitActor || !IsValid(HitActor)) {
      continue;
    }

    // 只接受战斗单位（角色或士兵）
    bool bIsValidUnit = Cast<AXBCharacterBase>(HitActor) != nullptr ||
                        Cast<AXBSoldierCharacter>(HitActor) != nullptr;
    if (!bIsValidUnit) {
      continue;
    }

    // 检查是否存活
    if (bIgnoreDead && !IsActorAlive(HitActor)) {
      continue;
    }

    // 添加到结果
    OutResult.DetectedActors.Add(HitActor);

    // 计算距离，更新最近目标
    float Distance = FVector::Dist(Origin, HitActor->GetActorLocation());
    if (Distance < OutResult.NearestDistance) {
      OutResult.NearestDistance = Distance;
      OutResult.NearestActor = HitActor;
    }
  }

  OutResult.Count = OutResult.DetectedActors.Num();
  return OutResult.Count > 0;
}

AActor *UXBBlueprintFunctionLibrary::FindNearestEnemy(
    const UObject *WorldContext, const FVector &Origin, float Radius,
    EXBFaction SourceFaction, bool bIgnoreDead) {
  FXBDetectionResult Result;
  if (DetectEnemiesInRadius(WorldContext, Origin, Radius, SourceFaction,
                            bIgnoreDead, Result)) {
    return Result.NearestActor;
  }
  return nullptr;
}

// ==================== 距离计算实现 ====================

float UXBBlueprintFunctionLibrary::GetDistance2D(const AActor *ActorA,
                                                 const AActor *ActorB) {
  if (!ActorA || !ActorB) {
    return MAX_FLT;
  }

  return FVector::Dist2D(ActorA->GetActorLocation(),
                         ActorB->GetActorLocation());
}

float UXBBlueprintFunctionLibrary::GetDistance3D(const AActor *ActorA,
                                                 const AActor *ActorB) {
  if (!ActorA || !ActorB) {
    return MAX_FLT;
  }

  return FVector::Dist(ActorA->GetActorLocation(), ActorB->GetActorLocation());
}
