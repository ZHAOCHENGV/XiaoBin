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

	// 默认禁用 Tick，仅在预览/编辑状态启用
	SetComponentTickEnabled(false);
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
	PreviewRotation = Entry->DefaultRotation;

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

	// ✨ 新增 - 调整 Actor 位置使其紧贴地面（不陷入地面）
	FVector Origin;
	FVector BoxExtent;
	NewActor->GetActorBounds(false, Origin, BoxExtent);
	// 计算 Actor 底部到原点的偏移
	const float BottomOffset = BoxExtent.Z;
	// 将 Actor 向上移动，使底部紧贴地面
	FVector AdjustedLocation = PreviewLocation;
	AdjustedLocation.Z += BottomOffset;
	NewActor->SetActorLocation(AdjustedLocation);

	// ✨ 新增 - 配置阶段禁用磁场组件（防止提前招募士兵）
	if (UXBMagnetFieldComponent* MagnetComp = NewActor->FindComponentByClass<UXBMagnetFieldComponent>())
	{
		MagnetComp->SetFieldEnabled(false);
		UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已禁用磁场组件: %s"), *NewActor->GetName());
	}

	// 记录放置数据（使用调整后的位置）
	FXBPlacedActorData PlacedData;
	PlacedData.PlacedActor = NewActor;
	PlacedData.EntryIndex = CurrentPreviewEntryIndex;
	PlacedData.ActorClassPath = FSoftClassPath(Entry->ActorClass);
	PlacedData.Location = AdjustedLocation;
	PlacedData.Rotation = PreviewRotation;
	PlacedData.Scale = Entry->DefaultScale;
	PlacedActors.Add(PlacedData);

	// 销毁预览 Actor
	DestroyPreviewActor();

	// 广播事件
	OnActorPlaced.Broadcast(NewActor, CurrentPreviewEntryIndex);

	UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 已放置 Actor: %s 位置: %s"), *NewActor->GetName(), *AdjustedLocation.ToString());

	// ✨ 新增 - 连续放置模式：自动继续预览同类型 Actor
	const int32 PlacedEntryIndex = CurrentPreviewEntryIndex;
	if (PlacementConfig && PlacementConfig->bContinuousPlacementMode)
	{
		// 自动开始新的预览
		StartPreview(PlacedEntryIndex);
		UE_LOG(LogXBConfig, Log, TEXT("[放置组件] 连续放置模式：自动开始预览索引 %d"), PlacedEntryIndex);
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
		PreviewRotation.Yaw += RotationStep;
		PreviewActor->SetActorRotation(PreviewRotation);
	}
	else if (CurrentState == EXBPlacementState::Editing && SelectedActor.IsValid())
	{
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

		PreviewActor->SetActorLocation(PreviewLocation);
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

	if (World->LineTraceSingleByChannel(HitResult, WorldLocation, TraceEnd, ECC_Visibility, QueryParams))
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
	const bool bNeedsTick = (NewState == EXBPlacementState::Previewing);
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
