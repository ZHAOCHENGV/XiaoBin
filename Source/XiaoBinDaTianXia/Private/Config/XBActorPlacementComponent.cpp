/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Config/XBActorPlacementComponent.cpp

/**
 * @file XBActorPlacementComponent.cpp
 * @brief 配置阶段 Actor 放置管理组件实现
 * 
 * @note ✨ 新增文件
 */

#include "Config/XBActorPlacementComponent.h"
#include "Config/XBPlacementConfigAsset.h"
#include "Character/Components/XBMagnetFieldComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Utils/XBLogCategories.h"

UXBActorPlacementComponent::UXBActorPlacementComponent()
{
	// 启用 Tick 用于更新预览位置
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UXBActorPlacementComponent::BeginPlay()
{
	Super::BeginPlay();

	// 缓存玩家控制器
	CachedPlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	// 初始化为 Idle 状态，启用 Tick 以支持悬停检测
	CurrentState = EXBPlacementState::Idle;
	SetComponentTickEnabled(true);

	UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 初始化完成，PlacementConfig: %s"), PlacementConfig ? *PlacementConfig->GetName() : TEXT("None"));
}

void UXBActorPlacementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 清理预览 Actor
	DestroyPreviewActor();

	Super::EndPlay(EndPlayReason);
}

void UXBActorPlacementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 预览状态时更新预览位置
	if (CurrentState == EXBPlacementState::Previewing)
	{
		UpdatePreviewLocation();
	}

	// 空闲状态时更新悬停状态（用于高亮显示）
	if (CurrentState == EXBPlacementState::Idle)
	{
		UpdateHoverState();
	}
}

bool UXBActorPlacementComponent::HandleClick()
{
	switch (CurrentState)
	{
	case EXBPlacementState::Idle:
		{
			// 空闲状态 -> 检测是否点击已放置 Actor 或请求显示菜单
			AActor* HitActor = nullptr;
			if (GetHitPlacedActor(HitActor))
			{
				// 点击到已放置的 Actor，进入编辑状态
				SelectActor(HitActor);
				return true;
			}

			// 未点击到已放置 Actor，获取点击位置并广播显示菜单事件
			FVector HitLocation;
			FVector HitNormal;
			if (GetMouseHitLocation(HitLocation, HitNormal))
			{
				LastClickLocation = HitLocation;
				OnRequestShowMenu.Broadcast(HitLocation);
				return true;
			}
			return false;
		}

	case EXBPlacementState::Previewing:
		{
			// 预览状态 -> 确认放置
			if (bIsPreviewLocationValid)
			{
				ConfirmPlacement();
				return true;
			}
			return false;
		}

	case EXBPlacementState::Editing:
		{
			// 编辑状态 -> 检测是否点击其他 Actor 或取消选中
			AActor* HitActor = nullptr;
			if (GetHitPlacedActor(HitActor))
			{
				if (HitActor != SelectedActor.Get())
				{
					// 选中其他 Actor
					SelectActor(HitActor);
				}
				return true;
			}

			// 点击空白区域，取消选中
			DeselectActor();
			return true;
		}

	default:
		return false;
	}
}

