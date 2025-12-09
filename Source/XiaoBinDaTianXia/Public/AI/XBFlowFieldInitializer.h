/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/AI/XBFlowFieldInitializer.h

/**
 * @file XBFlowFieldInitializer.h
 * @brief 流场初始化器 - 用于配置和初始化流场系统
 * 
 * @details 功能说明：
 *          1. 在编辑器和运行时可视化流场
 *          2. 检测地形和障碍物并标记到流场
 *          3. 支持通过标签、类或手动指定查找主将
 *          4. 提供完整的调试绘制选项
 * 
 * @note 注意事项：
 *       - 需要场景中存在 UXBFlowFieldSubsystem
 *       - 障碍物检测使用 ObjectTypes 配置
 *       - 地形检测使用 TerrainChannel 配置
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "XBFlowFieldInitializer.generated.h"

class UXBFlowFieldSubsystem;
class UXBFlowField;
class AXBCharacterBase;

/**
 * @brief 障碍物检测配置结构体
 * @details 定义障碍物检测的各项参数
 */
USTRUCT(BlueprintType)
struct FXBObstacleDetectionConfig
{
    GENERATED_BODY()

    // ✨ 新增 - 障碍物检测的对象类型（如 WorldStatic, WorldDynamic）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "障碍物检测", meta = (DisplayName = "障碍物对象类型"))
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;

    // ✨ 新增 - 地形检测使用的碰撞通道
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "地形检测", meta = (DisplayName = "地形碰撞通道"))
    TEnumAsByte<ECollisionChannel> TerrainChannel = ECC_Visibility;

    // 检测高度偏移（从地面向上）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "障碍物检测", meta = (DisplayName = "检测高度偏移", ClampMin = "0"))
    float DetectionHeightOffset = 50.0f;

    // 检测区域高度
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "障碍物检测", meta = (DisplayName = "检测区域高度", ClampMin = "10"))
    float DetectionHeight = 300.0f;

    // 检测半径缩放（相对于格子大小）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "障碍物检测", meta = (DisplayName = "检测半径缩放", ClampMin = "0.1", ClampMax = "2.0"))
    float DetectionRadiusScale = 0.6f;

    // 扩展检测范围（格子数）- 用于更好地包裹障碍物
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "障碍物检测", meta = (DisplayName = "障碍物扩展格数", ClampMin = "0", ClampMax = "3"))
    int32 ExpansionCells = 1;

    // 使用盒体检测而非球体
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "障碍物检测", meta = (DisplayName = "使用盒体检测"))
    bool bUseBoxDetection = true;

    // 忽略的Actor类
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "障碍物检测|忽略", meta = (DisplayName = "忽略的Actor类"))
    TArray<TSubclassOf<AActor>> IgnoredActorClasses;

    // 忽略带有这些标签的Actor
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "障碍物检测|忽略", meta = (DisplayName = "忽略的Actor标签"))
    TArray<FName> IgnoredActorTags;

    // 最小障碍物尺寸（小于此尺寸的碰撞体忽略）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "障碍物检测|过滤", meta = (DisplayName = "最小障碍物尺寸", ClampMin = "0"))
    float MinObstacleSize = 10.0f;
};

/**
 * @brief 调试绘制配置结构体
 * @details 定义流场调试可视化的各项参数
 */
USTRUCT(BlueprintType)
struct FXBFlowFieldDebugConfig
{
    GENERATED_BODY()

    // 启用调试绘制（运行时）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试", meta = (DisplayName = "启用调试绘制"))
    bool bEnableDebugDraw = true;

    // 编辑器预览
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试", meta = (DisplayName = "编辑器预览"))
    bool bShowEditorPreview = true;

    // 绘制边界框
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|绘制元素", meta = (DisplayName = "绘制边界框"))
    bool bDrawBounds = true;

    // 绘制网格线
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|绘制元素", meta = (DisplayName = "绘制网格线"))
    bool bDrawGrid = false;

    // 绘制格子
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|绘制元素", meta = (DisplayName = "绘制格子"))
    bool bDrawCells = true;

    // 绘制流向箭头
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|绘制元素", meta = (DisplayName = "绘制流向箭头"))
    bool bDrawFlowDirections = true;

    // 绘制障碍物
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|绘制元素", meta = (DisplayName = "绘制障碍物"))
    bool bDrawObstacles = true;

    // 绘制积分值
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|绘制元素", meta = (DisplayName = "绘制积分值"))
    bool bDrawIntegrationValues = false;

    // 编辑器中预览障碍物
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|编辑器", meta = (DisplayName = "编辑器预览障碍物"))
    bool bEditorPreviewObstacles = true;

