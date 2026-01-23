/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Config/XBLeaderSpawnConfigData.h

/**
 * @file XBLeaderSpawnConfigData.h
 * @brief 主将放置配置数据结构
 *
 * @note ✨ 新增文件 - 用于配置阶段放置主将时的属性配置
 */

#pragma once

#include "Army/XBSoldierTypes.h"
#include "CoreMinimal.h"
#include "XBLeaderSpawnConfigData.generated.h"


struct FXBGameConfigData;

/**
 * @brief 主将放置配置数据
 * @note 用于配置阶段放置主将时的属性配置，复用 FXBGameConfigData
 * 的核心字段（排除地图选项）
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBLeaderSpawnConfigData {
  GENERATED_BODY()

  /** 主将配置行名 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "主将配置",
            meta = (DisplayName = "主将配置行"))
  FName LeaderConfigRowName;

  /** 主将自定义名称 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "主将配置",
            meta = (DisplayName = "主将名称"))
  FString LeaderDisplayName;

  /** 主将初始缩放 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "主将配置",
            meta = (DisplayName = "主将初始大小", ClampMin = "0.1",
                    ClampMax = "5.0"))
  float LeaderInitialScale = 1.0f;

  /** 主将生命值倍率 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "主将配置",
            meta = (DisplayName = "主将生命值倍率", ClampMin = "0.1"))
  float LeaderHealthMultiplier = 1.0f;

  /** 主将攻击倍率 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "主将配置",
            meta = (DisplayName = "主将攻击倍率", ClampMin = "0.1"))
  float LeaderDamageMultiplier = 1.0f;

  /** 主将移动速度 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "主将配置",
            meta = (DisplayName = "主将移动速度", ClampMin = "100.0"))
  float LeaderMoveSpeed = 600.0f;

  /** 主将最大体型大小 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "主将配置",
            meta = (DisplayName = "主将最大体型", ClampMin = "1.0"))
  float LeaderMaxScale = 3.0f;

  /** 阵营 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "主将配置",
            meta = (DisplayName = "阵营"))
  EXBFaction Faction = EXBFaction::Enemy;

  // ==================== 士兵配置 ====================

  /** 初始兵种行名 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "士兵配置",
            meta = (DisplayName = "初始兵种配置行"))
  FName InitialSoldierRowName;

  /** 初始带兵数 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "士兵配置",
            meta = (DisplayName = "初始带兵数", ClampMin = "0"))
  int32 InitialSoldierCount = 0;

  /** 士兵生命值倍率 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "士兵配置",
            meta = (DisplayName = "士兵生命值倍率", ClampMin = "0.1"))
  float SoldierHealthMultiplier = 1.0f;

  /** 士兵伤害倍率 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "士兵配置",
            meta = (DisplayName = "士兵伤害倍率", ClampMin = "0.1"))
  float SoldierDamageMultiplier = 1.0f;

  /** 士兵初始大小 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "士兵配置",
            meta = (DisplayName = "士兵初始大小", ClampMin = "0.1"))
  float SoldierInitialScale = 1.0f;

  /** 磁场范围 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "士兵配置",
            meta = (DisplayName = "磁场范围", ClampMin = "100.0"))
  float MagnetFieldRadius = 300.0f;

  // ==================== 转换方法 ====================

  /**
   * @brief 转换为 FXBGameConfigData
   * @return 游戏配置数据
   */
  FXBGameConfigData ToGameConfigData() const;

  /**
   * @brief 从 FXBGameConfigData 转换
   * @param GameConfig 游戏配置数据
   */
  void FromGameConfigData(const FXBGameConfigData &GameConfig);
};