bool UXBActorPlacementComponent::StartPreview(int32 EntryIndex)
{
	if (!PlacementConfig)
	{
		UE_LOG(LogXBConfig, Warning, TEXT("[放置组件] 未配置 PlacementConfig"));
		return false;
	}

	const FXBSpawnableActorEntry* Entry = PlacementConfig->GetEntryByIndexPtr(EntryIndex);
	if (!Entry || !Entry->ActorClass)
	{
		UE_LOG(LogXBConfig, Warning, TEXT("[放置组件] 无效的条目索引: %d"), EntryIndex);
		return false;
	}

	// 销毁旧的预览 Actor
	DestroyPreviewActor();

	// 创建预览 Actor
	if (!CreatePreviewActor(EntryIndex))
	{
		return false;
	}

	CurrentPreviewEntryIndex = EntryIndex;

	// 根据旋转模式设置初始旋转
	switch (Entry->RotationMode)
	{
	case EXBPlacementRotationMode::Manual:
		// 手动模式使用默认旋转
		PreviewRotation = Entry->DefaultRotation;
		break;

	case EXBPlacementRotationMode::FacePlayer:
		// 朝向玩家模式：计算面朝玩家 Pawn 的方向
		if (APawn* PlayerPawn = CachedPlayerController.IsValid() ? CachedPlayerController->GetPawn() : nullptr)
		{
			FVector PawnLocation = PlayerPawn->GetActorLocation();
			FVector PreviewDir = PawnLocation - PreviewLocation;
			PreviewDir.Z = 0.0f;
			if (!PreviewDir.IsNearlyZero())
			{
				PreviewRotation = PreviewDir.Rotation();
			}
			else
			{
				PreviewRotation = Entry->DefaultRotation;
			}
		}
		else
		{
			PreviewRotation = Entry->DefaultRotation;
		}
		break;

	case EXBPlacementRotationMode::Random:
		// 随机模式：随机 Yaw 角度
		PreviewRotation = Entry->DefaultRotation;
		PreviewRotation.Yaw = FMath::FRandRange(0.0f, 360.0f);
		break;

	default:
		PreviewRotation = Entry->DefaultRotation;
		break;
	}

	// 切换到预览状态
	SetPlacementState(EXBPlacementState::Previewing);

	return true;
}

AActor* UXBActorPlacementComponent::ConfirmPlacement()
{
	if (CurrentState != EXBPlacementState::Previewing)
	{
		return nullptr;
	}

	if (!PreviewActor.IsValid() || !PlacementConfig)
	{
		return nullptr;
	}

	const FXBSpawnableActorEntry* Entry = PlacementConfig->GetEntryByIndexPtr(CurrentPreviewEntryIndex);
	if (!Entry || !Entry->ActorClass)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// 生成实际 Actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* NewActor = World->SpawnActor<AActor>(
		Entry->ActorClass,
		PreviewLocation,
		PreviewRotation,
		SpawnParams
	);

	if (!NewActor)
	{
		UE_LOG(LogXBConfig, Warning, TEXT("[放置组件] 生成 Actor 失败: %s"), *Entry->ActorClass->GetName());
		return nullptr;
	}

	// 应用缩放
	NewActor->SetActorScale3D(Entry->DefaultScale);

	// ✨ 修复 - 使用预览 Actor 的位置（已经在 UpdatePreviewLocation 中计算过偏移）
	// 获取预览 Actor 的当前位置作为最终放置位置
	FVector FinalLocation = PreviewActor.IsValid() ? PreviewActor->GetActorLocation() : PreviewLocation;
	NewActor->SetActorLocation(FinalLocation);

	// ✨ 新增 - 配置阶段禁用磁场组件（防止提前招募士兵）
	if (UXBMagnetFieldComponent* MagnetComp = NewActor->FindComponentByClass<UXBMagnetFieldComponent>())
	{
		MagnetComp->SetFieldEnabled(false);
		UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已禁用磁场组件: %s"), *NewActor->GetName());
	}

	// 记录放置数据
	FXBPlacedActorData PlacedData;
	PlacedData.PlacedActor = NewActor;
	PlacedData.EntryIndex = CurrentPreviewEntryIndex;
	PlacedData.ActorClassPath = FSoftClassPath(Entry->ActorClass);
	PlacedData.Location = FinalLocation;
	PlacedData.Rotation = PreviewRotation;
	PlacedData.Scale = Entry->DefaultScale;
	PlacedActors.Add(PlacedData);

	// ✨ 重要：在销毁预览 Actor 前保存连续放置相关数据
	const int32 PlacedEntryIndex = CurrentPreviewEntryIndex;
	const bool bGlobalContinuousMode = PlacementConfig && PlacementConfig->bContinuousPlacementMode;
	const bool bEntryContinuousMode = Entry->bContinuousPlacement;
	const bool bShouldContinue = bGlobalContinuousMode || bEntryContinuousMode;

	UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 连续放置检查 - 全局: %s, 条目: %s, 索引: %d, 应继续: %s"),
		bGlobalContinuousMode ? TEXT("开启") : TEXT("关闭"),
		bEntryContinuousMode ? TEXT("开启") : TEXT("关闭"),
		PlacedEntryIndex,
		bShouldContinue ? TEXT("是") : TEXT("否"));

	// 销毁预览 Actor（这会重置 CurrentPreviewEntryIndex）
	DestroyPreviewActor();

	// 广播事件
	OnActorPlaced.Broadcast(NewActor, PlacedEntryIndex);

	UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已放置 Actor: %s 位置: %s"), *NewActor->GetName(), *FinalLocation.ToString());

	// 连续放置模式：放置后自动继续预览同类型 Actor
	if (bShouldContinue)
	{
		// 直接调用 StartPreview，使用之前保存的索引
		const bool bStarted = StartPreview(PlacedEntryIndex);
		UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 连续放置模式：自动开始预览索引 %d，结果: %s"),
			PlacedEntryIndex, bStarted ? TEXT("成功") : TEXT("失败"));
	}
	else
	{
		// 回到空闲状态
		SetPlacementState(EXBPlacementState::Idle);
	}

	return NewActor;
}

