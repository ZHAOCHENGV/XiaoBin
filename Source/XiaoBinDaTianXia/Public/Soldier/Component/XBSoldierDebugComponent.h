/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Soldier/Component/XBSoldierDebugComponent.h

/**
 * @file XBSoldierDebugComponent.h
 * @brief 士兵状态可视化调试组件
 *
 * @note ✨ 新增文件
 * @note 🔧 修改 - 修复位掩码在编辑器中不显示完整的问题
 *              - 修复默认值导致不选也显示的问题
 *              - 修复黑色区域问题
 */

#pragma once

#include "Army/XBSoldierTypes.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "XBSoldierDebugComponent.generated.h"


class AXBSoldierCharacter;

/**
 * @brief 士兵状态可视化调试组件
 *
 * @note 核心功能：
 *       - 实时显示士兵状态信息
 *       - 可配置的显示选项（使用独立布尔变量而非位掩码）
 *       - 支持全局开关和单个士兵开关
 *       - 性能友好（可按需开关）
 *
 * @note 使用方式：
 *       1. 在编辑器中勾选 "启用调试"
 *       2. 在 "显示选项" 分类中勾选需要显示的内容
 *       3. 或使用控制台命令 XB.Debug.Soldier.Toggle 全局切换
 */
UCLASS(ClassGroup = (Custom),
       meta = (BlueprintSpawnableComponent, DisplayName = "XB Soldier Debug"))
class XIAOBINDATIANXIA_API UXBSoldierDebugComponent : public UActorComponent {
  GENERATED_BODY()

public:
  UXBSoldierDebugComponent();

  virtual void BeginPlay() override;
  virtual void
  TickComponent(float DeltaTime, ELevelTick TickType,
                FActorComponentTickFunction *ThisTickFunction) override;

  // ==================== 调试控制 ====================

  /**
   * @brief 启用/禁用调试显示
   * @param bEnable 是否启用
   */
  UFUNCTION(BlueprintCallable, Category = "XB|Debug",
            meta = (DisplayName = "启用调试显示"))
  void SetDebugEnabled(bool bEnable);

  /**
   * @brief 获取是否启用调试
   */
  UFUNCTION(BlueprintPure, Category = "XB|Debug",
            meta = (DisplayName = "是否启用调试"))
  bool IsDebugEnabled() const { return bEnableDebug; }

  /**
   * @brief 启用所有显示选项
   */
  UFUNCTION(BlueprintCallable, Category = "XB|Debug",
            meta = (DisplayName = "启用所有选项"))
  void EnableAllOptions();

  /**
   * @brief 禁用所有显示选项
   */
  UFUNCTION(BlueprintCallable, Category = "XB|Debug",
            meta = (DisplayName = "禁用所有选项"))
  void DisableAllOptions();

  // ==================== 静态控制（全局开关） ====================

  /**
   * @brief 全局启用/禁用所有士兵的调试显示
   * @param bEnable 是否启用
   */
  UFUNCTION(BlueprintCallable, Category = "XB|Debug",
            meta = (DisplayName = "全局启用调试"))
  static void SetGlobalDebugEnabled(bool bEnable);

  /**
   * @brief 获取全局调试状态
   */
  UFUNCTION(BlueprintPure, Category = "XB|Debug",
            meta = (DisplayName = "获取全局调试状态"))
  static bool IsGlobalDebugEnabled() { return bGlobalDebugEnabled; }

protected:
  // ==================== 主开关 ====================

