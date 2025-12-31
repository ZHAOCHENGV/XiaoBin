// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Army/XBSoldierTypes.h"
#include "XBSaveGame.generated.h"

/**
 * 场景物件存档数据
 */
USTRUCT(BlueprintType)
struct FXBSceneObjectData
{
    GENERATED_BODY()

    /** 物件类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "物件类型"))
    FString ObjectType;

    /** 位置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "位置"))
    FVector Location = FVector::ZeroVector;

    /** 旋转 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "旋转"))
    FRotator Rotation = FRotator::ZeroRotator;

    /** 缩放 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "缩放"))
    FVector Scale = FVector::OneVector;

    /** 额外数据（JSON格式） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "额外数据"))
    FString ExtraData;
};

/**
 * 游戏配置存档数据
 */
USTRUCT(BlueprintType)
struct FXBGameConfigData
{
    GENERATED_BODY()

    /** 玩家血量 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "玩家血量"))
    float PlayerHealth = 1000.0f;

    /** 玩家伤害倍率 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "玩家伤害倍率"))
    float PlayerDamageMultiplier = 1.0f;

    /** 主将配置行 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "主将配置行"))
    FName LeaderConfigRowName;

    /** 主将名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "主将名称"))
    FString LeaderDisplayName;

    /** 主将生命值倍率 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "主将生命值倍率"))
    float LeaderHealthMultiplier = 1.0f;

    /** 主将攻击倍率 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "主将攻击倍率"))
    float LeaderDamageMultiplier = 1.0f;

    /** 主将移动速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "主将移动速度"))
    float LeaderMoveSpeed = 600.0f;

    /** 主将冲刺速度倍率 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "主将冲刺速度倍率"))
    float LeaderSprintSpeedMultiplier = 2.0f;

    /** 主将死亡掉落士兵数量 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "主将死亡掉落士兵数量"))
    int32 LeaderDeathDropCount = 5;

    /** 士兵血量 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "士兵血量"))
    float SoldierHealth = 100.0f;

    /** 士兵生命值倍率 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "士兵生命值倍率"))
    float SoldierHealthMultiplier = 1.0f;

    /** 士兵伤害倍率 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "士兵伤害倍率"))
    float SoldierDamageMultiplier = 1.0f;

    /** 获取士兵的缩放比例 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "获取士兵的缩放比例"))
    float SoldierScalePerRecruit = 0.05f;

    /** 获取士兵的回复效果 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "获取士兵的回复效果"))
    float SoldierHealthPerRecruit = 50.0f;

    /** 磁场范围 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "磁场范围"))
    float MagnetFieldRadius = 300.0f;

    /** 初始兵种行 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "初始兵种行"))
    FName InitialSoldierRowName;

    /** 初始带兵数 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "初始带兵数"))
    int32 InitialSoldierCount = 0;

    /** 敌人死亡掉落数 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "敌人死亡掉落数"))
    int32 EnemyDeathDropCount = 5;

    /** 假人行动延迟秒数 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "假人行动延迟秒数"))
    float DummyActionDelay = 1.0f;

    /** 地图选项 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "地图选项"))
    FName SelectedMapName;
};

/**
 * 存档类
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UXBSaveGame();

    // ============ 基本信息 ============

    /** 存档名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Save", meta = (DisplayName = "存档名称"))
    FString SaveSlotName;

    /** 存档时间 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Save", meta = (DisplayName = "存档时间"))
    FDateTime SaveTime;

    // ============ 玩家信息 ============

    /** 玩家名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Save", meta = (DisplayName = "玩家名称"))
    FString PlayerName = TEXT("Player");

    /** 假人名称列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Save", meta = (DisplayName = "假人名称列表"))
    TArray<FString> DummyNames;

    // ============ 游戏配置 ============

    /** 游戏配置数据 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Save", meta = (DisplayName = "游戏配置数据"))
    FXBGameConfigData GameConfig;

    // ============ 场景数据 ============

    /** 场景物件列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Save", meta = (DisplayName = "场景物件列表"))
    TArray<FXBSceneObjectData> SceneObjects;
};
