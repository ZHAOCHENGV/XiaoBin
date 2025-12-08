// Source/XiaoBinDaTianXia/Private/Army/XBSoldierRenderer.cpp

#include "Army/XBSoldierRenderer.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

// 构造函数
UXBSoldierRenderer::UXBSoldierRenderer()
{
    // 初始化默认动画数据 (示例数据)
    FXBVATAnimationData IdleAnim;
    IdleAnim.AnimationId = 0;
    IdleAnim.FrameCount = 30;
    AnimationData.Add(IdleAnim);
    
    FXBVATAnimationData WalkAnim;
    WalkAnim.AnimationId = 1;
    WalkAnim.FrameCount = 30;
    AnimationData.Add(WalkAnim);
}

// 初始化
void UXBSoldierRenderer::Initialize(UWorld* InWorld)
{
    WorldRef = InWorld;
    UE_LOG(LogTemp, Log, TEXT("士兵渲染器已初始化"));
}

// 清理资源
void UXBSoldierRenderer::Cleanup()
{
    // 遍历并清理所有 HISM 组件
    for (auto& Pair : HISMComponents)
    {
        if (Pair.Value)
        {
            Pair.Value->ClearInstances();
            Pair.Value->DestroyComponent();
        }
    }
    
    // 清空容器
    HISMComponents.Empty();
    SoldierIdToInstanceIndex.Empty();
    InstanceIndexToSoldierId.Empty();
    SoldierIdToType.Empty();
    
    UE_LOG(LogTemp, Log, TEXT("士兵渲染器资源已清理"));
}

// 添加实例（默认步兵）
int32 UXBSoldierRenderer::AddInstance(int32 SoldierId, const FVector& Location)
{
    // 调用带类型的添加函数
    return AddInstanceWithType(SoldierId, Location, EXBSoldierType::Infantry);
}

// 添加指定类型的实例
int32 UXBSoldierRenderer::AddInstanceWithType(int32 SoldierId, const FVector& Location, EXBSoldierType Type)
{
    // 获取或创建 HISM 组件
    UHierarchicalInstancedStaticMeshComponent* HISM = GetOrCreateHISM(Type);
    if (!HISM)
    {
        UE_LOG(LogTemp, Warning, TEXT("无法为士兵 %d 创建 HISM 组件，类型: %d"), SoldierId, (int32)Type);
        return INDEX_NONE;
    }

    // 创建初始变换
    FTransform InstanceTransform(FRotator::ZeroRotator, Location, FVector::OneVector);
    
    // 添加实例并获得索引
    int32 InstanceIndex = HISM->AddInstance(InstanceTransform, true);

    // 记录映射关系
    SoldierIdToInstanceIndex.Add(SoldierId, InstanceIndex);
    InstanceIndexToSoldierId.Add(InstanceIndex, SoldierId);
    SoldierIdToType.Add(SoldierId, Type);

    // 初始化自定义数据 (Custom Data 0-2)
    if (HISM->NumCustomDataFloats >= 3)
    {
        HISM->SetCustomDataValue(InstanceIndex, 0, 0.0f, false); // 动画时间
        HISM->SetCustomDataValue(InstanceIndex, 1, 0.0f, false); // 动画ID
        HISM->SetCustomDataValue(InstanceIndex, 2, 1.0f, false); // 播放速率
    }

    return InstanceIndex;
}

// 移除实例
void UXBSoldierRenderer::RemoveInstance(int32 SoldierId)
{
    // 检查是否存在
    if (!SoldierIdToInstanceIndex.Contains(SoldierId))
    {
        return;
    }
    
    int32 InstanceIndex = SoldierIdToInstanceIndex[SoldierId];
    EXBSoldierType Type = SoldierIdToType[SoldierId];
    
    if (UHierarchicalInstancedStaticMeshComponent* HISM = HISMComponents.FindRef(Type))
    {
        HISM->RemoveInstance(InstanceIndex);
    }
    
    // 移除映射
    SoldierIdToInstanceIndex.Remove(SoldierId);
    InstanceIndexToSoldierId.Remove(InstanceIndex);
    SoldierIdToType.Remove(SoldierId);
    
    // 注意：HISM RemoveInstance 会导致后续索引移动，实际项目中需要更复杂的索引重映射逻辑
    // 这里为了简化暂不处理索引移位问题，仅作演示
}

// 批量更新实例
void UXBSoldierRenderer::UpdateInstancesFromData(const TMap<int32, FXBSoldierData>& SoldierMap)
{
    // 1. 遍历逻辑层数据，更新渲染层
    for (const auto& Pair : SoldierMap)
    {
        const FXBSoldierData& Soldier = Pair.Value;
        
        // 跳过死亡单位
        if (!Soldier.IsAlive())
        {
            continue;
        }
        
        // 如果是新兵，先添加
        if (!SoldierIdToInstanceIndex.Contains(Soldier.SoldierId))
        {
            AddInstanceForSoldier(Soldier);
        }

        // 更新变换
        if (int32* IdxPtr = SoldierIdToInstanceIndex.Find(Soldier.SoldierId))
        {
            int32 InstanceIndex = *IdxPtr;
            EXBSoldierType Type = SoldierIdToType[Soldier.SoldierId];
            
            if (UHierarchicalInstancedStaticMeshComponent* HISM = HISMComponents.FindRef(Type))
            {
                FTransform NewTransform(Soldier.Rotation, Soldier.Position, FVector::OneVector);
                // 更新变换，只标记脏，不立即重建树
                HISM->UpdateInstanceTransform(InstanceIndex, NewTransform, true, true, false);
                
                // 更新动画状态
                UpdateInstanceCustomData(HISM, InstanceIndex, Soldier);
            }
        }
    }
    
    // 2. 标记渲染状态脏，触发批量渲染更新
    // 🔧 修正：之前错误使用了 MeshComponents，现更正为 HISMComponents
    for (auto& Pair : HISMComponents)
    {
        if (Pair.Value)
        {
            Pair.Value->MarkRenderStateDirty();
        }
    }
}

