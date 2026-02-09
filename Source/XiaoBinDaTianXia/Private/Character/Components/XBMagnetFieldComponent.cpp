/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Character/Components/XBMagnetFieldComponent.cpp

/**
 * @file XBMagnetFieldComponent.cpp
 * @brief 磁场组件实现 - 简化版招募系统
 * 
 * @note 🔧 修改记录:
 *       1. ❌ 删除 TryRecruitVillager 方法
 *       2. 🔧 修复 OnSphereBeginOverlap 招募逻辑
 *       3. ✨ 新增 详细的招募调试日志
 */

#include "Character/Components/XBMagnetFieldComponent.h"
#include "GameplayEffectTypes.h"
#include "Character/XBCharacterBase.h"
#include "Soldier/XBSoldierCharacter.h"
#include "GAS/XBAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "XBCollisionChannels.h"
#include "Data/XBSoldierDataTable.h"
#include "Engine/DataTable.h"
#include "DrawDebugHelpers.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

UXBMagnetFieldComponent::UXBMagnetFieldComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    bWantsInitializeComponent = true;

    SetGenerateOverlapEvents(false);
    InitSphereRadius(300.0f);
    SetHiddenInGame(true);
    // 贴花组件将在 BeginPlay 中动态创建
}

void UXBMagnetFieldComponent::InitializeComponent()
{
    Super::InitializeComponent();

    SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SetCollisionObjectType(ECC_WorldDynamic);
    SetCollisionResponseToAllChannels(ECR_Ignore);
    SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    SetCollisionResponseToChannel(XBCollision::Soldier, ECR_Overlap);
    SetCollisionResponseToChannel(XBCollision::Leader, ECR_Overlap);
    SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
    
    UE_LOG(LogTemp, Log, TEXT("磁场组件 %s: 碰撞配置完成"), 
        GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));
}

void UXBMagnetFieldComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!bOverlapEventsBound)
    {
        OnComponentBeginOverlap.AddDynamic(this, &UXBMagnetFieldComponent::OnSphereBeginOverlap);
        OnComponentEndOverlap.AddDynamic(this, &UXBMagnetFieldComponent::OnSphereEndOverlap);
        bOverlapEventsBound = true;
    }

    SetGenerateOverlapEvents(bIsFieldEnabled);

    // 动态创建范围贴花组件
    if (!RangeDecalComponent && RangeDecalMaterial)
    {
        RangeDecalComponent = NewObject<UDecalComponent>(GetOwner(), TEXT("RangeDecalComponent"));
        if (RangeDecalComponent)
        {
            RangeDecalComponent->SetupAttachment(this);
            RangeDecalComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
            RangeDecalComponent->SetVisibility(false);
            
            // ✨ 新增 - 提高贴花排序顺序，使其渲染在其他贴花之上
            RangeDecalComponent->SortOrder = 999;
            
            // 创建动态材质实例
            DecalMaterialInstance = UMaterialInstanceDynamic::Create(RangeDecalMaterial, this);
            if (DecalMaterialInstance)
            {
                // 如果使用随机颜色，生成随机颜色
                if (bUseRandomColor)
                {
                    DecalColor = FLinearColor::MakeRandomColor();
                    DecalColor.A = 1.0f;
                }
                DecalMaterialInstance->SetVectorParameterValue(TEXT("Color"), DecalColor);
                RangeDecalComponent->SetDecalMaterial(DecalMaterialInstance);
            }
            else
            {
                RangeDecalComponent->SetDecalMaterial(RangeDecalMaterial);
            }
            
            
            RangeDecalComponent->RegisterComponent();
            UpdateRangeDecalSize();
            
            UE_LOG(LogTemp, Log, TEXT("磁场贴花已创建，颜色: (%.2f, %.2f, %.2f)"), 
                DecalColor.R, DecalColor.G, DecalColor.B);
        }
    }

    // ✨ 新增 - 动态创建范围 Plane 组件
    if (bUsePlaneForRange && !RangePlaneComponent)
    {
        RangePlaneComponent = NewObject<UStaticMeshComponent>(GetOwner(), TEXT("RangePlaneComponent"));
        if (RangePlaneComponent)
        {
            RangePlaneComponent->SetupAttachment(this);
            RangePlaneComponent->SetRelativeLocation(FVector(0.0f, 0.0f, PlaneHeightOffset));
            RangePlaneComponent->SetRelativeRotation(FRotator::ZeroRotator);
            RangePlaneComponent->SetVisibility(false);
            
            // 运行时加载默认 Plane 静态网格
            UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(
                nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
            if (PlaneMesh)
            {
                RangePlaneComponent->SetStaticMesh(PlaneMesh);
            }
            
            // 禁用碰撞
            RangePlaneComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            RangePlaneComponent->SetGenerateOverlapEvents(false);
            
            // 创建动态材质实例
            if (RangePlaneMaterial)
            {
                PlaneMaterialInstance = UMaterialInstanceDynamic::Create(RangePlaneMaterial, this);
                if (PlaneMaterialInstance)
                {
                    if (bUseRandomColor)
                    {
                        PlaneMaterialInstance->SetVectorParameterValue(TEXT("Color"), DecalColor);
                    }
                    RangePlaneComponent->SetMaterial(0, PlaneMaterialInstance);
                }
                else
                {
                    RangePlaneComponent->SetMaterial(0, RangePlaneMaterial);
                }
            }
            
            RangePlaneComponent->RegisterComponent();
            UpdateRangePlaneSize();
            
            UE_LOG(LogTemp, Log, TEXT("磁场 Plane 组件已创建"));
        }
    }

    if (bDrawDebug)
    {
        SetComponentTickEnabled(true);
    }

    UE_LOG(LogTemp, Warning, TEXT("磁场组件 %s BeginPlay - 半径: %.1f, 启用: %s, 调试: %s"), 
        GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"), 
        GetScaledSphereRadius(),
        bIsFieldEnabled ? TEXT("是") : TEXT("否"),
        bDrawDebug ? TEXT("是") : TEXT("否"));
}

