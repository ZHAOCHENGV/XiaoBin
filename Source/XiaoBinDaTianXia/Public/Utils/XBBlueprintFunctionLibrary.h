// Copyright XiaoBing Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Army/XBSoldierTypes.h"
#include "XBBlueprintFunctionLibrary.generated.h"

/**
 * 蓝图函数库
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // ============ 数值格式化 ============

    /**
     * 将数值格式化为 K 单位显示
     * 例如：1500 -> "1.5K"，13100 -> "13.1K"
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "XB|Utils")
    static FString FormatNumberWithK(float Value);

    /**
     * 格式化血量显示
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "XB|Utils")
    static FString FormatHealth(float CurrentHealth, float MaxHealth);

    // ============ 阵营判断 ============

    /**
     * 检查两个阵营是否敌对
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "XB|Faction")
    static bool AreFactionHostile(EXBFaction FactionA, EXBFaction FactionB);

    /**
     * 获取阵营显示名称
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "XB|Faction")
    static FString GetFactionDisplayName(EXBFaction Faction);

    /**
     * 获取阵营颜色
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "XB|Faction")
    static FLinearColor GetFactionColor(EXBFaction Faction);

    // ============ 编队计算 ============

    /**
     * 计算编队维度
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "XB|Formation")
    static void GetFormationDimensions(int32 SoldierCount, int32& OutColumns, int32& OutRows);

    /**
     * 计算槽位偏移
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "XB|Formation")
    static FVector GetFormationSlotOffset(int32 SlotIndex, int32 TotalSoldiers, 
        float HorizontalSpacing = 100.0f, float VerticalSpacing = 100.0f, float MinDistanceToLeader = 150.0f);

    // ============ 系统访问 ============

    /**
     * 获取军队子系统
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "XB|System", meta = (WorldContext = "WorldContextObject"))
    static class UXBArmySubsystem* GetArmySubsystem(const UObject* WorldContextObject);

    /**
     * 获取存档子系统
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "XB|System", meta = (WorldContext = "WorldContextObject"))
    static class UXBSaveSubsystem* GetSaveSubsystem(const UObject* WorldContextObject);
};