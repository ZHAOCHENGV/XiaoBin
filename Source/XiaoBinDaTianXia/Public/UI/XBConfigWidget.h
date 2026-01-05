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
     * @brief  开始游戏
     * @param  bSaveToDisk 是否保存到存档
     * @return 是否开始成功
     * @note   详细流程分析: 写入配置 -> 应用到主将与士兵 -> 加载选定地图
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "开始游戏"))
    bool StartGame(bool bSaveToDisk = true);

    /**
     * @brief  设置配置数据
     * @param  NewConfig 新配置数据
     * @param  bSyncToUI 是否同步到 UI
     * @return 无
     * @note   详细流程分析: 覆盖 ConfigData -> 可选同步到 UI
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "设置配置数据"))
    void SetConfigData(const FXBGameConfigData& NewConfig, bool bSyncToUI = true);

    /**
     * @brief  同步 UI 控件数据到配置数据
     * @note   详细流程分析: 由蓝图实现，将当前 UI 值写回 ConfigData
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "XB|Config", meta = (DisplayName = "同步UI到配置"))
    void SyncConfigFromUI();

    /**
     * @brief  同步配置数据到 UI 控件
     * @note   详细流程分析: 由蓝图实现，将 ConfigData 刷新到 UI
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "XB|Config", meta = (DisplayName = "同步配置到UI"))
    void SyncUIFromConfig();

    /**
     * @brief  保存配置数据
     * @return 是否保存成功
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "保存配置"))
    bool SaveConfig();

    /**
     * @brief  使用自定义名称保存配置数据
     * @param  SlotName 存档名称
     * @return 是否保存成功
     * @note   详细流程分析: 写入 GameConfig -> 使用名称保存
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "保存配置(名称)"))
    bool SaveConfigByName(const FString& SlotName);

    /**
     * @brief  读取配置数据
     * @return 是否读取成功
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "读取配置"))
    bool LoadConfig();

    /**
     * @brief  使用自定义名称读取配置数据
     * @param  SlotName 存档名称
     * @return 是否读取成功
     * @note   详细流程分析: 使用名称加载 -> 同步到 ConfigData
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Config", meta = (DisplayName = "读取配置(名称)"))
    bool LoadConfigByName(const FString& SlotName);

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

private:
    UPROPERTY(VisibleAnywhere, Category = "配置", meta = (DisplayName = "初始配置缓存"))
    FXBGameConfigData InitialConfigData;

    UPROPERTY(VisibleAnywhere, Category = "配置", meta = (DisplayName = "是否已缓存初始配置"))
    bool bHasCachedInitialConfig = false;
};
