/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Config/XBLeaderSpawnConfigData.h

/**
 * @file XBLeaderSpawnConfigData.h
 * @brief 主将放置配置数据结构
 *
 * @note ✨ 新增文件 - 用于配置阶段放置主将时的属性配置
 * @note 🔧 修改 - 重构为直接包含 FXBGameConfigData，避免重复定义字段
 */

#pragma once

#include "Army/XBSoldierTypes.h"
#include "CoreMinimal.h"
#include "Save/XBSaveGame.h"
#include "XBLeaderSpawnConfigData.generated.h"

/**
 * @brief 主将放置配置数据
 * @note 用于配置阶段放置主将时的属性配置
 *       直接包含 FXBGameConfigData，共享所有主将/士兵配置字段
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBLeaderSpawnConfigData {
  GENERATED_BODY()

  /** 阵营（配置阶段专用，区分敌我） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "主将配置",
            meta = (DisplayName = "阵营"))
  EXBFaction Faction = EXBFaction::Enemy;

  /** 游戏配置数据（复用全部主将/士兵配置项） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置数据",
            meta = (DisplayName = "游戏配置"))
  FXBGameConfigData GameConfig;

  // ==================== 便捷访问方法 ====================

  /** 获取主将配置行名 */
  FName GetLeaderConfigRowName() const {
    return GameConfig.LeaderConfigRowName;
  }

  /** 设置主将配置行名 */
  void SetLeaderConfigRowName(FName InName) {
    GameConfig.LeaderConfigRowName = InName;
  }

  /** 获取初始士兵数 */
  int32 GetInitialSoldierCount() const {
    return GameConfig.InitialSoldierCount;
  }

  /** 设置初始士兵数 */
  void SetInitialSoldierCount(int32 InCount) {
    GameConfig.InitialSoldierCount = InCount;
  }
};
