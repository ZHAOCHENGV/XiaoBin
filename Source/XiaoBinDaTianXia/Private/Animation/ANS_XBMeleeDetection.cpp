/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Animation/ANS_XBMeleeDetection.cpp

/**
 * @file ANS_XBMeleeDetection.cpp
 * @brief 近战检测动画通知状态实现 - 支持攻击范围缩放
 * 
 * @note 🔧 修改记录:
 *       1. 在 PerformCapsuleTrace 中应用角色缩放
 *       2. 新增 GetOwnerScale 方法获取角色实际缩放
 *       3. 🔧 修复士兵检测问题 - 添加自定义碰撞通道检测
 *       4. ✨ 新增阵营过滤 - 只伤害敌对阵营
 *       5. ✨ 新增对士兵的伤害支持
 *       6. ❌ 删除 BaseDamage 成员变量
 *       7. ✨ 新增 GetAttackDamage() 从战斗组件获取伤害
 *       8. 🔧 修改 - 主将命中时通知战斗逻辑
 */

#include "Animation/ANS_XBMeleeDetection.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Character/XBCharacterBase.h"
#include "Character/Components/XBCombatComponent.h"
#include "Soldier/XBSoldierCharacter.h"
#include "GAS/XBAttributeSet.h"
#include "XBCollisionChannels.h"
#include "Utils/XBBlueprintFunctionLibrary.h"
#include "Utils/XBLogCategories.h"

UANS_XBMeleeDetection::UANS_XBMeleeDetection()
{
    DamageTag = FGameplayTag::RequestGameplayTag(FName("Damage.Base"), false);
}

void UANS_XBMeleeDetection::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    HitActors.Empty();

    // 🔧 修改 - 从战斗组件获取伤害值
    float Damage = 0.0f;
    if (MeshComp)
    {
        if (AActor* Owner = MeshComp->GetOwner())
        {
            Damage = GetAttackDamage(Owner);
        }
    }

    UE_LOG(LogXBCombat, Log, TEXT("近战检测开始 - 起始插槽: %s, 结束插槽: %s, 伤害: %.1f"), 
        *DetectionConfig.StartSocketName.ToString(),
        *DetectionConfig.EndSocketName.ToString(),
        Damage);
}

void UANS_XBMeleeDetection::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

    if (!MeshComp)
    {
        return;
    }

    TArray<FHitResult> HitResults = PerformCapsuleTrace(MeshComp);

    if (HitResults.Num() > 0)
    {
        AActor* OwnerActor = MeshComp->GetOwner();
        ApplyDamageToTargets(HitResults, OwnerActor);
    }
}

void UANS_XBMeleeDetection::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    UE_LOG(LogXBCombat, Log, TEXT("近战检测结束 - 共命中 %d 个目标"), HitActors.Num());
    HitActors.Empty();
}

/**
 * @brief 获取当前攻击的伤害值
 * @param OwnerActor 攻击者
 * @return 伤害值（已应用倍率）
 * @note ✨ 新增方法 - 从战斗组件获取
 */
float UANS_XBMeleeDetection::GetAttackDamage(AActor* OwnerActor) const
{
    if (!OwnerActor)
    {
        return 0.0f;
    }

    // 尝试从将领角色获取战斗组件
    if (AXBCharacterBase* Character = Cast<AXBCharacterBase>(OwnerActor))
    {
        if (UXBCombatComponent* CombatComp = Character->GetCombatComponent())
        {
            float FinalDamage = CombatComp->GetCurrentAttackFinalDamage();
            
            if (FinalDamage > 0.0f)
            {
                UE_LOG(LogXBCombat, Verbose, TEXT("从战斗组件获取伤害: %.1f (类型: %d)"),
                    FinalDamage,
                    static_cast<int32>(CombatComp->GetCurrentAttackType()));
                return FinalDamage;
            }
            else
            {
                UE_LOG(LogXBCombat, Warning, TEXT("战斗组件返回伤害为 0，当前攻击类型: %d"),
                    static_cast<int32>(CombatComp->GetCurrentAttackType()));
            }
        }
        else
        {
            UE_LOG(LogXBCombat, Warning, TEXT("角色 %s 没有战斗组件"), *OwnerActor->GetName());
        }
    }

    // 如果是士兵，从士兵配置获取伤害
    if (AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(OwnerActor))
    {
        float SoldierDamage = Soldier->GetBaseDamage();
        UE_LOG(LogXBCombat, Verbose, TEXT("从士兵获取伤害: %.1f"), SoldierDamage);
        return SoldierDamage;
    }

    UE_LOG(LogXBCombat, Warning, TEXT("无法获取 %s 的伤害值，返回默认值 10"), *OwnerActor->GetName());
    return 10.0f; // 默认伤害
}

