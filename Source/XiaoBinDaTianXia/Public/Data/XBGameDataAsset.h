// Copyright XiaoBing Project. All Rights Reserved.

/**
 * @file XBGameDataAsset.h
 * @brief 游戏数据资产定义
 * 
 * 功能说明：
 * - 定义可在编辑器中配置的数据资产类
 * - 集中管理所有游戏配置数据
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/XBDataTypes.h"
#include "XBGameDataAsset.generated.h"

/**
 * @brief 将领数据资产
 * 
 * 使用方法：
 * - 在编辑器中创建：右键 → Miscellaneous → Data Asset → XBLeaderDataAsset
 * - 配置将领的各项属性
 */
UCLASS(BlueprintType)
class XIAOBINDATIANXIA_API UXBLeaderDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    /** 将领唯一标识 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "标识", meta = (DisplayName = "将领ID"))
    FName LeaderId;

    /** 将领配置数据 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "配置", meta = (DisplayName = "将领配置"))
    FXBLeaderConfig LeaderConfig;

    /** 获取主资产ID */
    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("Leader", LeaderId);
    }
};

/**
 * @brief 士兵数据资产
 */
UCLASS(BlueprintType)
class XIAOBINDATIANXIA_API UXBSoldierDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    /** 士兵类型标识 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "标识", meta = (DisplayName = "士兵类型"))
    EXBSoldierType SoldierType = EXBSoldierType::Infantry;

    /** 士兵配置数据 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "配置", meta = (DisplayName = "士兵配置"))
    FXBSoldierConfig SoldierConfig;

    /** 获取主资产ID */
    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("Soldier", *UEnum::GetValueAsString(SoldierType));
    }
};

/**
 * @brief 全局游戏数据资产
 * 
 * 功能说明：
 * - 包含所有全局配置
 * - 在项目设置中指定
 */
UCLASS(BlueprintType)
class XIAOBINDATIANXIA_API UXBGlobalDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    /** 编队配置 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "编队", meta = (DisplayName = "编队配置"))
    FXBFormationConfig FormationConfig;

    /** 战斗配置 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "战斗", meta = (DisplayName = "战斗配置"))
    FXBCombatConfig CombatConfig;

    /** 默认将领数据 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "默认", meta = (DisplayName = "默认将领数据"))
    TObjectPtr<UXBLeaderDataAsset> DefaultLeaderData;

    /** 士兵数据列表 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "士兵", meta = (DisplayName = "士兵数据列表"))
    TArray<TObjectPtr<UXBSoldierDataAsset>> SoldierDataList;

    /** 获取主资产ID */
    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("GlobalData", "Main");
    }
};
