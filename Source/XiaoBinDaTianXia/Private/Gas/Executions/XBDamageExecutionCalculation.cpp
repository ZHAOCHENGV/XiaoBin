// Source/XiaoBinDaTianXia/Private/GAS/Executions/XBDamageExecutionCalculation.cpp
/* --- 完整文件代码 --- */

// Copyright XiaoBing Project. All Rights Reserved.

#include "GAS/Executions/XBDamageExecutionCalculation.h"
#include "GAS/XBAttributeSet.h"
#include "AbilitySystemComponent.h"

// ✨ 新增 - 声明捕获属性的结构体
struct FXBDamageStatics
{
    // 声明需要捕获的属性

    DECLARE_ATTRIBUTE_CAPTUREDEF(DamageMultiplier);
    DECLARE_ATTRIBUTE_CAPTUREDEF(Scale);
    DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);

    FXBDamageStatics()
    {
       
        // 从源捕获 DamageMultiplier（快照）
        DEFINE_ATTRIBUTE_CAPTUREDEF(UXBAttributeSet, DamageMultiplier, Source, true);
        // 从源捕获 Scale（快照）
        DEFINE_ATTRIBUTE_CAPTUREDEF(UXBAttributeSet, Scale, Source, true);
        // 目标的 IncomingDamage（不快照，实时值）
        DEFINE_ATTRIBUTE_CAPTUREDEF(UXBAttributeSet, IncomingDamage, Target, false);
    }
};

// 静态单例获取器
static const FXBDamageStatics& DamageStatics()
{
    static FXBDamageStatics Statics;
    return Statics;
}

UXBDamageExecutionCalculation::UXBDamageExecutionCalculation()
{
    // 注册需要捕获的属性

    RelevantAttributesToCapture.Add(DamageStatics().DamageMultiplierDef);
    RelevantAttributesToCapture.Add(DamageStatics().ScaleDef);
}

void UXBDamageExecutionCalculation::Execute_Implementation(
    const FGameplayEffectCustomExecutionParameters& ExecutionParams,
    FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
    // 获取源和目标的 ASC
    UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
    UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();

    if (!SourceASC || !TargetASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("伤害计算失败: ASC 无效"));
        return;
    }

    // 获取 Effect Spec
    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

    // 准备聚合求值参数
    FAggregatorEvaluateParameters EvaluateParams;
    EvaluateParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    EvaluateParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    // 捕获源属性值
    float BaseDamage = 0.0f;
    float DamageMultiplier = 1.0f;
    float Scale = 1.0f;
    
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
        DamageStatics().DamageMultiplierDef, EvaluateParams, DamageMultiplier);
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
        DamageStatics().ScaleDef, EvaluateParams, Scale);

    // 尝试获取 SetByCaller 的额外伤害（技能可以通过此传递额外伤害）
    float SetByCallerDamage = Spec.GetSetByCallerMagnitude(
        FGameplayTag::RequestGameplayTag(FName("Data.Damage")), false, 0.0f);

    // ✨ 新增 - 核心伤害公式
    // FinalDamage = (BaseDamage * DamageMultiplier * Scale) + SetByCallerDamage
    float FinalDamage = (BaseDamage * DamageMultiplier * Scale) + SetByCallerDamage;

    // 确保伤害非负
    FinalDamage = FMath::Max(0.0f, FinalDamage);

    UE_LOG(LogTemp, Log, TEXT("伤害计算: Base=%.1f, Mult=%.1f, Scale=%.1f, SetByCaller=%.1f, Final=%.1f"),
        BaseDamage, DamageMultiplier, Scale, SetByCallerDamage, FinalDamage);

    // 输出到目标的 IncomingDamage 属性
    if (FinalDamage > 0.0f)
    {
        OutExecutionOutput.AddOutputModifier(
            FGameplayModifierEvaluatedData(
                DamageStatics().IncomingDamageProperty,
                EGameplayModOp::Additive,
                FinalDamage));
    }
}
