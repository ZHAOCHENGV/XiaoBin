/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Army/XBSoldierRenderer.cpp

#include "Army/XBSoldierRenderer.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

UXBSoldierRenderer::UXBSoldierRenderer()
{
    FXBVATAnimationData IdleAnim;
    IdleAnim.AnimationId = 0;
    IdleAnim.FrameCount = 30;
    AnimationData.Add(IdleAnim);
    
    FXBVATAnimationData WalkAnim;
    WalkAnim.AnimationId = 1;
    WalkAnim.FrameCount = 30;
    AnimationData.Add(WalkAnim);
}

void UXBSoldierRenderer::Initialize(UWorld* InWorld)
{
    WorldRef = InWorld;
    UE_LOG(LogTemp, Log, TEXT("士兵渲染器已初始化"));
}

void UXBSoldierRenderer::Cleanup()
{
    for (auto& Pair : HISMComponents)
    {
        if (Pair.Value)
        {
            Pair.Value->ClearInstances();
            Pair.Value->DestroyComponent();
        }
    }
    
    HISMComponents.Empty();
    SoldierIdToInstance.Empty();
    FreeInstanceIndices.Empty();
    
    UE_LOG(LogTemp, Log, TEXT("士兵渲染器资源已清理"));
}

int32 UXBSoldierRenderer::AddInstance(int32 SoldierId, const FVector& Location)
{
    return AddInstanceWithType(SoldierId, Location, EXBSoldierType::Infantry);
}

int32 UXBSoldierRenderer::AddInstanceWithType(int32 SoldierId, const FVector& Location, EXBSoldierType Type)
{
    // 🔧 修改 - 使用新的索引分配机制
    FTransform InstanceTransform(FRotator::ZeroRotator, Location, FVector::OneVector);
    int32 InstanceIndex = AcquireInstanceIndex(Type, InstanceTransform);

    if (InstanceIndex == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("无法为士兵 %d 分配实例索引"), SoldierId);
        return INDEX_NONE;
    }

    // 🔧 修改 - 使用简化的单一映射表
    SoldierIdToInstance.Add(SoldierId, TPair<EXBSoldierType, int32>(Type, InstanceIndex));

    // 初始化 CustomData
    if (UHierarchicalInstancedStaticMeshComponent* HISM = HISMComponents.FindRef(Type))
    {
        if (HISM->NumCustomDataFloats >= 3)
        {
            HISM->SetCustomDataValue(InstanceIndex, 0, 0.0f, false);
            HISM->SetCustomDataValue(InstanceIndex, 1, 0.0f, false);
            HISM->SetCustomDataValue(InstanceIndex, 2, 1.0f, false);
        }
    }

    return InstanceIndex;
}

void UXBSoldierRenderer::RemoveInstance(int32 SoldierId)
{
    // 🔧 修改 - 使用新的索引回收机制
    TPair<EXBSoldierType, int32>* InstanceInfo = SoldierIdToInstance.Find(SoldierId);
    if (!InstanceInfo)
    {
        return;
    }

    EXBSoldierType Type = InstanceInfo->Key;
    int32 InstanceIndex = InstanceInfo->Value;

    // 回收索引到空闲池（不立即从 HISM 移除）
    ReleaseInstanceIndex(Type, InstanceIndex);

    // 移除映射
    SoldierIdToInstance.Remove(SoldierId);

    UE_LOG(LogTemp, Verbose, TEXT("士兵 %d 的实例索引 %d 已回收"), SoldierId, InstanceIndex);
}

void UXBSoldierRenderer::UpdateInstancesFromData(const TMap<int32, FXBSoldierData>& SoldierMap)
{
    for (const auto& Pair : SoldierMap)
    {
        const FXBSoldierData& Soldier = Pair.Value;
        
        if (!Soldier.IsAlive())
        {
            continue;
        }
        
        if (!SoldierIdToInstance.Contains(Soldier.SoldierId))
        {
            AddInstanceForSoldier(Soldier);
        }

        // 🔧 修改 - 使用新的映射表结构
        if (TPair<EXBSoldierType, int32>* InstanceInfo = SoldierIdToInstance.Find(Soldier.SoldierId))
        {
            int32 InstanceIndex = InstanceInfo->Value;
            EXBSoldierType Type = InstanceInfo->Key;
            
            if (UHierarchicalInstancedStaticMeshComponent* HISM = HISMComponents.FindRef(Type))
            {
                FTransform NewTransform(Soldier.Rotation, Soldier.Position, FVector::OneVector);
                HISM->UpdateInstanceTransform(InstanceIndex, NewTransform, true, true, false);
                UpdateInstanceCustomData(HISM, InstanceIndex, Soldier);
            }
        }
    }
    
    for (auto& Pair : HISMComponents)
    {
        if (Pair.Value)
        {
            Pair.Value->MarkRenderStateDirty();
        }
    }
}

