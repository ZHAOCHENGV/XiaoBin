// XBSoldierRenderer.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Army/XBSoldierTypes.h"
#include "XBSoldierRenderer.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;

USTRUCT(BlueprintType)
struct FXBVATAnimationData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 AnimationId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 FrameCount = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FrameRate = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bLoop = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 StartFrame = 0;
};

UCLASS(BlueprintType)
class XIAOBINDATIANXIA_API UXBSoldierRenderer : public UObject
{
    GENERATED_BODY()

public:
    UXBSoldierRenderer();

    /** 初始化渲染器 */
    UFUNCTION(BlueprintCallable, Category = "XB|Renderer")
    void Initialize(UWorld* InWorld);

    /** 清理渲染器 */
    UFUNCTION(BlueprintCallable, Category = "XB|Renderer")
    void Cleanup();

    /** 添加实例 */
    UFUNCTION(BlueprintCallable, Category = "XB|Renderer")
    int32 AddInstance(int32 SoldierId, const FVector& Location);

    /** 移除实例 */
    UFUNCTION(BlueprintCallable, Category = "XB|Renderer")
    void RemoveInstance(int32 SoldierId);

    /** 
     * 批量更新实例变换
     * TMap<int32, FXBSoldierAgent> 不支持蓝图，所以移除 UFUNCTION
     */
    void UpdateInstances(const TMap<int32, FXBSoldierAgent>& SoldierMap);  // 移除 UFUNCTION

    /** 设置将领士兵的可见性 */
    UFUNCTION(BlueprintCallable, Category = "XB|Renderer")
    void SetVisibilityForLeader(AActor* Leader, bool bVisible);

    /** 更新将领士兵的网格（兵种变化） */
    UFUNCTION(BlueprintCallable, Category = "XB|Renderer")
    void UpdateMeshForLeader(AActor* Leader, EXBSoldierType NewType);

    /** 设置兵种网格 */
    UFUNCTION(BlueprintCallable, Category = "XB|Renderer")
    void SetSoldierMesh(EXBSoldierType Type, UStaticMesh* Mesh);

    /** 设置 VAT 材质 */
    UFUNCTION(BlueprintCallable, Category = "XB|Renderer")
    void SetVATMaterial(UMaterialInterface* Material);

protected:
    UPROPERTY()
    TMap<EXBSoldierType, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> MeshComponents;

    TMap<int32, int32> SoldierIdToInstanceIndex;
    TMap<int32, int32> InstanceIndexToSoldierId;

    UPROPERTY()
    TArray<FXBVATAnimationData> AnimationData;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> VATMaterialInstance;

    UPROPERTY()
    TWeakObjectPtr<UWorld> OwningWorld;

    UPROPERTY()
    TObjectPtr<AActor> HostActor;

private:
    UHierarchicalInstancedStaticMeshComponent* GetOrCreateHISM(EXBSoldierType Type);
    void UpdateInstanceCustomData(UHierarchicalInstancedStaticMeshComponent* HISM, int32 InstanceIndex, const FXBSoldierAgent& Soldier);
    
    // 辅助函数
    int32 AddInstanceWithType(int32 SoldierId, const FVector& Location, EXBSoldierType Type);
};