void UXBActorPlacementComponent::CancelOperation()
{
	switch (CurrentState)
	{
	case EXBPlacementState::Previewing:
		DestroyPreviewActor();
		SetPlacementState(EXBPlacementState::Idle);
		break;

	case EXBPlacementState::Editing:
		DeselectActor();
		break;

	default:
		break;
	}
}

bool UXBActorPlacementComponent::DeleteSelectedActor()
{
	if (CurrentState != EXBPlacementState::Editing)
	{
		return false;
	}

	if (!SelectedActor.IsValid())
	{
		return false;
	}

	AActor* ActorToDelete = SelectedActor.Get();

	// 从已放置列表中移除
	PlacedActors.RemoveAll([ActorToDelete](const FXBPlacedActorData& Data)
	{
		return Data.PlacedActor.Get() == ActorToDelete;
	});

	// 广播删除事件
	OnActorDeleted.Broadcast(ActorToDelete);

	// 销毁 Actor
	ActorToDelete->Destroy();

	// 清空选中
	SelectedActor.Reset();

	UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已删除选中 Actor"));

	// 回到空闲状态
	SetPlacementState(EXBPlacementState::Idle);

	return true;
}

void UXBActorPlacementComponent::RotateActor(float YawDelta)
{
	if (!PlacementConfig)
	{
		return;
	}

	const float RotationStep = PlacementConfig->RotationSpeed * YawDelta;

	if (CurrentState == EXBPlacementState::Previewing && PreviewActor.IsValid())
	{
		// 预览模式下只有手动旋转模式才允许旋转
		const FXBSpawnableActorEntry* Entry = PlacementConfig->GetEntryByIndexPtr(CurrentPreviewEntryIndex);
		if (Entry && Entry->RotationMode == EXBPlacementRotationMode::Manual)
		{
			PreviewRotation.Yaw += RotationStep;
			PreviewActor->SetActorRotation(PreviewRotation);
		}
	}
	else if (CurrentState == EXBPlacementState::Editing && SelectedActor.IsValid())
	{
		// 编辑模式下始终允许旋转已放置的 Actor
		FRotator CurrentRot = SelectedActor->GetActorRotation();
		CurrentRot.Yaw += RotationStep;
		SelectedActor->SetActorRotation(CurrentRot);

		// 更新记录
		for (FXBPlacedActorData& Data : PlacedActors)
		{
			if (Data.PlacedActor.Get() == SelectedActor.Get())
			{
				Data.Rotation = CurrentRot;
				break;
			}
		}
	}
}

