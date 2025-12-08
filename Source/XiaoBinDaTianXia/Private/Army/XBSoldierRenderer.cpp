// Copyright XiaoBing Project. All Rights Reserved.

#include "Army/XBSoldierRenderer.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

UXBSoldierRenderer::UXBSoldierRenderer()
{
    // 初始化默认动画数据
    FXBVATAnimationData IdleAnim;
    IdleAnim.AnimationId = 0;
    IdleAnim.FrameCount = 30;
    IdleAnim.FrameRate = 30.0f;
    IdleAnim.bLoop = true;
    IdleAnim.StartFrame = 0;
    AnimationData.Add(IdleAnim);

    FXBVATAnimationData WalkAnim;
    WalkAnim.AnimationId = 1;
    WalkAnim.FrameCount = 24;
    WalkAnim.FrameRate = 30.0f;
    WalkAnim.bLoop = true;
    WalkAnim.StartFrame = 30;
    AnimationData.Add(WalkAnim);

    FXBVATAnimationData AttackAnim;
    AttackAnim.AnimationId = 2;
    AttackAnim.FrameCount = 20;
    AttackAnim.FrameRate = 30.0f;
    AttackAnim.bLoop = false;
    AttackAnim.StartFrame = 54;
    AnimationData.Add(AttackAnim);

    FXBVATAnimationData DeathAnim;
    DeathAnim.AnimationId = 3;
    DeathAnim.FrameCount = 30;
    DeathAnim.FrameRate = 30.0f;
    DeathAnim.bLoop = false;
    DeathAnim.StartFrame = 74;
    AnimationData.Add(DeathAnim);
}

void UXBSoldierRenderer::Initialize(UWorld* InWorld)
{
    if (!InWorld)
    {
        UE_LOG(LogTemp, Error, TEXT("XBSoldierRenderer::Initialize - Invalid World!"));
        return;
    }

    OwningWorld = InWorld;

    // 创建宿主 Actor 用于挂载 HISM 组件
    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = FName(TEXT("SoldierRendererHost"));
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    HostActor = InWorld->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    
    if (HostActor)
    {
        HostActor->SetRootComponent(NewObject<USceneComponent>(HostActor, TEXT("RootComponent")));
        HostActor->GetRootComponent()->RegisterComponent();
        
#if WITH_EDITOR
        HostActor->SetActorLabel(TEXT("SoldierRendererHost"));
#endif
    }

    UE_LOG(LogTemp, Log, TEXT("XBSoldierRenderer Initialized"));
}

void UXBSoldierRenderer::Cleanup()
{
    // 清理所有 HISM 组件
    for (auto& Pair : MeshComponents)
    {
        if (Pair.Value)
        {
            Pair.Value->ClearInstances();
            Pair.Value->DestroyComponent();
        }
    }
    MeshComponents.Empty();

    // 清理映射
    SoldierIdToInstanceIndex.Empty();
    InstanceIndexToSoldierId.Empty();

    // 销毁宿主 Actor
    if (HostActor)
    {
        HostActor->Destroy();
        HostActor = nullptr;
    }

    OwningWorld.Reset();

    UE_LOG(LogTemp, Log, TEXT("XBSoldierRenderer Cleaned up"));
}

int32 UXBSoldierRenderer::AddInstance(int32 SoldierId, const FVector& Location)
{
    // 默认使用步兵类型
    return AddInstanceWithType(SoldierId, Location, EXBSoldierType::Infantry);
}

int32 UXBSoldierRenderer::AddInstanceWithType(int32 SoldierId, const FVector& Location, EXBSoldierType Type)
{
    UHierarchicalInstancedStaticMeshComponent* HISM = GetOrCreateHISM(Type);
    if (!HISM)
    {
        UE_LOG(LogTemp, Warning, TEXT("XBSoldierRenderer::AddInstance - Failed to get HISM for type %d"), static_cast<int32>(Type));
        return INDEX_NONE;
    }

    // 创建变换
    FTransform InstanceTransform(FRotator::ZeroRotator, Location, FVector::OneVector);

    // 添加实例
    int32 InstanceIndex = HISM->AddInstance(InstanceTransform, true);

    // 记录映射
    SoldierIdToInstanceIndex.Add(SoldierId, InstanceIndex);
    InstanceIndexToSoldierId.Add(InstanceIndex, SoldierId);

    // 设置初始自定义数据（用于 VAT 动画）
    // CustomData[0] = AnimationTime
    // CustomData[1] = AnimationId
    // CustomData[2] = PlayRate
    if (HISM->NumCustomDataFloats >= 3)
    {
        HISM->SetCustomDataValue(InstanceIndex, 0, 0.0f);  // AnimationTime
        HISM->SetCustomDataValue(InstanceIndex, 1, 0.0f);  // AnimationId (Idle)
        HISM->SetCustomDataValue(InstanceIndex, 2, 1.0f);  // PlayRate
    }

    return InstanceIndex;
}