void UXBMagnetFieldComponent::TickComponent(float DeltaTime, ELevelTick TickType, 
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bDrawDebug)
    {
        UpdateActorsInField();
        DrawDebugField(0.0f);
    }
}

// ==================== 🔧 核心修复：招募逻辑 ====================

/**
 * @brief 磁场进入回调
 * @note 🔧 修复 - 简化招募逻辑，直接招募休眠态士兵
 */
void UXBMagnetFieldComponent::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!bIsFieldEnabled || !OtherActor)
    {
        return;
    }

    if (OtherActor == GetOwner())
    {
        return;
    }

    // 添加到范围内Actor列表
    ActorsInField.AddUnique(OtherActor);
    FieldStats.ActorsInRange = ActorsInField.Num();

    UE_LOG(LogTemp, Verbose, TEXT("磁场检测到 Actor: %s (类: %s)"), 
        *OtherActor->GetName(), *OtherActor->GetClass()->GetName());

    // 获取将领
    AXBCharacterBase* Leader = Cast<AXBCharacterBase>(GetOwner());
    if (!Leader)
    {
        UE_LOG(LogTemp, Warning, TEXT("磁场: 所有者不是将领类型"));
        return;
    }
    
    if (Leader->IsDead())
    {
        UE_LOG(LogTemp, Verbose, TEXT("磁场: 将领已死亡，停止招募"));
        return;
    }

    // ✨ 核心逻辑 - 检测并招募士兵
    AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(OtherActor);
    if (!Soldier)
    {
        // 不是士兵类型，跳过
        if (IsActorDetectable(OtherActor))
        {
            OnActorEnteredField.Broadcast(OtherActor);
        }
        return;
    }

    // 🔧 详细的招募检查日志
    UE_LOG(LogTemp, Log, TEXT("磁场: 检测到士兵 %s"), *Soldier->GetName());
    UE_LOG(LogTemp, Log, TEXT("  - 状态: %d"), static_cast<int32>(Soldier->GetSoldierState()));
    UE_LOG(LogTemp, Log, TEXT("  - 阵营: %d"), static_cast<int32>(Soldier->GetFaction()));
    UE_LOG(LogTemp, Log, TEXT("  - 已招募: %s"), Soldier->IsRecruited() ? TEXT("是") : TEXT("否"));
    UE_LOG(LogTemp, Log, TEXT("  - 已死亡: %s"), Soldier->IsDead() ? TEXT("是") : TEXT("否"));
    UE_LOG(LogTemp, Log, TEXT("  - CanBeRecruited: %s"), Soldier->CanBeRecruited() ? TEXT("是") : TEXT("否"));

    // 检查是否可招募
    if (!Soldier->CanBeRecruited())
    {
        UE_LOG(LogTemp, Log, TEXT("磁场: 士兵 %s 不可招募，跳过"), *Soldier->GetName());
        
        if (IsActorDetectable(OtherActor))
        {
            OnActorEnteredField.Broadcast(OtherActor);
        }
        return;
    }

    // ==================== 执行招募 ====================
    
    UE_LOG(LogTemp, Warning, TEXT(">>> 开始招募士兵 %s <<<"), *Soldier->GetName());

    // 获取将领的士兵数据表配置
    UDataTable* SoldierDT = Leader->GetSoldierDataTable();
    FName SoldierRowName = Leader->GetRecruitSoldierRowName();

    if (SoldierDT && !SoldierRowName.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("磁场: 使用将领配置初始化士兵 - 数据表: %s, 行: %s"), 
            *SoldierDT->GetName(), *SoldierRowName.ToString());
        
        // 使用将领的配置初始化士兵（改变兵种）
        Soldier->InitializeFromDataTable(SoldierDT, SoldierRowName, Leader->GetFaction());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("磁场: 将领未配置士兵数据表，使用士兵原有配置"));
    }

    // 获取槽位索引
    int32 SlotIndex = Leader->GetSoldierCount();
    UE_LOG(LogTemp, Log, TEXT("磁场: 分配槽位索引: %d"), SlotIndex);

    // 执行招募
    Soldier->OnRecruited(Leader, SlotIndex);
    
    // 添加到将领的士兵列表
    Leader->AddSoldier(Soldier);
    
    // 应用招募增益效果
    ApplyRecruitEffect(Leader, Soldier);

    // 更新统计
    FieldStats.TotalSoldiersRecruited++;
    if (UWorld* World = GetWorld())
    {
        FieldStats.LastRecruitTime = World->GetTimeSeconds();
    }

    UE_LOG(LogTemp, Warning, TEXT(">>> 士兵 %s 招募成功，将领当前士兵数: %d <<<"), 
        *Soldier->GetName(), Leader->GetSoldierCount());

    // 广播事件
    if (IsActorDetectable(OtherActor))
    {
        OnActorEnteredField.Broadcast(OtherActor);
    }
}

