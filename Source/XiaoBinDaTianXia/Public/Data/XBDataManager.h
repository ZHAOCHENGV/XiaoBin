// Copyright XiaoBing Project. All Rights Reserved.

/**
 * @file XBDataManager.h
 * @brief 数据管理器 - 游戏数据的统一访问接口
 * 
 * 功能说明：
 * - 提供全局数据访问接口
 * - 管理数据资产的加载和缓存
 * - 单例模式，通过 GameInstance 访问
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/XBDataTypes.h"
#include "XBDataManager.generated.h"

// 前向声明
class UXBGlobalDataAsset;
class UXBLeaderDataAsset;
class UXBSoldierDataAsset;

/**
 * @brief 数据管理器子系统
 * 
 * 使用方法：
 * - C++: GetWorld()->GetGameInstance()->GetSubsystem<UXBDataManager>()
 * - 蓝图: Get Data Manager 节点
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBDataManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // ==================== 生命周期 ====================

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ==================== 数据访问接口 ====================

    /**
     * @brief 获取全局数据资产
     * @return 全局数据资产指针
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Data", meta = (DisplayName = "获取全局数据"))
    UXBGlobalDataAsset* GetGlobalData() const { return GlobalDataAsset; }

    /**
     * @brief 获取将领配置
     * @param LeaderId 将领ID
     * @return 将领配置数据
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Data", meta = (DisplayName = "获取将领配置"))
    FXBLeaderConfig GetLeaderConfig(FName LeaderId) const;

    /**
     * @brief 获取默认将领配置
     * @return 默认将领配置数据
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Data", meta = (DisplayName = "获取默认将领配置"))
    FXBLeaderConfig GetDefaultLeaderConfig() const;

    /**
     * @brief 获取士兵配置
     * @param SoldierType 士兵类型
     * @return 士兵配置数据
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Data", meta = (DisplayName = "获取士兵配置"))
    FXBSoldierConfig GetSoldierConfig(EXBSoldierType SoldierType) const;

    /**
     * @brief 获取编队配置
     * @return 编队配置数据
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Data", meta = (DisplayName = "获取编队配置"))
    FXBFormationConfig GetFormationConfig() const;

    /**
     * @brief 获取战斗配置
     * @return 战斗配置数据
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Data", meta = (DisplayName = "获取战斗配置"))
    FXBCombatConfig GetCombatConfig() const;

    // ==================== 数据加载 ====================

    /**
     * @brief 加载全局数据资产
     * @param DataAssetPath 资产路径
     * @return 是否加载成功
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Data", meta = (DisplayName = "加载全局数据"))
    bool LoadGlobalData(const FSoftObjectPath& DataAssetPath);

    /**
     * @brief 设置全局数据资产（直接设置）
     * @param InGlobalData 全局数据资产
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Data", meta = (DisplayName = "设置全局数据"))
    void SetGlobalData(UXBGlobalDataAsset* InGlobalData);

protected:
    /** 全局数据资产 */
    UPROPERTY()
    TObjectPtr<UXBGlobalDataAsset> GlobalDataAsset;

    /** 将领数据缓存 */
    UPROPERTY()
    TMap<FName, TObjectPtr<UXBLeaderDataAsset>> LeaderDataCache;

    /** 士兵数据缓存 */
    UPROPERTY()
    TMap<EXBSoldierType, TObjectPtr<UXBSoldierDataAsset>> SoldierDataCache;

private:
    /** 初始化数据缓存 */
    void InitializeDataCache();
};
