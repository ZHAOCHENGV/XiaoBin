// Copyright XiaoBing Project. All Rights Reserved.

#include "Save/XBSaveSubsystem.h"
#include "Save/XBSaveGame.h"
#include "Save/XBSaveSlotIndex.h"
#include "Kismet/GameplayStatics.h"

void UXBSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // ✨ 新增 - 初始化存档槽位索引，确保多存档列表可用
    InitializeSaveSlotIndex();

    // 尝试加载默认存档
    if (DoesSaveGameExist(TEXT("Default"), 0))
    {
        LoadGame(TEXT("Default"), 0);
    }
    else
    {
        CreateNewSaveGame();
    }

    UE_LOG(LogTemp, Log, TEXT("存档子系统初始化完成"));
}

void UXBSaveSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

bool UXBSaveSubsystem::SaveGame(const FString& SlotName, int32 UserIndex)
{
    if (!CurrentSaveGame)
    {
        UE_LOG(LogTemp, Warning, TEXT("保存存档失败：当前存档为空"));
        return false;
    }

    const FString FullSlotName = BuildFullSlotName(SlotName);

    // ✨ 新增 - 保存前确保索引有效，避免出现空索引导致列表丢失
    InitializeSaveSlotIndex();
    if (UGameplayStatics::SaveGameToSlot(CurrentSaveGame, FullSlotName, UserIndex))
    {
        // ✨ 新增 - 保存成功后登记槽位名称，保证列表可列举
        if (SaveSlotIndex)
        {
            // ✨ 新增 - 使用逻辑名称，避免外部依赖前缀格式
            SaveSlotIndex->SlotNames.AddUnique(SlotName);
            SaveSlotIndexToDisk();
        }

        UE_LOG(LogTemp, Log, TEXT("存档已保存到槽位：%s"), *FullSlotName);
        return true;
    }

    UE_LOG(LogTemp, Error, TEXT("保存存档失败：%s"), *FullSlotName);
    return false;
}

bool UXBSaveSubsystem::LoadGame(const FString& SlotName, int32 UserIndex)
{
    const FString FullSlotName = BuildFullSlotName(SlotName);

    if (!UGameplayStatics::DoesSaveGameExist(FullSlotName, UserIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("加载存档失败：存档不存在 %s"), *FullSlotName);
        return false;
    }

    UXBSaveGame* LoadedGame = Cast<UXBSaveGame>(
        UGameplayStatics::LoadGameFromSlot(FullSlotName, UserIndex));

    if (LoadedGame)
    {
        CurrentSaveGame = LoadedGame;
        UE_LOG(LogTemp, Log, TEXT("存档已加载：%s"), *FullSlotName);
        return true;
    }

    UE_LOG(LogTemp, Error, TEXT("加载存档失败：%s"), *FullSlotName);
    return false;
}

bool UXBSaveSubsystem::DeleteSaveGame(const FString& SlotName, int32 UserIndex)
{
    const FString FullSlotName = BuildFullSlotName(SlotName);

    if (UGameplayStatics::DeleteGameInSlot(FullSlotName, UserIndex))
    {
        // ✨ 新增 - 删除成功后同步更新索引列表
        InitializeSaveSlotIndex();
        if (SaveSlotIndex)
        {
            SaveSlotIndex->SlotNames.Remove(SlotName);
            SaveSlotIndexToDisk();
        }

        UE_LOG(LogTemp, Log, TEXT("存档已删除：%s"), *FullSlotName);
        return true;
    }

    return false;
}

bool UXBSaveSubsystem::DoesSaveGameExist(const FString& SlotName, int32 UserIndex) const
{
    const FString FullSlotName = BuildFullSlotName(SlotName);
    return UGameplayStatics::DoesSaveGameExist(FullSlotName, UserIndex);
}

TArray<FString> UXBSaveSubsystem::GetAllSaveSlotNames() const
{
    // ✨ 新增 - 通过索引存档返回所有槽位
    if (SaveSlotIndex)
    {
        return SaveSlotIndex->SlotNames;
    }

    return TArray<FString>();
}

