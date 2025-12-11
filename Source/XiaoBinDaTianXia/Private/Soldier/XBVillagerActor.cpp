/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Village/XBVillagerActor.cpp

/**
 * @file XBVillagerActor.cpp
 * @brief 村民Actor实现
 * 
 * @note ✨ 新增文件
 */

#include "Soldier/XBVillagerActor.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Animation/AnimInstance.h"

AXBVillagerActor::AXBVillagerActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // 🔧 修改 - 配置胶囊体（与士兵相同）
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->InitCapsuleSize(34.0f, 88.0f);
        Capsule->SetCollisionProfileName(TEXT("Pawn"));
    }

    // 🔧 修改 - 配置网格体偏移
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
    }

    // ✨ 新增 - 创建 Zzz 特效组件
    ZzzEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ZzzEffectComponent"));
    ZzzEffectComponent->SetupAttachment(RootComponent);
    ZzzEffectComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f)); // 头顶上方
    ZzzEffectComponent->SetAutoActivate(false);

    // 🔧 修改 - 禁用移动（村民静止）
    if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
    {
        MovementComp->DisableMovement();
        MovementComp->SetComponentTickEnabled(false);
    }

    // 禁用AI控制
    AutoPossessAI = EAutoPossessAI::Disabled;
}

void AXBVillagerActor::BeginPlay()
{
    Super::BeginPlay();

    // ✨ 新增 - 加载 Zzz 特效资源
    if (!ZzzEffectAsset.IsNull())
    {
        UNiagaraSystem* LoadedEffect = ZzzEffectAsset.LoadSynchronous();
        if (LoadedEffect && ZzzEffectComponent)
        {
            ZzzEffectComponent->SetAsset(LoadedEffect);
        }
    }

    // 初始化状态
    SetVillagerState(CurrentState);
}

void AXBVillagerActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

/**
 * @brief 设置村民状态
 * @param NewState 新状态
 * @note 功能：
 *       1. 切换动画蒙太奇
 *       2. 控制 Zzz 特效显示
 */
void AXBVillagerActor::SetVillagerState(EXBVillagerState NewState)
{
    if (CurrentState == NewState)
    {
        return;
    }

    CurrentState = NewState;

    // 更新动画
    UpdateAnimationState();

    // 更新 Zzz 特效
    UpdateZzzEffect();

    UE_LOG(LogTemp, Log, TEXT("村民 %s 状态切换为: %d"), *GetName(), static_cast<int32>(NewState));
}

bool AXBVillagerActor::CanBeRecruited() const
{
    // 条件：未被招募且存活
    return !bIsRecruited && !IsPendingKillPending();
}

/**
 * @brief 被招募回调
 * @param Leader 招募的将领
 * @note 功能：
 *       1. 标记为已招募
 *       2. 禁用 Zzz 特效
 *       3. 销毁自身（转化为士兵由磁场组件处理）
 */
void AXBVillagerActor::OnRecruited(AActor* Leader)
{
    if (!Leader)
    {
        return;
    }

    bIsRecruited = true;

    // 关闭 Zzz 特效
    if (ZzzEffectComponent)
    {
        ZzzEffectComponent->Deactivate();
    }

    UE_LOG(LogTemp, Log, TEXT("村民 %s 被 %s 招募"), *GetName(), *Leader->GetName());

    // ✨ 新增 - 延迟销毁，给磁场组件时间转化士兵
    SetLifeSpan(0.1f);
}

void AXBVillagerActor::SetZzzEffectEnabled(bool bEnabled)
{
    bEnableZzzEffect = bEnabled;
    UpdateZzzEffect();
}

/**
 * @brief 更新动画状态
 * @note 根据当前状态播放对应蒙太奇
 */
void AXBVillagerActor::UpdateAnimationState()
{
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    // 停止当前蒙太奇
    AnimInstance->StopAllMontages(0.2f);

    // 播放新蒙太奇
    UAnimMontage* MontageToPlay = nullptr;

    switch (CurrentState)
    {
    case EXBVillagerState::Sleeping:
        MontageToPlay = SleepingMontage;
        break;

    case EXBVillagerState::Idle:
        MontageToPlay = IdleMontage;
        break;

    default:
        break;
    }

    if (MontageToPlay)
    {
        AnimInstance->Montage_Play(MontageToPlay);
    }
}

/**
 * @brief 更新 Zzz 特效显示
 * @note 只有睡眠状态且启用特效时才显示
 */
void AXBVillagerActor::UpdateZzzEffect()
{
    if (!ZzzEffectComponent)
    {
        return;
    }

    bool bShouldShowZzz = (CurrentState == EXBVillagerState::Sleeping) && bEnableZzzEffect;

    if (bShouldShowZzz)
    {
        ZzzEffectComponent->Activate(true);
    }
    else
    {
        ZzzEffectComponent->Deactivate();
    }
}