TArray<FHitResult> UANS_XBMeleeDetection::PerformCapsuleTrace(USkeletalMeshComponent* MeshComp)
{
    TArray<FHitResult> OutHitResults;

    if (!MeshComp)
    {
        return OutHitResults;
    }

    AActor* OwnerActor = MeshComp->GetOwner();
    if (!OwnerActor)
    {
        return OutHitResults;
    }

    float OwnerScale = GetOwnerScale(OwnerActor);

    FTransform StartSocketTransform = MeshComp->GetSocketTransform(DetectionConfig.StartSocketName, ERelativeTransformSpace::RTS_World);
    if (DetectionConfig.StartSocketName.IsNone() || !MeshComp->DoesSocketExist(DetectionConfig.StartSocketName))
    {
        UE_LOG(LogXBCombat, Warning, TEXT("起始骨骼插槽 '%s' 不存在，使用角色位置"), *DetectionConfig.StartSocketName.ToString());
        StartSocketTransform = MeshComp->GetComponentTransform();
    }

    FTransform EndSocketTransform = MeshComp->GetSocketTransform(DetectionConfig.EndSocketName, ERelativeTransformSpace::RTS_World);
    if (DetectionConfig.EndSocketName.IsNone() || !MeshComp->DoesSocketExist(DetectionConfig.EndSocketName))
    {
        UE_LOG(LogXBCombat, Warning, TEXT("结束骨骼插槽 '%s' 不存在，使用起始位置+偏移"), *DetectionConfig.EndSocketName.ToString());
        EndSocketTransform = StartSocketTransform;
        EndSocketTransform.SetLocation(StartSocketTransform.GetLocation() + FVector(0, 0, 100.0f));
    }

    FVector StartLocation = StartSocketTransform.GetLocation() + 
        StartSocketTransform.GetRotation().RotateVector(DetectionConfig.StartLocationOffset);
    FVector EndLocation = EndSocketTransform.GetLocation() + 
        EndSocketTransform.GetRotation().RotateVector(DetectionConfig.EndLocationOffset);

    FVector Direction = (EndLocation - StartLocation).GetSafeNormal();
    FQuat CapsuleRotation = FQuat::FindBetweenNormals(FVector::UpVector, Direction);

    FVector CapsuleCenter = (StartLocation + EndLocation) * 0.5f;
    float Distance = FVector::Dist(StartLocation, EndLocation);

    float ScaledRadius = DetectionConfig.CapsuleRadius * OwnerScale * DetectionConfig.ScaleMultiplier;
    
    float BaseHalfHeight = DetectionConfig.CapsuleHalfHeight > 0.0f ? 
        DetectionConfig.CapsuleHalfHeight : 
        FMath::Max(Distance * 0.5f, DetectionConfig.CapsuleRadius);
    
    float ScaledHalfHeight = BaseHalfHeight * OwnerScale * DetectionConfig.ScaleMultiplier;

    UE_LOG(LogXBCombat, VeryVerbose, TEXT("近战检测缩放: 角色=%.2f, 半径=%.0f→%.0f, 半高=%.0f→%.0f"), 
        OwnerScale, 
        DetectionConfig.CapsuleRadius, ScaledRadius,
        BaseHalfHeight, ScaledHalfHeight);

    TArray<AActor*> IgnoreActors = DetectionConfig.ActorsToIgnore;
    IgnoreActors.AddUnique(OwnerActor);

    TArray<TEnumAsByte<EObjectTypeQuery>> AllObjectTypes = DetectionConfig.ObjectTypes;
    
    if (DetectionConfig.bDetectSoldierChannel)
    {
        EObjectTypeQuery SoldierQuery = UEngineTypes::ConvertToObjectType(XBCollision::Soldier);
        AllObjectTypes.AddUnique(SoldierQuery);
    }
    
    if (DetectionConfig.bDetectLeaderChannel)
    {
        EObjectTypeQuery LeaderQuery = UEngineTypes::ConvertToObjectType(XBCollision::Leader);
        AllObjectTypes.AddUnique(LeaderQuery);
    }

    TArray<FHitResult> HitResults;
    bool bHit = UKismetSystemLibrary::CapsuleTraceMultiForObjects(
        MeshComp->GetWorld(),
        CapsuleCenter,
        CapsuleCenter,
        ScaledRadius,
        ScaledHalfHeight,
        AllObjectTypes,
        false,
        IgnoreActors,
        DetectionConfig.bEnableDebugDraw ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
        HitResults,
        true,
        FLinearColor::Red,
        FLinearColor::Green,
        DetectionConfig.DebugDrawDuration
    );

    if (DetectionConfig.bEnableDebugDraw)
    {
        UWorld* World = MeshComp->GetWorld();
        if (World)
        {
            DrawDebugSphere(World, StartLocation, 10.0f * OwnerScale, 8, FColor::Blue, false, DetectionConfig.DebugDrawDuration);
            DrawDebugSphere(World, EndLocation, 10.0f * OwnerScale, 8, FColor::Cyan, false, DetectionConfig.DebugDrawDuration);
            DrawDebugLine(World, StartLocation, EndLocation, FColor::Yellow, false, DetectionConfig.DebugDrawDuration, 0, 2.0f);
            
            DrawDebugCapsule(World, CapsuleCenter, ScaledHalfHeight, ScaledRadius, 
                CapsuleRotation, bHit ? FColor::Red : FColor::Green, false, DetectionConfig.DebugDrawDuration);

            DrawDebugString(World, CapsuleCenter + FVector(0, 0, ScaledHalfHeight + 20.0f),
                FString::Printf(TEXT("缩放: %.2fx, 命中: %d"), OwnerScale, HitResults.Num()),
                nullptr, FColor::White, DetectionConfig.DebugDrawDuration);
        }
    }

    OutHitResults = HitResults;
    return OutHitResults;
}