void UXBSoldierRenderer::RemoveInstance(int32 SoldierId)
{
    int32* InstanceIndexPtr = SoldierIdToInstanceIndex.Find(SoldierId);
    if (!InstanceIndexPtr)
    {
        return;
    }

    int32 InstanceIndex = *InstanceIndexPtr;

    // 查找该实例所属的 HISM
    for (auto& Pair : MeshComponents)
    {
        UHierarchicalInstancedStaticMeshComponent* HISM = Pair.Value;
        if (HISM && InstanceIndex < HISM->GetInstanceCount())
        {
            // 使用交换删除法移除实例
            // 注意：这会改变最后一个实例的索引
            if (HISM->RemoveInstance(InstanceIndex))
            {
                // 更新被交换实例的映射
                int32 LastIndex = HISM->GetInstanceCount();
                if (InstanceIndex != LastIndex)
                {
                    if (int32* SwappedSoldierId = InstanceIndexToSoldierId.Find(LastIndex))
                    {
                        SoldierIdToInstanceIndex[*SwappedSoldierId] = InstanceIndex;
                        InstanceIndexToSoldierId.Add(InstanceIndex, *SwappedSoldierId);
                    }
                }
            }
            break;
        }
    }

    // 清理映射
    SoldierIdToInstanceIndex.Remove(SoldierId);
    InstanceIndexToSoldierId.Remove(InstanceIndex);
}

void UXBSoldierRenderer::UpdateInstances(const TMap<int32, FXBSoldierAgent>& SoldierMap)
{
    // 批量更新所有实例的变换和自定义数据
    for (const auto& Pair : SoldierMap)
    {
        const int32 SoldierId = Pair.Key;
        const FXBSoldierAgent& Soldier = Pair.Value;

        int32* InstanceIndexPtr = SoldierIdToInstanceIndex.Find(SoldierId);
        if (!InstanceIndexPtr)
        {
            continue;
        }

        // 获取对应的 HISM
        UHierarchicalInstancedStaticMeshComponent* HISM = nullptr;
        for (auto& MeshPair : MeshComponents)
        {
            if (MeshPair.Key == Soldier.SoldierType)
            {
                HISM = MeshPair.Value;
                break;
            }
        }

        if (!HISM)
        {
            continue;
        }

        const int32 InstanceIndex = *InstanceIndexPtr;

        // 更新变换
        FTransform NewTransform(Soldier.Rotation, Soldier.Position, FVector::OneVector);
        HISM->UpdateInstanceTransform(InstanceIndex, NewTransform, true, false, false);

        // 更新自定义数据（VAT 动画参数）
        UpdateInstanceCustomData(HISM, InstanceIndex, Soldier);
    }

    // 标记需要重建渲染数据
    for (auto& Pair : MeshComponents)
    {
        if (Pair.Value)
        {
            Pair.Value->MarkRenderStateDirty();
        }
    }
}

void UXBSoldierRenderer::SetVisibilityForLeader(AActor* Leader, bool bVisible)
{
    // 这里需要从 ArmySubsystem 获取将领的士兵列表
    // 然后设置对应实例的可见性
    // 由于 HISM 不支持单实例可见性，我们使用缩放为 0 的方式模拟隐藏

    if (!Leader)
    {
        return;
    }

    // TODO: 实现隐身效果
    // 方案1：将实例缩放设为 0
    // 方案2：使用材质参数控制透明度
    // 方案3：移动到地下
}

void UXBSoldierRenderer::UpdateMeshForLeader(AActor* Leader, EXBSoldierType NewType)
{
    // 当士兵兵种变化时，需要将实例从旧 HISM 移动到新 HISM
    // 这是一个昂贵的操作，应该尽量避免频繁调用

    if (!Leader)
    {
        return;
    }

    // TODO: 实现兵种切换的网格更新
    // 1. 从 ArmySubsystem 获取将领的士兵列表
    // 2. 从旧 HISM 移除实例
    // 3. 添加到新 HISM
}

