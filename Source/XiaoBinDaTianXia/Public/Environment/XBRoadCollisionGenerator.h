// Source/XiaoBinDaTianXia/Public/Environment/XBRoadCollisionGenerator.h

/**
 * @file XBRoadCollisionGenerator.h
 * @brief 道路碰撞生成器 - 沿样条线两侧生成碰撞墙
 *
 * @note 使用方法:
 *       1. 将此 Actor 拖入场景
 *       2. 编辑 Spline 组件的点，沿道路绘制路径
 *       3. 调整参数（宽度、间距、高度等）
 *       4. 点击 "生成碰撞" 按钮生成碰撞体
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "XBRoadCollisionGenerator.generated.h"

class USplineComponent;
class UBoxComponent;
class UInstancedStaticMeshComponent;

/**
 * @brief 道路碰撞生成器
 * @note 基于样条线在道路两侧自动生成碰撞墙
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "XB Road Collision Generator"))
class XIAOBINDATIANXIA_API AXBRoadCollisionGenerator : public AActor
{
    GENERATED_BODY()

public:
    AXBRoadCollisionGenerator();

    // ============ 核心组件 ============

    /** 样条线组件 - 定义道路中心线 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "样条线"))
    TObjectPtr<USplineComponent> SplineComponent;

    /** 预览网格实例组件 - 可视化碰撞位置 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "预览网格"))
    TObjectPtr<UInstancedStaticMeshComponent> PreviewMeshComponent;

    // ============ 生成参数 ============

    /** 碰撞体间距（沿样条线方向） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "生成参数", meta = (DisplayName = "碰撞间距", ClampMin = "10.0", ClampMax = "1000.0"))
    float CollisionSpacing = 100.0f;

    /** 道路半宽（碰撞体距中心线的距离） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "生成参数", meta = (DisplayName = "道路半宽", ClampMin = "10.0"))
    float RoadHalfWidth = 200.0f;

    /** 碰撞体高度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "生成参数", meta = (DisplayName = "碰撞高度", ClampMin = "10.0"))
    float CollisionHeight = 200.0f;

    /** 碰撞体厚度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "生成参数", meta = (DisplayName = "碰撞厚度", ClampMin = "10.0"))
    float CollisionThickness = 50.0f;

    /** 碰撞体长度（沿样条线方向） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "生成参数", meta = (DisplayName = "碰撞长度", ClampMin = "10.0"))
    float CollisionLength = 100.0f;

    /** 碰撞体垂直偏移（正值向上） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "生成参数", meta = (DisplayName = "垂直偏移"))
    float VerticalOffset = 0.0f;

    // ============ 生成选项 ============

    /** 是否生成左侧碰撞 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "生成选项", meta = (DisplayName = "生成左侧"))
    bool bGenerateLeftSide = false;

    /** 是否生成右侧碰撞 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "生成选项", meta = (DisplayName = "生成右侧"))
    bool bGenerateRightSide = true;

    /** 是否显示预览网格 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "生成选项", meta = (DisplayName = "显示预览"))
    bool bShowPreview = true;

    /** 是否在编辑时自动生成碰撞（拖拽样条线实时更新） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "生成选项", meta = (DisplayName = "自动生成碰撞"))
    bool bAutoGenerateCollision = true;

    /** 预览网格资源 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "生成选项", meta = (DisplayName = "预览网格资源"))
    TObjectPtr<UStaticMesh> PreviewMesh;

    // ============ 碰撞配置 ============

    /** 碰撞预设名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "碰撞配置", meta = (DisplayName = "碰撞预设"))
    FName CollisionProfileName = FName("BlockAll");

    /** 是否阻挡投射物 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "碰撞配置", meta = (DisplayName = "阻挡投射物"))
    bool bBlockProjectiles = true;

    // ============ 功能接口 ============

    /**
     * @brief 生成碰撞体
     * @note 清除现有碰撞后重新生成
     */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "道路碰撞", meta = (DisplayName = "生成碰撞"))
    void GenerateCollision();

    /**
     * @brief 清除所有已生成的碰撞体
     */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "道路碰撞", meta = (DisplayName = "清除碰撞"))
    void ClearCollision();

    /**
     * @brief 更新预览显示
     */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "道路碰撞", meta = (DisplayName = "更新预览"))
    void UpdatePreview();

    /**
     * @brief 获取已生成的碰撞体数量
     */
    UFUNCTION(BlueprintPure, Category = "道路碰撞", meta = (DisplayName = "获取碰撞数量"))
    int32 GetCollisionCount() const { return GeneratedCollisions.Num(); }

protected:
    virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    /** 已生成的碰撞组件列表 */
    UPROPERTY()
    TArray<TObjectPtr<UBoxComponent>> GeneratedCollisions;

    /**
     * @brief 在指定位置创建碰撞盒
     * @param Location 世界位置
     * @param Tangent 样条线切线方向
     * @param bIsLeftSide 是否为左侧
     */
    void CreateCollisionBox(const FVector& Location, const FVector& Tangent, bool bIsLeftSide);

    /**
     * @brief 计算采样点信息
     * @param Distance 沿样条线的距离
     * @param OutLocation 输出位置
     * @param OutTangent 输出切线
     * @param OutRight 输出右向量
     */
    void GetSplinePointInfo(float Distance, FVector& OutLocation, FVector& OutTangent, FVector& OutRight) const;
};
