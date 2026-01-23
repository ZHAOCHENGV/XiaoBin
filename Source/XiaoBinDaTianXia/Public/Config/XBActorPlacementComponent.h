/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/Config/XBActorPlacementComponent.h

/**
 * @file XBActorPlacementComponent.h
 * @brief 配置阶段 Actor 放置管理组件
 * 
 * @note ✨ 新增文件 - 负责射线检测、预览、放置、编辑功能
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "XBPlacementTypes.h"
#include "XBActorPlacementComponent.generated.h"

class UXBPlacementConfigAsset;
class UMaterialInterface;
class UMaterialInstanceDynamic;

// ============ 代理声明 ============

/** 放置状态变更代理 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlacementStateChanged, EXBPlacementState, NewState);

/** 请求显示选择菜单代理 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRequestShowMenu, FVector, ClickLocation);

/** Actor 放置完成代理 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActorPlaced, AActor*, PlacedActor, int32, EntryIndex);

/** Actor 被删除代理 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorDeleted, AActor*, DeletedActor);

/** 选中 Actor 变更代理 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectionChanged, AActor*, SelectedActor);

/**
 * @brief 配置阶段 Actor 放置管理组件
 * @note 挂载在 AXBConfigCameraPawn 上，负责放置系统的核心逻辑
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent, DisplayName = "Actor放置组件"))
class XIAOBINDATIANXIA_API UXBActorPlacementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UXBActorPlacementComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============ 公共接口 ============

	/**
	 * @brief 处理点击输入
	 * @return 是否成功处理
	 * @note   详细流程分析: 根据当前状态决定行为（空闲->显示菜单，预览->确认放置，编辑->取消选中或选中新目标）
	 */
	UFUNCTION(BlueprintCallable, Category = "放置系统", meta = (DisplayName = "处理点击"))
	bool HandleClick();

	/**
	 * @brief 开始预览指定索引的 Actor
	 * @param EntryIndex 配置条目索引
	 * @return 是否成功开始预览
	 */
	UFUNCTION(BlueprintCallable, Category = "放置系统", meta = (DisplayName = "开始预览"))
	bool StartPreview(int32 EntryIndex);

	/**
	 * @brief 确认放置当前预览的 Actor
	 * @return 放置的 Actor 指针
	 */
	UFUNCTION(BlueprintCallable, Category = "放置系统", meta = (DisplayName = "确认放置"))
	AActor* ConfirmPlacement();

	/**
	 * @brief 取消当前操作（预览或编辑）
	 */
	UFUNCTION(BlueprintCallable, Category = "放置系统", meta = (DisplayName = "取消操作"))
	void CancelOperation();

	/**
	 * @brief 删除当前选中的 Actor
	 * @return 是否成功删除
	 */
	UFUNCTION(BlueprintCallable, Category = "放置系统", meta = (DisplayName = "删除选中"))
	bool DeleteSelectedActor();

	/**
	 * @brief 旋转当前预览或选中的 Actor
	 * @param YawDelta Yaw 旋转增量
	 */
	UFUNCTION(BlueprintCallable, Category = "放置系统", meta = (DisplayName = "旋转Actor"))
	void RotateActor(float YawDelta);

	/**
	 * @brief 获取当前放置状态
	 * @return 状态枚举
	 */
	UFUNCTION(BlueprintPure, Category = "放置系统", meta = (DisplayName = "获取放置状态"))
	EXBPlacementState GetPlacementState() const { return CurrentState; }

	/**
	 * @brief 获取当前选中的 Actor
	 * @return 选中的 Actor 指针
	 */
	UFUNCTION(BlueprintPure, Category = "放置系统", meta = (DisplayName = "获取选中Actor"))
	AActor* GetSelectedActor() const { return SelectedActor.Get(); }

	/**
	 * @brief 获取所有已放置的 Actor 数据
	 * @return 已放置 Actor 数据数组
	 */
	UFUNCTION(BlueprintPure, Category = "放置系统", meta = (DisplayName = "获取已放置数据"))
	const TArray<FXBPlacedActorData>& GetPlacedActorData() const { return PlacedActors; }

	/**
	 * @brief 根据存档数据恢复放置的 Actor
	 * @param SavedData 存档的 Actor 数据数组
	 */
	UFUNCTION(BlueprintCallable, Category = "放置系统", meta = (DisplayName = "从存档恢复"))
	void RestoreFromSaveData(const TArray<FXBPlacedActorData>& SavedData);

	// ============ 数据资产访问接口 ============

	/**
	 * @brief 获取放置配置资产
	 * @return 放置配置 DataAsset 指针
	 */
	UFUNCTION(BlueprintPure, Category = "放置系统|数据", meta = (DisplayName = "获取放置配置"))
	UXBPlacementConfigAsset* GetPlacementConfig() const { return PlacementConfig; }

	/**
	 * @brief 获取可放置 Actor 条目数量
	 * @return 条目数量
	 */
	UFUNCTION(BlueprintPure, Category = "放置系统|数据", meta = (DisplayName = "获取可放置数量"))
	int32 GetSpawnableActorCount() const;

	/**
	 * @brief 按索引获取可放置 Actor 条目
	 * @param Index 索引
	 * @param OutEntry 输出条目
	 * @return 是否获取成功
	 */
	UFUNCTION(BlueprintCallable, Category = "放置系统|数据", meta = (DisplayName = "获取可放置条目"))
	bool GetSpawnableActorEntry(int32 Index, FXBSpawnableActorEntry& OutEntry) const;

	/**
	 * @brief 获取所有可放置 Actor 条目数组
	 * @return 条目数组引用
	 * @note 用于 UI 遍历生成按钮
	 */
	UFUNCTION(BlueprintPure, Category = "放置系统|数据", meta = (DisplayName = "获取全部可放置条目"))
	const TArray<FXBSpawnableActorEntry>& GetAllSpawnableActorEntries() const;

	// ============ 代理事件 ============

	/** 放置状态变更事件 */
	UPROPERTY(BlueprintAssignable, Category = "放置系统|事件")
	FOnPlacementStateChanged OnPlacementStateChanged;

	/** 请求显示选择菜单事件 */
	UPROPERTY(BlueprintAssignable, Category = "放置系统|事件")
	FOnRequestShowMenu OnRequestShowMenu;

	/** Actor 放置完成事件 */
	UPROPERTY(BlueprintAssignable, Category = "放置系统|事件")
	FOnActorPlaced OnActorPlaced;

	/** Actor 被删除事件 */
	UPROPERTY(BlueprintAssignable, Category = "放置系统|事件")
	FOnActorDeleted OnActorDeleted;

	/** 选中 Actor 变更事件 */
	UPROPERTY(BlueprintAssignable, Category = "放置系统|事件")
	FOnSelectionChanged OnSelectionChanged;

