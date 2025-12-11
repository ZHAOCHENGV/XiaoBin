/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Character/Components/XBCombatComponent.cpp

/**
 * @file XBCombatComponent.cpp
 * @brief 战斗组件实现
 */

#include "Character/Components/XBCombatComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/DataTable.h"
#include "Data/XBLeaderDataTable.h"

UXBCombatComponent::UXBCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UXBCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* Owner = GetOwner())
    {
        CachedASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);

        if (ACharacter* Character = Cast<ACharacter>(Owner))
        {
            if (USkeletalMeshComponent* Mesh = Character->GetMesh())
            {
                CachedAnimInstance = Mesh->GetAnimInstance();
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("战斗组件初始化 - ASC: %s, AnimInstance: %s"),
        CachedASC.IsValid() ? TEXT("有效") : TEXT("无效"),
        CachedAnimInstance.IsValid() ? TEXT("有效") : TEXT("无效"));
}

void UXBCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (BasicAttackCooldownTimer > 0.0f)
    {
        BasicAttackCooldownTimer -= DeltaTime;
        if (BasicAttackCooldownTimer < 0.0f)
        {
            BasicAttackCooldownTimer = 0.0f;
        }
    }

    if (SkillCooldownTimer > 0.0f)
    {
        SkillCooldownTimer -= DeltaTime;
        if (SkillCooldownTimer < 0.0f)
        {
            SkillCooldownTimer = 0.0f;
        }
    }
}

/**
 * @brief 从数据表初始化战斗组件
 * @param DataTable 数据表资源
 * @param RowName 行名称
 * @note 核心流程：
 *       1. 加载配置数据（普攻/技能）
 *       2. 预加载蒙太奇资源
 *       3. 🔧 修改 - 自动赋予 GA 到 ASC
 */