void UXBMagnetFieldComponent::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!bIsFieldEnabled || !OtherActor)
    {
        return;
    }

    if (OtherActor == GetOwner())
    {
        return;
    }

    // 从范围内Actor列表移除
    ActorsInField.RemoveAll([OtherActor](const TWeakObjectPtr<AActor>& WeakActor)
    {
        return !WeakActor.IsValid() || WeakActor.Get() == OtherActor;
    });
    FieldStats.ActorsInRange = ActorsInField.Num();

    if (IsActorDetectable(OtherActor))
    {
        OnActorExitedField.Broadcast(OtherActor);
    }
}

// ==================== 功能方法 ====================

void UXBMagnetFieldComponent::SetFieldRadius(float NewRadius)
{
    SetSphereRadius(NewRadius);
    // 同步更新贴花和 Plane 大小
    UpdateRangeDecalSize();
    UpdateRangePlaneSize();
}

void UXBMagnetFieldComponent::SetFieldEnabled(bool bEnabled)
{
    bIsFieldEnabled = bEnabled;
    SetGenerateOverlapEvents(bEnabled);
    
    // 同步贴花和 Plane 显示状态
    SetRangeDecalEnabled(bEnabled);
    if (bUsePlaneForRange)
    {
        SetRangePlaneEnabled(bEnabled);
    }
    
    UE_LOG(LogTemp, Log, TEXT("磁场组件 %s: 启用状态 = %s"), 
        GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"),
        bEnabled ? TEXT("是") : TEXT("否"));

    // ✨ 新增 - 启用时扫描并招募已经在范围内的士兵
    if (bEnabled)
    {
        // 使用下一帧延迟执行，确保所有 Actor 的 BeginPlay 都已完成
        GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<UXBMagnetFieldComponent>(this)]()
        {
            if (WeakThis.IsValid())
            {
                WeakThis->ScanAndRecruitExistingActors();
            }
        });
    }
}

void UXBMagnetFieldComponent::ResetStats()
{
    FieldStats = FXBMagnetFieldStats();
    UE_LOG(LogTemp, Log, TEXT("磁场统计已重置"));
}

