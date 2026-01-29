/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Environment/XBBushVolume.h

/**
 * @file XBBushVolume.h
 * @brief 草丛体积触发器 - 将领进入后全军隐身
 * 
 * @note ✨ 新增文件
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "XBBushVolume.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class AXBCharacterBase;
struct FHitResult;

UCLASS()
class XIAOBINDATIANXIA_API AXBBushVolume : public AActor
{
    GENERATED_BODY()

public:
    AXBBushVolume();

protected:
    virtual void BeginPlay() override;

private:
    /**
     * @brief  触发进入
     * @param  OverlappedComponent 触发组件
     * @param  OtherActor 进入的Actor
     * @param  OtherComp 进入Actor的组件
     * @param  OtherBodyIndex 体索引
     * @param  bFromSweep 是否扫掠
     * @param  SweepResult 命中信息
     * @note   详细流程分析: 仅处理主将进入 -> 设置全军隐身
     *         性能/架构注意事项: 使用弱引用集合，避免悬挂引用
     */
    UFUNCTION()
    void OnBushOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    /**
     * @brief  触发离开
     * @param  OverlappedComponent 触发组件
     * @param  OtherActor 离开的Actor
     * @param  OtherComp 离开Actor的组件
     * @param  OtherBodyIndex 体索引
     * @note   详细流程分析: 主将离开后恢复全军可见
     *         性能/架构注意事项: 防止重复调用导致状态抖动
     */
    UFUNCTION()
    void OnBushOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
    UPROPERTY(VisibleAnywhere, Category = "组件", meta = (DisplayName = "草丛盒体"))
    TObjectPtr<UBoxComponent> BushBox;

    UPROPERTY()
    TSet<TWeakObjectPtr<AXBCharacterBase>> OverlappingLeaders;

public:
    /**
     * @brief 存档排除标记
     * @note 为 true 时，存档系统将跳过此 Actor 的保存/读取/删除操作
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "存档",
              meta = (DisplayName = "排除存档"))
    bool bExcludeFromSave = false;

    /**
     * @brief 检查是否应该排除存档
     * @return 是否排除
     */
    UFUNCTION(BlueprintPure, Category = "存档",
              meta = (DisplayName = "是否排除存档"))
    bool ShouldExcludeFromSave() const { return bExcludeFromSave; }
};
