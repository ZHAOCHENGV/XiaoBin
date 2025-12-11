/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Animation/ANS_XBMeleeDetection.cpp

/**
 * @file ANS_XBMeleeDetection.cpp
 * @brief 近战检测动画通知状态实现 - 支持攻击范围缩放
 * 
 * @note 🔧 修改记录:
 *       1. 在 PerformCapsuleTrace 中应用角色缩放
 *       2. 新增 GetOwnerScale 方法获取角色实际缩放
 */

#include "Animation/ANS_XBMeleeDetection.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Character/XBCharacterBase.h"  // ✨ 新增
#include "GAS/XBAttributeSet.h"          // ✨ 新增

UANS_XBMeleeDetection::UANS_XBMeleeDetection()
{
    DamageTag = FGameplayTag::RequestGameplayTag(FName("Damage.Base"), false);
}

void UANS_XBMeleeDetection::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    HitActors.Empty();

    UE_LOG(LogTemp, Log, TEXT("近战检测开始 - 起始插槽: %s, 结束插槽: %s"), 
        *DetectionConfig.StartSocketName.ToString(),
        *DetectionConfig.EndSocketName.ToString());
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

    UE_LOG(LogTemp, Log, TEXT("近战检测结束 - 共命中 %d 个目标"), HitActors.Num());
    HitActors.Empty();
}

/**
 * @brief 执行胶囊体检测
 * @param MeshComp 骨骼网格体组件
 * @return 检测到的所有命中结果
 * @note 🔧 修改 - 应用角色缩放到胶囊体参数
 */
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

    // ✨ 新增 - 获取角色的实际缩放倍率
    float OwnerScale = GetOwnerScale(OwnerActor);

    // 获取起始插槽变换
    FTransform StartSocketTransform = MeshComp->GetSocketTransform(DetectionConfig.StartSocketName, ERelativeTransformSpace::RTS_World);
    if (DetectionConfig.StartSocketName.IsNone() || !MeshComp->DoesSocketExist(DetectionConfig.StartSocketName))
    {
        UE_LOG(LogTemp, Warning, TEXT("起始骨骼插槽 '%s' 不存在，使用角色位置"), *DetectionConfig.StartSocketName.ToString());
        StartSocketTransform = MeshComp->GetComponentTransform();
    }

    // 获取结束插槽变换
    FTransform EndSocketTransform = MeshComp->GetSocketTransform(DetectionConfig.EndSocketName, ERelativeTransformSpace::RTS_World);
    if (DetectionConfig.EndSocketName.IsNone() || !MeshComp->DoesSocketExist(DetectionConfig.EndSocketName))
    {
        UE_LOG(LogTemp, Warning, TEXT("结束骨骼插槽 '%s' 不存在，使用起始位置+偏移"), *DetectionConfig.EndSocketName.ToString());
        EndSocketTransform = StartSocketTransform;
        EndSocketTransform.SetLocation(StartSocketTransform.GetLocation() + FVector(0, 0, 100.0f));
    }

    // 应用偏移
    FVector StartLocation = StartSocketTransform.GetLocation() + 
        StartSocketTransform.GetRotation().RotateVector(DetectionConfig.StartLocationOffset);
    FVector EndLocation = EndSocketTransform.GetLocation() + 
        EndSocketTransform.GetRotation().RotateVector(DetectionConfig.EndLocationOffset);

    // 计算胶囊体朝向
    FVector Direction = (EndLocation - StartLocation).GetSafeNormal();
    FQuat CapsuleRotation = FQuat::FindBetweenNormals(FVector::UpVector, Direction);

    // 计算胶囊体中心和半高
    FVector CapsuleCenter = (StartLocation + EndLocation) * 0.5f;
    float Distance = FVector::Dist(StartLocation, EndLocation);

    // 🔧 修改 - 应用缩放到胶囊体参数
    // 计算实际的胶囊体半径和半高
    float ScaledRadius = DetectionConfig.CapsuleRadius * OwnerScale * DetectionConfig.ScaleMultiplier;
    
    float BaseHalfHeight = DetectionConfig.CapsuleHalfHeight > 0.0f ? 
        DetectionConfig.CapsuleHalfHeight : 
        FMath::Max(Distance * 0.5f, DetectionConfig.CapsuleRadius);
    
    float ScaledHalfHeight = BaseHalfHeight * OwnerScale * DetectionConfig.ScaleMultiplier;

    // ✨ 新增 - 日志输出缩放信息
    UE_LOG(LogTemp, VeryVerbose, TEXT("近战检测缩放: 角色=%.2f, 半径=%.0f→%.0f, 半高=%.0f→%.0f"), 
        OwnerScale, 
        DetectionConfig.CapsuleRadius, ScaledRadius,
        BaseHalfHeight, ScaledHalfHeight);

    // 设置忽略的Actor
    TArray<AActor*> IgnoreActors = DetectionConfig.ActorsToIgnore;
    IgnoreActors.AddUnique(OwnerActor);

    // 执行胶囊体多重检测
    TArray<FHitResult> HitResults;
    bool bHit = UKismetSystemLibrary::CapsuleTraceMultiForObjects(
        MeshComp->GetWorld(),
        CapsuleCenter,
        CapsuleCenter,
        ScaledRadius,        // ✅ 使用缩放后的半径
        ScaledHalfHeight,    // ✅ 使用缩放后的半高
        DetectionConfig.ObjectTypes,
        false,
        IgnoreActors,
        DetectionConfig.bEnableDebugDraw ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
        HitResults,
        true,
        FLinearColor::Red,
        FLinearColor::Green,
        DetectionConfig.DebugDrawDuration
    );

    // 调试绘制
    if (DetectionConfig.bEnableDebugDraw)
    {
        UWorld* World = MeshComp->GetWorld();
        if (World)
        {
            DrawDebugSphere(World, StartLocation, 10.0f * OwnerScale, 8, FColor::Blue, false, DetectionConfig.DebugDrawDuration);
            DrawDebugSphere(World, EndLocation, 10.0f * OwnerScale, 8, FColor::Cyan, false, DetectionConfig.DebugDrawDuration);
            DrawDebugLine(World, StartLocation, EndLocation, FColor::Yellow, false, DetectionConfig.DebugDrawDuration, 0, 2.0f);
            
            // ✅ 绘制缩放后的胶囊体
            DrawDebugCapsule(World, CapsuleCenter, ScaledHalfHeight, ScaledRadius, 
                CapsuleRotation, bHit ? FColor::Red : FColor::Green, false, DetectionConfig.DebugDrawDuration);

            // ✨ 新增 - 显示缩放信息
            DrawDebugString(World, CapsuleCenter + FVector(0, 0, ScaledHalfHeight + 20.0f),
                FString::Printf(TEXT("缩放: %.2fx"), OwnerScale),
                nullptr, FColor::White, DetectionConfig.DebugDrawDuration);
        }
    }

    OutHitResults = HitResults;
    return OutHitResults;
}

