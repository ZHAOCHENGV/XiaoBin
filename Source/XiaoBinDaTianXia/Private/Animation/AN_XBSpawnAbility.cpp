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
            // 🔧 修改 - 输出弓手发射流程关键日志，便于定位配置与目标问题
            UE_LOG(LogXBCombat, Log, TEXT("弓手 %s 触发发射物通知，目标=%s"), 
                *Soldier->GetName(),
                Soldier->CurrentAttackTarget.IsValid() ? *Soldier->CurrentAttackTarget->GetName() : TEXT("无"));

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
                    // 🔧 修改 - 投射物伤害使用弓手基础伤害，避免额外数值配置
                    Projectile->Damage = Soldier->GetBaseDamage();
                    Projectile->LinearSpeed = ProjectileConfig.Speed;
                    Projectile->ArcLaunchSpeed = ProjectileConfig.ArcLaunchSpeed;
                    Projectile->ArcGravityScale = ProjectileConfig.ArcGravityScale;
                    Projectile->bUseArc = ProjectileConfig.bUseArc;
                    Projectile->LifeSeconds = ProjectileConfig.LifeSeconds;
                    Projectile->DamageEffectClass = ProjectileConfig.DamageEffectClass;

                    // 🔧 修改 - 优先锁定当前目标方向，确保发射物朝目标飞行
                    FVector ShootDirection = SpawnRotation.Vector();
                    if (AActor* CurrentTarget = Soldier->CurrentAttackTarget.Get())
                    {
                        ShootDirection = CurrentTarget->GetActorLocation() - SpawnLocation;
                        SpawnRotation = ShootDirection.Rotation();
                        Projectile->SetActorRotation(SpawnRotation);
                    }
                    else
                    {
                        UE_LOG(LogXBCombat, Warning, TEXT("弓手 %s 发射失败：没有有效目标，使用默认朝向"), *Soldier->GetName());
                    }

                    const FVector TargetLocation = Soldier->CurrentAttackTarget.IsValid()
                        ? Soldier->CurrentAttackTarget->GetActorLocation()
                        : FVector::ZeroVector;
                    Projectile->InitializeProjectileWithTarget(OwnerActor, Projectile->Damage, ShootDirection, ProjectileConfig.Speed, ProjectileConfig.bUseArc, TargetLocation);
                    UE_LOG(LogXBCombat, Log, TEXT("弓手 %s 发射物生成成功，类=%s 伤害=%.1f"), 
                        *Soldier->GetName(),
                        ProjectileClass ? *ProjectileClass->GetName() : TEXT("未知"),
                        Projectile->Damage);
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