float UANS_XBMeleeDetection::GetOwnerScale(AActor* OwnerActor) const
{
    if (!OwnerActor)
    {
        return 1.0f;
    }

    if (AXBCharacterBase* Character = Cast<AXBCharacterBase>(OwnerActor))
    {
        float CharScale = Character->GetCurrentScale();
        if (CharScale > 0.0f)
        {
            return CharScale;
        }
    }

    if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor))
    {
        float ScaleAttribute = ASC->GetNumericAttribute(UXBAttributeSet::GetScaleAttribute());
        if (ScaleAttribute > 0.0f)
        {
            return ScaleAttribute;
        }
    }

    FVector ActorScale = OwnerActor->GetActorScale3D();
    if (ActorScale.X > 0.0f)
    {
        return ActorScale.X;
    }

    return 1.0f;
}

EXBFaction UANS_XBMeleeDetection::GetOwnerFaction(AActor* OwnerActor) const
{
    if (!OwnerActor)
    {
        return EXBFaction::Neutral;
    }

    if (AXBCharacterBase* Character = Cast<AXBCharacterBase>(OwnerActor))
    {
        return Character->GetFaction();
    }

    if (AXBSoldierCharacter* Soldier = Cast<AXBSoldierCharacter>(OwnerActor))
    {
        return Soldier->GetFaction();
    }

    return EXBFaction::Neutral;
}

bool UANS_XBMeleeDetection::ShouldDamageTarget(AActor* OwnerActor, AActor* TargetActor) const
{
    if (!OwnerActor || !TargetActor)
    {
        return false;
    }

    if (OwnerActor == TargetActor)
    {
        return false;
    }

    if (!DetectionConfig.bEnableFactionFilter)
    {
        return true;
    }

    EXBFaction OwnerFaction = GetOwnerFaction(OwnerActor);

    EXBFaction TargetFaction = EXBFaction::Neutral;
    
    if (AXBCharacterBase* TargetCharacter = Cast<AXBCharacterBase>(TargetActor))
    {
        TargetFaction = TargetCharacter->GetFaction();
    }
    else if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(TargetActor))
    {
        TargetFaction = TargetSoldier->GetFaction();
    }
    else
    {
        return true;
    }

    bool bHostile = UXBBlueprintFunctionLibrary::AreFactionsHostile(OwnerFaction, TargetFaction);
    
    UE_LOG(LogXBCombat, Verbose, TEXT("阵营检查: %s(%d) vs %s(%d) = %s"),
        *OwnerActor->GetName(), static_cast<int32>(OwnerFaction),
        *TargetActor->GetName(), static_cast<int32>(TargetFaction),
        bHostile ? TEXT("敌对") : TEXT("友好"));

    return bHostile;
}