void UXBMagnetFieldComponent::SetRangeDecalEnabled(bool bEnabled)
{
    if (!RangeDecalComponent)
    {
        return;
    }

    RangeDecalComponent->SetVisibility(bEnabled);
    
    if (bEnabled)
    {
        UpdateRangeDecalSize();
        UE_LOG(LogTemp, Log, TEXT("磁场范围贴花已启用"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("磁场范围贴花已禁用"));
    }
}

bool UXBMagnetFieldComponent::IsRangeDecalEnabled() const
{
    return RangeDecalComponent && RangeDecalComponent->IsVisible();
}

void UXBMagnetFieldComponent::UpdateRangeDecalSize()
{
    if (!RangeDecalComponent)
    {
        return;
    }

    // 贴花大小根据磁场半径设置
    // DecalSize: X=投影深度（向下投影距离）, Y=半径X, Z=半径Y
    const float Radius = GetScaledSphereRadius();
    RangeDecalComponent->DecalSize = FVector(DecalProjectionDepth, Radius, Radius);
    RangeDecalComponent->SetRelativeLocation(FVector(0.0f, 0.0f, DecalHeightOffset));
    
    UE_LOG(LogTemp, Verbose, TEXT("磁场贴花大小已更新: 半径=%.1f, 投影深度=%.1f"), Radius, DecalProjectionDepth);
}

void UXBMagnetFieldComponent::UpdateRangePlaneSize()
{
    if (!RangePlaneComponent)
    {
        return;
    }

    // Plane 默认大小是 100x100，需要根据磁场半径计算缩放
    // 直径 = 半径 * 2，缩放 = 直径 / 100
    const float Radius = GetScaledSphereRadius();
    const float Diameter = Radius * 2.0f;
    const float PlaneScale = Diameter / 100.0f;  // 默认 Plane 是 100x100 单位
    
    RangePlaneComponent->SetRelativeScale3D(FVector(PlaneScale, PlaneScale, 1.0f));
    RangePlaneComponent->SetRelativeLocation(FVector(0.0f, 0.0f, PlaneHeightOffset));
    
    UE_LOG(LogTemp, Verbose, TEXT("磁场 Plane 大小已更新: 半径=%.1f, 缩放=%.2f"), Radius, PlaneScale);
}

void UXBMagnetFieldComponent::SetRangePlaneEnabled(bool bEnabled)
{
    if (!RangePlaneComponent)
    {
        return;
    }

    RangePlaneComponent->SetVisibility(bEnabled);
    
    if (bEnabled)
    {
        UpdateRangePlaneSize();
        UE_LOG(LogTemp, Log, TEXT("磁场 Plane 已启用"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("磁场 Plane 已禁用"));
    }
}

void UXBMagnetFieldComponent::SetDecalColor(FLinearColor NewColor)
{
    DecalColor = NewColor;
    
    if (DecalMaterialInstance)
    {
        DecalMaterialInstance->SetVectorParameterValue(TEXT("Color"), DecalColor);
        UE_LOG(LogTemp, Log, TEXT("磁场贴花颜色已更新: (%.2f, %.2f, %.2f)"), 
            DecalColor.R, DecalColor.G, DecalColor.B);
    }
}

bool UXBMagnetFieldComponent::IsActorDetectable(AActor* Actor) const
{
    if (!Actor)
    {
        return false;
    }

    if (DetectableActorClasses.Num() == 0)
    {
        return true;
    }

    for (const TSubclassOf<AActor>& ActorClass : DetectableActorClasses)
    {
        if (ActorClass && Actor->IsA(ActorClass))
        {
            return true;
        }
    }

    return false;
}

bool UXBMagnetFieldComponent::IsActorRecruitable(AActor* Actor) const
{
    if (!Actor || !IsValid(Actor))
    {
        return false;
    }
    
    if (AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(Actor))
    {
        return Soldier->CanBeRecruited();
    }
    
    return false;
}

void UXBMagnetFieldComponent::ApplyRecruitEffect(AXBCharacterBase* Leader, AXBSoldierCharacter* Soldier)
{
    if (!Leader)
    {
        return;
    }

    UAbilitySystemComponent* LeaderASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Leader);
    if (!LeaderASC)
    {
        UE_LOG(LogTemp, Verbose, TEXT("将领没有 AbilitySystemComponent"));
        return;
    }

    if (RecruitBonusEffectClass)
    {
        FGameplayEffectContextHandle ContextHandle = LeaderASC->MakeEffectContext();
        ContextHandle.AddSourceObject(Soldier);

        FGameplayEffectSpecHandle SpecHandle = LeaderASC->MakeOutgoingSpec(
            RecruitBonusEffectClass, 1, ContextHandle);

        if (SpecHandle.IsValid())
        {
            LeaderASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
            UE_LOG(LogTemp, Log, TEXT("已应用招募增益效果"));
        }
    }

    FGameplayEventData EventData;
    EventData.Instigator = Leader;
    EventData.Target = Soldier;
    LeaderASC->HandleGameplayEvent(
        FGameplayTag::RequestGameplayTag(FName("Event.Soldier.Recruited")), &EventData);
}

/**
 * @brief 扫描并招募已经在磁场范围内的休眠态士兵
 * @note  用于解决士兵在游戏开始时就在磁场范围内无法触发 Overlap 事件的问题
 */
void UXBMagnetFieldComponent::ScanAndRecruitExistingActors()
{
    if (!bIsFieldEnabled)
    {
        return;
    }

    AXBCharacterBase* Leader = Cast<AXBCharacterBase>(GetOwner());
    if (!Leader || Leader->IsDead())
    {
        return;
    }

    // 获取当前重叠的所有 Actor
    TArray<AActor*> OverlappingActors;
    GetOverlappingActors(OverlappingActors, AXBSoldierCharacter::StaticClass());

    UE_LOG(LogTemp, Log, TEXT("磁场: 扫描范围内士兵，找到 %d 个"), OverlappingActors.Num());

    for (AActor* Actor : OverlappingActors)
    {
        if (!Actor || Actor == GetOwner())
        {
            continue;
        }

        AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(Actor);
        if (!Soldier)
        {
            continue;
        }

        // 🔧 详细的招募检查日志
        UE_LOG(LogTemp, Log, TEXT("磁场扫描: 检测到士兵 %s"), *Soldier->GetName());
        UE_LOG(LogTemp, Log, TEXT("  - 状态: %d"), static_cast<int32>(Soldier->GetSoldierState()));
        UE_LOG(LogTemp, Log, TEXT("  - 阵营: %d"), static_cast<int32>(Soldier->GetFaction()));
        UE_LOG(LogTemp, Log, TEXT("  - 已招募: %s"), Soldier->IsRecruited() ? TEXT("是") : TEXT("否"));
        UE_LOG(LogTemp, Log, TEXT("  - CanBeRecruited: %s"), Soldier->CanBeRecruited() ? TEXT("是") : TEXT("否"));

        // 检查是否可招募
        if (!Soldier->CanBeRecruited())
        {
            UE_LOG(LogTemp, Log, TEXT("磁场扫描: 士兵 %s 不可招募，跳过"), *Soldier->GetName());
            continue;
        }

        // ==================== 执行招募 ====================
        
        UE_LOG(LogTemp, Warning, TEXT(">>> 扫描招募士兵 %s <<<"), *Soldier->GetName());

        // 获取将领的士兵数据表配置
        UDataTable* SoldierDT = Leader->GetSoldierDataTable();
        FName SoldierRowName = Leader->GetRecruitSoldierRowName();

        if (SoldierDT && !SoldierRowName.IsNone())
        {
            UE_LOG(LogTemp, Log, TEXT("磁场扫描: 使用将领配置初始化士兵 - 数据表: %s, 行: %s"), 
                *SoldierDT->GetName(), *SoldierRowName.ToString());
            
            // 使用将领的配置初始化士兵（改变兵种）
            Soldier->InitializeFromDataTable(SoldierDT, SoldierRowName, Leader->GetFaction());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("磁场扫描: 将领未配置士兵数据表，使用士兵原有配置"));
        }

        // 获取槽位索引
        int32 SlotIndex = Leader->GetSoldierCount();
        UE_LOG(LogTemp, Log, TEXT("磁场扫描: 分配槽位索引: %d"), SlotIndex);

        // 执行招募
        Soldier->OnRecruited(Leader, SlotIndex);
        
        // 添加到将领的士兵列表
        Leader->AddSoldier(Soldier);
        
        // 应用招募增益效果
        ApplyRecruitEffect(Leader, Soldier);

        // 更新统计
        FieldStats.TotalSoldiersRecruited++;
        if (UWorld* World = GetWorld())
        {
            FieldStats.LastRecruitTime = World->GetTimeSeconds();
        }

        // 添加到范围内Actor列表
        ActorsInField.AddUnique(Soldier);
        FieldStats.ActorsInRange = ActorsInField.Num();

        UE_LOG(LogTemp, Warning, TEXT(">>> 士兵 %s 扫描招募成功，将领当前士兵数: %d <<<"), 
            *Soldier->GetName(), Leader->GetSoldierCount());

        // 广播事件
        if (IsActorDetectable(Soldier))
        {
            OnActorEnteredField.Broadcast(Soldier);
        }
    }
}

