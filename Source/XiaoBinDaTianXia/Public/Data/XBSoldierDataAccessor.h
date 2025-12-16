/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Data/XBSoldierDataAccessor.h

/**
 * @file XBSoldierDataAccessor.h
 * @brief 士兵数据访问器 - 统一资源加载和数据访问接口
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/XBSoldierDataTable.h"
#include "Engine/StreamableManager.h"      // ✨ 新增 - 修复 FStreamableHandle 未声明
#include "XBSoldierDataAccessor.generated.h"

// ============================================
// 资源加载策略枚举
// ============================================

UENUM(BlueprintType)
enum class EXBResourceLoadStrategy : uint8
{
    Synchronous     UMETA(DisplayName = "同步加载"),
    Asynchronous    UMETA(DisplayName = "异步加载"),
    Lazy            UMETA(DisplayName = "延迟加载")
};

// ============================================
// 资源加载完成委托
// ============================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResourcesLoadedDelegate, bool, bSuccess);

// ============================================
// 智能数据访问器组件
// ============================================

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName="XB 士兵数据访问器"))
class XIAOBINDATIANXIA_API UXBSoldierDataAccessor : public UActorComponent
{
    GENERATED_BODY()

public:
    UXBSoldierDataAccessor();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // ==================== 初始化接口 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|Data", meta = (DisplayName = "初始化数据"))
    bool Initialize(UDataTable* DataTable, FName RowName, EXBResourceLoadStrategy LoadStrategy = EXBResourceLoadStrategy::Synchronous);

    UFUNCTION(BlueprintPure, Category = "XB|Data", meta = (DisplayName = "是否已初始化"))
    bool IsInitialized() const { return bIsInitialized; }

    UFUNCTION(BlueprintPure, Category = "XB|Data", meta = (DisplayName = "资源是否已加载"))
    bool AreResourcesLoaded() const { return bResourcesLoaded; }

    // ==================== 数据访问接口 ====================

    FORCEINLINE const FXBSoldierTableRow& GetRawData() const { return CachedTableRow; }

    // --- 基础属性访问 ---

    UFUNCTION(BlueprintPure, Category = "XB|Data|Basic", meta = (DisplayName = "获取士兵类型"))
    EXBSoldierType GetSoldierType() const { return CachedTableRow.SoldierType; }

    UFUNCTION(BlueprintPure, Category = "XB|Data|Basic", meta = (DisplayName = "获取显示名称"))
    FText GetDisplayName() const { return CachedTableRow.DisplayName; }

    UFUNCTION(BlueprintPure, Category = "XB|Data|Basic", meta = (DisplayName = "获取描述"))
    FText GetDescription() const { return CachedTableRow.Description; }

    UFUNCTION(BlueprintPure, Category = "XB|Data|Basic", meta = (DisplayName = "获取标签"))
    FGameplayTagContainer GetSoldierTags() const { return CachedTableRow.SoldierTags; }

    // --- 战斗属性访问 ---

    UFUNCTION(BlueprintPure, Category = "XB|Data|Combat", meta = (DisplayName = "获取最大血量"))
    float GetMaxHealth() const { return CachedTableRow.MaxHealth; }

    UFUNCTION(BlueprintPure, Category = "XB|Data|Combat", meta = (DisplayName = "获取基础伤害"))
    float GetBaseDamage() const { return CachedTableRow.BaseDamage; }

    UFUNCTION(BlueprintPure, Category = "XB|Data|Combat", meta = (DisplayName = "获取攻击范围"))
    float GetAttackRange() const { return CachedTableRow.AttackRange; }

    UFUNCTION(BlueprintPure, Category = "XB|Data|Combat", meta = (DisplayName = "获取攻击间隔"))
    float GetAttackInterval() const { return CachedTableRow.AttackInterval; }

    // --- 移动属性访问 ---

    UFUNCTION(BlueprintPure, Category = "XB|Data|Movement", meta = (DisplayName = "获取移动速度"))
    float GetMoveSpeed() const { return CachedTableRow.MoveSpeed; }

    UFUNCTION(BlueprintPure, Category = "XB|Data|Movement", meta = (DisplayName = "获取冲刺倍率"))
    float GetSprintSpeedMultiplier() const { return CachedTableRow.SprintSpeedMultiplier; }

    UFUNCTION(BlueprintPure, Category = "XB|Data|Movement", meta = (DisplayName = "获取跟随插值速度"))
    float GetFollowInterpSpeed() const { return CachedTableRow.FollowInterpSpeed; }

    UFUNCTION(BlueprintPure, Category = "XB|Data|Movement", meta = (DisplayName = "获取旋转速度"))
    float GetRotationSpeed() const { return CachedTableRow.RotationSpeed; }

    // --- AI属性访问 ---

    UFUNCTION(BlueprintPure, Category = "XB|Data|AI", meta = (DisplayName = "获取视野范围"))
    float GetVisionRange() const { return CachedTableRow.AIConfig.VisionRange; }

    UFUNCTION(BlueprintPure, Category = "XB|Data|AI", meta = (DisplayName = "获取脱离距离"))
    float GetDisengageDistance() const { return CachedTableRow.AIConfig.DisengageDistance; }

    UFUNCTION(BlueprintPure, Category = "XB|Data|AI", meta = (DisplayName = "获取返回延迟"))
    float GetReturnDelay() const { return CachedTableRow.AIConfig.ReturnDelay; }

    UFUNCTION(BlueprintPure, Category = "XB|Data|AI", meta = (DisplayName = "获取到达阈值"))
    float GetArrivalThreshold() const { return CachedTableRow.AIConfig.ArrivalThreshold; }

    UFUNCTION(BlueprintPure, Category = "XB|Data|AI", meta = (DisplayName = "获取避让半径"))
    float GetAvoidanceRadius() const { return CachedTableRow.AIConfig.AvoidanceRadius; }

    UFUNCTION(BlueprintPure, Category = "XB|Data|AI", meta = (DisplayName = "获取避让权重"))
    float GetAvoidanceWeight() const { return CachedTableRow.AIConfig.AvoidanceWeight; }

    // --- 加成属性访问 ---

    UFUNCTION(BlueprintPure, Category = "XB|Data|Bonus", meta = (DisplayName = "获取血量加成"))
    float GetHealthBonusToLeader() const { return CachedTableRow.HealthBonusToLeader; }

    UFUNCTION(BlueprintPure, Category = "XB|Data|Bonus", meta = (DisplayName = "获取伤害加成"))
    float GetDamageBonusToLeader() const { return CachedTableRow.DamageBonusToLeader; }

    UFUNCTION(BlueprintPure, Category = "XB|Data|Bonus", meta = (DisplayName = "获取缩放加成"))
    float GetScaleBonusToLeader() const { return CachedTableRow.ScaleBonusToLeader; }

    // ==================== 资源访问接口 ====================

    UFUNCTION(BlueprintPure, Category = "XB|Data|Resources", meta = (DisplayName = "获取骨骼网格"))
    USkeletalMesh* GetSkeletalMesh();

    UFUNCTION(BlueprintPure, Category = "XB|Data|Resources", meta = (DisplayName = "获取动画蓝图类"))
    TSubclassOf<UAnimInstance> GetAnimClass();

    UFUNCTION(BlueprintPure, Category = "XB|Data|Resources", meta = (DisplayName = "获取死亡蒙太奇"))
    UAnimMontage* GetDeathMontage();

    UFUNCTION(BlueprintPure, Category = "XB|Data|Resources", meta = (DisplayName = "获取行为树"))
    UBehaviorTree* GetBehaviorTree();

    UFUNCTION(BlueprintPure, Category = "XB|Data|Resources", meta = (DisplayName = "获取普攻蒙太奇"))
    UAnimMontage* GetBasicAttackMontage();

    // ==================== 委托事件 ====================

    UPROPERTY(BlueprintAssignable, Category = "XB|Data|Events", meta = (DisplayName = "资源加载完成"))
    FOnResourcesLoadedDelegate OnResourcesLoaded;

protected:
    // ==================== 内部方法 ====================

    bool LoadResourcesSynchronous();
    void LoadResourcesAsynchronous();
    void OnAsyncLoadComplete();

    /**
     * @brief 延迟加载单个资源
     * @note 🔧 修改 - 支持 TSoftObjectPtr 和 TSoftClassPtr
     */
    template<typename T>
    T* LazyLoadResource(const TSoftObjectPtr<T>& SoftObjectPtr);

    // ✨ 新增 - 支持 TSoftClassPtr
    template<typename T>
    TSubclassOf<T> LazyLoadClass(const TSoftClassPtr<T>& SoftClassPtr);