/**
 * @brief 获取角色的当前缩放倍率
 * @param OwnerActor 角色Actor
 * @return 缩放倍率
 * @note ✨ 新增方法
 *       优先从ASC读取Scale属性，否则使用Actor缩放
 */
float UANS_XBMeleeDetection::GetOwnerScale(AActor* OwnerActor) const
{
    if (!OwnerActor)
    {
        return 1.0f;
    }

    // 🔧 修改 - 调整优先级顺序，避免 return 后的代码

    // 方案1（最优）：从 XBCharacterBase 直接读取
    if (AXBCharacterBase* Character = Cast<AXBCharacterBase>(OwnerActor))
    {
        float CharScale = Character->GetCurrentScale();
        if (CharScale > 0.0f)
        {
            return CharScale;
        }
    }

    // 方案2：从 GAS 属性读取
    if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor))
    {
        float ScaleAttribute = ASC->GetNumericAttribute(UXBAttributeSet::GetScaleAttribute());
        if (ScaleAttribute > 0.0f)
        {
            return ScaleAttribute;
        }
    }

    // 方案3（回退）：从 Actor 的 Scale 读取
    FVector ActorScale = OwnerActor->GetActorScale3D();
    if (ActorScale.X > 0.0f)
    {
        return ActorScale.X; // 假设均匀缩放
    }

    // 默认返回
    return 1.0f;
}

void UANS_XBMeleeDetection::ApplyDamageToTargets(const TArray<FHitResult>& HitResults, AActor* OwnerActor)
{
    if (!OwnerActor)
    {
        return;
    }

    UAbilitySystemComponent* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor);
    if (!SourceASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("攻击者没有AbilitySystemComponent"));
        return;
    }

    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (!HitActor || HitActors.Contains(HitActor))
        {
            continue;
        }

        UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor);
        if (!TargetASC)
        {
            continue;
        }

        if (HitActor == OwnerActor)
        {
            continue;
        }

        if (DamageEffectClass)
        {
            FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
            ContextHandle.AddSourceObject(OwnerActor);
            ContextHandle.AddHitResult(Hit);

            FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
            if (SpecHandle.IsValid())
            {
                if (DamageTag.IsValid())
                {
                    SpecHandle.Data->SetSetByCallerMagnitude(DamageTag, BaseDamage);
                }

                SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data, TargetASC);
                HitActors.Add(HitActor);

                UE_LOG(LogTemp, Log, TEXT("近战命中: %s, 伤害: %.1f"), *HitActor->GetName(), BaseDamage);
            }
        }
        else
        {
            HitActors.Add(HitActor);
            UE_LOG(LogTemp, Warning, TEXT("近战命中: %s, 但未配置伤害Effect"), *HitActor->GetName());
        }
    }
}
