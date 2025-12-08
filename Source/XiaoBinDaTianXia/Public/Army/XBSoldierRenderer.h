#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Army/XBSoldierTypes.h"
#include "XBSoldierRenderer.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

UCLASS(BlueprintType)
class XIAOBINDATIANXIA_API UXBSoldierRenderer : public UObject
{
    GENERATED_BODY()

public:
    UXBSoldierRenderer();

    // 初始化
    void Initialize(UWorld* InWorld);
    void Cleanup();

    // 更新实例 - 使用 FXBSoldierData
    void UpdateInstancesFromData(const TMap<int32, FXBSoldierData>& SoldierMap);

    // 设置网格体
    void SetMeshForType(EXBSoldierType SoldierType, UStaticMesh* Mesh, UMaterialInterface* Material = nullptr);

protected:
    UHierarchicalInstancedStaticMeshComponent* GetOrCreateHISM(EXBSoldierType SoldierType);
    void AddInstanceForSoldier(const FXBSoldierData& Soldier);
    void UpdateInstanceTransform(int32 SoldierId, const FTransform& NewTransform);
    void RemoveInstance(int32 SoldierId);

protected:
    UPROPERTY()
    TWeakObjectPtr<UWorld> WorldRef;

    UPROPERTY()
    TMap<EXBSoldierType, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> HISMComponents;

    UPROPERTY()
    TMap<EXBSoldierType, TObjectPtr<UStaticMesh>> MeshAssets;

    // 小兵ID到实例索引的映射
    TMap<int32, int32> SoldierIdToInstanceIndex;
    TMap<int32, int32> InstanceIndexToSoldierId;

    // 小兵ID到类型的映射
    TMap<int32, EXBSoldierType> SoldierIdToType;
};
