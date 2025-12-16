/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Data/XBSoldierDataAccessor.cpp

/**
 * @file XBSoldierDataAccessor.cpp
 * @brief 士兵数据访问器实现
 */

#include "Data/XBSoldierDataAccessor.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/DataTable.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"

UXBSoldierDataAccessor::UXBSoldierDataAccessor()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UXBSoldierDataAccessor::BeginPlay()
{
    Super::BeginPlay();
}

void UXBSoldierDataAccessor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 🔧 清理异步加载句柄
    if (AsyncLoadHandle.IsValid())
    {
        AsyncLoadHandle->CancelHandle();
        AsyncLoadHandle.Reset();
    }

    Super::EndPlay(EndPlayReason);
}

// ==================== 初始化接口实现 ====================

bool UXBSoldierDataAccessor::Initialize(UDataTable* DataTable, FName RowName, EXBResourceLoadStrategy LoadStrategy)
{
    if (!DataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("数据访问器初始化失败: 数据表为空"));
        return false;
    }

    if (RowName.IsNone())
    {
        UE_LOG(LogTemp, Error, TEXT("数据访问器初始化失败: 行名为空"));
        return false;
    }

    FXBSoldierTableRow* Row = DataTable->FindRow<FXBSoldierTableRow>(
        RowName, 
        TEXT("UXBSoldierDataAccessor::Initialize")
    );

    if (!Row)
    {
        UE_LOG(LogTemp, Error, TEXT("数据访问器初始化失败: 找不到行 '%s'"), *RowName.ToString());
        return false;
    }

    FText ValidationError;
    if (!Row->Validate(ValidationError))
    {
        UE_LOG(LogTemp, Error, TEXT("数据访问器初始化失败: 数据校验不通过 - %s"), *ValidationError.ToString());
        return false;
    }

    CachedTableRow = *Row;
    CachedRowName = RowName;
    CachedDataTable = DataTable;
    CurrentLoadStrategy = LoadStrategy;
    bIsInitialized = true;

    UE_LOG(LogTemp, Log, TEXT("数据访问器初始化成功: %s (类型=%s, 血量=%.1f)"), 
        *RowName.ToString(),
        *UEnum::GetValueAsString(CachedTableRow.SoldierType),
        CachedTableRow.MaxHealth);

    switch (LoadStrategy)
    {
    case EXBResourceLoadStrategy::Synchronous:
        return LoadResourcesSynchronous();

    case EXBResourceLoadStrategy::Asynchronous:
        LoadResourcesAsynchronous();
        return true;

    case EXBResourceLoadStrategy::Lazy:
        bResourcesLoaded = true;
        return true;

    default:
        return false;
    }
}

// ==================== 资源加载实现 ====================

bool UXBSoldierDataAccessor::LoadResourcesSynchronous()
{
    if (!bIsInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("数据访问器未初始化，无法加载资源"));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("开始同步加载资源: %s"), *CachedRowName.ToString());

    if (!CachedTableRow.VisualConfig.SkeletalMesh.IsNull())
    {
        LoadedSkeletalMesh = CachedTableRow.VisualConfig.SkeletalMesh.LoadSynchronous();
        if (!LoadedSkeletalMesh)
        {
            UE_LOG(LogTemp, Warning, TEXT("骨骼网格体加载失败: %s"), 
                *CachedTableRow.VisualConfig.SkeletalMesh.ToString());
        }
    }

    // 🔧 修复 - 使用 LazyLoadClass 加载动画蓝图类
    if (!CachedTableRow.VisualConfig.AnimClass.IsNull())
    {
        LoadedAnimClass = LazyLoadClass(CachedTableRow.VisualConfig.AnimClass);
        if (!LoadedAnimClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("动画蓝图类加载失败: %s"), 
                *CachedTableRow.VisualConfig.AnimClass.ToString());
        }
    }

    if (!CachedTableRow.VisualConfig.DeathMontage.IsNull())
    {
        LoadedDeathMontage = CachedTableRow.VisualConfig.DeathMontage.LoadSynchronous();
    }

    if (!CachedTableRow.AIConfig.BehaviorTree.IsNull())
    {
        LoadedBehaviorTree = CachedTableRow.AIConfig.BehaviorTree.LoadSynchronous();
        if (!LoadedBehaviorTree)
        {
            UE_LOG(LogTemp, Warning, TEXT("行为树加载失败: %s"), 
                *CachedTableRow.AIConfig.BehaviorTree.ToString());
        }
    }

    if (!CachedTableRow.BasicAttack.AbilityMontage.IsNull())
    {
        LoadedBasicAttackMontage = CachedTableRow.BasicAttack.AbilityMontage.LoadSynchronous();
    }

    bResourcesLoaded = true;

    UE_LOG(LogTemp, Log, TEXT("资源加载完成: %s (网格=%s, 动画=%s, 行为树=%s)"), 
        *CachedRowName.ToString(),
        LoadedSkeletalMesh ? TEXT("✓") : TEXT("✗"),
        LoadedAnimClass ? TEXT("✓") : TEXT("✗"),
        LoadedBehaviorTree ? TEXT("✓") : TEXT("✗"));

    OnResourcesLoaded.Broadcast(true);

    return true;
}

