/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Army/XBSoldierRenderer.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Army/XBSoldierTypes.h"
#include "XBSoldierRenderer.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * @brief VAT 动画数据结构
 */
USTRUCT(BlueprintType)
struct FXBVATAnimationData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "动画ID"))
    int32 AnimationId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "总帧数"))
    int32 FrameCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "帧率"))
    float FrameRate = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "循环播放"))
    bool bLoop = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "起始帧"))
    int32 StartFrame = 0;
};

/**
 * @brief 士兵渲染器 - 使用稀疏索引池管理 HISM 实例
 * @note 核心优化：
 *       1. 引入空闲索引池，避免 RemoveInstance 导致的索引移位
 *       2. 移除时将索引标记为空闲，下次添加时优先复用
 *       3. 定期执行碎片整理（Defragmentation）
 */
UCLASS(BlueprintType)
class XIAOBINDATIANXIA_API UXBSoldierRenderer : public UObject
{
    GENERATED_BODY()

public:
    UXBSoldierRenderer();

    void Initialize(UWorld* InWorld);
    void Cleanup();

    // ============ 实例管理接口 ============

    int32 AddInstance(int32 SoldierId, const FVector& Location);
    int32 AddInstanceWithType(int32 SoldierId, const FVector& Location, EXBSoldierType Type);
    void RemoveInstance(int32 SoldierId);

    // ============ 批量更新接口 ============

    void UpdateInstancesFromData(const TMap<int32, FXBSoldierData>& SoldierMap);

    // ============ 资源设置接口 ============

    void SetMeshForType(EXBSoldierType SoldierType, UStaticMesh* Mesh, UMaterialInterface* Material = nullptr);
    void SetVATMaterial(UMaterialInterface* Material);

    UFUNCTION(BlueprintCallable, Category = "XB|Renderer")
    void SetSoldierMesh(EXBSoldierType Type, UStaticMesh* Mesh);

    // ✨ 新增 - 碎片整理接口（在性能允许时调用）
    /**
     * @brief 整理 HISM 实例索引，消除碎片
     * @param SoldierType 要整理的兵种类型
     * @note 该操作开销较大，建议在关卡切换或过场时调用
     */
    UFUNCTION(BlueprintCallable, Category = "XB|Renderer")
    void DefragmentInstances(EXBSoldierType SoldierType);

    // ============ 可见性与状态接口 ============

    void SetVisibilityForLeader(AActor* Leader, bool bVisible);
    void UpdateMeshForLeader(AActor* Leader, EXBSoldierType NewType);

protected:
    UHierarchicalInstancedStaticMeshComponent* GetOrCreateHISM(EXBSoldierType SoldierType);
    void AddInstanceForSoldier(const FXBSoldierData& Soldier);
    void UpdateInstanceTransform(int32 SoldierId, const FTransform& NewTransform);
    void UpdateInstanceCustomData(UHierarchicalInstancedStaticMeshComponent* HISM, int32 InstanceIndex, const FXBSoldierData& Soldier);

    // ✨ 新增 - 获取或复用空闲索引
    /**
     * @brief 从空闲池获取索引，如果池空则创建新实例
     * @param Type 兵种类型
     * @param Transform 初始变换
     * @return 实例索引
     */
    int32 AcquireInstanceIndex(EXBSoldierType Type, const FTransform& Transform);

    // ✨ 新增 - 回收索引到空闲池
    /**
     * @brief 将索引标记为空闲，不立即从 HISM 移除
     * @param Type 兵种类型
     * @param InstanceIndex 实例索引
     */
    void ReleaseInstanceIndex(EXBSoldierType Type, int32 InstanceIndex);

protected:
    UPROPERTY()
    TWeakObjectPtr<UWorld> WorldRef;

    UPROPERTY()
    TMap<EXBSoldierType, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> HISMComponents;

    UPROPERTY()
    TMap<EXBSoldierType, TObjectPtr<UStaticMesh>> MeshAssets;

    UPROPERTY()
    TArray<FXBVATAnimationData> AnimationData;
    
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> VATMaterialInstance;

    // ============ 索引映射 ============
    
    // 🔧 修改 - 简化为单一映射，移除冗余的反向映射
    /**
     * @brief 士兵ID -> (兵种类型, HISM实例索引) 映射
     * @note 单一数据源，避免多映射表不同步问题
     */
    TMap<int32, TPair<EXBSoldierType, int32>> SoldierIdToInstance;

    // ✨ 新增 - 空闲索引池
    /**
     * @brief 每个兵种类型的空闲索引池
     * @note 移除实例时将索引加入池中，添加实例时优先从池中取
     */
    TMap<EXBSoldierType, TArray<int32>> FreeInstanceIndices;
};
