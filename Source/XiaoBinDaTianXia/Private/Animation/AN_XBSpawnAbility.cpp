// Source/XiaoBinDaTianXia/Private/Animation/AN_XBSpawnAbility.cpp
/* --- 完整文件代码 --- */

// Copyright XiaoBing Project. All Rights Reserved.

#include "Animation/AN_XBSpawnAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Combat/XBProjectile.h"
#include "Combat/XBProjectilePoolSubsystem.h"
#include "Soldier/XBSoldierCharacter.h"
#include "Utils/XBLogCategories.h"

UAN_XBSpawnAbility::UAN_XBSpawnAbility()
{
    // 设置默认事件标签
    EventTag = FGameplayTag::RequestGameplayTag(FName("Event.Ability.Spawn"));
}

FString UAN_XBSpawnAbility::GetNotifyName_Implementation() const
{
    // 在动画编辑器中显示的名称
    if (EventTag.IsValid())
    {
        return FString::Printf(TEXT("XB释放技能: %s"), *EventTag.ToString());
    }
    return TEXT("XB释放技能");
}

void UAN_XBSpawnAbility::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (!MeshComp)
    {
        return;
    }

    // 获取拥有者Actor
    AActor* OwnerActor = MeshComp->GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    // 计算生成位置和旋转
    FVector SpawnLocation = OwnerActor->GetActorLocation();
    FRotator SpawnRotation = OwnerActor->GetActorRotation();

    if (SpawnSocketName != NAME_None)
    {
        // 使用骨骼插槽位置
        SpawnLocation = MeshComp->GetSocketLocation(SpawnSocketName);
        
        if (!bUseActorRotation)
        {
            SpawnRotation = MeshComp->GetSocketRotation(SpawnSocketName);
        }
    }

    // 应用偏移
    SpawnLocation += SpawnRotation.RotateVector(LocationOffset);

    // 播放特效
    if (!SpawnVFX.IsNull())
    {
        UNiagaraSystem* VFX = SpawnVFX.LoadSynchronous();
        if (VFX)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                OwnerActor->GetWorld(),
                VFX,
                SpawnLocation,
                SpawnRotation);
        }
    }

    // 播放音效
    if (!SpawnSound.IsNull())
    {
        USoundBase* Sound = SpawnSound.LoadSynchronous();
        if (Sound)
        {
            UGameplayStatics::PlaySoundAtLocation(
                OwnerActor->GetWorld(),
                Sound,
                SpawnLocation);
        }
    }

    // ✨ 新增 - 弓手投射物生成（结合对象池）
    if (AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(OwnerActor))
    {
        if (Soldier->GetSoldierType() == EXBSoldierType::Archer)
        {
            const FXBProjectileConfig ProjectileConfig = Soldier->GetProjectileConfig();
            TSubclassOf<AXBProjectile> ProjectileClass = ProjectileClassOverride ? ProjectileClassOverride : ProjectileConfig.ProjectileClass;

            if (!ProjectileClass)
            {
                UE_LOG(LogXBCombat, Warning, TEXT("弓手 %s 发射失败：未配置投射物类"), *Soldier->GetName());
            }
            else
            {
                AXBProjectile* Projectile = nullptr;
                UWorld* World = OwnerActor->GetWorld();

                if (bUseProjectilePool && World)
                {
                    if (UXBProjectilePoolSubsystem* PoolSubsystem = World->GetSubsystem<UXBProjectilePoolSubsystem>())
                    {
                        Projectile = PoolSubsystem->AcquireProjectile(ProjectileClass, SpawnLocation, SpawnRotation);
                    }
                }

                if (!Projectile && World)
                {
                    FActorSpawnParameters SpawnParams;
                    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                    Projectile = World->SpawnActor<AXBProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
                }

                if (Projectile)
                {
                    // 🔧 修改 - 先写入配置，再初始化投射物运动参数
                    Projectile->Damage = ProjectileConfig.Damage;
                    Projectile->LinearSpeed = ProjectileConfig.Speed;
                    Projectile->ArcLaunchSpeed = ProjectileConfig.ArcLaunchSpeed;
                    Projectile->ArcGravityScale = ProjectileConfig.ArcGravityScale;
                    Projectile->bUseArc = ProjectileConfig.bUseArc;
                    Projectile->DamageEffectClass = ProjectileConfig.DamageEffectClass;

                    FVector ShootDirection = SpawnRotation.Vector();
                    if (AActor* CurrentTarget = Soldier->CurrentAttackTarget.Get())
                    {
                        ShootDirection = CurrentTarget->GetActorLocation() - SpawnLocation;
                    }

                    Projectile->InitializeProjectile(OwnerActor, Projectile->Damage, ShootDirection, ProjectileConfig.Speed, ProjectileConfig.bUseArc);
                }
            }
        }
    }

    // ✨ 新增 - 发送GameplayEvent
    if (EventTag.IsValid())
    {
        if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor))
        {
            FGameplayEventData EventData;
            EventData.Instigator = OwnerActor;
            EventData.EventMagnitude = EventMagnitude;
            
            // 存储生成位置到TargetData（供技能读取）
            FGameplayAbilityTargetData_LocationInfo* LocationData = new FGameplayAbilityTargetData_LocationInfo();
            LocationData->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
            LocationData->TargetLocation.LiteralTransform = FTransform(SpawnRotation, SpawnLocation);
            
            FGameplayAbilityTargetDataHandle TargetDataHandle;
            TargetDataHandle.Add(LocationData);
            EventData.TargetData = TargetDataHandle;

            ASC->HandleGameplayEvent(EventTag, &EventData);

            UE_LOG(LogTemp, Log, TEXT("动画通知触发技能事件: %s，位置: %s"), 
                *EventTag.ToString(), *SpawnLocation.ToString());
        }
    }
}