void UXBSoldierRenderer::SetMeshForType(EXBSoldierType SoldierType, UStaticMesh* Mesh, UMaterialInterface* Material)
{
    if (Mesh)
    {
        MeshAssets.Add(SoldierType, Mesh);
        
        if (UHierarchicalInstancedStaticMeshComponent* HISM = GetOrCreateHISM(SoldierType))
        {
            HISM->SetStaticMesh(Mesh);
            if (Material) 
            {
                HISM->SetMaterial(0, Material);
            }
        }
    }
}

void UXBSoldierRenderer::SetSoldierMesh(EXBSoldierType Type, UStaticMesh* Mesh)
{
    SetMeshForType(Type, Mesh, nullptr);
}

void UXBSoldierRenderer::SetVATMaterial(UMaterialInterface* Material)
{
    if (!Material) return;

    VATMaterialInstance = UMaterialInstanceDynamic::Create(Material, this);
    
    for (auto& Pair : HISMComponents)
    {
        if (Pair.Value) 
        {
            Pair.Value->SetMaterial(0, VATMaterialInstance);
        }
    }
}

// ✨ 新增 - 索引分配函数
/**
 * @brief 从空闲池获取索引或创建新实例
 * @param Type 兵种类型
 * @param Transform 初始变换
 * @return 实例索引
 * @note 优先复用空闲索引，减少 HISM 实例数量增长
 */
int32 UXBSoldierRenderer::AcquireInstanceIndex(EXBSoldierType Type, const FTransform& Transform)
{
    UHierarchicalInstancedStaticMeshComponent* HISM = GetOrCreateHISM(Type);
    if (!HISM)
    {
        return INDEX_NONE;
    }

    // 检查是否有空闲索引可复用
    TArray<int32>& FreeIndices = FreeInstanceIndices.FindOrAdd(Type);
    if (FreeIndices.Num() > 0)
    {
        // 从池中取出最后一个索引（LIFO，缓存友好）
        int32 ReusedIndex = FreeIndices.Pop();
        
        // 更新该索引的变换（复用实例）
        HISM->UpdateInstanceTransform(ReusedIndex, Transform, true, true, false);
        
        UE_LOG(LogTemp, Verbose, TEXT("复用空闲索引 %d（兵种 %d）"), ReusedIndex, (int32)Type);
        return ReusedIndex;
    }

    // 没有空闲索引，创建新实例
    int32 NewIndex = HISM->AddInstance(Transform, true);
    UE_LOG(LogTemp, Verbose, TEXT("创建新实例索引 %d（兵种 %d）"), NewIndex, (int32)Type);
    return NewIndex;
}

// ✨ 新增 - 索引回收函数
/**
 * @brief 将索引标记为空闲
 * @param Type 兵种类型
 * @param InstanceIndex 实例索引
 * @note 不立即从 HISM 移除，避免索引移位
 */
void UXBSoldierRenderer::ReleaseInstanceIndex(EXBSoldierType Type, int32 InstanceIndex)
{
    if (UHierarchicalInstancedStaticMeshComponent* HISM = HISMComponents.FindRef(Type))
    {
        // 将实例移动到地下（隐藏但不删除）
        FTransform HiddenTransform(FRotator::ZeroRotator, FVector(0, 0, -100000), FVector::ZeroVector);
        HISM->UpdateInstanceTransform(InstanceIndex, HiddenTransform, true, true, false);
    }

    // 加入空闲池
    TArray<int32>& FreeIndices = FreeInstanceIndices.FindOrAdd(Type);
    FreeIndices.Add(InstanceIndex);
}

// ✨ 新增 - 碎片整理函数
/**
 * @brief 整理 HISM 实例，消除空洞
 * @param SoldierType 要整理的兵种类型
 * @note 该操作会重建 HISM，性能开销较大，建议在关卡切换时调用
 */
