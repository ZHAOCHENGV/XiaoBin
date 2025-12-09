/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Animation/ANS_XBMeleeDetection.cpp

/**
 * @file ANS_XBMeleeDetection.cpp
 * @brief 近战检测动画通知状态实现
 */

#include "Animation/ANS_XBMeleeDetection.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"

UANS_XBMeleeDetection::UANS_XBMeleeDetection()
{
    // 初始化默认伤害Tag
    DamageTag = FGameplayTag::RequestGameplayTag(FName("Damage.Base"), false);
}

void UANS_XBMeleeDetection::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    // 清空已命中列表
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

    // 执行检测
    TArray<FHitResult> HitResults = PerformCapsuleTrace(MeshComp);

    // 应用伤害
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

TArray<FHitResult> UANS_XBMeleeDetection::PerformCapsuleTrace(USkeletalMeshComponent* MeshComp)
{
    TArray<FHitResult> OutHitResults;

    if (!MeshComp)
    {
        return OutHitResults;
    }

    // 🔧 修改 - 使用双插槽获取起始和结束位置
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
    float ActualHalfHeight = DetectionConfig.CapsuleHalfHeight > 0.0f ? 
        DetectionConfig.CapsuleHalfHeight : 
        FMath::Max(Distance * 0.5f, DetectionConfig.CapsuleRadius);

    // 设置忽略的Actor
    TArray<AActor*> IgnoreActors = DetectionConfig.ActorsToIgnore;
    if (AActor* Owner = MeshComp->GetOwner())
    {
        IgnoreActors.AddUnique(Owner);
    }

    // 执行胶囊体多重检测
    TArray<FHitResult> HitResults;
    bool bHit = UKismetSystemLibrary::CapsuleTraceMultiForObjects(
        MeshComp->GetWorld(),
        CapsuleCenter,
        CapsuleCenter, // 单点检测，使用胶囊体覆盖范围
        DetectionConfig.CapsuleRadius,
        ActualHalfHeight,
        DetectionConfig.ObjectTypes,
        false, // bTraceComplex
        IgnoreActors,
        DetectionConfig.bEnableDebugDraw ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
        HitResults,
        true, // bIgnoreSelf
        FLinearColor::Red,
        FLinearColor::Green,
        DetectionConfig.DebugDrawDuration
    );

    // 调试绘制 - 显示双插槽位置
    if (DetectionConfig.bEnableDebugDraw)
    {
        UWorld* World = MeshComp->GetWorld();
        if (World)
        {
            // 绘制起始点
            DrawDebugSphere(World, StartLocation, 10.0f, 8, FColor::Blue, false, DetectionConfig.DebugDrawDuration);
            // 绘制结束点
            DrawDebugSphere(World, EndLocation, 10.0f, 8, FColor::Cyan, false, DetectionConfig.DebugDrawDuration);
            // 绘制连接线
            DrawDebugLine(World, StartLocation, EndLocation, FColor::Yellow, false, DetectionConfig.DebugDrawDuration, 0, 2.0f);
            // 绘制胶囊体
            DrawDebugCapsule(World, CapsuleCenter, ActualHalfHeight, DetectionConfig.CapsuleRadius, 
                CapsuleRotation, bHit ? FColor::Red : FColor::Green, false, DetectionConfig.DebugDrawDuration);
        }
    }

    OutHitResults = HitResults;
    return OutHitResults;
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
            continue; // 跳过已命中的目标
        }

        // 获取目标的ASC
        UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor);
        if (!TargetASC)
        {
            continue;
        }

        // 检查是否为敌对目标 (可以添加阵营检查)
        if (HitActor == OwnerActor)
        {
            continue;
        }

        // 应用伤害Effect
        if (DamageEffectClass)
        {
            FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
            ContextHandle.AddSourceObject(OwnerActor);
            ContextHandle.AddHitResult(Hit);

            FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
            if (SpecHandle.IsValid())
            {
                // 设置伤害值
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
            // 如果没有配置Effect，直接记录命中
            HitActors.Add(HitActor);
            UE_LOG(LogTemp, Warning, TEXT("近战命中: %s, 但未配置伤害Effect"), *HitActor->GetName());
        }
    }
}