void UXBActorPlacementComponent::RestoreFromSaveData(const TArray<FXBPlacedActorData>& SavedData)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 清空当前已放置的 Actor
	for (const FXBPlacedActorData& Data : PlacedActors)
	{
		if (Data.PlacedActor.IsValid())
		{
			Data.PlacedActor->Destroy();
		}
	}
	PlacedActors.Empty();

	// 恢复存档中的 Actor
	for (const FXBPlacedActorData& SavedItem : SavedData)
	{
		UClass* ActorClass = SavedItem.ActorClassPath.TryLoadClass<AActor>();
		if (!ActorClass)
		{
			UE_LOG(LogXBConfig, Warning, TEXT("[放置组件] 无法加载类: %s"), *SavedItem.ActorClassPath.ToString());
			continue;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AActor* NewActor = World->SpawnActor<AActor>(
			ActorClass,
			SavedItem.Location,
			SavedItem.Rotation,
			SpawnParams
		);

		if (NewActor)
		{
			NewActor->SetActorScale3D(SavedItem.Scale);

			FXBPlacedActorData RestoredData = SavedItem;
			RestoredData.PlacedActor = NewActor;
			PlacedActors.Add(RestoredData);

			UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已恢复 Actor: %s"), *NewActor->GetName());
		}
	}

	UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 存档恢复完成，共恢复 %d 个 Actor"), PlacedActors.Num());
}

int32 UXBActorPlacementComponent::GetSpawnableActorCount() const
{
	if (!PlacementConfig)
	{
		return 0;
	}
	return PlacementConfig->GetEntryCount();
}

bool UXBActorPlacementComponent::GetSpawnableActorEntry(int32 Index, FXBSpawnableActorEntry& OutEntry) const
{
	if (!PlacementConfig)
	{
		return false;
	}
	return PlacementConfig->GetEntryByIndex(Index, OutEntry);
}

const TArray<FXBSpawnableActorEntry>& UXBActorPlacementComponent::GetAllSpawnableActorEntries() const
{
	// 返回空数组的静态引用作为 fallback
	static const TArray<FXBSpawnableActorEntry> EmptyArray;
	
	if (!PlacementConfig)
	{
		return EmptyArray;
	}
	return PlacementConfig->SpawnableActors;
}

void UXBActorPlacementComponent::SetPlacementConfig(UXBPlacementConfigAsset* Config)
{
	PlacementConfig = Config;
	UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已设置放置配置: %s"), Config ? *Config->GetName() : TEXT("None"));
}

bool UXBActorPlacementComponent::GetMouseHitLocation(FVector& OutLocation, FVector& OutNormal) const
{
	if (!CachedPlayerController.IsValid())
	{
		return false;
	}

	// 获取鼠标位置
	float MouseX, MouseY;
	if (!CachedPlayerController->GetMousePosition(MouseX, MouseY))
	{
		return false;
	}

	// 将屏幕坐标转换为世界射线
	FVector WorldLocation, WorldDirection;
	if (!CachedPlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection))
	{
		return false;
	}

	// 执行射线检测
	const float TraceDistance = PlacementConfig ? PlacementConfig->TraceDistance : 50000.0f;
	const FVector TraceEnd = WorldLocation + WorldDirection * TraceDistance;

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	// 忽略预览 Actor
	if (PreviewActor.IsValid())
	{
		QueryParams.AddIgnoredActor(PreviewActor.Get());
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if (World->LineTraceSingleByChannel(HitResult, WorldLocation, TraceEnd, ECC_Visibility, QueryParams))
	{
		OutLocation = HitResult.Location;
		OutNormal = HitResult.ImpactNormal;
		return true;
	}

	return false;
}

