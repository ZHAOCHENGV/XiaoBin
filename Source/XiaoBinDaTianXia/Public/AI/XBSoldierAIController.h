/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/XBSoldierAIController.h

/**
 * @file XBSoldierAIController.h
 * @brief 士兵AI控制器 - 支持行为树和黑板系统
 * 
 * @note 🔧 修改记录:
 *       1. 黑板键 SoldierState 改用 Int 类型（蓝图中 Enum 不可用）
 *       2. 添加黑板键类型校验
 *       3. 增强安全检查
 */

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "XBSoldierAIController.generated.h"

// 前向声明
class UBehaviorTreeComponent;
class UBlackboardComponent;
class UBehaviorTree;
class AXBSoldierCharacter;

/**
 * @brief 士兵黑板键名常量
 * @note 统一管理所有黑板变量名，避免字符串硬编码
 *       🔧 修改 - 添加键类型注释，SoldierState 使用 Int 类型
 */
namespace XBSoldierBBKeys
{
    // ==================== 对象类型键 (Object) ====================
    const FName Leader = TEXT("Leader");                    // AActor*
    const FName CurrentTarget = TEXT("CurrentTarget");      // AActor*
    const FName Self = TEXT("Self");                        // AActor*
    
    // ==================== 位置类型键 (Vector) ====================
    const FName TargetLocation = TEXT("TargetLocation");        // FVector
    const FName FormationPosition = TEXT("FormationPosition");  // FVector
    const FName HomeLocation = TEXT("HomeLocation");            // FVector
    
    // ==================== 整数类型键 (Int) ====================
    // 🔧 修改 - SoldierState 改用 Int 类型（蓝图中 Enum 不可搜索）
    const FName SoldierState = TEXT("SoldierState");        // Int32 (对应 EXBSoldierState)
    const FName FormationSlot = TEXT("FormationSlot");      // Int32
    
    // ==================== 浮点类型键 (Float) ====================
    const FName AttackRange = TEXT("AttackRange");          // float
    const FName DetectionRange = TEXT("DetectionRange");    // float
    const FName VisionRange = TEXT("VisionRange");          // float (✨ 新增)
    const FName DistanceToTarget = TEXT("DistanceToTarget");// float
    const FName DistanceToLeader = TEXT("DistanceToLeader");// float
    
    // ==================== 布尔类型键 (Bool) ====================
    const FName HasTarget = TEXT("HasTarget");              // bool
    const FName IsInCombat = TEXT("IsInCombat");            // bool
    const FName ShouldRetreat = TEXT("ShouldRetreat");      // bool
    const FName IsAtFormation = TEXT("IsAtFormation");      // bool
    const FName CanAttack = TEXT("CanAttack");              // bool
}

/**
 * @brief 黑板键类型枚举
 * @note ✨ 新增 - 用于类型校验
 */
UENUM()
enum class EXBBlackboardKeyType : uint8
{
    Object,
    Vector,
    Int,
    Float,
    Bool,
    Unknown
};

/**
 * @brief 士兵AI控制器
 */
UCLASS()
class XIAOBINDATIANXIA_API AXBSoldierAIController : public AAIController
{
    GENERATED_BODY()

public:
    AXBSoldierAIController();

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

public:
    virtual void Tick(float DeltaTime) override;

    // ==================== 行为树控制 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "启动行为树"))
    bool StartBehaviorTree(UBehaviorTree* BehaviorTreeAsset);

    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "停止行为树"))
    void StopBehaviorTreeLogic();

    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "暂停行为树"))
    void PauseBehaviorTree(bool bPause);

    // ==================== 黑板值更新 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "设置目标"))
    void SetTargetActor(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "设置将领"))
    void SetLeader(AActor* Leader);

    /**
     * @brief 设置士兵状态
     * @param NewState 新状态（使用 Int 类型）
     * @note 🔧 修改 - 使用 SetValueAsInt 替代 SetValueAsEnum
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "设置状态"))
    void SetSoldierState(uint8 NewState);

    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "设置攻击范围"))
    void SetAttackRange(float Range);

    // ✨ 新增 - 设置视野范围
    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "设置视野范围"))
    void SetVisionRange(float Range);

    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "更新战斗状态"))
    void UpdateCombatState(bool bInCombat);

    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "刷新黑板"))
    void RefreshBlackboardValues();

    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "安全刷新黑板"))
    void RefreshBlackboardValuesSafe();

    // ==================== 访问器 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "获取士兵"))
    AXBSoldierCharacter* GetSoldierActor() const;

    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "获取行为树组件"))
    UBehaviorTreeComponent* GetBehaviorTreeComponent() const { return BehaviorTreeComp; }

    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "获取黑板组件"))
    UBlackboardComponent* GetSoldierBlackboard() const { return BlackboardComp; }

    // ==================== 黑板键校验 ====================

    /**
     * @brief 校验黑板键是否存在且类型正确
     * @param KeyName 键名
     * @param ExpectedType 期望的类型
     * @return 是否校验通过
     * @note ✨ 新增 - 运行时类型校验
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "校验黑板键"))
    bool ValidateBlackboardKey(FName KeyName, EXBBlackboardKeyType ExpectedType) const;

    /**
     * @brief 校验所有必需的黑板键
     * @return 是否所有键都存在且类型正确
     * @note ✨ 新增 - 初始化时调用
     */
    UFUNCTION(BlueprintCallable, Category = "XB|AI", meta = (DisplayName = "校验所有黑板键"))
    bool ValidateAllBlackboardKeys() const;

protected:
    // ==================== 组件 ====================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "行为树组件"))
    TObjectPtr<UBehaviorTreeComponent> BehaviorTreeComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "组件", meta = (DisplayName = "黑板组件"))
    TObjectPtr<UBlackboardComponent> BlackboardComp;

    // ==================== 配置 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI配置", meta = (DisplayName = "默认行为树"))
    TObjectPtr<UBehaviorTree> DefaultBehaviorTree;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI配置", meta = (DisplayName = "黑板更新间隔", ClampMin = "0.05"))
    float BlackboardUpdateInterval = 0.1f;

    // ✨ 新增 - 是否在初始化时校验黑板键
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI配置", meta = (DisplayName = "启用黑板键校验"))
    bool bValidateBlackboardKeys = true;

private:
    TWeakObjectPtr<AXBSoldierCharacter> CachedSoldier;
    float BlackboardUpdateTimer = 0.0f;
    bool bIsInitialized = false;

    bool SetupSoldierBlackboard(UBehaviorTree* BT);
    void UpdateDistanceValuesSafe();

    UFUNCTION()
    void DelayedOnPossess();

    // ✨ 新增 - 获取黑板键类型
    EXBBlackboardKeyType GetBlackboardKeyType(FName KeyName) const;
};