void UXBCombatComponent::InitializeFromDataTable(UDataTable* DataTable, FName RowName)
{
    if (!DataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("战斗组件初始化失败: 数据表为空"));
        return;
    }

    if (RowName.IsNone())
    {
        UE_LOG(LogTemp, Error, TEXT("战斗组件初始化失败: 行名为空"));
        return;
    }

    FXBLeaderTableRow* Row = DataTable->FindRow<FXBLeaderTableRow>(RowName, TEXT("XBCombatComponent::InitializeFromDataTable"));
    if (!Row)
    {
        UE_LOG(LogTemp, Error, TEXT("战斗组件初始化失败: 找不到行 '%s'"), *RowName.ToString());
        return;
    }

    // 复制配置（包含冷却时间）
    BasicAttackConfig = Row->BasicAttackConfig;
    SpecialSkillConfig = Row->SpecialSkillConfig;

    UE_LOG(LogTemp, Log, TEXT("===== 战斗组件配置加载 ====="));
    UE_LOG(LogTemp, Log, TEXT("数据表: %s, 行: %s"), *DataTable->GetName(), *RowName.ToString());
    
    // 加载普攻蒙太奇
    if (!BasicAttackConfig.AbilityMontage.IsNull())
    {
        UE_LOG(LogTemp, Log, TEXT("普攻蒙太奇路径: %s"), *BasicAttackConfig.AbilityMontage.ToString());
        LoadedBasicAttackMontage = BasicAttackConfig.AbilityMontage.LoadSynchronous();
        
        if (LoadedBasicAttackMontage)
        {
            UE_LOG(LogTemp, Log, TEXT("普攻蒙太奇加载成功: %s"), *LoadedBasicAttackMontage->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("普攻蒙太奇加载失败!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("普攻蒙太奇路径为空"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("普攻冷却时间: %.2f秒"), BasicAttackConfig.Cooldown);

    // 加载技能蒙太奇
    if (!SpecialSkillConfig.AbilityMontage.IsNull())
    {
        UE_LOG(LogTemp, Log, TEXT("技能蒙太奇路径: %s"), *SpecialSkillConfig.AbilityMontage.ToString());
        LoadedSkillMontage = SpecialSkillConfig.AbilityMontage.LoadSynchronous();
        
        if (LoadedSkillMontage)
        {
            UE_LOG(LogTemp, Log, TEXT("技能蒙太奇加载成功: %s"), *LoadedSkillMontage->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("技能蒙太奇加载失败!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("技能蒙太奇路径为空"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("技能冷却时间: %.2f秒"), SpecialSkillConfig.Cooldown);

    // 🔧 修改 - 核心修复：自动赋予 GA 到 ASC
    /**
     * @note 修复原因：
     *       之前仅配置了 GA 类，但未调用 GiveAbility，导致运行时激活失败。
     *       现在在初始化时自动赋予，确保技能系统完整。
     * @note 权限检查：
     *       只有服务端或单机模式下才能赋予 GA。
     */
    if (GetOwner()->HasAuthority() && CachedASC.IsValid())
    {
        // 赋予普攻技能
        if (BasicAttackConfig.AbilityClass)
        {
            FGameplayAbilitySpec BasicAttackSpec(BasicAttackConfig.AbilityClass, 1, INDEX_NONE, this);
            FGameplayAbilitySpecHandle BasicAttackHandle = CachedASC->GiveAbility(BasicAttackSpec);
            
            UE_LOG(LogTemp, Log, TEXT("已赋予普攻GA: %s (Handle: %s)"), 
                *BasicAttackConfig.AbilityClass->GetName(),
                *BasicAttackHandle.ToString());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("普攻GA类未配置"));
        }

        // 赋予特殊技能
        if (SpecialSkillConfig.AbilityClass)
        {
            FGameplayAbilitySpec SkillSpec(SpecialSkillConfig.AbilityClass, 1, INDEX_NONE, this);
            FGameplayAbilitySpecHandle SkillHandle = CachedASC->GiveAbility(SkillSpec);
            
            UE_LOG(LogTemp, Log, TEXT("已赋予技能GA: %s (Handle: %s)"), 
                *SpecialSkillConfig.AbilityClass->GetName(),
                *SkillHandle.ToString());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("技能GA类未配置"));
        }
    }
    else if (!CachedASC.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("尝试赋予技能失败：CachedASC 无效，请确保在 BeginPlay 后调用 InitializeFromDataTable"));
    }
    else if (!GetOwner()->HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("客户端无法赋予GA，技能系统将由服务端同步"));
    }

    UE_LOG(LogTemp, Log, TEXT("===== 配置加载完成 ====="));

    bInitialized = true;
}

bool UXBCombatComponent::PerformBasicAttack()
{
    UE_LOG(LogTemp, Log, TEXT("执行普通攻击 - bIsAttacking: %s, Cooldown: %.2f"), 
        bIsAttacking ? TEXT("true") : TEXT("false"),
        BasicAttackCooldownTimer);

    if (BasicAttackCooldownTimer > 0.0f)
    {
        UE_LOG(LogTemp, Log, TEXT("普攻冷却中: %.2f秒"), BasicAttackCooldownTimer);
        return false;
    }

    if (bIsAttacking)
    {
        if (CachedAnimInstance.IsValid() && LoadedBasicAttackMontage)
        {
            if (!CachedAnimInstance->Montage_IsPlaying(LoadedBasicAttackMontage))
            {
                UE_LOG(LogTemp, Warning, TEXT("检测到攻击状态异常，正在重置"));
                ResetAttackState();
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("普攻正在进行中，忽略输入"));
                return false;
            }
        }
        else
        {
            ResetAttackState();
        }
    }

    UAnimMontage* MontageToPlay = LoadedBasicAttackMontage;
    if (!MontageToPlay)
    {
        UE_LOG(LogTemp, Warning, TEXT("普攻蒙太奇未配置或加载失败"));
        
        if (!BasicAttackConfig.AbilityMontage.IsNull())
        {
            MontageToPlay = BasicAttackConfig.AbilityMontage.LoadSynchronous();
            if (MontageToPlay)
            {
                LoadedBasicAttackMontage = MontageToPlay;
                UE_LOG(LogTemp, Log, TEXT("普攻蒙太奇延迟加载成功"));
            }
        }
        
        if (!MontageToPlay)
        {
            return false;
        }
    }

    if (!PlayMontage(MontageToPlay))
    {
        UE_LOG(LogTemp, Warning, TEXT("普攻蒙太奇播放失败"));
        return false;
    }

    bIsAttacking = true;
    BasicAttackCooldownTimer = BasicAttackConfig.Cooldown;

    if (BasicAttackConfig.AbilityClass)
    {
        TryActivateAbility(BasicAttackConfig.AbilityClass);
    }

    UE_LOG(LogTemp, Log, TEXT("普攻释放成功，冷却: %.2f秒"), BasicAttackConfig.Cooldown);
    return true;
}

bool UXBCombatComponent::PerformSpecialSkill()
{
    UE_LOG(LogTemp, Log, TEXT("尝试使用技能 - bIsAttacking: %s, Cooldown: %.2f"), 
        bIsAttacking ? TEXT("true") : TEXT("false"), SkillCooldownTimer);

    if (SkillCooldownTimer > 0.0f)
    {
        UE_LOG(LogTemp, Log, TEXT("技能冷却中: %.2f秒"), SkillCooldownTimer);
        return false;
    }

    if (bIsAttacking && CachedAnimInstance.IsValid())
    {
        CachedAnimInstance->Montage_Stop(0.2f);
        ResetAttackState();
        UE_LOG(LogTemp, Log, TEXT("技能打断了普攻"));
    }

    UAnimMontage* MontageToPlay = LoadedSkillMontage;
    if (!MontageToPlay)
    {
        UE_LOG(LogTemp, Warning, TEXT("技能蒙太奇未配置或加载失败"));
        
        if (!SpecialSkillConfig.AbilityMontage.IsNull())
        {
            MontageToPlay = SpecialSkillConfig.AbilityMontage.LoadSynchronous();
            if (MontageToPlay)
            {
                LoadedSkillMontage = MontageToPlay;
                UE_LOG(LogTemp, Log, TEXT("技能蒙太奇延迟加载成功"));
            }
        }
        
        if (!MontageToPlay)
        {
            return false;
        }
    }

    if (!PlayMontage(MontageToPlay))
    {
        UE_LOG(LogTemp, Warning, TEXT("技能蒙太奇播放失败"));
        return false;
    }

    bIsAttacking = true;
    SkillCooldownTimer = SpecialSkillConfig.Cooldown;

    if (SpecialSkillConfig.AbilityClass)
    {
        TryActivateAbility(SpecialSkillConfig.AbilityClass);
    }

    UE_LOG(LogTemp, Log, TEXT("技能释放成功，冷却: %.2f秒"), SpecialSkillConfig.Cooldown);
    return true;
}

void UXBCombatComponent::ResetAttackState()
{
    bIsAttacking = false;
    UE_LOG(LogTemp, Log, TEXT("攻击状态已重置"));
}

bool UXBCombatComponent::PlayMontage(UAnimMontage* Montage, float PlayRate)
{
    if (!Montage)
    {
        return false;
    }

    if (!CachedAnimInstance.IsValid())
    {
        if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
        {
            if (USkeletalMeshComponent* Mesh = Character->GetMesh())
            {
                CachedAnimInstance = Mesh->GetAnimInstance();
            }
        }
    }

    if (!CachedAnimInstance.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("无法获取AnimInstance"));
        return false;
    }

    float Duration = CachedAnimInstance->Montage_Play(Montage, PlayRate);
    if (Duration <= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("蒙太奇播放返回时长为0"));
        return false;
    }

    FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &UXBCombatComponent::OnMontageEnded);
    CachedAnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);

    UE_LOG(LogTemp, Log, TEXT("蒙太奇播放: %s, 时长: %.2f"), *Montage->GetName(), Duration);
    return true;
}

