// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "XBInputConfig.generated.h"

class UInputAction;

/**
 * @brief 输入动作与 GameplayTag 的映射结构体
 * * 用于在配置中建立 Action 和 Tag 的动态关联
 */
USTRUCT(BlueprintType)
struct FXBInputActionMapping
{
    GENERATED_BODY()

    
    /** 增强输入系统中的 InputAction 资产 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "输入动作"))
    TObjectPtr<UInputAction> InputAction = nullptr;

    
    /** 关联的 GameplayTag，用于逻辑层识别 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "Input", DisplayName = "输入标签"))
    FGameplayTag InputTag;
};

/**
 * @brief 输入配置数据资产 (Data Asset)
 * * 集中管理项目中所有的 InputAction 引用。
 * * @details 避免在 C++ 代码中硬编码输入资产路径，允许策划在编辑器中配置输入方案。
 */
UCLASS(BlueprintType, Const)
class XIAOBINDATIANXIA_API UXBInputConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    /**
     * @brief 根据 Tag 查找对应的 InputAction
     * * @param InputTag 要查找的标签
     * * @return const UInputAction* 对应的输入动作指针，未找到返回 nullptr
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Input")
    const UInputAction* FindInputActionByTag(const FGameplayTag& InputTag) const;

    /**
     * @brief 根据 InputAction 查找对应的 Tag
     * * @param InputAction 要查找的输入动作
     * * @return FGameplayTag 对应的标签，未找到返回 Empty
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Input")
    FGameplayTag FindTagByInputAction(const UInputAction* InputAction) const;

public:
    // ============ 基础移动 ============

    
    /** 控制角色移动 (Vector2D: WASD) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input|Movement", meta = (DisplayName = "移动输入"))
    TObjectPtr<UInputAction> MoveAction;

    
    /** 触发冲刺 (Bool: Space/Shift) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input|Movement", meta = (DisplayName = "冲刺输入"))
    TObjectPtr<UInputAction> DashAction;

    // ============ 镜头操作 ============

    
    /** 镜头缩放 (Axis1D: Mouse Wheel) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input|Camera", meta = (DisplayName = "镜头缩放"))
    TObjectPtr<UInputAction> CameraZoomAction;

    
    /** 镜头视角旋转 (Axis2D: Mouse Move) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input|Camera", meta = (DisplayName = "镜头视角"))
    TObjectPtr<UInputAction> LookAction;

    
    /** 镜头向左持续旋转 (Bool: Q) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input|Camera", meta = (DisplayName = "镜头左旋"))
    TObjectPtr<UInputAction> CameraRotateLeftAction;

    
    /** 镜头向右持续旋转 (Bool: E) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input|Camera", meta = (DisplayName = "镜头右旋"))
    TObjectPtr<UInputAction> CameraRotateRightAction;

    
    /** 重置镜头视角 (Bool: Middle Mouse Button) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input|Camera", meta = (DisplayName = "镜头重置"))
    TObjectPtr<UInputAction> CameraResetAction;

    // ============ 战斗操作 ============

    
    /** 普通攻击 (Bool: Left Mouse Button) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input|Combat", meta = (DisplayName = "攻击输入"))
    TObjectPtr<UInputAction> AttackAction;

    
    /** 释放技能 (Bool: Right Mouse Button) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input|Combat", meta = (DisplayName = "技能输入"))
    TObjectPtr<UInputAction> SkillAction;

    
    /** 召回/解散士兵 (Bool: R key) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input|Combat", meta = (DisplayName = "召回士兵"))
    TObjectPtr<UInputAction> RecallAction;

    /** 生成主将 (Bool: Enter key) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input|Extended", meta = (DisplayName = "生成主将"))
    TObjectPtr<UInputAction> SpawnLeaderAction;

    // ============ 配置阶段放置 ============

    /** 放置点击 (Bool: Left Mouse Button) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input|Placement", meta = (DisplayName = "放置点击"))
    TObjectPtr<UInputAction> PlacementClickAction;

    /** 放置取消 (Bool: ESC / Right Mouse Button) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input|Placement", meta = (DisplayName = "放置取消"))
    TObjectPtr<UInputAction> PlacementCancelAction;

    /** 放置删除 (Bool: Delete key) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input|Placement", meta = (DisplayName = "放置删除"))
    TObjectPtr<UInputAction> PlacementDeleteAction;

    /** 放置旋转 (Axis1D: Mouse Wheel) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input|Placement", meta = (DisplayName = "放置旋转"))
    TObjectPtr<UInputAction> PlacementRotateAction;

    /** 切换放置菜单 (Bool: Tab key) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input|Placement", meta = (DisplayName = "切换放置菜单"))
    TObjectPtr<UInputAction> TogglePlacementMenuAction;

    // ============ 系统操作 ============

    /** 暂停菜单 (Bool: ESC key) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input|System", meta = (DisplayName = "暂停菜单"))
    TObjectPtr<UInputAction> PauseMenuAction;

    // ============ 扩展配置 ============

    
    /** 扩展输入映射列表，用于不常用或动态绑定的输入 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|Input|Extended", Meta = (TitleProperty = "InputTag", DisplayName = "扩展输入映射"))
    TArray<FXBInputActionMapping> ExtendedInputMappings;
};