void UXBActorPlacementComponent::UpdatePreviewLocation()
{
	if (!PreviewActor.IsValid())
	{
		return;
	}

	FVector HitLocation;
	FVector HitNormal;
	if (GetMouseHitLocation(HitLocation, HitNormal))
	{
		// 检测地面
		FVector GroundLocation;
		const FXBSpawnableActorEntry* Entry = PlacementConfig ? PlacementConfig->GetEntryByIndexPtr(CurrentPreviewEntryIndex) : nullptr;
		
		if (Entry && Entry->bSnapToGround && TraceForGround(HitLocation, GroundLocation))
		{
			PreviewLocation = GroundLocation;
		}
		else
		{
			PreviewLocation = HitLocation;
		}

		// ✨ 修改 - 使用 CalculateActorBottomOffset 计算偏移，支持角色类型特殊处理
		const float ZOffset = CalculateActorBottomOffset(PreviewActor.Get());
		FVector AdjustedLocation = PreviewLocation;
		AdjustedLocation.Z += ZOffset;

		PreviewActor->SetActorLocation(AdjustedLocation);
		bIsPreviewLocationValid = true;

		// 更新预览材质颜色
		ApplyPreviewMaterial(PreviewActor.Get(), true);
	}
	else
	{
		bIsPreviewLocationValid = false;
		ApplyPreviewMaterial(PreviewActor.Get(), false);
	}
}

bool UXBActorPlacementComponent::CreatePreviewActor(int32 EntryIndex)
{
	if (!PlacementConfig)
	{
		return false;
	}

	const FXBSpawnableActorEntry* Entry = PlacementConfig->GetEntryByIndexPtr(EntryIndex);
	if (!Entry || !Entry->ActorClass)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// 生成预览 Actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* NewPreview = World->SpawnActor<AActor>(
		Entry->ActorClass,
		FVector::ZeroVector,
		Entry->DefaultRotation,
		SpawnParams
	);

	if (!NewPreview)
	{
		return false;
	}

	// 禁用碰撞
	NewPreview->SetActorEnableCollision(false);

	// 应用缩放
	NewPreview->SetActorScale3D(Entry->DefaultScale);

	// 应用预览材质
	ApplyPreviewMaterial(NewPreview, true);

	PreviewActor = NewPreview;

	return true;
}

void UXBActorPlacementComponent::DestroyPreviewActor()
{
	if (PreviewActor.IsValid())
	{
		PreviewActor->Destroy();
		PreviewActor.Reset();
	}

	CurrentPreviewEntryIndex = -1;
	bIsPreviewLocationValid = false;
}

void UXBActorPlacementComponent::ApplyPreviewMaterial(AActor* Actor, bool bValid)
{
	if (!Actor || !PlacementConfig)
	{
		return;
	}

	// 加载预览材质
	UMaterialInterface* PreviewMat = PlacementConfig->PreviewMaterial.LoadSynchronous();
	if (!PreviewMat)
	{
		return;
	}

	// 创建或更新动态材质实例
	if (!CachedPreviewMID)
	{
		CachedPreviewMID = UMaterialInstanceDynamic::Create(PreviewMat, this);
	}

	// 设置颜色
	const FLinearColor& Color = bValid ? PlacementConfig->ValidPreviewColor : PlacementConfig->InvalidPreviewColor;
	CachedPreviewMID->SetVectorParameterValue(TEXT("Color"), Color);

	// 应用到所有 Mesh 组件
	TArray<UPrimitiveComponent*> PrimitiveComps;
	Actor->GetComponents<UPrimitiveComponent>(PrimitiveComps);

	for (UPrimitiveComponent* PrimComp : PrimitiveComps)
	{
		if (PrimComp)
		{
			const int32 NumMaterials = PrimComp->GetNumMaterials();
			for (int32 i = 0; i < NumMaterials; ++i)
			{
				PrimComp->SetMaterial(i, CachedPreviewMID);
			}
		}
	}
}