void UXBCombatComponent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    UE_LOG(LogTemp, Log, TEXT("蒙太奇结束: %s, 被打断: %s"), 
        Montage ? *Montage->GetName() : TEXT("null"),
        bInterrupted ? TEXT("是") : TEXT("否"));

    ResetAttackState();
}

/**
 * @brief 尝试激活 GameplayAbility
 * @param AbilityClass GA 类
 * @return 是否成功激活
 * @note 核心逻辑：通过 ASC 的 TryActivateAbilityByClass 激活技能
 */
bool UXBCombatComponent::TryActivateAbility(TSubclassOf<UGameplayAbility> AbilityClass)
{
    if (!AbilityClass)
    {
        return false;
    }

    if (!CachedASC.IsValid())
    {
        if (AActor* Owner = GetOwner())
        {
            CachedASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
        }
    }

    if (!CachedASC.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("无法激活GA: ASC无效"));
        return false;
    }

    bool bSuccess = CachedASC->TryActivateAbilityByClass(AbilityClass);
    UE_LOG(LogTemp, Log, TEXT("激活GA '%s': %s"), 
        *AbilityClass->GetName(),
        bSuccess ? TEXT("成功") : TEXT("失败"));

    return bSuccess;
}

/**
 * @brief 设置攻击范围缩放倍率
 * @param ScaleMultiplier 缩放倍率
 * @note ✨ 新增方法
 */
void UXBCombatComponent::SetAttackRangeScale(float ScaleMultiplier)
{
    AttackRangeScaleMultiplier = FMath::Max(0.1f, ScaleMultiplier);

    UE_LOG(LogTemp, Verbose, TEXT("攻击范围缩放: %.2f → 实际范围: %.0f"), 
        AttackRangeScaleMultiplier, GetScaledAttackRange());
}

/**
 * @brief 设置攻击范围缩放倍率
 * @param ScaleMultiplier 缩放倍率
 * @note ✨ 新增方法
 */
float UXBCombatComponent::GetScaledAttackRange() const
{
    return BaseAttackRange * AttackRangeScaleMultiplier;
}

/**
 * @brief 检查目标是否在攻击范围内
 * @param Target 目标Actor
 * @return 是否在范围内
 * @note ✨ 新增方法
 */
bool UXBCombatComponent::IsTargetInRange(AActor* Target) const
{
    if (!Target)
    {
        return false;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return false;
    }

    float Distance = FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation());
    return Distance <= GetScaledAttackRange();
    
}
