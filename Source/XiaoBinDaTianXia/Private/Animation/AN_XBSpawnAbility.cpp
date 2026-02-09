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

    // 🔧 调试日志 - 函数入口
    UE_LOG(LogXBCombat, Log, TEXT("【技能通知】Notify 触发，MeshComp=%s, Animation=%s"),
        MeshComp ? *MeshComp->GetName() : TEXT("空"),
        Animation ? *Animation->GetName() : TEXT("空"));

    if (!MeshComp)
    {
        UE_LOG(LogXBCombat, Warning, TEXT("【技能通知】失败：MeshComp 为空"));
        return;
    }

    // 获取拥有者Actor
    AActor* OwnerActor = MeshComp->GetOwner();
    if (!OwnerActor)
    {
        UE_LOG(LogXBCombat, Warning, TEXT("【技能通知】失败：OwnerActor 为空"));
        return;
    }

    // 🔧 调试日志 - 拥有者信息
    UE_LOG(LogXBCombat, Log, TEXT("【技能通知】OwnerActor=%s, 类=%s"),
        *OwnerActor->GetName(),
        *OwnerActor->GetClass()->GetName());

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
        UE_LOG(LogXBCombat, Verbose, TEXT("【技能通知】使用插槽 %s，位置=%s"),
            *SpawnSocketName.ToString(), *SpawnLocation.ToString());
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

    // ✨ 弓手投射物生成逻辑（结合对象池）
    AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(OwnerActor);
    
    // 🔧 调试日志 - 士兵类型检查
    if (!Soldier)
    {
        UE_LOG(LogXBCombat, Log, TEXT("【技能通知】OwnerActor 不是士兵类型，跳过投射物生成"));
    }
    else
    {
        EXBSoldierType CurrentSoldierType = Soldier->GetSoldierType();
        UE_LOG(LogXBCombat, Log, TEXT("【技能通知】士兵 %s 类型=%s（需要=Archer）"),
            *Soldier->GetName(),
            *UEnum::GetValueAsString(CurrentSoldierType));

        if (CurrentSoldierType == EXBSoldierType::Archer)
        {
            // 🔧 调试日志 - 弓手发射流程
            UE_LOG(LogXBCombat, Log, TEXT("【技能通知】弓手 %s 开始生成投射物，目标=%s"), 
                *Soldier->GetName(),
                Soldier->CurrentAttackTarget.IsValid() ? *Soldier->CurrentAttackTarget->GetName() : TEXT("无"));

            const FXBProjectileConfig ProjectileConfig = Soldier->GetProjectileConfig();
            TSubclassOf<AXBProjectile> ProjectileClass = ProjectileClassOverride ? ProjectileClassOverride : ProjectileConfig.ProjectileClass;

            // 🔧 调试日志 - 投射物类配置
            UE_LOG(LogXBCombat, Log, TEXT("【技能通知】投射物配置：Override=%s, 数据表=%s, 最终=%s"),
                ProjectileClassOverride ? *ProjectileClassOverride->GetName() : TEXT("空"),
                ProjectileConfig.ProjectileClass ? *ProjectileConfig.ProjectileClass->GetName() : TEXT("空"),
                ProjectileClass ? *ProjectileClass->GetName() : TEXT("空"));

            if (!ProjectileClass)
            {
                UE_LOG(LogXBCombat, Warning, TEXT("【技能通知】弓手 %s 发射失败：未配置投射物类！请检查数据表 ProjectileConfig.ProjectileClass"), *Soldier->GetName());
            }
            else
            {
                AXBProjectile* Projectile = nullptr;
                UWorld* World = OwnerActor->GetWorld();

                // 🔧 调试日志 - World 有效性
                if (!World)
                {
                    UE_LOG(LogXBCombat, Error, TEXT("【技能通知】弓手 %s 发射失败：World 为空！"), *Soldier->GetName());
                }
                else
                {
                    // 尝试从对象池获取
                    if (bUseProjectilePool)
                    {
                        UXBProjectilePoolSubsystem* PoolSubsystem = World->GetSubsystem<UXBProjectilePoolSubsystem>();
                        if (PoolSubsystem)
                        {
                            Projectile = PoolSubsystem->AcquireProjectile(ProjectileClass, SpawnLocation, SpawnRotation);
                            UE_LOG(LogXBCombat, Log, TEXT("【技能通知】对象池获取投射物：%s"),
                                Projectile ? *Projectile->GetName() : TEXT("失败"));
                        }
                        else
                        {
                            UE_LOG(LogXBCombat, Warning, TEXT("【技能通知】对象池子系统不存在，将直接生成"));
                        }
                    }

                    // 对象池获取失败时直接生成
                    if (!Projectile)
                    {
                        FActorSpawnParameters SpawnParams;
                        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                        Projectile = World->SpawnActor<AXBProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
                        UE_LOG(LogXBCombat, Log, TEXT("【技能通知】直接生成投射物：%s"),
                            Projectile ? *Projectile->GetName() : TEXT("失败"));
                    }

                    if (Projectile)
                    {
                        // 先写入配置，再初始化投射物运动参数
                        Projectile->Damage = Soldier->GetBaseDamage();
                        Projectile->LinearSpeed = ProjectileConfig.Speed;
                        Projectile->ArcSpeed = ProjectileConfig.ArcLaunchSpeed;
                        Projectile->ArcGravityScale = ProjectileConfig.ArcGravityScale;
                        Projectile->LaunchMode = ProjectileConfig.bUseArc ? EXBProjectileLaunchMode::Arc : EXBProjectileLaunchMode::Linear;
                        Projectile->LifeSeconds = ProjectileConfig.LifeSeconds;
                        Projectile->DamageEffectClass = ProjectileConfig.DamageEffectClass;

                        // 优先锁定当前目标方向，确保发射物朝目标飞行
                        FVector ShootDirection = SpawnRotation.Vector();
                        if (AActor* CurrentTarget = Soldier->CurrentAttackTarget.Get())
                        {
                            ShootDirection = CurrentTarget->GetActorLocation() - SpawnLocation;
                            SpawnRotation = ShootDirection.Rotation();
                            Projectile->SetActorRotation(SpawnRotation);
                        }
                        else
                        {
                            UE_LOG(LogXBCombat, Warning, TEXT("【技能通知】弓手 %s 没有有效目标，使用默认朝向"), *Soldier->GetName());
                        }

                        const FVector TargetLocation = Soldier->CurrentAttackTarget.IsValid()
                            ? Soldier->CurrentAttackTarget->GetActorLocation()
                            : FVector::ZeroVector;
                        Projectile->InitializeProjectileWithTarget(OwnerActor, Projectile->Damage, ShootDirection, ProjectileConfig.Speed, ProjectileConfig.bUseArc, TargetLocation);
                        
                        UE_LOG(LogXBCombat, Log, TEXT("【技能通知】✅ 弓手 %s 投射物生成成功！类=%s 伤害=%.1f 位置=%s"), 
                            *Soldier->GetName(),
                            *ProjectileClass->GetName(),
                            Projectile->Damage,
                            *SpawnLocation.ToString());
                    }
                    else
                    {
                        UE_LOG(LogXBCombat, Error, TEXT("【技能通知】❌ 弓手 %s 投射物生成失败！SpawnActor 返回空"), *Soldier->GetName());
                    }
                }
            }
        }
        else
        {
            UE_LOG(LogXBCombat, Verbose, TEXT("【技能通知】士兵 %s 不是弓手，跳过投射物生成"), *Soldier->GetName());
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