void UXBActorPlacementComponent::RestoreOriginalMaterials(AActor* Actor)
{
	// 注意：此函数需要缓存原始材质才能恢复
	// 当前实现中预览 Actor 是销毁重建的，不需要恢复
	// 选中高亮功能可以在此扩展
}

bool UXBActorPlacementComponent::GetHitPlacedActor(AActor*& OutActor) const
{
	if (!CachedPlayerController.IsValid())
	{
		return false;
	}

	float MouseX, MouseY;
	if (!CachedPlayerController->GetMousePosition(MouseX, MouseY))
	{
		return false;
	}

	FVector WorldLocation, WorldDirection;
	if (!CachedPlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection))
	{
		return false;
	}

	const float TraceDistance = PlacementConfig ? PlacementConfig->TraceDistance : 50000.0f;
	const FVector TraceEnd = WorldLocation + WorldDirection * TraceDistance;

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	if (PreviewActor.IsValid())
	{
		QueryParams.AddIgnoredActor(PreviewActor.Get());
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// 使用 Pawn 通道以支持角色类型检测，同时也尝试 Visibility
	if (World->LineTraceSingleByChannel(HitResult, WorldLocation, TraceEnd, ECC_Pawn, QueryParams) ||
		World->LineTraceSingleByChannel(HitResult, WorldLocation, TraceEnd, ECC_Visibility, QueryParams))
	{
		AActor* HitActor = HitResult.GetActor();
		
		UE_LOG(LogXBConfig, Verbose, TEXT("[放置组件] 射线命中 Actor: %s"), HitActor ? *HitActor->GetName() : TEXT("None"));
		
		// 检查是否是已放置的 Actor
		for (const FXBPlacedActorData& Data : PlacedActors)
		{
			if (Data.PlacedActor.Get() == HitActor)
			{
				OutActor = HitActor;
				UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 检测到已放置的 Actor: %s"), *HitActor->GetName());
				return true;
			}
		}
		
		UE_LOG(LogXBConfig, Verbose, TEXT("[放置组件] 命中的 Actor 不在已放置列表中，PlacedActors 数量: %d"), PlacedActors.Num());
	}

	return false;
}

void UXBActorPlacementComponent::SelectActor(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	// 取消之前的选中
	if (SelectedActor.IsValid() && SelectedActor.Get() != Actor)
	{
		RestoreOriginalMaterials(SelectedActor.Get());
	}

	SelectedActor = Actor;

	// TODO: 应用选中高亮效果

	SetPlacementState(EXBPlacementState::Editing);

	OnSelectionChanged.Broadcast(Actor);

	UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 选中 Actor: %s"), *Actor->GetName());
}

void UXBActorPlacementComponent::DeselectActor()
{
	if (SelectedActor.IsValid())
	{
		RestoreOriginalMaterials(SelectedActor.Get());
	}

	SelectedActor.Reset();

	SetPlacementState(EXBPlacementState::Idle);

	OnSelectionChanged.Broadcast(nullptr);
}

void UXBActorPlacementComponent::SetPlacementState(EXBPlacementState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	CurrentState = NewState;

	// 根据状态启用/禁用 Tick
	// Idle 状态需要 Tick 用于悬停检测，Previewing 状态需要 Tick 用于更新预览位置
	const bool bNeedsTick = (NewState == EXBPlacementState::Idle || NewState == EXBPlacementState::Previewing);
	SetComponentTickEnabled(bNeedsTick);

	// 广播状态变更事件
	OnPlacementStateChanged.Broadcast(NewState);

	UE_LOG(LogXBConfig, Verbose, TEXT("[放置组件] 状态变更: %d"), static_cast<int32>(NewState));
}