    // 调试绘制间隔
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|时间", meta = (DisplayName = "调试绘制间隔", ClampMin = "0.01"))
    float DebugDrawInterval = 0.1f;

    // 线条粗细 - 边界
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|线条粗细", meta = (DisplayName = "边界线粗细", ClampMin = "0.5", ClampMax = "10.0"))
    float BoundsLineThickness = 3.0f;

    // 线条粗细 - 网格
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|线条粗细", meta = (DisplayName = "网格线粗细", ClampMin = "0.1", ClampMax = "5.0"))
    float GridLineThickness = 0.5f;

    // 线条粗细 - 格子边框
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|线条粗细", meta = (DisplayName = "格子边框粗细", ClampMin = "0.1", ClampMax = "5.0"))
    float CellLineThickness = 1.0f;

    // 线条粗细 - 箭头
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|线条粗细", meta = (DisplayName = "箭头线粗细", ClampMin = "0.5", ClampMax = "5.0"))
    float ArrowLineThickness = 2.0f;

    // 线条粗细 - 障碍物
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|线条粗细", meta = (DisplayName = "障碍物线粗细", ClampMin = "0.5", ClampMax = "10.0"))
    float ObstacleLineThickness = 3.0f;

    // 箭头长度缩放
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|箭头", meta = (DisplayName = "箭头长度缩放", ClampMin = "0.1", ClampMax = "2.0"))
    float ArrowLengthScale = 0.6f;

    // 箭头头部大小
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|箭头", meta = (DisplayName = "箭头头部大小", ClampMin = "1.0", ClampMax = "50.0"))
    float ArrowHeadSize = 15.0f;

    // 障碍物填充透明度
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|颜色", meta = (DisplayName = "障碍物填充透明度", ClampMin = "0", ClampMax = "255"))
    uint8 ObstacleFillAlpha = 100;

    // 障碍物颜色
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|颜色", meta = (DisplayName = "障碍物颜色"))
    FColor ObstacleColor = FColor::Red;

    // 流向箭头颜色
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|颜色", meta = (DisplayName = "流向箭头颜色"))
    FColor FlowArrowColor = FColor::Blue;

    // 边界颜色
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "调试|颜色", meta = (DisplayName = "边界颜色"))
    FColor BoundsColor = FColor::Green;
};

/**
 * @brief 流场配置结构体
 * @details 定义流场的基本参数和子配置
 */
USTRUCT(BlueprintType)
struct FXBFlowFieldConfig
{
    GENERATED_BODY()

    // 流场大小
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "流场", meta = (DisplayName = "流场大小", ClampMin = "1000"))
    FVector2D FlowFieldSize = FVector2D(5000.0f, 5000.0f);

    // 格子大小
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "流场", meta = (DisplayName = "格子大小", ClampMin = "50"))
    float CellSize = 100.0f;

    // 障碍物检测配置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "流场", meta = (DisplayName = "障碍物检测配置"))
    FXBObstacleDetectionConfig ObstacleConfig;

    // 调试配置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "流场", meta = (DisplayName = "调试配置"))
    FXBFlowFieldDebugConfig DebugConfig;

    // 流场更新间隔
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "流场|更新", meta = (DisplayName = "流场更新间隔", ClampMin = "0.05"))
    float UpdateInterval = 0.2f;

    // 障碍物扫描间隔
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "流场|更新", meta = (DisplayName = "障碍物扫描间隔", ClampMin = "0.1"))
    float ObstacleScanInterval = 0.5f;
};

/**
 * @brief 主将查找模式枚举
 */
UENUM(BlueprintType)
enum class EXBLeaderFindMode : uint8
{
    ByTag       UMETA(DisplayName = "通过标签"),
    ByClass     UMETA(DisplayName = "通过类"),
    Manual      UMETA(DisplayName = "手动指定")
};

/**
 * @brief 流场初始化器Actor
 * @details 详细流程：
 *          1. BeginPlay时自动初始化流场子系统
 *          2. 扫描场景中的障碍物和地形
 *          3. 查找并注册主将到流场
 *          4. Tick中持续更新障碍物和流场
 *          5. 提供编辑器预览和运行时调试绘制
 */
UCLASS(Blueprintable)
class XIAOBINDATIANXIA_API AXBFlowFieldInitializer : public AActor
{
    GENERATED_BODY()

public:
    AXBFlowFieldInitializer();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }
