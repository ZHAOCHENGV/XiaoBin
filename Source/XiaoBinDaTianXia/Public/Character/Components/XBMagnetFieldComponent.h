/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Character/Components/XBMagnetFieldComponent.h

/**
 * @file XBMagnetFieldComponent.h
 * @brief 磁场组件 - 招募系统增强版
 * 
 * @note 🔧 修改记录:
 *       1. ✨ 新增完整的调试可视化系统
 *       2. ✨ 新增磁场状态统计
 *       3. ✨ 新增调试配置选项
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "XBMagnetFieldComponent.generated.h"

class UGameplayEffect;
class AXBCharacterBase;
class AXBSoldierCharacter;
class AXBVillagerActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FXBOnActorEnteredField, AActor*, Actor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FXBOnActorExitedField, AActor*, Actor);

// ✨ 新增 - 磁场统计数据
USTRUCT(BlueprintType)
struct FXBMagnetFieldStats
{
    GENERATED_BODY()

    /** @brief 总招募士兵数 */
    UPROPERTY(BlueprintReadOnly, Category = "统计", meta = (DisplayName = "总招募士兵数"))
    int32 TotalSoldiersRecruited = 0;

    /** @brief 总招募村民数 */
    UPROPERTY(BlueprintReadOnly, Category = "统计", meta = (DisplayName = "总招募村民数"))
    int32 TotalVillagersRecruited = 0;

    /** @brief 当前范围内Actor数量 */
    UPROPERTY(BlueprintReadOnly, Category = "统计", meta = (DisplayName = "范围内Actor数"))
    int32 ActorsInRange = 0;

    /** @brief 上次招募时间 */
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

    // ✨ 新增 - 获取统计数据
    UFUNCTION(BlueprintPure, Category = "XB|MagnetField", meta = (DisplayName = "获取磁场统计"))
    const FXBMagnetFieldStats& GetFieldStats() const { return FieldStats; }

    // ✨ 新增 - 重置统计数据
    UFUNCTION(BlueprintCallable, Category = "XB|MagnetField", meta = (DisplayName = "重置统计"))
    void ResetStats();

    // ============ ✨ 新增：调试系统 ============

    /**
     * @brief 启用/禁用调试绘制
     * @param bEnabled 是否启用
     */
    UFUNCTION(BlueprintCallable, Category = "XB|MagnetField|Debug", meta = (DisplayName = "启用调试绘制"))
    void SetDebugDrawEnabled(bool bEnabled);

    /**
     * @brief 获取调试绘制状态
     */
    UFUNCTION(BlueprintPure, Category = "XB|MagnetField|Debug", meta = (DisplayName = "是否启用调试"))
    bool IsDebugDrawEnabled() const { return bDrawDebug; }

    /**
     * @brief 立即绘制调试信息
     * @param Duration 持续时间（0表示单帧）
     */
    UFUNCTION(BlueprintCallable, Category = "XB|MagnetField|Debug", meta = (DisplayName = "绘制调试信息"))
    void DrawDebugField(float Duration = 0.0f);

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

    // ============ ✨ 新增：调试配置 ============

    /** @brief 是否启用调试绘制 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField|Debug", meta = (DisplayName = "启用调试绘制"))
    bool bDrawDebug = false;

    /** @brief 磁场圆圈颜色（启用时） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField|Debug", meta = (DisplayName = "启用颜色"))
    FColor DebugEnabledColor = FColor::Green;

    /** @brief 磁场圆圈颜色（禁用时） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField|Debug", meta = (DisplayName = "禁用颜色"))
    FColor DebugDisabledColor = FColor::Red;

    /** @brief 可招募目标连线颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField|Debug", meta = (DisplayName = "可招募目标颜色"))
    FColor DebugRecruitableColor = FColor::Yellow;

    /** @brief 不可招募目标连线颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField|Debug", meta = (DisplayName = "不可招募目标颜色"))
    FColor DebugNonRecruitableColor = FColor::Orange;

    /** @brief 调试圆圈段数 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField|Debug", meta = (DisplayName = "圆圈段数", ClampMin = "8", ClampMax = "64"))
    int32 DebugCircleSegments = 32;

    /** @brief 调试文字高度偏移 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField|Debug", meta = (DisplayName = "文字高度偏移", ClampMin = "0.0"))
    float DebugTextHeightOffset = 150.0f;

    /** @brief 是否显示范围内Actor信息 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField|Debug", meta = (DisplayName = "显示Actor信息"))
    bool bShowActorInfo = true;

    /** @brief 是否显示统计数据 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|MagnetField|Debug", meta = (DisplayName = "显示统计数据"))
    bool bShowStats = true;

    // ============ 运行时数据 ============

    /** @brief 磁场统计数据 */
    UPROPERTY(BlueprintReadOnly, Category = "XB|MagnetField")
    FXBMagnetFieldStats FieldStats;

    /** @brief 当前范围内的Actor列表 */
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

    bool TryRecruitVillager(AXBVillagerActor* Villager);

    // ✨ 新增 - 更新范围内Actor列表
    void UpdateActorsInField();

    // ✨ 新增 - 检查Actor是否可招募
    bool IsActorRecruitable(AActor* Actor) const;

    // ✨ 新增 - 绘制单个Actor的调试信息
    void DrawDebugActorInfo(AActor* Actor, float Duration);

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XB|MagnetField", meta = (DisplayName = "招募增益效果"))
    TSubclassOf<UGameplayEffect> RecruitBonusEffectClass;

    void ApplyRecruitEffect(AXBCharacterBase* Leader, AXBSoldierCharacter* Soldier);
};