// 设置特定兵种的网格体
void UXBSoldierRenderer::SetMeshForType(EXBSoldierType SoldierType, UStaticMesh* Mesh, UMaterialInterface* Material)
{
    if (Mesh)
    {
        MeshAssets.Add(SoldierType, Mesh);
        
        // 如果组件已存在，立即更新
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

// 设置士兵网格体（蓝图调用）
void UXBSoldierRenderer::SetSoldierMesh(EXBSoldierType Type, UStaticMesh* Mesh)
{
    SetMeshForType(Type, Mesh, nullptr);
}

// 设置全局 VAT 材质
void UXBSoldierRenderer::SetVATMaterial(UMaterialInterface* Material)
{
    if (!Material) return;

    // 创建动态材质实例
    VATMaterialInstance = UMaterialInstanceDynamic::Create(Material, this);
    
    // 应用到所有已存在的 HISM
    for (auto& Pair : HISMComponents)
    {
        if (Pair.Value) 
        {
            Pair.Value->SetMaterial(0, VATMaterialInstance);
        }
    }
}

// 获取或创建 HISM 组件
UHierarchicalInstancedStaticMeshComponent* UXBSoldierRenderer::GetOrCreateHISM(EXBSoldierType SoldierType)
{
    // 如果已存在直接返回
    if (HISMComponents.Contains(SoldierType))
    {
        return HISMComponents[SoldierType];
    }
    
    UWorld* World = WorldRef.Get();
    if (!World) return nullptr;
    
    // 创建一个新的 Actor 来承载 HISM（防止拥挤在一个 Actor 上）
    AActor* HISMOwner = World->SpawnActor<AActor>();
    if (!HISMOwner) return nullptr;
    
#if WITH_EDITOR
    HISMOwner->SetActorLabel(FString::Printf(TEXT("HISM_Soldier_%d"), (int32)SoldierType));
#endif

    // 创建组件
    UHierarchicalInstancedStaticMeshComponent* NewHISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(HISMOwner);
    NewHISM->RegisterComponent();
    NewHISM->SetMobility(EComponentMobility::Movable);
    
    // 关键：分配自定义数据槽位（用于 VAT）
    NewHISM->SetNumCustomDataFloats(3); 
    
    // 如果有预设的网格体，应用它
    if (MeshAssets.Contains(SoldierType))
    {
        NewHISM->SetStaticMesh(MeshAssets[SoldierType]);
    }
    
    // 如果有预设的材质，应用它
    if (VATMaterialInstance)
    {
        NewHISM->SetMaterial(0, VATMaterialInstance);
    }
    
    // 存入映射
    HISMComponents.Add(SoldierType, NewHISM);
    
    return NewHISM;
}

// 为士兵添加实例（内部辅助）
void UXBSoldierRenderer::AddInstanceForSoldier(const FXBSoldierData& Soldier)
{
    AddInstanceWithType(Soldier.SoldierId, Soldier.Position, Soldier.SoldierType);
}

// 更新实例变换（内部辅助）
void UXBSoldierRenderer::UpdateInstanceTransform(int32 SoldierId, const FTransform& NewTransform)
{
    if (int32* IdxPtr = SoldierIdToInstanceIndex.Find(SoldierId))
    {
        EXBSoldierType Type = SoldierIdToType[SoldierId];
        if (UHierarchicalInstancedStaticMeshComponent* HISM = HISMComponents.FindRef(Type))
        {
            HISM->UpdateInstanceTransform(*IdxPtr, NewTransform, true, true);
        }
    }
}

// 更新自定义数据（VAT 动画核心）
void UXBSoldierRenderer::UpdateInstanceCustomData(UHierarchicalInstancedStaticMeshComponent* HISM, int32 InstanceIndex, const FXBSoldierData& Soldier)
{
    if (!HISM || HISM->NumCustomDataFloats < 3) return;

    // 简单的状态映射逻辑
    float AnimId = 0.0f; // 默认 Idle
    
    // 🔧 修正：显式处理枚举比较
    if (Soldier.State == EXBSoldierState::Following) 
    {
        AnimId = 1.0f; // Walk
    }
    else if (Soldier.State == EXBSoldierState::Combat) 
    {
        AnimId = 2.0f; // Attack
    }
    else if (Soldier.State == EXBSoldierState::Dead) 
    {
        AnimId = 3.0f; // Death
    }
    
    // 冲刺时播放速度加快
    float PlayRate = Soldier.bIsSprinting ? 1.5f : 1.0f;

    // 设置数据到 GPU
    HISM->SetCustomDataValue(InstanceIndex, 0, 0.0f, false); // Time (这里暂时传0，实际需要传入 GameTime)
    HISM->SetCustomDataValue(InstanceIndex, 1, AnimId, false);
    HISM->SetCustomDataValue(InstanceIndex, 2, PlayRate, false);
}

// 设置将领士兵可见性（空实现，防止链接错误）
void UXBSoldierRenderer::SetVisibilityForLeader(AActor* Leader, bool bVisible) 
{
    // TODO: 实现逻辑 - 遍历该 Leader 下属的所有小兵，将 Scale 设置为 0 或 1
}

// 更新将领士兵网格（空实现，防止链接错误）
void UXBSoldierRenderer::UpdateMeshForLeader(AActor* Leader, EXBSoldierType NewType) 
{
    // TODO: 实现逻辑 - 将实例从旧 HISM 移除，添加到新 HISM
}