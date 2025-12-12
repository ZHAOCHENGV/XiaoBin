/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Core/XBBlueprintFunctionLibrary.h

/**
 * @file XBBlueprintFunctionLibrary.h
 * @brief 项目通用蓝图函数库
 * 
 * @note ✨ 新增文件
 *       功能：
 *       1. 统一的敌对关系判断
 *       2. 球形范围检测
 *       3. 阵营工具函数
 */

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Army/XBSoldierTypes.h"
#include "XBBlueprintFunctionLibrary.generated.h"

class AXBCharacterBase;
class AXBSoldierCharacter;

/**
 * @brief 范围检测结果结构体
 */
USTRUCT(BlueprintType)
struct XIAOBINDATIANXIA_API FXBDetectionResult
{
    GENERATED_BODY()

    /** @brief 检测到的Actor列表 */
    UPROPERTY(BlueprintReadWrite, Category = "检测")
    TArray<AActor*> DetectedActors;

    /** @brief 最近的Actor */
    UPROPERTY(BlueprintReadWrite, Category = "检测")
    AActor* NearestActor = nullptr;

    /** @brief 到最近Actor的距离 */
    UPROPERTY(BlueprintReadWrite, Category = "检测")
    float NearestDistance = MAX_FLT;

    /** @brief 检测到的数量 */
    UPROPERTY(BlueprintReadWrite, Category = "检测")
    int32 Count = 0;
};

/**
 * @brief 项目通用蓝图函数库
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // ==================== 阵营关系判断 ====================

    /**
     * @brief 检查两个阵营是否敌对
     * @param FactionA 阵营A
     * @param FactionB 阵营B
     * @return 是否敌对
     * @note 规则：
     *       - 相同阵营不敌对
     *       - 中立阵营不与任何阵营敌对
     *       - 玩家/友军互不敌对
     *       - 玩家/友军与敌人敌对
     */
    UFUNCTION(BlueprintPure, Category = "XB|阵营", meta = (DisplayName = "阵营是否敌对"))
    static bool AreFactionsHostile(EXBFaction FactionA, EXBFaction FactionB);

    /**
     * @brief 检查两个阵营是否友好
     * @param FactionA 阵营A
     * @param FactionB 阵营B
     * @return 是否友好
     */
    UFUNCTION(BlueprintPure, Category = "XB|阵营", meta = (DisplayName = "阵营是否友好"))
    static bool AreFactionsFriendly(EXBFaction FactionA, EXBFaction FactionB);

    /**
     * @brief 检查两个Actor是否敌对
     * @param ActorA Actor A
     * @param ActorB Actor B
     * @return 是否敌对
     * @note 支持 AXBCharacterBase 和 AXBSoldierCharacter
     */
    UFUNCTION(BlueprintPure, Category = "XB|阵营", meta = (DisplayName = "Actor是否敌对"))
    static bool AreActorsHostile(const AActor* ActorA, const AActor* ActorB);

    /**
     * @brief 获取Actor的阵营
     * @param Actor 目标Actor
     * @return 阵营枚举
     */
    UFUNCTION(BlueprintPure, Category = "XB|阵营", meta = (DisplayName = "获取Actor阵营"))
    static EXBFaction GetActorFaction(const AActor* Actor);

    /**
     * @brief 检查Actor是否存活
     * @param Actor 目标Actor
     * @return 是否存活（未死亡且有效）
     */
    UFUNCTION(BlueprintPure, Category = "XB|阵营", meta = (DisplayName = "Actor是否存活"))
    static bool IsActorAlive(const AActor* Actor);

    // ==================== 范围检测 ====================

    /**
     * @brief 球形范围检测敌人
     * @param WorldContext 世界上下文
     * @param Origin 检测中心点
     * @param Radius 检测半径
     * @param SourceFaction 发起检测的阵营
     * @param bIgnoreDead 是否忽略死亡单位
     * @param OutResult 输出检测结果
     * @return 是否检测到敌人
     * @note 使用 OverlapMultiByChannel 进行高效检测
     */
    UFUNCTION(BlueprintCallable, Category = "XB|检测", meta = (DisplayName = "球形检测敌人", WorldContext = "WorldContext"))
    static bool DetectEnemiesInRadius(
        const UObject* WorldContext,
        const FVector& Origin,
        float Radius,
        EXBFaction SourceFaction,
        bool bIgnoreDead,
        FXBDetectionResult& OutResult
    );

    /**
     * @brief 球形范围检测友军
     * @param WorldContext 世界上下文
     * @param Origin 检测中心点
     * @param Radius 检测半径
     * @param SourceFaction 发起检测的阵营
     * @param bIgnoreDead 是否忽略死亡单位
     * @param OutResult 输出检测结果
     * @return 是否检测到友军
     */
    UFUNCTION(BlueprintCallable, Category = "XB|检测", meta = (DisplayName = "球形检测友军", WorldContext = "WorldContext"))
    static bool DetectAlliesInRadius(
        const UObject* WorldContext,
        const FVector& Origin,
        float Radius,
        EXBFaction SourceFaction,
        bool bIgnoreDead,
        FXBDetectionResult& OutResult
    );

    /**
     * @brief 球形范围检测所有战斗单位
     * @param WorldContext 世界上下文
     * @param Origin 检测中心点
     * @param Radius 检测半径
     * @param bIgnoreDead 是否忽略死亡单位
     * @param OutResult 输出检测结果
     * @return 是否检测到单位
     */
    UFUNCTION(BlueprintCallable, Category = "XB|检测", meta = (DisplayName = "球形检测所有单位", WorldContext = "WorldContext"))
    static bool DetectAllUnitsInRadius(
        const UObject* WorldContext,
        const FVector& Origin,
        float Radius,
        bool bIgnoreDead,
        FXBDetectionResult& OutResult
    );

    /**
     * @brief 寻找范围内最近的敌人
     * @param WorldContext 世界上下文
     * @param Origin 检测中心点
     * @param Radius 检测半径
     * @param SourceFaction 发起检测的阵营
     * @param bIgnoreDead 是否忽略死亡单位
     * @return 最近的敌人（无则返回nullptr）
     */
    UFUNCTION(BlueprintCallable, Category = "XB|检测", meta = (DisplayName = "寻找最近敌人", WorldContext = "WorldContext"))
    static AActor* FindNearestEnemy(
        const UObject* WorldContext,
        const FVector& Origin,
        float Radius,
        EXBFaction SourceFaction,
        bool bIgnoreDead = true
    );

    // ==================== 距离计算 ====================

    /**
     * @brief 计算两个Actor之间的2D距离
     * @param ActorA Actor A
     * @param ActorB Actor B
     * @return 2D距离（忽略Z轴）
     */
    UFUNCTION(BlueprintPure, Category = "XB|距离", meta = (DisplayName = "计算2D距离"))
    static float GetDistance2D(const AActor* ActorA, const AActor* ActorB);

    /**
     * @brief 计算两个Actor之间的3D距离
     * @param ActorA Actor A
     * @param ActorB Actor B
     * @return 3D距离
     */
    UFUNCTION(BlueprintPure, Category = "XB|距离", meta = (DisplayName = "计算3D距离"))
    static float GetDistance3D(const AActor* ActorA, const AActor* ActorB);

private:
    /**
     * @brief 内部球形检测实现
     * @param World 世界指针
     * @param Origin 中心点
     * @param Radius 半径
     * @param OutHits 输出命中结果
     * @return 是否有命中
     */
    static bool PerformSphereOverlap(
        UWorld* World,
        const FVector& Origin,
        float Radius,
        TArray<FOverlapResult>& OutHits
    );
};
