// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "XBMagnetFieldComponent.generated.h"

class UGameplayEffect;
class AXBCharacterBase;
class AXBSoldierActor;
// 定义委托，用于通知外部 Actor 进入或离开磁场
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FXBOnActorEnteredField, AActor*, Actor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FXBOnActorExitedField, AActor*, Actor);

/**
 * @brief 磁场组件 (Magnet Field Component)
 * * 继承自 USphereComponent，用于处理范围检测逻辑。
 * * @details 核心功能：
 * 1. 作为一个球形触发器，跟随主将移动。
 * [cite_start]2. 筛选特定的 Actor（如沉睡的村民 [cite: 16][cite_start]、士兵包 [cite: 34]）并触发转化逻辑。
 * 3. 提供动态开启/关闭检测的接口。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName = "XB Magnet Field"))
class XIAOBINDATIANXIA_API UXBMagnetFieldComponent : public USphereComponent
{
    GENERATED_BODY()

public:
    /**
     * @brief 构造函数
     * * 设置默认碰撞配置（QueryOnly, Trigger）
     */
    UXBMagnetFieldComponent();

    // ============ 生命周期 ============

    /**
     * @brief 组件初始化
     * * 相比 BeginPlay 更早执行，用于绑定初始状态
     */
    virtual void InitializeComponent() override;

    /**
     * @brief 游戏开始
     * * 确保碰撞状态正确
     */
    virtual void BeginPlay() override;

    // ============ 功能接口 ============

    /**
     * @brief 设置磁场半径
     * * @param NewRadius 新的半径值
     * 实时修改球体碰撞的大小
     */
    UFUNCTION(BlueprintCallable, Category = "XB|MagnetField")
    void SetFieldRadius(float NewRadius);

    /**
     * @brief 获取当前磁场半径
     * * @return float 缩放后的半径
     */
    UFUNCTION(BlueprintCallable, Category = "XB|MagnetField")
    float GetFieldRadius() const { return GetScaledSphereRadius(); }

    /**
     * @brief 启用或禁用磁场检测
     * * @param bEnabled 是否启用
     * 禁用时会关闭碰撞检测，节省性能
     */
    UFUNCTION(BlueprintCallable, Category = "XB|MagnetField")
    void SetFieldEnabled(bool bEnabled);

    /**
     * @brief 检查磁场是否启用
     * * @return true 启用中
     */
    UFUNCTION(BlueprintCallable, Category = "XB|MagnetField")
    bool IsFieldEnabled() const { return bIsFieldEnabled; }

    // ============ 委托事件 ============

    
    /** 当符合条件的 Actor 进入磁场时广播 */
    UPROPERTY(BlueprintAssignable, Category = "XB|MagnetField", meta = (DisplayName = "当 Actor 进入磁场时"))
    FXBOnActorEnteredField OnActorEnteredField;

    
    /** 当符合条件的 Actor 离开磁场时广播 */
    UPROPERTY(BlueprintAssignable, Category = "XB|MagnetField", meta = (DisplayName = "当 Actor 离开磁场时"))
    FXBOnActorExitedField OnActorExitedField;

protected:
    // ============ 配置属性 ============

    
    /** 磁场当前是否处于激活状态 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField", meta = (DisplayName = "启用磁场"))
    bool bIsFieldEnabled = true;

    
    /** 磁场的默认半径大小 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|MagnetField", meta = (DisplayName = "默认磁场半径"))
    float DefaultFieldRadius = 300.0f;

    
    /** * 可被磁场检测到的 Actor 类型列表
     * * 用于过滤无关物体，只对村民或道具生效
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|MagnetField", meta = (DisplayName = "可检测 Actor 类型列表"))
    TArray<TSubclassOf<AActor>> DetectableActorClasses;

    // ============ 内部回调 ============

    /**
     * @brief 球体碰撞开始重叠回调
     * * 筛选 Actor 类型并广播 OnActorEnteredField
     */
    UFUNCTION()
    void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    /**
     * @brief 球体碰撞结束重叠回调
     * * 广播 OnActorExitedField
     */
    UFUNCTION()
    void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
    /**
     * @brief 检查 Actor 是否在检测白名单中
     * * @param Actor 待检测对象
     * * @return true 如果属于 DetectableActorClasses 中的类型
     */
    bool IsActorDetectable(AActor* Actor) const;
    
    /** 内部标记：防止重复绑定重叠事件 */
    bool bOverlapEventsBound = false;


    
protected:
    // ✨ 新增 - 招募效果配置
    /** 招募士兵时应用的 GameplayEffect 类 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|MagnetField", meta = (DisplayName = "招募增益效果"))
    TSubclassOf<UGameplayEffect> RecruitBonusEffectClass;

    /**
     * @brief 应用招募效果到将领
     * @param Leader 将领角色
     * @param Soldier 被招募的士兵
     */
    void ApplyRecruitEffect(AXBCharacterBase* Leader, AXBSoldierActor* Soldier);
};