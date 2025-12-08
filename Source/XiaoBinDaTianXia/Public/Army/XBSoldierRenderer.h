// Source/XiaoBinDaTianXia/Public/Army/XBSoldierRenderer.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Army/XBSoldierTypes.h"
#include "XBSoldierRenderer.generated.h"

// 前向声明，减少编译依赖
class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * @brief VAT (Vertex Animation Texture) 动画数据结构
 * 用于驱动 HISM 实例的动画播放
 */
USTRUCT(BlueprintType)
struct FXBVATAnimationData
{
    GENERATED_BODY()

    // 动画ID，对应材质中的行号或索引
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "动画ID"))
    int32 AnimationId = 0;

    // 动画总帧数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "总帧数"))
    int32 FrameCount = 0;

    // 播放帧率
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "帧率"))
    float FrameRate = 30.0f;

    // 是否循环播放
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "循环播放"))
    bool bLoop = true;

    // 起始帧偏移
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "起始帧"))
    int32 StartFrame = 0;
};

/**
 * @brief 士兵渲染器 (Soldier Renderer)
 * @details 负责管理海量士兵的视觉表现。
 * 核心技术：使用 HISM (Hierarchical Instanced Static Mesh) 配合 VAT 技术实现高性能万人同屏。
 * 职责：
 * 1. 维护士兵ID到HISM实例索引的映射。
 * 2. 根据士兵状态更新位置、旋转和动画参数(CustomData)。
 * 3. 管理不同兵种的网格体资源。
 */
UCLASS(BlueprintType)
class XIAOBINDATIANXIA_API UXBSoldierRenderer : public UObject
{
    GENERATED_BODY()

public:
    /**
     * @brief 构造函数
     */
    UXBSoldierRenderer();

    /**
     * @brief 初始化渲染器
     * @param InWorld 游戏世界上下文
     */
    void Initialize(UWorld* InWorld);

    /**
     * @brief 清理资源
     * 销毁所有组件和清空映射
     */
    void Cleanup();

    // ============ 实例管理接口 ============

    /**
     * @brief 添加一个士兵实例（默认步兵）
     * @param SoldierId 士兵唯一ID
     * @param Location 初始位置
     * @return 实例索引
     */
    int32 AddInstance(int32 SoldierId, const FVector& Location);

    /**
     * @brief 添加指定类型的士兵实例
     * @param SoldierId 士兵唯一ID
     * @param Location 初始位置
     * @param Type 兵种类型
     * @return 实例索引
     */
    int32 AddInstanceWithType(int32 SoldierId, const FVector& Location, EXBSoldierType Type);

    /**
     * @brief 移除士兵实例
     * @param SoldierId 士兵唯一ID
     */
    void RemoveInstance(int32 SoldierId);

    // ============ 批量更新接口 ============

    /**
     * @brief 根据数据批量更新所有实例
     * @param SoldierMap 士兵数据映射表
     * 这是每帧调用的核心函数，负责同步逻辑数据到渲染层
     */
    void UpdateInstancesFromData(const TMap<int32, FXBSoldierData>& SoldierMap);

    // ============ 资源设置接口 ============

    /**
     * @brief 为特定兵种设置网格体
     */
    void SetMeshForType(EXBSoldierType SoldierType, UStaticMesh* Mesh, UMaterialInterface* Material = nullptr);

    /**
     * @brief 设置全局 VAT 材质（用于创建动态实例）
     */
    void SetVATMaterial(UMaterialInterface* Material);

    /**
     * @brief 设置士兵网格体（蓝图封装版）
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Renderer")
    void SetSoldierMesh(EXBSoldierType Type, UStaticMesh* Mesh);

    // ============ 可见性与状态接口 ============

    /**
     * @brief 设置将领麾下士兵的可见性
     * @param Leader 将领Actor
     * @param bVisible 是否可见
     */
    void SetVisibilityForLeader(AActor* Leader, bool bVisible);

    /**
     * @brief 更新将领麾下士兵的网格体类型
     */
    void UpdateMeshForLeader(AActor* Leader, EXBSoldierType NewType);

protected:
    // ============ 内部辅助函数 ============

    /**
     * @brief 获取或创建对应兵种的 HISM 组件
     */
    UHierarchicalInstancedStaticMeshComponent* GetOrCreateHISM(EXBSoldierType SoldierType);

    /**
     * @brief 为士兵数据添加实例（内部调用）
     */
    void AddInstanceForSoldier(const FXBSoldierData& Soldier);

    /**
     * @brief 更新实例变换
     */
    void UpdateInstanceTransform(int32 SoldierId, const FTransform& NewTransform);
    
    /**
     * @brief 更新实例自定义数据（用于驱动 VAT 动画）
     * CustomData 0: 动画时间/帧偏移
     * CustomData 1: 动画状态ID
     * CustomData 2: 播放速率
     */
    void UpdateInstanceCustomData(UHierarchicalInstancedStaticMeshComponent* HISM, int32 InstanceIndex, const FXBSoldierData& Soldier);

protected:
    // 弱引用持有世界上下文
    UPROPERTY()
    TWeakObjectPtr<UWorld> WorldRef;

    // 兵种类型到 HISM 组件的映射
    UPROPERTY()
    TMap<EXBSoldierType, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> HISMComponents;

    // 兵种类型到静态网格体资源的映射
    UPROPERTY()
    TMap<EXBSoldierType, TObjectPtr<UStaticMesh>> MeshAssets;

    // 动画配置数据
    UPROPERTY()
    TArray<FXBVATAnimationData> AnimationData;
    
    // 全局 VAT 动态材质实例
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> VATMaterialInstance;

    // ============ 索引映射 ============
    
    // 士兵ID -> HISM 实例索引
    TMap<int32, int32> SoldierIdToInstanceIndex;
    
    // HISM 实例索引 -> 士兵ID (用于反向查找)
    TMap<int32, int32> InstanceIndexToSoldierId;
    
    // 士兵ID -> 兵种类型 (用于快速查找对应的 HISM)
    TMap<int32, EXBSoldierType> SoldierIdToType;
};