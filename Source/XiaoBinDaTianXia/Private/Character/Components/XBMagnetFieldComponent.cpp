/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Character/Components/XBMagnetFieldComponent.cpp

/**
 * @file XBMagnetFieldComponent.cpp
 * @brief 磁场组件实现 - 增强调试系统
 * 
 * @note 🔧 修改记录:
 *       1. ✨ 新增 TickComponent 用于持续调试绘制
 *       2. ✨ 新增 DrawDebugField() 完整调试可视化
 *       3. ✨ 新增 统计数据追踪
 *       4. ✨ 新增 范围内Actor追踪
 */

#include "Character/Components/XBMagnetFieldComponent.h"
#include "GameplayEffectTypes.h"
#include "Character/XBCharacterBase.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Soldier/XBVillagerActor.h"
#include "GAS/XBAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "XBCollisionChannels.h"
#include "Data/XBSoldierDataTable.h"
#include "Engine/DataTable.h"
#include "DrawDebugHelpers.h"

UXBMagnetFieldComponent::UXBMagnetFieldComponent()
{
    // 🔧 修改 - 启用Tick以支持持续调试绘制
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false; // 默认禁用，调试时启用
    bWantsInitializeComponent = true;

    SetGenerateOverlapEvents(false);
    InitSphereRadius(300.0f);
    SetHiddenInGame(true);
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
        *GetOwner()->GetName());
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

    // ✨ 新增 - 如果启用调试，开启Tick
    if (bDrawDebug)
    {
        SetComponentTickEnabled(true);
    }

    // ✨ 新增 - 启动时预热士兵对象池，降低集中招募时的生成开销（可选）
    if (bEnableSoldierPooling)
    {
        PrewarmSoldierPool();
    }

    UE_LOG(LogTemp, Warning, TEXT("磁场组件 %s BeginPlay - 半径: %.1f, 启用: %s, 调试: %s"), 
        *GetOwner()->GetName(), 
        GetScaledSphereRadius(),
        bIsFieldEnabled ? TEXT("是") : TEXT("否"),
        bDrawDebug ? TEXT("是") : TEXT("否"));
}

/**
 * @brief Tick函数 - 用于持续调试绘制
 * @note ✨ 新增
 */
void UXBMagnetFieldComponent::TickComponent(float DeltaTime, ELevelTick TickType, 
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bDrawDebug)
    {
        // 更新范围内Actor列表
        UpdateActorsInField();
        
        // 绘制调试信息
        DrawDebugField(0.0f);
    }
}

// ==================== ✨ 新增：调试系统实现 ====================

/**
 * @brief 启用/禁用调试绘制
 * @param bEnabled 是否启用
 */
void UXBMagnetFieldComponent::SetDebugDrawEnabled(bool bEnabled)
{
    bDrawDebug = bEnabled;
    
    // 启用/禁用Tick
    SetComponentTickEnabled(bEnabled);
    
    if (bEnabled)
    {
        UE_LOG(LogTemp, Warning, TEXT("============================================="));
        UE_LOG(LogTemp, Warning, TEXT("磁场调试绘制已启用: %s"), *GetOwner()->GetName());
        UE_LOG(LogTemp, Warning, TEXT("磁场半径: %.1f"), GetScaledSphereRadius());
        UE_LOG(LogTemp, Warning, TEXT("磁场状态: %s"), bIsFieldEnabled ? TEXT("启用") : TEXT("禁用"));
        UE_LOG(LogTemp, Warning, TEXT("============================================="));
        
        // 立即绘制一次
        DrawDebugField(5.0f);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("磁场调试绘制已禁用: %s"), *GetOwner()->GetName());
    }
}

/**
 * @brief 更新范围内Actor列表
 * @note ✨ 新增 - 清理无效引用并更新统计
 */
void UXBMagnetFieldComponent::UpdateActorsInField()
{
    // 清理无效引用
    ActorsInField.RemoveAll([](const TWeakObjectPtr<AActor>& WeakActor)
    {
        return !WeakActor.IsValid();
    });
    
    // 更新统计
    FieldStats.ActorsInRange = ActorsInField.Num();
}

/**
 * @brief 检查Actor是否可招募
 * @param Actor 目标Actor
 * @return 是否可招募
 * @note ✨ 新增
 */
bool UXBMagnetFieldComponent::IsActorRecruitable(AActor* Actor) const
{
    if (!Actor || !IsValid(Actor))
    {
        return false;
    }
    
    // 检查村民
    if (AXBVillagerActor* Villager = Cast<AXBVillagerActor>(Actor))
    {
        return Villager->CanBeRecruited();
    }
    
    // 检查士兵
    if (AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(Actor))
    {
        return Soldier->CanBeRecruited();
    }
    
    return false;
}