bool UXBActorPlacementComponent::TraceForGround(const FVector& InLocation, FVector& OutGroundLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const float Offset = PlacementConfig ? PlacementConfig->GroundTraceOffset : 500.0f;
	const FVector TraceStart = InLocation + FVector(0.0f, 0.0f, Offset);
	const FVector TraceEnd = InLocation - FVector(0.0f, 0.0f, Offset * 10.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	if (PreviewActor.IsValid())
	{
		QueryParams.AddIgnoredActor(PreviewActor.Get());
	}

	if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
	{
		OutGroundLocation = HitResult.Location;
		return true;
	}

	OutGroundLocation = InLocation;
	return false;
}

// ============ 悬停与高亮相关实现 ============

void UXBActorPlacementComponent::UpdateHoverState()
{
	// 获取当前光标下的已放置 Actor
	AActor* NewHovered = nullptr;
	GetHitPlacedActor(NewHovered);

	// 悬停对象变化时更新材质
	if (NewHovered != HoveredActor.Get())
	{
		// 移除旧的高亮
		if (HoveredActor.IsValid())
		{
			ApplyHoverMaterial(HoveredActor.Get(), false);
		}

		// 更新悬停引用
		HoveredActor = NewHovered;

		// 应用新的高亮
		if (HoveredActor.IsValid())
		{
			ApplyHoverMaterial(HoveredActor.Get(), true);
		}
	}
}

void UXBActorPlacementComponent::ApplyHoverMaterial(AActor* Actor, bool bHovered)
{
	if (!Actor || !PlacementConfig)
	{
		return;
	}

	if (bHovered)
	{
		// 缓存原始材质（如果尚未缓存）
		CacheOriginalMaterials(Actor);

		// 加载高亮材质
		UMaterialInterface* HighlightMat = PlacementConfig->SelectionHighlightMaterial.LoadSynchronous();
		if (!HighlightMat)
		{
			UE_LOG(LogXBConfig, Warning, TEXT("[放置组件] 未配置选中高亮材质"));
			return;
		}

		// 创建或复用动态材质实例
		if (!CachedHoverMID)
		{
			CachedHoverMID = UMaterialInstanceDynamic::Create(HighlightMat, this);
		}

		// 设置高亮颜色
		CachedHoverMID->SetVectorParameterValue(TEXT("Color"), PlacementConfig->SelectionColor);

		// 应用到所有 Mesh 组件
		TArray<UPrimitiveComponent*> PrimitiveComps;
		Actor->GetComponents<UPrimitiveComponent>(PrimitiveComps);

		for (UPrimitiveComponent* PrimComp : PrimitiveComps)
		{
			if (PrimComp)
			{
				const int32 NumMaterials = PrimComp->GetNumMaterials();
				for (int32 i = 0; i < NumMaterials; ++i)
				{
					PrimComp->SetMaterial(i, CachedHoverMID);
				}
			}
		}

		UE_LOG(LogXBConfig, Verbose, TEXT("[放置组件] 应用悬停高亮: %s"), *Actor->GetName());
	}
	else
	{
		// 恢复原始材质
		RestoreCachedMaterials(Actor);

		UE_LOG(LogXBConfig, Verbose, TEXT("[放置组件] 移除悬停高亮: %s"), *Actor->GetName());
	}
}

bool UXBActorPlacementComponent::HandleRightClick()
{
	switch (CurrentState)
	{
	case EXBPlacementState::Idle:
		{
			// 空闲状态 -> 删除悬停的 Actor
			if (HoveredActor.IsValid())
			{
				return DeleteHoveredActor();
			}
			return false;
		}

	case EXBPlacementState::Previewing:
		{
			// 预览状态 -> 取消预览
			CancelOperation();
			return true;
		}

	case EXBPlacementState::Editing:
		{
			// 编辑状态 -> 删除选中的 Actor
			return DeleteSelectedActor();
		}

	default:
		return false;
	}
}

bool UXBActorPlacementComponent::DeleteHoveredActor()
{
	if (!HoveredActor.IsValid())
	{
		return false;
	}

	AActor* ActorToDelete = HoveredActor.Get();

	// 移除悬停高亮
	ApplyHoverMaterial(ActorToDelete, false);

	// 从已放置列表中移除
	PlacedActors.RemoveAll([ActorToDelete](const FXBPlacedActorData& Data)
	{
		return Data.PlacedActor.Get() == ActorToDelete;
	});

	// 广播删除事件
	OnActorDeleted.Broadcast(ActorToDelete);

	// 销毁 Actor
	ActorToDelete->Destroy();

	// 清空悬停引用
	HoveredActor.Reset();

	UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已删除悬停 Actor"));

	return true;
}

float UXBActorPlacementComponent::CalculateActorBottomOffset(AActor* Actor) const
{
	if (!Actor)
	{
		return 0.0f;
	}

	// 只对角色类型使用胶囊体半高偏移（使角色脚底贴地）
	if (ACharacter* CharActor = Cast<ACharacter>(Actor))
	{
		if (UCapsuleComponent* Capsule = CharActor->GetCapsuleComponent())
		{
			return Capsule->GetScaledCapsuleHalfHeight();
		}
	}

	// 普通 Actor 不需要偏移，保持原点在地面
	return 0.0f;
}

void UXBActorPlacementComponent::CacheOriginalMaterials(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	// 如果已缓存则跳过
	TWeakObjectPtr<AActor> WeakActor = Actor;
	if (OriginalMaterialsCache.Contains(WeakActor))
	{
		return;
	}

	TArray<TPair<int32, TArray<TObjectPtr<UMaterialInterface>>>> ComponentMaterials;

	TArray<UPrimitiveComponent*> PrimitiveComps;
	Actor->GetComponents<UPrimitiveComponent>(PrimitiveComps);

	for (int32 CompIdx = 0; CompIdx < PrimitiveComps.Num(); ++CompIdx)
	{
		UPrimitiveComponent* PrimComp = PrimitiveComps[CompIdx];
		if (!PrimComp)
		{
			continue;
		}

		TArray<TObjectPtr<UMaterialInterface>> Materials;
		const int32 NumMaterials = PrimComp->GetNumMaterials();
		for (int32 MatIdx = 0; MatIdx < NumMaterials; ++MatIdx)
		{
			Materials.Add(PrimComp->GetMaterial(MatIdx));
		}

		ComponentMaterials.Add(TPair<int32, TArray<TObjectPtr<UMaterialInterface>>>(CompIdx, Materials));
	}

	OriginalMaterialsCache.Add(WeakActor, ComponentMaterials);
}

void UXBActorPlacementComponent::RestoreCachedMaterials(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	TWeakObjectPtr<AActor> WeakActor = Actor;
	TArray<TPair<int32, TArray<TObjectPtr<UMaterialInterface>>>>* CachedData = OriginalMaterialsCache.Find(WeakActor);
	if (!CachedData)
	{
		return;
	}

	TArray<UPrimitiveComponent*> PrimitiveComps;
	Actor->GetComponents<UPrimitiveComponent>(PrimitiveComps);

	for (const TPair<int32, TArray<TObjectPtr<UMaterialInterface>>>& CompData : *CachedData)
	{
		const int32 CompIdx = CompData.Key;
		const TArray<TObjectPtr<UMaterialInterface>>& Materials = CompData.Value;

		if (CompIdx < PrimitiveComps.Num() && PrimitiveComps[CompIdx])
		{
			UPrimitiveComponent* PrimComp = PrimitiveComps[CompIdx];
			for (int32 MatIdx = 0; MatIdx < Materials.Num(); ++MatIdx)
			{
				if (MatIdx < PrimComp->GetNumMaterials())
				{
					PrimComp->SetMaterial(MatIdx, Materials[MatIdx]);
				}
			}
		}
	}

	// 清理缓存
	OriginalMaterialsCache.Remove(WeakActor);
}