void UXBSoldierDataAccessor::LoadResourcesAsynchronous()
{
    if (!bIsInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("数据访问器未初始化，无法加载资源"));
        OnResourcesLoaded.Broadcast(false);
        return;
    }

    TArray<FSoftObjectPath> AssetsToLoad;

    if (!CachedTableRow.VisualConfig.SkeletalMesh.IsNull())
    {
        AssetsToLoad.Add(CachedTableRow.VisualConfig.SkeletalMesh.ToSoftObjectPath());
    }

    if (!CachedTableRow.VisualConfig.AnimClass.IsNull())
    {
        AssetsToLoad.Add(CachedTableRow.VisualConfig.AnimClass.ToSoftObjectPath());
    }

    if (!CachedTableRow.VisualConfig.DeathMontage.IsNull())
    {
        AssetsToLoad.Add(CachedTableRow.VisualConfig.DeathMontage.ToSoftObjectPath());
    }

    if (!CachedTableRow.AIConfig.BehaviorTree.IsNull())
    {
        AssetsToLoad.Add(CachedTableRow.AIConfig.BehaviorTree.ToSoftObjectPath());
    }

    if (!CachedTableRow.BasicAttack.AbilityMontage.IsNull())
    {
        AssetsToLoad.Add(CachedTableRow.BasicAttack.AbilityMontage.ToSoftObjectPath());
    }

    if (AssetsToLoad.Num() == 0)
    {
        bResourcesLoaded = true;
        OnResourcesLoaded.Broadcast(true);
        return;
    }

    UAssetManager& AssetManager = UAssetManager::Get();
    FStreamableManager& StreamableManager = AssetManager.GetStreamableManager();

    AsyncLoadHandle = StreamableManager.RequestAsyncLoad(
        AssetsToLoad,
        FStreamableDelegate::CreateUObject(this, &UXBSoldierDataAccessor::OnAsyncLoadComplete),
        FStreamableManager::AsyncLoadHighPriority
    );

    UE_LOG(LogTemp, Log, TEXT("开始异步加载 %d 个资源: %s"), 
        AssetsToLoad.Num(), *CachedRowName.ToString());
}

void UXBSoldierDataAccessor::OnAsyncLoadComplete()
{
    if (!AsyncLoadHandle.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("异步加载句柄无效"));
        OnResourcesLoaded.Broadcast(false);
        return;
    }

    LoadedSkeletalMesh = Cast<USkeletalMesh>(CachedTableRow.VisualConfig.SkeletalMesh.Get());
    LoadedAnimClass = CachedTableRow.VisualConfig.AnimClass.Get();
    LoadedDeathMontage = Cast<UAnimMontage>(CachedTableRow.VisualConfig.DeathMontage.Get());
    LoadedBehaviorTree = Cast<UBehaviorTree>(CachedTableRow.AIConfig.BehaviorTree.Get());
    LoadedBasicAttackMontage = Cast<UAnimMontage>(CachedTableRow.BasicAttack.AbilityMontage.Get());

    bResourcesLoaded = true;

    UE_LOG(LogTemp, Log, TEXT("异步加载完成: %s (网格=%s, 动画=%s, 行为树=%s)"), 
        *CachedRowName.ToString(),
        LoadedSkeletalMesh ? TEXT("✓") : TEXT("✗"),
        LoadedAnimClass ? TEXT("✓") : TEXT("✗"),
        LoadedBehaviorTree ? TEXT("✓") : TEXT("✗"));

    OnResourcesLoaded.Broadcast(true);

    AsyncLoadHandle.Reset();
}