/**
 * @brief 绘制完整的调试信息
 * @param Duration 持续时间
 * @note ✨ 核心调试可视化方法
 */
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
    
    // ==================== 1. 绘制磁场圆圈 ====================
    
    FColor CircleColor = bIsFieldEnabled ? DebugEnabledColor : DebugDisabledColor;
    
    // 绘制水平圆圈（地面）
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
    
    // 绘制第二个稍高的圆圈（增强可见性）
    DrawDebugCircle(
        World,
        FieldCenter + FVector(0, 0, 50.0f),
        FieldRadius,
        DebugCircleSegments,
        CircleColor,
        false,
        Duration,
        0,
        3.0f,
        FVector(1, 0, 0),
        FVector(0, 1, 0),
        false
    );
    
    // 绘制垂直方向的标记线
    for (int32 i = 0; i < 4; ++i)
    {
        float Angle = (PI * 2.0f * i) / 4.0f;
        FVector EdgePoint = FieldCenter + FVector(FMath::Cos(Angle) * FieldRadius, FMath::Sin(Angle) * FieldRadius, 0.0f);
        
        DrawDebugLine(
            World,
            EdgePoint,
            EdgePoint + FVector(0, 0, 100.0f),
            CircleColor,
            false,
            Duration,
            0,
            2.0f
        );
    }
    
    // ==================== 2. 绘制中心标记 ====================
    
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
    
    // ==================== 3. 绘制磁场信息文字 ====================
    
    FString FieldInfo = FString::Printf(
        TEXT("磁场: %s\n半径: %.0f\n状态: %s"),
        *Owner->GetName(),
        FieldRadius,
        bIsFieldEnabled ? TEXT("启用") : TEXT("禁用")
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
    
    // ==================== 4. 绘制统计数据 ====================
    
    if (bShowStats)
    {
        FString StatsInfo = FString::Printf(
            TEXT("招募士兵: %d\n招募村民: %d\n范围内: %d"),
            FieldStats.TotalSoldiersRecruited,
            FieldStats.TotalVillagersRecruited,
            FieldStats.ActorsInRange
        );
        
        DrawDebugString(
            World,
            FieldCenter + FVector(0, 0, DebugTextHeightOffset + 80.0f),
            StatsInfo,
            nullptr,
            FColor::Cyan,
            Duration,
            true,
            1.2f
        );
    }
    
    // ==================== 5. 绘制范围内Actor信息 ====================
    
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

/**
 * @brief 绘制单个Actor的调试信息
 * @param Actor 目标Actor
 * @param Duration 持续时间
 * @note ✨ 新增
 */
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
    
    // 绘制目标标记
    DrawDebugSphere(
        World,
        ActorLocation + FVector(0, 0, 50.0f),
        15.0f,
        8,
        LineColor,
        false,
        Duration,
        0,
        1.5f
    );
    
    // 获取Actor类型名称
    FString ActorTypeName = TEXT("未知");
    if (Cast<AXBVillagerActor>(Actor))
    {
        ActorTypeName = TEXT("村民");
    }
    else if (AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(Actor))
    {
        ActorTypeName = FString::Printf(TEXT("士兵(%s)"), 
            Soldier->IsRecruited() ? TEXT("已招") : TEXT("未招"));
    }
    
    // 绘制Actor信息
    FString ActorInfo = FString::Printf(
        TEXT("%s\n%s\n%.0f"),
        *Actor->GetName(),
        *ActorTypeName,
        FVector::Dist(FieldCenter, ActorLocation)
    );
    
    DrawDebugString(
        World,
        ActorLocation + FVector(0, 0, 80.0f),
        ActorInfo,
        nullptr,
        LineColor,
        Duration,
        true,
        1.0f
    );
}

/**
 * @brief 重置统计数据
 * @note ✨ 新增
 */
void UXBMagnetFieldComponent::ResetStats()
{
    FieldStats = FXBMagnetFieldStats();
    UE_LOG(LogTemp, Log, TEXT("磁场统计已重置: %s"), *GetOwner()->GetName());
}

/**
 * @brief 预热士兵对象池
 * @note ✨ 新增 - 启动时生成并隐藏士兵，减少集中招募时的 Spawn 峰值
 */