protected:
    // ==================== 缓存数据 ====================

    UPROPERTY()
    FXBSoldierTableRow CachedTableRow;

    UPROPERTY()
    FName CachedRowName;

    UPROPERTY()
    TWeakObjectPtr<UDataTable> CachedDataTable;

    // ==================== 资源缓存 ====================

    UPROPERTY()
    TObjectPtr<USkeletalMesh> LoadedSkeletalMesh;

    UPROPERTY()
    TSubclassOf<UAnimInstance> LoadedAnimClass;

    UPROPERTY()
    TObjectPtr<UAnimMontage> LoadedDeathMontage;

    UPROPERTY()
    TObjectPtr<UBehaviorTree> LoadedBehaviorTree;

    UPROPERTY()
    TObjectPtr<UAnimMontage> LoadedBasicAttackMontage;

    // ==================== 状态标记 ====================

    UPROPERTY(BlueprintReadOnly, Category = "XB|Data|State", meta = (DisplayName = "已初始化"))
    bool bIsInitialized = false;

    UPROPERTY(BlueprintReadOnly, Category = "XB|Data|State", meta = (DisplayName = "资源已加载"))
    bool bResourcesLoaded = false;

    UPROPERTY()
    EXBResourceLoadStrategy CurrentLoadStrategy = EXBResourceLoadStrategy::Synchronous;

    /** @brief 🔧 修改 - 异步加载句柄（使用正确的智能指针类型） */
    TSharedPtr<FStreamableHandle> AsyncLoadHandle;
};
