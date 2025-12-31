/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/UI/XBConfigWidget.h

/**
 * @file XBConfigWidget.h
 * @brief 配置界面 Widget - 负责配置数据展示与修改
 *
 * @note 架构意图:
 *       1. 仅负责 UI 层数据绑定与触发应用
 *       2. 实际存档读写与运行时应用由 GameInstance/角色负责
 */

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Save/XBSaveGame.h"
#include "XBConfigWidget.generated.h"

class AXBCharacterBase;
class UDataTable;

/**
 * @brief 配置界面 Widget
 * @note 作为 UI 数据承载层，提供蓝图可调用的刷新/应用/保存接口
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBConfigWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief  初始化配置数据
     * @param  InLeader 目标主将（可选）
     * @return 无
     * @note   详细流程分析: 缓存主将引用 -> 读取全局配置 -> 刷新本地数据
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "初始化配置"))
    void InitializeConfig(AXBCharacterBase* InLeader);

    /**
     * @brief  从存档刷新配置数据
     * @return 是否刷新成功
     * @note   详细流程分析: 获取 GameInstance -> 读取 GameConfig -> 同步到 ConfigData
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "刷新配置数据"))
    bool RefreshConfigFromSave();

    /**
     * @brief  应用配置数据
     * @param  bSaveToDisk 是否保存到存档
     * @return 是否应用成功
     * @note   详细流程分析: 保存 GameConfig -> 应用到主将与士兵 -> 可选保存
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "应用配置"))
    bool ApplyConfig(bool bSaveToDisk = true);

    /**
     * @brief  保存配置数据
     * @return 是否保存成功
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "保存配置"))
    bool SaveConfig();

    /**
     * @brief  读取配置数据
     * @return 是否读取成功
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "读取配置"))
    bool LoadConfig();

    /**
     * @brief  重置为默认配置
     * @param  bSaveToDisk 是否保存到存档
     * @return 是否重置成功
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "重置默认配置"))
    bool ResetToDefault(bool bSaveToDisk = true);

    /**
     * @brief  获取主将配置行列表
     * @return 行名数组
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "获取主将配置行列表"))
    TArray<FName> GetLeaderRowNames() const;

    /**
     * @brief  获取士兵配置行列表
     * @return 行名数组
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "获取士兵配置行列表"))
    TArray<FName> GetSoldierRowNames() const;

public:
    // ==================== UI 数据 ====================

    UPROPERTY(BlueprintReadWrite, Category = "配置", meta = (DisplayName = "配置数据"))
    FXBGameConfigData ConfigData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "主将配置数据表"))
    TObjectPtr<UDataTable> LeaderConfigDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "士兵配置数据表"))
    TObjectPtr<UDataTable> SoldierConfigDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "配置", meta = (DisplayName = "地图选项"))
    TArray<FName> MapOptions;

    UPROPERTY(BlueprintReadWrite, Category = "配置", meta = (DisplayName = "目标主将"))
    TWeakObjectPtr<AXBCharacterBase> TargetLeader;
};