void UXBSoldierRenderer::SetSoldierMesh(EXBSoldierType Type, UStaticMesh* Mesh)
{
    if (!Mesh)
    {
        return;
    }

    UHierarchicalInstancedStaticMeshComponent* HISM = GetOrCreateHISM(Type);
    if (HISM)
    {
        HISM->SetStaticMesh(Mesh);
    }
}

void UXBSoldierRenderer::SetVATMaterial(UMaterialInterface* Material)
{
    if (!Material)
    {
        return;
    }

    // 创建动态材质实例
    VATMaterialInstance = UMaterialInstanceDynamic::Create(Material, this);

    // 应用到所有 HISM
    for (auto& Pair : MeshComponents)
    {
        if (Pair.Value)
        {
            Pair.Value->SetMaterial(0, VATMaterialInstance);
        }
    }
}

UHierarchicalInstancedStaticMeshComponent* UXBSoldierRenderer::GetOrCreateHISM(EXBSoldierType Type)
{
    // 检查是否已存在
    if (TObjectPtr<UHierarchicalInstancedStaticMeshComponent>* ExistingHISM = MeshComponents.Find(Type))
    {
        return *ExistingHISM;
    }

    // 创建新的 HISM
    if (!HostActor)
    {
        UE_LOG(LogTemp, Error, TEXT("XBSoldierRenderer::GetOrCreateHISM - HostActor is null!"));
        return nullptr;
    }

    FName ComponentName = *FString::Printf(TEXT("HISM_%d"), static_cast<int32>(Type));
    UHierarchicalInstancedStaticMeshComponent* NewHISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(
        HostActor, ComponentName);

    if (NewHISM)
    {
        NewHISM->SetupAttachment(HostActor->GetRootComponent());
        NewHISM->RegisterComponent();

        // 配置 HISM 优化参数
        NewHISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        NewHISM->SetCastShadow(true);
        NewHISM->bUseDefaultCollision = false;
        NewHISM->SetGenerateOverlapEvents(false);
        
        // VAT 需要的自定义数据数量
        NewHISM->NumCustomDataFloats = 4; // AnimTime, AnimId, PlayRate, Reserved

        // 启用 GPU 实例化
        NewHISM->bUseGpuLodSelection = true;

        // 添加到映射
        MeshComponents.Add(Type, NewHISM);

        // 如果有 VAT 材质，应用之
        if (VATMaterialInstance)
        {
            NewHISM->SetMaterial(0, VATMaterialInstance);
        }
    }

    return NewHISM;
}

void UXBSoldierRenderer::UpdateInstanceCustomData(UHierarchicalInstancedStaticMeshComponent* HISM, int32 InstanceIndex, const FXBSoldierAgent& Soldier)
{
    if (!HISM || HISM->NumCustomDataFloats < 3)
    {
        return;
    }

    // 根据士兵状态选择动画
    int32 AnimId = 0;
    float PlayRate = 1.0f;

    switch (Soldier.State)
    {
    case EXBSoldierState::Idle:
        AnimId = 0; // Idle
        break;
    case EXBSoldierState::Following:
    case EXBSoldierState::Returning:
        AnimId = 1; // Walk
        // 根据速度调整播放速率
        PlayRate = FMath::Clamp(Soldier.Velocity.Size() / 400.0f, 0.5f, 2.0f);
        break;
    case EXBSoldierState::Engaging:
    case EXBSoldierState::Seeking:
        // 如果在攻击冷却中，播放攻击动画
        if (Soldier.AttackCooldown > 0.5f)
        {
            AnimId = 2; // Attack
        }
        else
        {
            AnimId = 1; // Walk
        }
        break;
    case EXBSoldierState::Dead:
        AnimId = 3; // Death
        break;
    default:
        AnimId = 0;
        break;
    }

    // 设置自定义数据
    HISM->SetCustomDataValue(InstanceIndex, 0, Soldier.AnimationTime, false);
    HISM->SetCustomDataValue(InstanceIndex, 1, static_cast<float>(AnimId), false);
    HISM->SetCustomDataValue(InstanceIndex, 2, PlayRate, false);
}