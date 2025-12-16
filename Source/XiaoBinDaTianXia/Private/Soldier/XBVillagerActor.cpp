/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Village/XBVillagerActor.cpp

/**
 * @file XBVillagerActor.cpp
 * @brief 村民Actor实现 - 继承AActor版本
 */

#include "Soldier/XBVillagerActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Animation/AnimSequence.h"
#include "XBCollisionChannels.h"

AXBVillagerActor::AXBVillagerActor()
{
    // 不需要 Tick
    PrimaryActorTick.bCanEverTick = false;

    // ============ 1. 创建胶囊体作为根组件 ============
    CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
    SetRootComponent(CapsuleComponent);
    CapsuleComponent->InitCapsuleSize(34.0f, 88.0f);

    // 配置碰撞
    CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CapsuleComponent->SetCollisionObjectType(XBCollision::Soldier);
    CapsuleComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
    CapsuleComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    CapsuleComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
    CapsuleComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    CapsuleComponent->SetCollisionResponseToChannel(XBCollision::Leader, ECR_Overlap);
    CapsuleComponent->SetCollisionResponseToChannel(XBCollision::Soldier, ECR_Overlap);

    // 物理配置（静止村民不需要物理模拟）
    CapsuleComponent->SetSimulatePhysics(false);
    CapsuleComponent->SetEnableGravity(false);

    // ============ 2. 创建骨骼网格体 ============
    MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(CapsuleComponent);
    // 调整位置使脚部对齐胶囊体底部（胶囊体半高88）
    MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));

    // 网格体不参与碰撞
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // ============ 3. 创建 Zzz 特效组件 ============
    ZzzEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ZzzEffectComponent"));
    ZzzEffectComponent->SetupAttachment(CapsuleComponent);
    ZzzEffectComponent->SetRelativeLocation(ZzzEffectOffset);
    ZzzEffectComponent->SetAutoActivate(false);
}

void AXBVillagerActor::BeginPlay()
{
    Super::BeginPlay();

    // 加载 Zzz 特效资源
    if (!ZzzEffectAsset.IsNull() && ZzzEffectComponent)
    {
        if (UNiagaraSystem* LoadedEffect = ZzzEffectAsset.LoadSynchronous())
        {
            ZzzEffectComponent->SetAsset(LoadedEffect);
        }
    }

    // 更新特效位置（以防在编辑器中修改了偏移值）
    if (ZzzEffectComponent)
    {
        ZzzEffectComponent->SetRelativeLocation(ZzzEffectOffset);
    }

    // 初始化状态
    UpdateAnimation();
    UpdateZzzEffect();

    UE_LOG(LogTemp, Log, TEXT("村民 %s 初始化完成 - 状态: %s"),
        *GetName(),
        CurrentState == EXBVillagerState::Sleeping ? TEXT("睡眠") : TEXT("待机"));
}

void AXBVillagerActor::SetVillagerState(EXBVillagerState NewState)
{
    if (CurrentState == NewState)
    {
        return;
    }

    CurrentState = NewState;

    UpdateAnimation();
    UpdateZzzEffect();

    UE_LOG(LogTemp, Log, TEXT("村民 %s 状态切换为: %s"),
        *GetName(),
        CurrentState == EXBVillagerState::Sleeping ? TEXT("睡眠") : TEXT("待机"));
}

bool AXBVillagerActor::CanBeRecruited() const
{
    return !bIsRecruited && !IsPendingKillPending();
}

void AXBVillagerActor::OnRecruited(AActor* Leader)
{
    if (!Leader || bIsRecruited)
    {
        return;
    }

    bIsRecruited = true;

    // 关闭 Zzz 特效
    if (ZzzEffectComponent)
    {
        ZzzEffectComponent->Deactivate();
    }

    // 停止动画
    if (MeshComponent)
    {
        MeshComponent->Stop();
    }

    UE_LOG(LogTemp, Log, TEXT("村民 %s 被 %s 招募"), *GetName(), *Leader->GetName());

    // 延迟销毁，给磁场组件时间处理转化逻辑
    SetLifeSpan(0.1f);
}

void AXBVillagerActor::SetZzzEffectEnabled(bool bEnabled)
{
    bEnableZzzEffect = bEnabled;
    UpdateZzzEffect();
}

void AXBVillagerActor::UpdateAnimation()
{
    UAnimSequence* AnimToPlay = nullptr;

    switch (CurrentState)
    {
    case EXBVillagerState::Idle:
        AnimToPlay = IdleAnimation;
        break;

    case EXBVillagerState::Sleeping:
        AnimToPlay = SleepingAnimation;
        break;

    default:
        break;
    }

    PlayAnimation(AnimToPlay, true);
}

void AXBVillagerActor::UpdateZzzEffect()
{
    if (!ZzzEffectComponent)
    {
        return;
    }

    const bool bShouldShowZzz = (CurrentState == EXBVillagerState::Sleeping) && bEnableZzzEffect;

    if (bShouldShowZzz)
    {
        ZzzEffectComponent->Activate(true);
    }
    else
    {
        ZzzEffectComponent->Deactivate();
    }
}

void AXBVillagerActor::PlayAnimation(UAnimSequence* Animation, bool bLoop)
{
    if (!MeshComponent)
    {
        return;
    }

    if (Animation)
    {
        // 使用 PlayAnimation 直接播放动画序列
        // 第二个参数 bLooping 控制是否循环
        MeshComponent->PlayAnimation(Animation, bLoop);
    }
    else
    {
        // 没有动画时停止播放
        MeshComponent->Stop();
    }
}