  /**
   * @brief 是否启用调试显示（本地开关）
   * @note 需要同时满足：本地开关启用 或 全局开关启用
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|开关",
            meta = (DisplayName = "启用调试"))
  bool bEnableDebug = false;

  // ==================== 显示选项（使用独立布尔变量） ====================
  // 🔧 修改 - 不再使用位掩码，改用独立的布尔变量，解决编辑器显示问题

  /** @brief 显示状态信息（状态、阵营、类型等） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|显示选项",
            meta = (DisplayName = "状态信息", EditCondition = "bEnableDebug"))
  bool bShowState = false;

  /** @brief 显示血量条和数值 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|显示选项",
            meta = (DisplayName = "血量信息", EditCondition = "bEnableDebug"))
  bool bShowHealth = false;

  /** @brief 显示攻击目标和连线 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|显示选项",
            meta = (DisplayName = "攻击目标", EditCondition = "bEnableDebug"))
  bool bShowTarget = false;

  /** @brief 显示编队位置标记 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|显示选项",
            meta = (DisplayName = "编队位置", EditCondition = "bEnableDebug"))
  bool bShowFormation = false;

  /** @brief 显示攻击范围圆圈 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|显示选项",
            meta = (DisplayName = "攻击范围", EditCondition = "bEnableDebug"))
  bool bShowAttackRange = false;

  /** @brief 显示视野/检测范围圆圈 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|显示选项",
            meta = (DisplayName = "视野范围", EditCondition = "bEnableDebug"))
  bool bShowVisionRange = false;

  /** @brief 显示速度方向箭头 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|显示选项",
            meta = (DisplayName = "速度向量", EditCondition = "bEnableDebug"))
  bool bShowVelocity = false;

  /** @brief 显示到将领的连线 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|显示选项",
            meta = (DisplayName = "将领连线", EditCondition = "bEnableDebug"))
  bool bShowLeaderLine = false;

  /** @brief 显示AI信息（控制器和任务） */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|显示选项",
            meta = (DisplayName = "AI详细信息", EditCondition = "bEnableDebug"))
  bool bShowAIInfo = false;

  // ==================== 显示配置 ====================

  /** @brief 文字显示高度偏移 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|显示配置",
            meta = (DisplayName = "文字高度偏移", ClampMin = "0.0"))
  float TextHeightOffset = 120.0f;

  /** @brief 文字缩放 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|显示配置",
            meta = (DisplayName = "文字缩放", ClampMin = "0.5",
                    ClampMax = "3.0"))
  float TextScale = 1.0f;

  // ==================== 颜色配置 ====================

  /** @brief 状态文字颜色 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|颜色",
            meta = (DisplayName = "状态文字颜色"))
  FColor StateTextColor = FColor::White;

  /** @brief 血量充足颜色 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|颜色",
            meta = (DisplayName = "血量充足颜色"))
  FColor HealthFullColor = FColor::Green;

  /** @brief 血量不足颜色 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|颜色",
            meta = (DisplayName = "血量不足颜色"))
  FColor HealthLowColor = FColor::Red;

  /** @brief 目标连线颜色 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|颜色",
            meta = (DisplayName = "目标连线颜色"))
  FColor TargetLineColor = FColor::Red;

  /** @brief 将领连线颜色 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|颜色",
            meta = (DisplayName = "将领连线颜色"))
  FColor LeaderLineColor = FColor::Cyan;

  /** @brief 编队位置颜色 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|颜色",
            meta = (DisplayName = "编队位置颜色"))
  FColor FormationColor = FColor::Yellow;

  /** @brief 攻击范围颜色 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|颜色",
            meta = (DisplayName = "攻击范围颜色"))
  FColor AttackRangeColor = FColor::Orange;

  /** @brief 视野范围颜色 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|颜色",
            meta = (DisplayName = "视野范围颜色"))
  FColor VisionRangeColor = FColor::Blue;

  /** @brief 速度向量颜色 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|颜色",
            meta = (DisplayName = "速度向量颜色"))
  FColor VelocityColor = FColor::Magenta;

  /** @brief AI信息文字颜色 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|颜色",
            meta = (DisplayName = "AI信息颜色"))
  FColor AIInfoColor = FColor::Cyan;

  // ==================== 范围显示配置 ====================

  /** @brief 范围圆圈段数 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|显示配置",
            meta = (DisplayName = "圆圈段数", ClampMin = "8", ClampMax = "64"))
  int32 CircleSegments = 24;

  /** @brief 范围圆圈线宽 */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|Debug|显示配置",
            meta = (DisplayName = "圆圈线宽", ClampMin = "1.0"))
  float CircleThickness = 2.0f;

private:
  // ==================== 内部方法 ====================

  /**
   * @brief 检查是否应该显示调试信息
   * @return 本地开关或全局开关启用时返回 true
   */
  bool ShouldDrawDebug() const;

  /**
   * @brief 检查是否有任何选项被启用
   * @return 至少有一个选项启用时返回 true
   */
  bool HasAnyOptionEnabled() const;

  /** @brief 绘制状态文字 */
  void DrawStateText();

  /** @brief 绘制血量信息 */
  void DrawHealthInfo();

  /** @brief 绘制目标信息和连线 */
  void DrawTargetInfo();

  /** @brief 绘制编队位置 */
  void DrawFormationPosition();

  /** @brief 绘制攻击范围 */
  void DrawAttackRange();

  /** @brief 绘制视野范围 */
  void DrawVisionRange();

  /** @brief 绘制速度向量 */
  void DrawVelocity();

  /** @brief 绘制将领连线 */
  void DrawLeaderLine();

  /** @brief 绘制AI信息 */
  void DrawAIInfo();

  /** @brief 获取状态名称 */
  FString GetStateName(EXBSoldierState State) const;

  /** @brief 获取阵营名称 */
  FString GetFactionName(EXBFaction Faction) const;

  /** @brief 获取士兵类型名称 */
  FString GetSoldierTypeName(EXBSoldierType Type) const;

  /** @brief 缓存的士兵引用 */
  UPROPERTY()
  TWeakObjectPtr<AXBSoldierCharacter> CachedSoldier;

  /** @brief 全局调试开关 */
  static bool bGlobalDebugEnabled;
};