protected:
	// ============ 配置引用 ============

	/** 放置配置 DataAsset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "放置配置", meta = (DisplayName = "放置配置"))
	TObjectPtr<UXBPlacementConfigAsset> PlacementConfig;

private:
	// ============ 内部方法 ============

	/**
	 * @brief 获取鼠标射线检测命中位置
	 * @param OutLocation 输出命中位置
	 * @param OutNormal 输出命中法线
	 * @return 是否命中
	 */
	bool GetMouseHitLocation(FVector& OutLocation, FVector& OutNormal) const;

	/**
	 * @brief 更新预览 Actor 位置
	 */
	void UpdatePreviewLocation();

	/**
	 * @brief 创建预览 Actor
	 * @param EntryIndex 配置条目索引
	 * @return 是否成功创建
	 */
	bool CreatePreviewActor(int32 EntryIndex);

	/**
	 * @brief 销毁预览 Actor
	 */
	void DestroyPreviewActor();

	/**
	 * @brief 应用预览材质到 Actor
	 * @param Actor 目标 Actor
	 * @param bValid 是否有效位置
	 */
	void ApplyPreviewMaterial(AActor* Actor, bool bValid);

	/**
	 * @brief 恢复 Actor 原始材质
	 * @param Actor 目标 Actor
	 */
	void RestoreOriginalMaterials(AActor* Actor);

	/**
	 * @brief 检测射线命中的已放置 Actor
	 * @param OutActor 输出命中的 Actor
	 * @return 是否命中
	 */
	bool GetHitPlacedActor(AActor*& OutActor) const;

	/**
	 * @brief 选中指定 Actor
	 * @param Actor 要选中的 Actor
	 */
	void SelectActor(AActor* Actor);

	/**
	 * @brief 取消选中
	 */
	void DeselectActor();

	/**
	 * @brief 设置放置状态
	 * @param NewState 新状态
	 */
	void SetPlacementState(EXBPlacementState NewState);

	/**
	 * @brief 对指定位置进行地面检测
	 * @param InLocation 输入位置
	 * @param OutGroundLocation 输出地面位置
	 * @return 是否检测到地面
	 */
	bool TraceForGround(const FVector& InLocation, FVector& OutGroundLocation) const;

	// ============ 内部状态 ============

	/** 当前放置状态 */
	EXBPlacementState CurrentState = EXBPlacementState::Idle;

	/** 当前预览的配置条目索引 */
	int32 CurrentPreviewEntryIndex = -1;

	/** 预览 Actor 引用 */
	UPROPERTY()
	TWeakObjectPtr<AActor> PreviewActor;

	/** 当前选中的 Actor */
	UPROPERTY()
	TWeakObjectPtr<AActor> SelectedActor;

	/** 预览 Actor 当前位置 */
	FVector PreviewLocation = FVector::ZeroVector;

	/** 预览 Actor 当前旋转 */
	FRotator PreviewRotation = FRotator::ZeroRotator;

	/** 预览位置是否有效 */
	bool bIsPreviewLocationValid = false;

	/** 已放置的 Actor 数据列表 */
	UPROPERTY()
	TArray<FXBPlacedActorData> PlacedActors;

	/** 缓存的预览动态材质实例 */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> CachedPreviewMID;

	/** 缓存的玩家控制器 */
	UPROPERTY()
	TWeakObjectPtr<APlayerController> CachedPlayerController;

	/** 上一次点击的世界位置（用于显示菜单） */
	FVector LastClickLocation = FVector::ZeroVector;
};
