/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Config/XBLeaderSpawnConfigData.cpp

/**
 * @file XBLeaderSpawnConfigData.cpp
 * @brief 主将放置配置数据结构实现
 */

#include "Config/XBLeaderSpawnConfigData.h"
#include "Save/XBSaveGame.h"

FXBGameConfigData FXBLeaderSpawnConfigData::ToGameConfigData() const {
  FXBGameConfigData GameConfig;

  // 主将配置
  GameConfig.LeaderConfigRowName = LeaderConfigRowName;
  GameConfig.LeaderDisplayName = LeaderDisplayName;
  GameConfig.LeaderInitialScale = LeaderInitialScale;
  GameConfig.LeaderHealthMultiplier = LeaderHealthMultiplier;
  GameConfig.LeaderDamageMultiplier = LeaderDamageMultiplier;
  GameConfig.LeaderMoveSpeed = LeaderMoveSpeed;
  GameConfig.LeaderMaxScale = LeaderMaxScale;

  // 士兵配置
  GameConfig.InitialSoldierRowName = InitialSoldierRowName;
  GameConfig.InitialSoldierCount = InitialSoldierCount;
  GameConfig.SoldierHealthMultiplier = SoldierHealthMultiplier;
  GameConfig.SoldierDamageMultiplier = SoldierDamageMultiplier;
  GameConfig.SoldierInitialScale = SoldierInitialScale;
  GameConfig.MagnetFieldRadius = MagnetFieldRadius;

  return GameConfig;
}

void FXBLeaderSpawnConfigData::FromGameConfigData(
    const FXBGameConfigData &GameConfig) {
  // 主将配置
  LeaderConfigRowName = GameConfig.LeaderConfigRowName;
  LeaderDisplayName = GameConfig.LeaderDisplayName;
  LeaderInitialScale = GameConfig.LeaderInitialScale;
  LeaderHealthMultiplier = GameConfig.LeaderHealthMultiplier;
  LeaderDamageMultiplier = GameConfig.LeaderDamageMultiplier;
  LeaderMoveSpeed = GameConfig.LeaderMoveSpeed;
  LeaderMaxScale = GameConfig.LeaderMaxScale;

  // 士兵配置
  InitialSoldierRowName = GameConfig.InitialSoldierRowName;
  InitialSoldierCount = GameConfig.InitialSoldierCount;
  SoldierHealthMultiplier = GameConfig.SoldierHealthMultiplier;
  SoldierDamageMultiplier = GameConfig.SoldierDamageMultiplier;
  SoldierInitialScale = GameConfig.SoldierInitialScale;
  MagnetFieldRadius = GameConfig.MagnetFieldRadius;
}