/**
 * @brief 对检测到的目标应用伤害
 * @param HitResults 命中结果
 * @param OwnerActor 攻击者
 * @note 🔧 修改 - 从战斗组件获取伤害值
 */
void UANS_XBMeleeDetection::ApplyDamageToTargets(const TArray<FHitResult>& HitResults, AActor* OwnerActor)
{
    if (!OwnerActor)
    {
        return;
    }

    // ✨ 核心修改 - 从战斗组件获取伤害
    float AttackDamage = GetAttackDamage(OwnerActor);
    
    if (AttackDamage <= 0.0f)
    {
        UE_LOG(LogXBCombat, Warning, TEXT("攻击伤害为 0，跳过伤害应用"));
        return;
    }

    UAbilitySystemComponent* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor);

    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (!HitActor || HitActors.Contains(HitActor))
        {
            continue;
        }

        if (HitActor == OwnerActor)
        {
            continue;
        }

        if (!ShouldDamageTarget(OwnerActor, HitActor))
        {
            UE_LOG(LogXBCombat, Verbose, TEXT("阵营过滤: %s 不攻击 %s（非敌对）"),
                *OwnerActor->GetName(), *HitActor->GetName());
            continue;
        }

        // 🔧 修改 - 当主将命中目标时，通知主将触发战斗逻辑（命中敌方主将时会驱动士兵进入战斗）
        if (AXBCharacterBase* OwnerLeader = Cast<AXBCharacterBase>(OwnerActor))
        {
            // 为什么这里调用：近战命中是主将攻击敌方主将的确定时机，可确保士兵及时进入战斗
            OwnerLeader->OnAttackHit(HitActor);
        }

        // 记录已命中，避免重复伤害
        HitActors.Add(HitActor);

        // 检查目标是否是士兵
        if (AXBSoldierCharacter* TargetSoldier = Cast<AXBSoldierCharacter>(HitActor))
        {
            float ActualDamage = TargetSoldier->TakeSoldierDamage(AttackDamage, OwnerActor);
            
            UE_LOG(LogXBCombat, Log, TEXT("近战命中士兵: %s, 伤害: %.1f, 实际: %.1f"), 
                *HitActor->GetName(), AttackDamage, ActualDamage);
            continue;
        }

        // 目标是将领或其他有 ASC 的角色
        UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor);
        if (!TargetASC)
        {
            UE_LOG(LogXBCombat, Warning, TEXT("近战命中: %s, 但目标没有ASC且不是士兵"), *HitActor->GetName());
            continue;
        }

        // 使用 GAS 伤害系统
        if (DamageEffectClass && SourceASC)
        {
            FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
            ContextHandle.AddSourceObject(OwnerActor);
            ContextHandle.AddHitResult(Hit);

            FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
            if (SpecHandle.IsValid())
            {
                if (DamageTag.IsValid())
                {
                    SpecHandle.Data->SetSetByCallerMagnitude(DamageTag, AttackDamage);
                }

                SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data, TargetASC);

                UE_LOG(LogXBCombat, Log, TEXT("近战命中将领: %s, 伤害: %.1f (GAS)"), 
                    *HitActor->GetName(), AttackDamage);
            }
        }
        else
        {
            if (TargetASC)
            {
                TargetASC->SetNumericAttributeBase(UXBAttributeSet::GetIncomingDamageAttribute(), AttackDamage);
                
                UE_LOG(LogXBCombat, Log, TEXT("近战命中: %s, 伤害: %.1f (直接属性)"), 
                    *HitActor->GetName(), AttackDamage);
            }
            else
            {
                UE_LOG(LogXBCombat, Warning, TEXT("近战命中: %s, 但无法应用伤害（无ASC且无伤害Effect）"), 
                    *HitActor->GetName());
            }
        }
    }
}
