/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Village/XBVillagerActor.h

/**
 * @file XBVillagerActor.h
 * @brief 村民Actor - 可被招募的中立单位
 * 
 * @note 继承 AActor，使用胶囊体+骨骼网格体组合
 *       功能：
 *       1. 睡眠/待机状态切换（直接播放动画序列）
 *       2. 可配置的 Zzz 特效
 *       3. 被招募后转化为士兵
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "XBVillagerActor.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UAnimSequence;
class AXBSoldierCharacter;

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
class XIAOBINDATIANXIA_API AXBVillagerActor : public AActor
{
    GENERATED_BODY()

public:
    AXBVillagerActor();

protected:
    virtual void BeginPlay() override;

public:
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

    // ============ 组件访问器 ============

    UFUNCTION(BlueprintPure, Category = "Village")
    UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }

    UFUNCTION(BlueprintPure, Category = "Village")
    USkeletalMeshComponent* GetMesh() const { return MeshComponent; }

protected:
    // ============ 组件 ============

    /** @brief 胶囊体组件 (Root) - 负责碰撞检测 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "胶囊体"))
    TObjectPtr<UCapsuleComponent> CapsuleComponent;

    /** @brief 骨骼网格体组件 - 负责显示和动画 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "网格体"))
    TObjectPtr<USkeletalMeshComponent> MeshComponent;

    /** @brief Zzz 特效组件（使用 Niagara 粒子系统） */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "Zzz特效"))
    TObjectPtr<UNiagaraComponent> ZzzEffectComponent;

    // ============ 状态配置 ============

    /** @brief 当前状态 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "村民|状态", meta = (DisplayName = "初始状态"))
    EXBVillagerState CurrentState = EXBVillagerState::Idle;

    /** @brief 是否启用 Zzz 特效 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "村民|状态", meta = (DisplayName = "启用Zzz特效"))
    bool bEnableZzzEffect = true;

    // ============ 动画配置 ============

    /** @brief 待机动画序列（循环播放） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "村民|动画", meta = (DisplayName = "待机动画"))
    TObjectPtr<UAnimSequence> IdleAnimation;

    /** @brief 睡眠动画序列（循环播放） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "村民|动画", meta = (DisplayName = "睡眠动画"))
    TObjectPtr<UAnimSequence> SleepingAnimation;

    // ============ 特效配置 ============

    /** @brief Zzz 特效资源 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "村民|特效", meta = (DisplayName = "Zzz特效资源"))
    TSoftObjectPtr<UNiagaraSystem> ZzzEffectAsset;

    /** @brief Zzz 特效相对位置偏移 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "村民|特效", meta = (DisplayName = "Zzz特效位置偏移"))
    FVector ZzzEffectOffset = FVector(0.0f, 0.0f, 100.0f);

    // ============ 招募配置 ============

    /** @brief 转化的士兵类（可选，如不设置则使用将领默认配置） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "村民|招募", meta = (DisplayName = "转化士兵类"))
    TSubclassOf<AXBSoldierCharacter> SoldierClassToSpawn;

private:
    /** @brief 是否已被招募 */
    bool bIsRecruited = false;

    /** @brief 更新动画播放 */
    void UpdateAnimation();

    /** @brief 更新 Zzz 特效 */
    void UpdateZzzEffect();

    /** @brief 播放指定动画序列 */
    void PlayAnimation(UAnimSequence* Animation, bool bLoop = true);
};