// ==================== 资源访问接口实现 ====================

template<typename T>
T* UXBSoldierDataAccessor::LazyLoadResource(const TSoftObjectPtr<T>& SoftObjectPtr)
{
    if (SoftObjectPtr.IsNull())
    {
        return nullptr;
    }

    T* LoadedObject = SoftObjectPtr.Get();
    if (LoadedObject)
    {
        return LoadedObject;
    }

    LoadedObject = SoftObjectPtr.LoadSynchronous();

    if (LoadedObject)
    {
        UE_LOG(LogTemp, Verbose, TEXT("延迟加载资源: %s"), *SoftObjectPtr.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("延迟加载失败: %s"), *SoftObjectPtr.ToString());
    }

    return LoadedObject;
}

// ✨ 新增 - 支持 TSoftClassPtr 的延迟加载
template<typename T>
TSubclassOf<T> UXBSoldierDataAccessor::LazyLoadClass(const TSoftClassPtr<T>& SoftClassPtr)
{
    if (SoftClassPtr.IsNull())
    {
        return nullptr;
    }

    TSubclassOf<T> LoadedClass = SoftClassPtr.Get();
    if (LoadedClass)
    {
        return LoadedClass;
    }

    LoadedClass = SoftClassPtr.LoadSynchronous();

    if (LoadedClass)
    {
        UE_LOG(LogTemp, Verbose, TEXT("延迟加载类: %s"), *SoftClassPtr.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("延迟加载类失败: %s"), *SoftClassPtr.ToString());
    }

    return LoadedClass;
}

USkeletalMesh* UXBSoldierDataAccessor::GetSkeletalMesh()
{
    if (CurrentLoadStrategy == EXBResourceLoadStrategy::Lazy)
    {
        if (!LoadedSkeletalMesh)
        {
            LoadedSkeletalMesh = LazyLoadResource(CachedTableRow.VisualConfig.SkeletalMesh);
        }
    }

    return LoadedSkeletalMesh;
}

TSubclassOf<UAnimInstance> UXBSoldierDataAccessor::GetAnimClass()
{
    if (CurrentLoadStrategy == EXBResourceLoadStrategy::Lazy)
    {
        if (!LoadedAnimClass)
        {
            // 🔧 使用 LazyLoadClass 处理 TSoftClassPtr
            LoadedAnimClass = LazyLoadClass(CachedTableRow.VisualConfig.AnimClass);
        }
    }

    return LoadedAnimClass;
}

UAnimMontage* UXBSoldierDataAccessor::GetDeathMontage()
{
    if (CurrentLoadStrategy == EXBResourceLoadStrategy::Lazy)
    {
        if (!LoadedDeathMontage)
        {
            LoadedDeathMontage = LazyLoadResource(CachedTableRow.VisualConfig.DeathMontage);
        }
    }

    return LoadedDeathMontage;
}

UBehaviorTree* UXBSoldierDataAccessor::GetBehaviorTree()
{
    if (CurrentLoadStrategy == EXBResourceLoadStrategy::Lazy)
    {
        if (!LoadedBehaviorTree)
        {
            LoadedBehaviorTree = LazyLoadResource(CachedTableRow.AIConfig.BehaviorTree);
        }
    }

    return LoadedBehaviorTree;
}

UAnimMontage* UXBSoldierDataAccessor::GetBasicAttackMontage()
{
    if (CurrentLoadStrategy == EXBResourceLoadStrategy::Lazy)
    {
        if (!LoadedBasicAttackMontage)
        {
            LoadedBasicAttackMontage = LazyLoadResource(CachedTableRow.BasicAttack.AbilityMontage);
        }
    }

    return LoadedBasicAttackMontage;
}
