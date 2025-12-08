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
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ObjectType;

    /** 位置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Location = FVector::ZeroVector;

    /** 旋转 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRotator Rotation = FRotator::ZeroRotator;

    /** 缩放 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Scale = FVector::OneVector;

    /** 额外数据（JSON格式） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
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
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PlayerHealth = 1000.0f;

    /** 玩家伤害倍率 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PlayerDamageMultiplier = 1.0f;

    /** 士兵血量 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SoldierHealth = 100.0f;

    /** 士兵伤害倍率 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SoldierDamageMultiplier = 1.0f;

    /** 获取士兵的缩放比例 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SoldierScalePerRecruit = 0.05f;

    /** 获取士兵的回复效果 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SoldierHealthPerRecruit = 50.0f;

    /** 磁场范围 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MagnetFieldRadius = 300.0f;

    /** 初始带兵数 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 InitialSoldierCount = 0;

    /** 敌人死亡掉落数 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 EnemyDeathDropCount = 5;

    /** 假人行动延迟秒数 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DummyActionDelay = 1.0f;
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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Save")
    FString SaveSlotName;

    /** 存档时间 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Save")
    FDateTime SaveTime;

    // ============ 玩家信息 ============

    /** 玩家名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Save")
    FString PlayerName = TEXT("Player");

    /** 假人名称列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Save")
    TArray<FString> DummyNames;

    // ============ 游戏配置 ============

    /** 游戏配置数据 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Save")
    FXBGameConfigData GameConfig;

    // ============ 场景数据 ============

    /** 场景物件列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Save")
    TArray<FXBSceneObjectData> SceneObjects;
};