// ==================== 调试系统 ====================

void UXBMagnetFieldComponent::SetDebugDrawEnabled(bool bEnabled)
{
    bDrawDebug = bEnabled;
    SetComponentTickEnabled(bEnabled);
    
    if (bEnabled)
    {
        UE_LOG(LogTemp, Warning, TEXT("磁场调试绘制已启用: %s"), 
            GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));
        DrawDebugField(5.0f);
    }
}

void UXBMagnetFieldComponent::UpdateActorsInField()
{
    ActorsInField.RemoveAll([](const TWeakObjectPtr<AActor>& WeakActor)
    {
        return !WeakActor.IsValid();
    });
    FieldStats.ActorsInRange = ActorsInField.Num();
}

void UXBMagnetFieldComponent::DrawDebugField(float Duration)
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    
    if (!World || !Owner)
    {
        return;
    }
    
    FVector FieldCenter = GetComponentLocation();
    float FieldRadius = GetScaledSphereRadius();
    
    FColor CircleColor = bIsFieldEnabled ? DebugEnabledColor : DebugDisabledColor;
    
    // 绘制水平圆圈
    DrawDebugCircle(
        World,
        FieldCenter,
        FieldRadius,
        DebugCircleSegments,
        CircleColor,
        false,
        Duration,
        0,
        5.0f,
        FVector(1, 0, 0),
        FVector(0, 1, 0),
        false
    );
    
    // 绘制中心标记
    DrawDebugSphere(
        World,
        FieldCenter + FVector(0, 0, 20.0f),
        20.0f,
        12,
        CircleColor,
        false,
        Duration,
        0,
        2.0f
    );
    
    // 绘制信息文字
    FString FieldInfo = FString::Printf(
        TEXT("磁场: %s\n半径: %.0f\n状态: %s\n招募数: %d"),
        *Owner->GetName(),
        FieldRadius,
        bIsFieldEnabled ? TEXT("启用") : TEXT("禁用"),
        FieldStats.TotalSoldiersRecruited
    );
    
    DrawDebugString(
        World,
        FieldCenter + FVector(0, 0, DebugTextHeightOffset),
        FieldInfo,
        nullptr,
        FColor::White,
        Duration,
        true,
        1.5f
    );
    
    // 绘制范围内Actor信息
    if (bShowActorInfo)
    {
        for (const auto& WeakActor : ActorsInField)
        {
            AActor* Actor = WeakActor.Get();
            if (Actor && IsValid(Actor))
            {
                DrawDebugActorInfo(Actor, Duration);
            }
        }
    }
}