#endif

    // ========== 配置 ==========
    
    // 流场配置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "流场", meta = (DisplayName = "流场配置"))
    FXBFlowFieldConfig Config;

    // 自动初始化
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "流场|初始化", meta = (DisplayName = "自动初始化"))
    bool bAutoInitialize = true;

    // 自动扫描障碍物
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "流场|初始化", meta = (DisplayName = "自动扫描障碍物"))
    bool bAutoScanObstacles = true;

    // ========== 主将配置 ==========
    
    // 主将查找模式
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "流场|主将", meta = (DisplayName = "主将查找模式"))
    EXBLeaderFindMode LeaderFindMode = EXBLeaderFindMode::ByTag;

    // 主将标签（通过标签查找时使用）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "流场|主将", 
        meta = (DisplayName = "主将标签", EditCondition = "LeaderFindMode == EXBLeaderFindMode::ByTag"))
    FGameplayTag LeaderTag;

    // 主将类（通过类查找时使用）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "流场|主将", 
        meta = (DisplayName = "主将类", EditCondition = "LeaderFindMode == EXBLeaderFindMode::ByClass"))
    TSubclassOf<AXBCharacterBase> LeaderClass;

    // 手动指定的主将
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "流场|主将", 
        meta = (DisplayName = "手动指定主将", EditCondition = "LeaderFindMode == EXBLeaderFindMode::Manual"))
    TSoftObjectPtr<AXBCharacterBase> ManualLeader;

    // ========== 公开函数 ==========

    /**
     * @brief 初始化流场
     * @details 获取流场子系统，查找主将并注册
     */
    UFUNCTION(BlueprintCallable, Category = "流场", meta = (DisplayName = "初始化流场"))
    void InitializeFlowField();

    /**
     * @brief 扫描障碍物
     * @details 扫描场景中的障碍物并标记到流场
     */
    UFUNCTION(BlueprintCallable, Category = "流场", meta = (DisplayName = "扫描障碍物"))
    void ScanObstacles();

    /**
     * @brief 刷新流场
     * @details 更新流场数据
     */
    UFUNCTION(BlueprintCallable, Category = "流场", meta = (DisplayName = "刷新流场"))
    void RefreshFlowField();

    /**
     * @brief 切换调试绘制
     * @param bEnable 是否启用
     */
    UFUNCTION(BlueprintCallable, Category = "流场", meta = (DisplayName = "切换调试绘制"))
    void ToggleDebugDraw(bool bEnable);

    /**
     * @brief 查找主将
     * @return 找到的主将，未找到返回nullptr
     */
    UFUNCTION(BlueprintCallable, Category = "流场", meta = (DisplayName = "查找主将"))
    AXBCharacterBase* FindLeader();

    /**
     * @brief 设置主将
     * @param NewLeader 新的主将
     */
    UFUNCTION(BlueprintCallable, Category = "流场", meta = (DisplayName = "设置主将"))
    void SetLeader(AXBCharacterBase* NewLeader);

protected:
    // 流场子系统引用
    UPROPERTY()
    TObjectPtr<UXBFlowFieldSubsystem> FlowFieldSubsystem;

    // 当前主将引用
    UPROPERTY()
    TWeakObjectPtr<AXBCharacterBase> CurrentLeader;

    // 运行时障碍物缓存（格子坐标）
    TSet<FIntPoint> RuntimeObstacleCache;

    // 编辑器障碍物缓存
    TSet<FIntPoint> EditorObstacleCache;

    // 障碍物扫描计时器
    float ObstacleScanTimer = 0.0f;
    
    // 调试绘制计时器
    float DebugDrawTimer = 0.0f;
    
    // 流场更新计时器
    float FlowFieldUpdateTimer = 0.0f;

    // 是否已初始化
    bool bIsInitialized = false;

    // ========== 内部函数 ==========

    /**
     * @brief 执行障碍物扫描
     * @details 遍历流场格子，检测每个格子是否有障碍物
     */
    void PerformObstacleScan();

    /**
     * @brief 检查指定位置是否有障碍物
     * @param WorldPosition 世界坐标
     * @return 是否有障碍物
     */
    bool CheckObstacleAt(const FVector& WorldPosition) const;

    /**
     * @brief 获取地形高度
     * @param WorldPosition 世界坐标（XY）
     * @return 地形Z坐标
     */
    float GetTerrainHeight(const FVector& WorldPosition) const;

    /**
     * @brief 扩展障碍物标记
     * @details 将障碍物标记扩展到周围格子
     */
    void ExpandObstacleMarking();

    /**
     * @brief 绘制调试信息（运行时）
     */
    void DrawDebugInfo();

    /**
     * @brief 扫描障碍物（编辑器预览用）
     */
    void ScanObstaclesForEditorPreview();

#if WITH_EDITOR
    /**
     * @brief 绘制编辑器预览
     */
    void DrawEditorPreview();
#endif
};