void UXBSoldierRenderer::DefragmentInstances(EXBSoldierType SoldierType)
{
    UHierarchicalInstancedStaticMeshComponent* HISM = HISMComponents.FindRef(SoldierType);
    if (!HISM)
    {
        return;
    }

    TArray<int32>& FreeIndices = FreeInstanceIndices.FindOrAdd(SoldierType);
    if (FreeIndices.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("兵种 %d 无需碎片整理"), (int32)SoldierType);
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("开始整理兵种 %d 的实例，空闲索引数: %d"), (int32)SoldierType, FreeIndices.Num());

    // 按索引降序排序，从后往前删除，避免索引移位影响
    FreeIndices.Sort([](int32 A, int32 B) { return A > B; });

    for (int32 FreeIndex : FreeIndices)
    {
        HISM->RemoveInstance(FreeIndex);
    }

    // 清空空闲池
    FreeIndices.Empty();

    // 🔧 修改 - 重建士兵ID到索引的映射
    // 因为删除操作会导致后续索引前移，需要重新计算
    TArray<int32> SoldierIdsToUpdate;
    for (auto& Pair : SoldierIdToInstance)
    {
        if (Pair.Value.Key == SoldierType)
        {
            SoldierIdsToUpdate.Add(Pair.Key);
        }
    }

    // 重新映射（假设 HISM 内部按顺序压缩了索引）
    // 注意：这里简化处理，实际项目中需要更复杂的索引重映射逻辑
    for (int32 SoldierId : SoldierIdsToUpdate)
    {
        // TODO: 实际项目中需要从 HISM 反查实例的新索引
        // 这里暂时标记为待重建
        SoldierIdToInstance.Remove(SoldierId);
    }

    HISM->MarkRenderStateDirty();
    UE_LOG(LogTemp, Warning, TEXT("兵种 %d 整理完成，需要重新添加 %d 个士兵"), (int32)SoldierType, SoldierIdsToUpdate.Num());
}

UHierarchicalInstancedStaticMeshComponent* UXBSoldierRenderer::GetOrCreateHISM(EXBSoldierType SoldierType)
{
    if (HISMComponents.Contains(SoldierType))
    {
        return HISMComponents[SoldierType];
    }
    
    UWorld* World = WorldRef.Get();
    if (!World) return nullptr;
    
    AActor* HISMOwner = World->SpawnActor<AActor>();
    if (!HISMOwner) return nullptr;
    
#if WITH_EDITOR
    HISMOwner->SetActorLabel(FString::Printf(TEXT("HISM_Soldier_%d"), (int32)SoldierType));
#endif

    UHierarchicalInstancedStaticMeshComponent* NewHISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(HISMOwner);
    NewHISM->RegisterComponent();
    NewHISM->SetMobility(EComponentMobility::Movable);
    NewHISM->SetNumCustomDataFloats(3);
    
    if (MeshAssets.Contains(SoldierType))
    {
        NewHISM->SetStaticMesh(MeshAssets[SoldierType]);
    }
    
    if (VATMaterialInstance)
    {
        NewHISM->SetMaterial(0, VATMaterialInstance);
    }
    
    HISMComponents.Add(SoldierType, NewHISM);
    
    return NewHISM;
}

void UXBSoldierRenderer::AddInstanceForSoldier(const FXBSoldierData& Soldier)
{
    AddInstanceWithType(Soldier.SoldierId, Soldier.Position, Soldier.SoldierType);
}

void UXBSoldierRenderer::UpdateInstanceTransform(int32 SoldierId, const FTransform& NewTransform)
{
    // 🔧 修改 - 使用新的映射表结构
    if (TPair<EXBSoldierType, int32>* InstanceInfo = SoldierIdToInstance.Find(SoldierId))
    {
        EXBSoldierType Type = InstanceInfo->Key;
        int32 InstanceIndex = InstanceInfo->Value;
        
        if (UHierarchicalInstancedStaticMeshComponent* HISM = HISMComponents.FindRef(Type))
        {
            HISM->UpdateInstanceTransform(InstanceIndex, NewTransform, true, true);
        }
    }
}

void UXBSoldierRenderer::UpdateInstanceCustomData(UHierarchicalInstancedStaticMeshComponent* HISM, int32 InstanceIndex, const FXBSoldierData& Soldier)
{
    if (!HISM || HISM->NumCustomDataFloats < 3) return;

    float AnimId = 0.0f;
    
    if (Soldier.State == EXBSoldierState::Following) 
    {
        AnimId = 1.0f;
    }
    else if (Soldier.State == EXBSoldierState::Combat) 
    {
        AnimId = 2.0f;
    }
    else if (Soldier.State == EXBSoldierState::Dead) 
    {
        AnimId = 3.0f;
    }
    
    float PlayRate = Soldier.bIsSprinting ? 1.5f : 1.0f;

    HISM->SetCustomDataValue(InstanceIndex, 0, 0.0f, false);
    HISM->SetCustomDataValue(InstanceIndex, 1, AnimId, false);
    HISM->SetCustomDataValue(InstanceIndex, 2, PlayRate, false);
}

void UXBSoldierRenderer::SetVisibilityForLeader(AActor* Leader, bool bVisible) 
{
    // TODO: 实现逻辑
}

void UXBSoldierRenderer::UpdateMeshForLeader(AActor* Leader, EXBSoldierType NewType) 
{
    // TODO: 实现逻辑
}
