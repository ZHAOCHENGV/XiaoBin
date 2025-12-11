/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Character/Components/XBMagnetFieldComponent.h

/**
 * @file XBMagnetFieldComponent.h
 * @brief 磁场组件 - 招募系统增强版
 * 
 * @note 🔧 修改记录:
 *       1. 新增村民检测逻辑
 *       2. 支持自动转化村民为士兵
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "XBMagnetFieldComponent.generated.h"

class UGameplayEffect;
class AXBCharacterBase;
class AXBSoldierActor;
class AXBVillagerActor; // ✨ 新增

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FXBOnActorEnteredField, AActor*, Actor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FXBOnActorExitedField, AActor*, Actor);

/**
 * @brief 磁场组件
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName = "XB Magnet Field"))
class XIAOBINDATIANXIA_API UXBMagnetFieldComponent : public USphereComponent
{
    GENERATED_BODY()

public:
    UXBMagnetFieldComponent();

    virtual void InitializeComponent() override;
    virtual void BeginPlay() override;

    // ============ 功能接口 ============

    UFUNCTION(BlueprintCallable, Category = "XB|MagnetField")
    void SetFieldRadius(float NewRadius);

    UFUNCTION(BlueprintCallable, Category = "XB|MagnetField")
    float GetFieldRadius() const { return GetScaledSphereRadius(); }

    UFUNCTION(BlueprintCallable, Category = "XB|MagnetField")
    void SetFieldEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "XB|MagnetField")
    bool IsFieldEnabled() const { return bIsFieldEnabled; }

    // ============ 委托事件 ============

    UPROPERTY(BlueprintAssignable, Category = "XB|MagnetField", meta = (DisplayName = "当 Actor 进入磁场时"))
    FXBOnActorEnteredField OnActorEnteredField;

    UPROPERTY(BlueprintAssignable, Category = "XB|MagnetField", meta = (DisplayName = "当 Actor 离开磁场时"))
    FXBOnActorExitedField OnActorExitedField;

protected:
    // ============ 配置属性 ============

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField", meta = (DisplayName = "启用磁场"))
    bool bIsFieldEnabled = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|MagnetField", meta = (DisplayName = "默认磁场半径"))
    float DefaultFieldRadius = 300.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|MagnetField", meta = (DisplayName = "可检测 Actor 类型列表"))
    TArray<TSubclassOf<AActor>> DetectableActorClasses;

    // ============ 内部回调 ============

    UFUNCTION()
    void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
    bool IsActorDetectable(AActor* Actor) const;
    bool bOverlapEventsBound = false;

    // ✨ 新增 - 村民招募逻辑
    /**
     * @brief 处理村民招募
     * @param Villager 村民Actor
     * @return 是否成功招募
     */
    bool TryRecruitVillager(AXBVillagerActor* Villager);

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|MagnetField", meta = (DisplayName = "招募增益效果"))
    TSubclassOf<UGameplayEffect> RecruitBonusEffectClass;

    void ApplyRecruitEffect(AXBCharacterBase* Leader, AXBSoldierActor* Soldier);
};