void UXBMagnetFieldComponent::PrewarmSoldierPool()
{
    AXBCharacterBase* Leader = Cast<AXBCharacterBase>(GetOwner());
    if (!Leader || SoldierPoolWarmCount <= 0)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FVector BaseLocation = GetComponentLocation();
    FRotator BaseRotation = GetComponentRotation();

    for (int32 Index = 0; Index < SoldierPoolWarmCount; ++Index)
    {
        // 🔧 修改 - 分散预热位置，避免重叠碰撞
        FVector Offset = FVector(FMath::FRandRange(-50.f, 50.f), FMath::FRandRange(-50.f, 50.f), 0.f);
        AXBSoldierCharacter* Soldier = SpawnNewSoldierInstance(BaseLocation + Offset, BaseRotation, Leader);
        if (Soldier)
        {
            Soldier->ResetForRecruitment();
            Soldier->SetActorHiddenInGame(true);
            Soldier->SetActorEnableCollision(false);
            Soldier->SetActorTickEnabled(false);
            SoldierPool.Add(Soldier);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("磁场组件 %s 预热士兵对象池完成，数量: %d"), *GetOwner()->GetName(), SoldierPool.Num());
}

/**
 * @brief 从对象池获取士兵，不足时视配置扩容
 * @param SpawnLocation 生成位置
 * @param SpawnRotation 生成旋转
 * @param Leader 所属将领
 * @return 可用士兵实例
 * @note ✨ 新增 - 优先复用隐藏的士兵实例，降低运行时 Spawn 开销
 */
AXBSoldierCharacter* UXBMagnetFieldComponent::AcquireSoldierFromPool(const FVector& SpawnLocation, const FRotator& SpawnRotation, AXBCharacterBase* Leader)
{
    // 清理无效引用
    SoldierPool.RemoveAll([](const TWeakObjectPtr<AXBSoldierCharacter>& WeakSoldier)
    {
        return !WeakSoldier.IsValid();
    });

    for (int32 Index = SoldierPool.Num() - 1; Index >= 0; --Index)
    {
        if (AXBSoldierCharacter* Soldier = SoldierPool[Index].Get())
        {
            // 🔧 修改 - 重置状态确保可招募
            Soldier->ResetForRecruitment();
            if (AXBCharacterBase* PrevLeader = Soldier->GetLeaderCharacter())
            {
                // 🔧 修改 - 防御性：仅在有效且不同将领时移除，避免对无效队列进行 RemoveAt
                if (PrevLeader != Leader && IsValid(PrevLeader) && !PrevLeader->IsPendingKillPending())
                {
                    PrevLeader->RemoveSoldier(Soldier);
                }
            }
            Soldier->SetActorLocationAndRotation(SpawnLocation, SpawnRotation);
            Soldier->SetActorHiddenInGame(false);
            Soldier->SetActorEnableCollision(true);
            Soldier->SetActorTickEnabled(true);
            SoldierPool.RemoveAt(Index);
            return Soldier;
        }
    }

    if (!bAllowSoldierPoolExpansion)
    {
        return nullptr;
    }

    // 🔧 修改 - 池子不足时动态扩容
    return SpawnNewSoldierInstance(SpawnLocation, SpawnRotation, Leader);
}

/**
 * @brief 生成新的士兵实例
 * @param SpawnLocation 生成位置
 * @param SpawnRotation 生成旋转
 * @param Leader 所属将领
 * @return 新生成的士兵
 * @note 🔧 修改 - 统一生成流程，便于池化/动态扩容共享
 */
AXBSoldierCharacter* UXBMagnetFieldComponent::SpawnNewSoldierInstance(const FVector& SpawnLocation, const FRotator& SpawnRotation, AXBCharacterBase* Leader)
{
    if (!Leader)
    {
        return nullptr;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    UDataTable* SoldierDT = Leader->GetSoldierDataTable();
    FName SoldierRowName = Leader->GetRecruitSoldierRowName();

    if (!SoldierDT || SoldierRowName.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("将领 %s 未配置士兵数据表，无法生成士兵"), *Leader->GetName());
        return nullptr;
    }

    TSubclassOf<AXBSoldierCharacter> SoldierClass = Leader->GetSoldierActorClass();
    if (!SoldierClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("将领 %s 未配置士兵Actor类"), *Leader->GetName());
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AXBSoldierCharacter* NewSoldier = World->SpawnActor<AXBSoldierCharacter>(
        SoldierClass,
        SpawnLocation,
        SpawnRotation,
        SpawnParams
    );

    if (!NewSoldier)
    {
        return nullptr;
    }

    NewSoldier->InitializeFromDataTable(SoldierDT, SoldierRowName, Leader->GetFaction());
    return NewSoldier;
}

/**
 * @brief 隐藏并停用村民，避免销毁开销
 * @param Villager 目标村民
 * @note ✨ 新增 - 禁用碰撞与Tick，保留 Actor 以减少反复销毁
 */
void UXBMagnetFieldComponent::DeactivateVillager(AXBVillagerActor* Villager)
{
    if (!Villager)
    {
        return;
    }

    if (bHideVillagerInsteadOfDestroy)
    {
        Villager->SetActorHiddenInGame(true);
        Villager->SetActorEnableCollision(false);
        Villager->SetActorTickEnabled(false);
    }
    else
    {
        Villager->SetLifeSpan(0.1f);
    }
}

// ==================== 原有功能实现（略微修改） ====================

void UXBMagnetFieldComponent::SetFieldRadius(float NewRadius)
{
    SetSphereRadius(NewRadius);
}

void UXBMagnetFieldComponent::SetFieldEnabled(bool bEnabled)
{
    bIsFieldEnabled = bEnabled;
    SetGenerateOverlapEvents(bEnabled);
}

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

    // ✨ 新增 - 添加到范围内Actor列表
    ActorsInField.AddUnique(OtherActor);
    FieldStats.ActorsInRange = ActorsInField.Num();

    UE_LOG(LogTemp, Log, TEXT("磁场检测到: %s (类型: %s)"), 
        *OtherActor->GetName(), 
        *OtherActor->GetClass()->GetName());

    AXBCharacterBase* Leader = Cast<AXBCharacterBase>(GetOwner());
    if (!Leader || Leader->IsDead())
    {
        UE_LOG(LogTemp, Log, TEXT("磁场组件: 将领已死亡，忽略招募"));
        return;
    }

    // 优先检测村民
    if (AXBVillagerActor* Villager = Cast<AXBVillagerActor>(OtherActor))
    {
        if (TryRecruitVillager(Villager))
        {
            // ✨ 新增 - 更新统计
            FieldStats.TotalVillagersRecruited++;
            FieldStats.LastRecruitTime = GetWorld()->GetTimeSeconds();
            return;
        }
    }

    // 招募已存在的士兵
    if (AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(OtherActor))
    {
        if (Soldier->CanBeRecruited())
        {
            UDataTable* SoldierDT = Leader->GetSoldierDataTable();
            FName SoldierRowName = Leader->GetRecruitSoldierRowName();

            if (SoldierDT && !SoldierRowName.IsNone())
            {
                Soldier->InitializeFromDataTable(SoldierDT, SoldierRowName, Leader->GetFaction());
            }

            int32 SlotIndex = Leader->GetSoldierCount();
            Soldier->OnRecruited(Leader, SlotIndex);
            Leader->AddSoldier(Soldier);
            ApplyRecruitEffect(Leader, Soldier);

            // ✨ 新增 - 更新统计
            FieldStats.TotalSoldiersRecruited++;
            FieldStats.LastRecruitTime = GetWorld()->GetTimeSeconds();

            UE_LOG(LogTemp, Log, TEXT("士兵被招募，将领当前士兵数: %d"), Leader->GetSoldierCount());
        }
    }

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

    // ✨ 新增 - 从范围内Actor列表移除
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

bool UXBMagnetFieldComponent::TryRecruitVillager(AXBVillagerActor* Villager)
{
    if (!Villager || !Villager->CanBeRecruited())
    {
        return false;
    }

    AXBCharacterBase* Leader = Cast<AXBCharacterBase>(GetOwner());
    if (!Leader)
    {
        return false;
    }

    UDataTable* SoldierDT = Leader->GetSoldierDataTable();
    FName SoldierRowName = Leader->GetRecruitSoldierRowName();

    if (!SoldierDT || SoldierRowName.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("将领 %s 未配置士兵数据表"), *Leader->GetName());
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    TSubclassOf<AXBSoldierCharacter> SoldierClass = Leader->GetSoldierActorClass();
    if (!SoldierClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("将领 %s 未配置士兵Actor类"), *Leader->GetName());
        return false;
    }

    FVector SpawnLocation = Villager->GetActorLocation();
    FRotator SpawnRotation = Villager->GetActorRotation();
    
    AXBSoldierCharacter* NewSoldier = nullptr;

    // ✨ 新增 - 优先尝试对象池；失败回退到直接生成，避免阻塞招募
    if (bEnableSoldierPooling)
    {
        NewSoldier = AcquireSoldierFromPool(SpawnLocation, SpawnRotation, Leader);
    }

    if (!NewSoldier)
    {
        // 🔧 修改 - 回退到直接生成，保证稳定性
        NewSoldier = SpawnNewSoldierInstance(SpawnLocation, SpawnRotation, Leader);
    }

    if (!NewSoldier)
    {
        UE_LOG(LogTemp, Error, TEXT("生成士兵失败（对象池或直接生成均失败）"));
        return false;
    }

    int32 SlotIndex = Leader->GetSoldierCount();
    NewSoldier->OnRecruited(Leader, SlotIndex);
    Leader->AddSoldier(NewSoldier);

    ApplyRecruitEffect(Leader, NewSoldier);

    // ✨ 新增 - 村民隐藏而非销毁，避免频繁析构
    DeactivateVillager(Villager);

    UE_LOG(LogTemp, Log, TEXT("村民 %s 转化为士兵 %s，将领当前士兵数: %d"),
        *Villager->GetName(), *NewSoldier->GetName(), Leader->GetSoldierCount());

    return true;
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
        UE_LOG(LogTemp, Warning, TEXT("将领没有 AbilitySystemComponent"));
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
