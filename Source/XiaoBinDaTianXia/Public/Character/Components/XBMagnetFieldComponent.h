/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Character/Components/XBMagnetFieldComponent.h

/**
 * @file XBMagnetFieldComponent.h
 * @brief 磁场组件 - 招募系统（简化版）
 * 
 * @note 🔧 修改记录:
 *       1. ❌ 删除 TryRecruitVillager 方法（不再需要）
 *       2. ❌ 删除 对象池相关的招募逻辑
 *       3. 🔧 简化 直接招募场景中的休眠态士兵
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "XBMagnetFieldComponent.generated.h"

class UGameplayEffect;
class AXBCharacterBase;
class AXBSoldierCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FXBOnActorEnteredField, AActor*, Actor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FXBOnActorExitedField, AActor*, Actor);

/**
 * @brief 磁场统计数据
 */
USTRUCT(BlueprintType)
struct FXBMagnetFieldStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "统计", meta = (DisplayName = "总招募士兵数"))
    int32 TotalSoldiersRecruited = 0;

    UPROPERTY(BlueprintReadOnly, Category = "统计", meta = (DisplayName = "范围内Actor数"))
    int32 ActorsInRange = 0;

    UPROPERTY(BlueprintReadOnly, Category = "统计", meta = (DisplayName = "上次招募时间"))
    float LastRecruitTime = 0.0f;
};

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
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ============ 功能接口 ============

    UFUNCTION(BlueprintCallable, Category = "XB|MagnetField")
    void SetFieldRadius(float NewRadius);

    UFUNCTION(BlueprintCallable, Category = "XB|MagnetField")
    float GetFieldRadius() const { return GetScaledSphereRadius(); }

    UFUNCTION(BlueprintCallable, Category = "XB|MagnetField")
    void SetFieldEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "XB|MagnetField")
    bool IsFieldEnabled() const { return bIsFieldEnabled; }

    /**
     * @brief 扫描并招募已经在磁场范围内的休眠态士兵
     * @note  用于解决士兵在游戏开始时就在磁场范围内无法触发 Overlap 事件的问题
     *        在 SetFieldEnabled(true) 时自动调用
     */
    UFUNCTION(BlueprintCallable, Category = "XB|MagnetField", meta = (DisplayName = "扫描并招募范围内士兵"))
    void ScanAndRecruitExistingActors();

    UFUNCTION(BlueprintPure, Category = "XB|MagnetField", meta = (DisplayName = "获取磁场统计"))
    const FXBMagnetFieldStats& GetFieldStats() const { return FieldStats; }

    UFUNCTION(BlueprintCallable, Category = "XB|MagnetField", meta = (DisplayName = "重置统计"))
    void ResetStats();

    // ============ 调试系统 ============

    UFUNCTION(BlueprintCallable, Category = "XB|MagnetField|Debug", meta = (DisplayName = "启用调试绘制"))
    void SetDebugDrawEnabled(bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "XB|MagnetField|Debug", meta = (DisplayName = "是否启用调试"))
    bool IsDebugDrawEnabled() const { return bDrawDebug; }

    UFUNCTION(BlueprintCallable, Category = "XB|MagnetField|Debug", meta = (DisplayName = "绘制调试信息"))
    void DrawDebugField(float Duration = 0.0f);

    // ============ 委托事件 ============

    UPROPERTY(BlueprintAssignable, Category = "XB|MagnetField", meta = (DisplayName = "当 Actor 进入磁场时"))
    FXBOnActorEnteredField OnActorEnteredField;

    UPROPERTY(BlueprintAssignable, Category = "XB|MagnetField", meta = (DisplayName = "当 Actor 离开磁场时"))
    FXBOnActorExitedField OnActorExitedField;

protected:
    // ============ 配置属性 ============

    /** 启用磁场（默认关闭，只有玩家主将生成后才启用） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField", meta = (DisplayName = "启用磁场"))
    bool bIsFieldEnabled = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|MagnetField", meta = (DisplayName = "默认磁场半径"))
    float DefaultFieldRadius = 300.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|MagnetField", meta = (DisplayName = "可检测 Actor 类型列表"))
    TArray<TSubclassOf<AActor>> DetectableActorClasses;

    // ============ 调试配置 ============

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField|Debug", meta = (DisplayName = "启用调试绘制"))
    bool bDrawDebug = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField|Debug", meta = (DisplayName = "启用颜色"))
    FColor DebugEnabledColor = FColor::Green;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField|Debug", meta = (DisplayName = "禁用颜色"))
    FColor DebugDisabledColor = FColor::Red;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField|Debug", meta = (DisplayName = "可招募目标颜色"))
    FColor DebugRecruitableColor = FColor::Yellow;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField|Debug", meta = (DisplayName = "不可招募目标颜色"))
    FColor DebugNonRecruitableColor = FColor::Orange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField|Debug", meta = (DisplayName = "圆圈段数", ClampMin = "8", ClampMax = "64"))
    int32 DebugCircleSegments = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField|Debug", meta = (DisplayName = "文字高度偏移", ClampMin = "0.0"))
    float DebugTextHeightOffset = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField|Debug", meta = (DisplayName = "显示Actor信息"))
    bool bShowActorInfo = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField|Debug", meta = (DisplayName = "显示统计数据"))
    bool bShowStats = true;

    // ============ 运行时数据 ============

    UPROPERTY(BlueprintReadOnly, Category = "XB|MagnetField")
    FXBMagnetFieldStats FieldStats;

    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> ActorsInField;

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

    void UpdateActorsInField();
    bool IsActorRecruitable(AActor* Actor) const;
    void DrawDebugActorInfo(AActor* Actor, float Duration);

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|MagnetField", meta = (DisplayName = "招募增益效果"))
    TSubclassOf<UGameplayEffect> RecruitBonusEffectClass;

    void ApplyRecruitEffect(AXBCharacterBase* Leader, AXBSoldierCharacter* Soldier);
};