UXBSaveGame* UXBSaveSubsystem::CreateNewSaveGame()
{
    CurrentSaveGame = Cast<UXBSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UXBSaveGame::StaticClass()));

    return CurrentSaveGame;
}

void UXBSaveSubsystem::SetCurrentSaveGame(UXBSaveGame* SaveGame)
{
    CurrentSaveGame = SaveGame;
}

void UXBSaveSubsystem::InitializeSaveSlotIndex()
{
    // 🔧 修改 - 检查地图是否变化，如果变化则重新加载对应的索引
    const FString CurrentMapName = GetCurrentMapName();
    if (SaveSlotIndex && CachedMapName == CurrentMapName)
    {
        return;
    }

    // 地图变化，更新缓存并重新加载索引
    CachedMapName = CurrentMapName;
    SaveSlotIndex = nullptr;

    // ✨ 新增 - 优先加载索引存档，保证槽位列表可持久化
    // 🔧 修改 - 使用地图特定索引名，实现按场景分离存档列表
    const FString MapIndexName = GetMapSpecificIndexName();
    if (UGameplayStatics::DoesSaveGameExist(MapIndexName, 0))
    {
        SaveSlotIndex = Cast<UXBSaveSlotIndex>(
            UGameplayStatics::LoadGameFromSlot(MapIndexName, 0));
    }

    if (!SaveSlotIndex)
    {
        // ✨ 新增 - 首次运行创建索引存档
        SaveSlotIndex = Cast<UXBSaveSlotIndex>(
            UGameplayStatics::CreateSaveGameObject(UXBSaveSlotIndex::StaticClass()));
    }

    if (!SaveSlotIndex)
    {
        UE_LOG(LogTemp, Warning, TEXT("初始化存档槽位索引失败：无法创建索引存档"));
        return;
    }

    if (SaveSlotIndex->SlotNames.Num() == 0)
    {
        // ✨ 新增 - 初始化默认存档槽位列表（存档1~存档N）
        for (int32 Index = 1; Index <= DefaultSaveSlotCount; ++Index)
        {
            // ✨ 新增 - 用中文名称，便于 UI 直接展示
            SaveSlotIndex->SlotNames.Add(FString::Printf(TEXT("存档%d"), Index));
        }

        SaveSlotIndexToDisk();
    }

    UE_LOG(LogTemp, Log, TEXT("存档索引已加载，地图: %s，槽位数: %d"),
        *CurrentMapName, SaveSlotIndex->SlotNames.Num());
}

void UXBSaveSubsystem::SaveSlotIndexToDisk() const
{
    if (!SaveSlotIndex)
    {
        return;
    }

    // ✨ 新增 - 保存索引存档，保证槽位列表可被列举
    // 🔧 修改 - 使用地图特定索引名
    const FString MapIndexName = GetMapSpecificIndexName();
    if (!UGameplayStatics::SaveGameToSlot(SaveSlotIndex, MapIndexName, 0))
    {
        UE_LOG(LogTemp, Warning, TEXT("保存存档槽位索引失败：%s"), *MapIndexName);
    }
}

FString UXBSaveSubsystem::BuildFullSlotName(const FString& SlotName) const
{
    // 🔧 修改 - 加入地图名称，实现按场景分离存档
    // 格式: XBSave_地图名_槽位名
    return SaveSlotPrefix + GetCurrentMapName() + TEXT("_") + SlotName;
}

FString UXBSaveSubsystem::GetCurrentMapName() const
{
    if (UWorld* World = GetWorld())
    {
        // 获取当前地图名称（不含路径和后缀）
        FString MapName = World->GetMapName();
        // 移除 UEDPIE 前缀（编辑器 PIE 模式下会有这个前缀）
        MapName.RemoveFromStart(World->StreamingLevelsPrefix);
        return MapName;
    }
    return TEXT("Default");
}

FString UXBSaveSubsystem::GetMapSpecificIndexName() const
{
    // 格式: XBSaveIndex_地图名
    return SaveSlotIndexName + TEXT("_") + GetCurrentMapName();
}

