/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Village/XBVillagerActor.h

/**
 * @file XBVillagerActor.h
 * @brief 村民Actor - 可被招募的中立单位
 * 
 * @note ✨ 新增文件
 *       功能：
 *       1. 睡眠/待机状态切换
 *       2. 可配置的 Zzz 特效
 *       3. 被招募后转化为士兵
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Army/XBSoldierTypes.h"
#include "XBVillagerActor.generated.h"

class UParticleSystemComponent;
class UWidgetComponent;
class UNiagaraComponent;

/**
 * @brief 村民状态枚举
 */
UENUM(BlueprintType)
enum class EXBVillagerState : uint8
{
    Idle        UMETA(DisplayName = "待机"),
    Sleeping    UMETA(DisplayName = "睡眠")
};

/**
 * @brief 村民Actor类
 * @note 核心逻辑：
 *       - 进入将领磁场范围后被招募
 *       - 转化为对应兵种的士兵
 *       - 睡眠状态显示 Zzz 特效
 */
UCLASS()
class XIAOBINDATIANXIA_API AXBVillagerActor : public ACharacter
{
    GENERATED_BODY()

public:
    AXBVillagerActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // ============ 状态管理 ============

    /**
     * @brief 设置村民状态
     * @param NewState 新状态
     */
    UFUNCTION(BlueprintCallable, Category = "Village", meta = (DisplayName = "设置状态"))
    void SetVillagerState(EXBVillagerState NewState);

    /**
     * @brief 获取当前状态
     */
    UFUNCTION(BlueprintPure, Category = "Village", meta = (DisplayName = "获取状态"))
    EXBVillagerState GetVillagerState() const { return CurrentState; }

    /**
     * @brief 是否可以被招募
     */
    UFUNCTION(BlueprintPure, Category = "Village", meta = (DisplayName = "是否可招募"))
    bool CanBeRecruited() const;

    /**
     * @brief 被招募（由磁场组件调用）
     * @param Leader 招募的将领
     */
    UFUNCTION(BlueprintCallable, Category = "Village", meta = (DisplayName = "被招募"))
    void OnRecruited(AActor* Leader);

    // ============ Zzz 特效控制 ============

    /**
     * @brief 启用/禁用 Zzz 特效
     * @param bEnabled 是否启用
     */
    UFUNCTION(BlueprintCallable, Category = "Village|Visual", meta = (DisplayName = "切换Zzz特效"))
    void SetZzzEffectEnabled(bool bEnabled);

protected:
    // ============ 组件 ============

    /** @brief Zzz 特效组件（使用 Niagara 粒子系统） */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "Zzz特效"))
    TObjectPtr<UNiagaraComponent> ZzzEffectComponent;

    // ============ 配置 ============

    /** @brief 当前状态 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "村民", meta = (DisplayName = "初始状态"))
    EXBVillagerState CurrentState = EXBVillagerState::Idle;

    /** @brief 是否启用 Zzz 特效 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "村民", meta = (DisplayName = "启用Zzz特效"))
    bool bEnableZzzEffect = true;

    /** @brief 睡眠动画蒙太奇 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "村民", meta = (DisplayName = "睡眠动画"))
    TObjectPtr<UAnimMontage> SleepingMontage;

    /** @brief 待机动画蒙太奇 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "村民", meta = (DisplayName = "待机动画"))
    TObjectPtr<UAnimMontage> IdleMontage;

    /** @brief Zzz 特效资源路径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "村民", meta = (DisplayName = "Zzz特效资源"))
    TSoftObjectPtr<class UNiagaraSystem> ZzzEffectAsset;

private:
    /** @brief 是否已被招募 */
    bool bIsRecruited = false;

    /** @brief 更新动画状态 */
    void UpdateAnimationState();

    /** @brief 更新 Zzz 特效 */
    void UpdateZzzEffect();
};