void UXBMagnetFieldComponent::DrawDebugActorInfo(AActor* Actor, float Duration)
{
    UWorld* World = GetWorld();
    if (!World || !Actor)
    {
        return;
    }
    
    FVector FieldCenter = GetComponentLocation();
    FVector ActorLocation = Actor->GetActorLocation();
    
    bool bRecruitable = IsActorRecruitable(Actor);
    FColor LineColor = bRecruitable ? DebugRecruitableColor : DebugNonRecruitableColor;
    
    // 绘制连线
    DrawDebugLine(
        World,
        FieldCenter + FVector(0, 0, 30.0f),
        ActorLocation + FVector(0, 0, 30.0f),
        LineColor,
        false,
        Duration,
        0,
        2.0f
    );
    
    // 获取Actor信息
    FString ActorTypeName = TEXT("未知");
    if (AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(Actor))
    {
        FString StateStr;
        switch (Soldier->GetSoldierState())
        {
        case EXBSoldierState::Dormant: StateStr = TEXT("休眠"); break;
        case EXBSoldierState::Idle: StateStr = TEXT("待机"); break;
        case EXBSoldierState::Following: StateStr = TEXT("跟随"); break;
        case EXBSoldierState::Combat: StateStr = TEXT("战斗"); break;
        default: StateStr = TEXT("其他"); break;
        }
        
        ActorTypeName = FString::Printf(TEXT("士兵[%s]\n可招募:%s"), 
            *StateStr,
            Soldier->CanBeRecruited() ? TEXT("是") : TEXT("否"));
    }
    
    DrawDebugString(
        World,
        ActorLocation + FVector(0, 0, 80.0f),
        ActorTypeName,
        nullptr,
        LineColor,
        Duration,
        true,
        1.0f
    );
}
