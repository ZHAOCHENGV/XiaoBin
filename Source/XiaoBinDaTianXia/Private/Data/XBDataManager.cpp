// Source/XiaoBinDaTianXia/Private/Data/XBDataManager.cpp

#include "Data/XBDataManager.h"
#include "Data/XBGameDataAsset.h"


void UXBDataManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // 初始化日志
    UE_LOG(LogTemp, Log, TEXT("XBDataManager Subsystem Initialized"));
}

void UXBDataManager::Deinitialize()
{
    // 清理引用和缓存
    GlobalDataAsset = nullptr;
    LeaderDataCache.Empty();
    SoldierDataCache.Empty();

    Super::Deinitialize();
}

bool UXBDataManager::LoadGlobalData(const FSoftObjectPath& DataAssetPath)
{
    if (!DataAssetPath.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("XBDataManager::LoadGlobalData - Invalid Asset Path"));
        return false;
    }

    // 同步加载全局数据资产
    // 注意：实际项目中大型资源建议使用异步加载，这里为了简化流程使用同步加载
    UObject* LoadedObject = DataAssetPath.TryLoad();
    UXBGlobalDataAsset* GlobalData = Cast<UXBGlobalDataAsset>(LoadedObject);

    if (GlobalData)
    {
        SetGlobalData(GlobalData);
        UE_LOG(LogTemp, Log, TEXT("XBDataManager::LoadGlobalData - Success loading: %s"), *DataAssetPath.ToString());
        return true;
    }

    UE_LOG(LogTemp, Error, TEXT("XBDataManager::LoadGlobalData - Failed to load: %s"), *DataAssetPath.ToString());
    return false;
}

void UXBDataManager::SetGlobalData(UXBGlobalDataAsset* InGlobalData)
{
    if (GlobalDataAsset != InGlobalData)
    {
        GlobalDataAsset = InGlobalData;
        
        // 数据变更后重新构建缓存
        InitializeDataCache();
    }
}

void UXBDataManager::InitializeDataCache()
{
    LeaderDataCache.Empty();
    SoldierDataCache.Empty();

    if (!GlobalDataAsset)
    {
        return;
    }

    // 1. 缓存默认将领数据
    if (GlobalDataAsset->DefaultLeaderData)
    {
        LeaderDataCache.Add(GlobalDataAsset->DefaultLeaderData->LeaderId, GlobalDataAsset->DefaultLeaderData);
    }

    // 2. 缓存士兵数据
    for (const auto& SoldierData : GlobalDataAsset->SoldierDataList)
    {
        if (SoldierData)
        {
            // 使用士兵类型作为 Key
            SoldierDataCache.Add(SoldierData->SoldierType, SoldierData);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("XBDataManager: Cached %d Leaders, %d Soldier Types"), 
        LeaderDataCache.Num(), SoldierDataCache.Num());
}

FXBLeaderConfig UXBDataManager::GetLeaderConfig(FName LeaderId) const
{
    // 尝试在缓存中查找
    if (const TObjectPtr<UXBLeaderDataAsset>* FoundData = LeaderDataCache.Find(LeaderId))
    {
        if (*FoundData)
        {
            return (*FoundData)->LeaderConfig;
        }
    }
    
    // 如果找不到特定ID的将领，或者缓存为空，尝试返回默认配置
    // 这样可以避免逻辑崩溃
    UE_LOG(LogTemp, Warning, TEXT("XBDataManager::GetLeaderConfig - LeaderId '%s' not found, using default."), *LeaderId.ToString());
    return GetDefaultLeaderConfig();
}

FXBLeaderConfig UXBDataManager::GetDefaultLeaderConfig() const
{
    if (GlobalDataAsset && GlobalDataAsset->DefaultLeaderData)
    {
        return GlobalDataAsset->DefaultLeaderData->LeaderConfig;
    }
    
    // 返回空配置
    return FXBLeaderConfig();
}

FXBSoldierConfig UXBDataManager::GetSoldierConfig(EXBSoldierType SoldierType) const
{
    if (const TObjectPtr<UXBSoldierDataAsset>* FoundData = SoldierDataCache.Find(SoldierType))
    {
        if (*FoundData)
        {
            return (*FoundData)->SoldierConfig;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("XBDataManager::GetSoldierConfig - Type %d not found"), (int32)SoldierType);
    return FXBSoldierConfig();
}

FXBFormationConfig UXBDataManager::GetFormationConfig() const
{
    if (GlobalDataAsset)
    {
        return GlobalDataAsset->FormationConfig;
    }
    return FXBFormationConfig();
}

FXBCombatConfig UXBDataManager::GetCombatConfig() const
{
    if (GlobalDataAsset)
    {
        return GlobalDataAsset->CombatConfig;
    }
    return FXBCombatConfig();
}