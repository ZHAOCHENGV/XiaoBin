// Copyright XiaoBing Project. All Rights Reserved.

#include "Utils/XBBlueprintFunctionLibrary.h"
#include "Army/XBArmySubsystem.h"
#include "Army/XBFormationManager.h"
#include "Save/XBSaveSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

FString UXBBlueprintFunctionLibrary::FormatNumberWithK(float Value)
{
    if (Value >= 1000.0f)
    {
        float KValue = Value / 1000.0f;
        // 保留一位小数
        return FString::Printf(TEXT("%.1fK"), KValue);
    }
    else
    {
        return FString::Printf(TEXT("%.0f"), Value);
    }
}

FString UXBBlueprintFunctionLibrary::FormatHealth(float CurrentHealth, float MaxHealth)
{
    FString CurrentStr = FormatNumberWithK(CurrentHealth);
    FString MaxStr = FormatNumberWithK(MaxHealth);
    return FString::Printf(TEXT("%s / %s"), *CurrentStr, *MaxStr);
}

bool UXBBlueprintFunctionLibrary::AreFactionHostile(EXBFaction FactionA, EXBFaction FactionB)
{
    // 中立对所有人不敌对
    if (FactionA == EXBFaction::Neutral || FactionB == EXBFaction::Neutral)
    {
        return false;
    }

    // 相同阵营不敌对
    if (FactionA == FactionB)
    {
        return false;
    }

    // 不同阵营敌对
    return true;
}

FString UXBBlueprintFunctionLibrary::GetFactionDisplayName(EXBFaction Faction)
{
    switch (Faction)
    {
    case EXBFaction::Neutral:
        return TEXT("中立");
    case EXBFaction::Player:
        return TEXT("玩家");
    case EXBFaction::Enemy1:
        return TEXT("敌方1");
    case EXBFaction::Enemy2:
        return TEXT("敌方2");
    case EXBFaction::Enemy3:
        return TEXT("敌方3");
    default:
        return TEXT("未知");
    }
}

FLinearColor UXBBlueprintFunctionLibrary::GetFactionColor(EXBFaction Faction)
{
    switch (Faction)
    {
    case EXBFaction::Neutral:
        return FLinearColor::White;
    case EXBFaction::Player:
        return FLinearColor::Blue;
    case EXBFaction::Enemy1:
        return FLinearColor::Red;
    case EXBFaction::Enemy2:
        return FLinearColor(1.0f, 0.5f, 0.0f); // 橙色
    case EXBFaction::Enemy3:
        return FLinearColor(0.5f, 0.0f, 0.5f); // 紫色
    default:
        return FLinearColor::Gray;
    }
}

void UXBBlueprintFunctionLibrary::GetFormationDimensions(int32 SoldierCount, int32& OutColumns, int32& OutRows)
{
    UXBFormationManager::CalculateFormationDimensions(SoldierCount, OutColumns, OutRows);
}

FVector UXBBlueprintFunctionLibrary::GetFormationSlotOffset(int32 SlotIndex, int32 TotalSoldiers,
    float HorizontalSpacing, float VerticalSpacing, float MinDistanceToLeader)
{
    FXBFormationConfig Config;
    Config.HorizontalSpacing = HorizontalSpacing;
    Config.VerticalSpacing = VerticalSpacing;
    Config.MinDistanceToLeader = MinDistanceToLeader;

    return UXBFormationManager::CalculateSlotOffset(SlotIndex, TotalSoldiers, Config);
}

UXBArmySubsystem* UXBBlueprintFunctionLibrary::GetArmySubsystem(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    return World->GetSubsystem<UXBArmySubsystem>();
}

UXBSaveSubsystem* UXBBlueprintFunctionLibrary::GetSaveSubsystem(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return nullptr;
    }

    return GameInstance->GetSubsystem<UXBSaveSubsystem>();
}