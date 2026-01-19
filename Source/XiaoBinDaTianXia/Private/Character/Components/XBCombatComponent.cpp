/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Character/Components/XBCombatComponent.cpp

/**
 * @file XBCombatComponent.cpp
 * @brief 战斗组件实现
 * 
 * @note 🔧 修改记录:
 *       1. ✨ 新增 攻击上下文追踪
 *       2. ✨ 新增 GetCurrentAttackDamage() 和 GetCurrentAttackFinalDamage()
 *       3. 🔧 修改 PerformBasicAttack/PerformSpecialSkill 设置攻击类型
 *       4. 🔧 修改 ResetAttackState 重置攻击类型
 *       5. ✨ 新增 攻击状态变化委托广播
 *       6. ✨ 新增 ShouldBlockMovement() 判断
 *       7. 🔧 修改 - 增加蒙太奇互斥检查，正在播放蒙太奇时禁止触发新攻击
 */

#include "Character/Components/XBCombatComponent.h"
#include "Character/XBCharacterBase.h"
#include "Character/XBPlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GAS/XBAttributeSet.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
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

    BasicAttackConfig = Row->BasicAttackConfig;
    SpecialSkillConfig = Row->SpecialSkillConfig;

    UE_LOG(LogTemp, Log, TEXT("===== 战斗组件配置加载 ====="));
    UE_LOG(LogTemp, Log, TEXT("数据表: %s, 行: %s"), *DataTable->GetName(), *RowName.ToString());
    
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
    
    UE_LOG(LogTemp, Log, TEXT("普攻基础伤害: %.1f"), BasicAttackConfig.BaseDamage);
    UE_LOG(LogTemp, Log, TEXT("普攻冷却时间: %.2f秒"), BasicAttackConfig.Cooldown);

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
    
    UE_LOG(LogTemp, Log, TEXT("技能基础伤害: %.1f"), SpecialSkillConfig.BaseDamage);
    UE_LOG(LogTemp, Log, TEXT("技能冷却时间: %.2f秒"), SpecialSkillConfig.Cooldown);

    if (GetOwner()->HasAuthority() && CachedASC.IsValid())
    {
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

void UXBCombatComponent::SetAttackingState(bool bNewAttacking)
{
    if (bIsAttacking != bNewAttacking)
    {
        bIsAttacking = bNewAttacking;
        
        OnAttackStateChanged.Broadcast(bIsAttacking);
        
        UE_LOG(LogTemp, Log, TEXT("攻击状态变化: %s"), bIsAttacking ? TEXT("开始攻击") : TEXT("结束攻击"));
    }
}

bool UXBCombatComponent::ShouldBlockMovement() const
{
    if (!bIsAttacking)
    {
        return false;
    }

    switch (CurrentAttackType)
    {
    case EXBAttackType::BasicAttack:
        return bBlockMovementDuringBasicAttack;
        
    case EXBAttackType::SpecialSkill:
        return bBlockMovementDuringSkill;
        
    default:
        return false;
    }
}

/**
 * @brief 检查是否有任何攻击/技能蒙太奇正在播放
 * @return 是否有蒙太奇正在播放
 * @note ✨ 新增 - 用于蒙太奇互斥检查
 */
bool UXBCombatComponent::IsAnyAttackMontagePlayingInternal() const
{
    if (!CachedAnimInstance.IsValid())
    {
        return false;
    }

    UAnimInstance* AnimInstance = CachedAnimInstance.Get();

    // 检查普攻蒙太奇是否正在播放
    if (LoadedBasicAttackMontage && AnimInstance->Montage_IsPlaying(LoadedBasicAttackMontage))
    {
        return true;
    }

    // 检查技能蒙太奇是否正在播放
    if (LoadedSkillMontage && AnimInstance->Montage_IsPlaying(LoadedSkillMontage))
    {
        return true;
    }

    return false;
}

/**
 * @brief 执行普通攻击
 * @return 是否成功
 * @note 🔧 修改 - 增加蒙太奇互斥检查
 */
bool UXBCombatComponent::PerformBasicAttack()
{
    UE_LOG(LogTemp, Log, TEXT("执行普通攻击 - bIsAttacking: %s, Cooldown: %.2f"), 
        bIsAttacking ? TEXT("true") : TEXT("false"),
        BasicAttackCooldownTimer);

    // 冷却检查
    // 🔧 修改 - 玩家角色不检查数值冷却，仅依赖蒙太奇播放状态
    const bool bIsPlayer = GetOwner() && GetOwner()->IsA(AXBPlayerCharacter::StaticClass());
    if (!bIsPlayer && BasicAttackCooldownTimer > 0.0f)
    {
        UE_LOG(LogTemp, Log, TEXT("普攻冷却中: %.2f秒"), BasicAttackCooldownTimer);
        return false;
    }

    // ✨ 新增 - 蒙太奇互斥检查：如果有任何攻击蒙太奇正在播放，则拒绝新的攻击
    if (IsAnyAttackMontagePlayingInternal())
    {
        UE_LOG(LogTemp, Verbose, TEXT("普攻被拒绝: 已有攻击蒙太奇正在播放"));
        // 🔧 修改 - 不设置冷却，避免影响AI的能力选择逻辑
        return false;
    }

    // 🔧 修改 - 移除旧的状态重置逻辑，由蒙太奇互斥检查替代
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

    SetCurrentAttackType(EXBAttackType::BasicAttack);
    SetAttackingState(true);

    if (AXBCharacterBase* OwnerLeader = Cast<AXBCharacterBase>(GetOwner()))
    {
        OwnerLeader->NotifyAttackStarted();
    }
    
    BasicAttackCooldownTimer = BasicAttackConfig.Cooldown;

    if (BasicAttackConfig.AbilityClass)
    {
        TryActivateAbility(BasicAttackConfig.AbilityClass);
    }

    UE_LOG(LogTemp, Log, TEXT("普攻释放成功，伤害: %.1f，冷却: %.2f秒"), 
        BasicAttackConfig.BaseDamage, BasicAttackConfig.Cooldown);
    return true;
}

/**
 * @brief 执行技能攻击
 * @return 是否成功
 * @note 🔧 修改 - 增加蒙太奇互斥检查
 */
bool UXBCombatComponent::PerformSpecialSkill()
{
    UE_LOG(LogTemp, Log, TEXT("尝试使用技能 - bIsAttacking: %s, Cooldown: %.2f"), 
        bIsAttacking ? TEXT("true") : TEXT("false"), SkillCooldownTimer);

    // 冷却检查
    // 🔧 修改 - 玩家角色不检查数值冷却，仅依赖蒙太奇播放状态
    const bool bIsPlayer = GetOwner() && GetOwner()->IsA(AXBPlayerCharacter::StaticClass());
    if (!bIsPlayer && SkillCooldownTimer > 0.0f)
    {
        UE_LOG(LogTemp, Log, TEXT("技能冷却中: %.2f秒"), SkillCooldownTimer);
        return false;
    }

    // ✨ 新增 - 蒙太奇互斥检查：如果有任何攻击蒙太奇正在播放，则拒绝新的攻击
    if (IsAnyAttackMontagePlayingInternal())
    {
        UE_LOG(LogTemp, Verbose, TEXT("技能被拒绝: 已有攻击蒙太奇正在播放"));
        // 🔧 修改 - 不设置冷却，避免影响AI的能力选择逻辑
        // 如果蒙太奇正在播放，应该由攻击任务等待，而不是设置冷却
        return false;
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

    SetCurrentAttackType(EXBAttackType::SpecialSkill);
    SetAttackingState(true);

    if (AXBCharacterBase* OwnerLeader = Cast<AXBCharacterBase>(GetOwner()))
    {
        OwnerLeader->NotifyAttackStarted();
    }
    
    SkillCooldownTimer = SpecialSkillConfig.Cooldown;

    if (SpecialSkillConfig.AbilityClass)
    {
        TryActivateAbility(SpecialSkillConfig.AbilityClass);
    }

    UE_LOG(LogTemp, Log, TEXT("技能释放成功，伤害: %.1f，冷却: %.2f秒"), 
        SpecialSkillConfig.BaseDamage, SpecialSkillConfig.Cooldown);
    return true;
}

void UXBCombatComponent::ResetAttackState()
{
    SetCurrentAttackType(EXBAttackType::None);
    SetAttackingState(false);
    UE_LOG(LogTemp, Log, TEXT("攻击状态已重置"));
}

void UXBCombatComponent::SetCurrentAttackType(EXBAttackType NewType)
{
    CurrentAttackType = NewType;
}

float UXBCombatComponent::GetCurrentAttackDamage() const
{
    switch (CurrentAttackType)
    {
    case EXBAttackType::BasicAttack:
        return BasicAttackConfig.BaseDamage;
        
    case EXBAttackType::SpecialSkill:
        return SpecialSkillConfig.BaseDamage;
        
    case EXBAttackType::None:
    default:
        UE_LOG(LogTemp, Warning, TEXT("GetCurrentAttackDamage: 当前没有活跃的攻击类型"));
        return 0.0f;
    }
}

float UXBCombatComponent::GetDamageMultiplier() const
{
    if (CachedASC.IsValid())
    {
        return CachedASC->GetNumericAttribute(UXBAttributeSet::GetDamageMultiplierAttribute());
    }
    return 1.0f;
}

float UXBCombatComponent::GetCurrentAttackFinalDamage() const
{
    float BaseDamage = GetCurrentAttackDamage();
    float Multiplier = GetDamageMultiplier();
    return BaseDamage * Multiplier;
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

    // 🔧 修改 - 若仍有其他攻击蒙太奇在播放（例如普攻被技能打断），避免过早清空攻击上下文
    if (IsAnyAttackMontagePlayingInternal())
    {
        UE_LOG(LogTemp, Verbose, TEXT("蒙太奇结束但仍有攻击蒙太奇在播放，保持当前攻击状态"));
        return;
    }

    // 🔧 修改 - 所有攻击蒙太奇均已结束，安全重置状态
    ResetAttackState();
}

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

void UXBCombatComponent::SetAttackRangeScale(float ScaleMultiplier)
{
    AttackRangeScaleMultiplier = FMath::Max(0.1f, ScaleMultiplier);

    UE_LOG(LogTemp, Verbose, TEXT("攻击范围缩放: %.2f → 实际范围: %.0f"), 
        AttackRangeScaleMultiplier, GetScaledAttackRange());
}

float UXBCombatComponent::GetScaledAttackRange() const
{
    return BaseAttackRange * AttackRangeScaleMultiplier;
}

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

    const float CenterDistance = FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation());
    const float OwnerRadius = Owner->GetSimpleCollisionRadius();
    const float TargetRadius = Target->GetSimpleCollisionRadius();
    const float EdgeDistance = CenterDistance - OwnerRadius - TargetRadius;

    return EdgeDistance <= GetScaledAttackRange();
}

// ✨ 新增 - 获取普攻攻击范围
/**
 * @brief 获取普攻攻击范围
 * @return 普攻攻击范围（应用缩放后）
 * @note 从 BasicAttackConfig.AttackRange 读取并应用缩放倍率
 */
float UXBCombatComponent::GetBasicAttackRange() const
{
    return BasicAttackConfig.AttackRange * AttackRangeScaleMultiplier;
}

// ✨ 新增 - 获取技能攻击范围
/**
 * @brief 获取技能攻击范围
 * @return 技能攻击范围（应用缩放后）
 * @note 从 SpecialSkillConfig.AttackRange 读取并应用缩放倍率
 */
float UXBCombatComponent::GetSkillAttackRange() const
{
    return SpecialSkillConfig.AttackRange * AttackRangeScaleMultiplier;
}

// ✨ 新增 - 检查目标是否在普攻范围内
/**
 * @brief 检查目标是否在普攻范围内
 * @param Target 目标Actor
 * @return 是否在范围内
 */
bool UXBCombatComponent::IsTargetInBasicAttackRange(AActor* Target) const
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

    // 🔧 关键修复 - 使用胶囊体组件获取半径，与移动任务保持一致
    const ACharacter* OwnerChar = Cast<ACharacter>(Owner);
    const ACharacter* TargetChar = Cast<ACharacter>(Target);
    const float OwnerRadius = (OwnerChar && OwnerChar->GetCapsuleComponent())
        ? OwnerChar->GetCapsuleComponent()->GetScaledCapsuleRadius()
        : Owner->GetSimpleCollisionRadius();
    const float TargetRadius = (TargetChar && TargetChar->GetCapsuleComponent())
        ? TargetChar->GetCapsuleComponent()->GetScaledCapsuleRadius()
        : Target->GetSimpleCollisionRadius();

    const float CenterDistance = FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation());
    const float EdgeDistance = FMath::Max(0.0f, CenterDistance - OwnerRadius - TargetRadius);
    const float AttackRange = GetBasicAttackRange();
    
    return EdgeDistance <= AttackRange;
}

// ✨ 新增 - 检查目标是否在技能范围内
/**
 * @brief 检查目标是否在技能范围内
 * @param Target 目标Actor
 * @return 是否在范围内
 */
bool UXBCombatComponent::IsTargetInSkillRange(AActor* Target) const
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

    // 🔧 关键修复 - 使用胶囊体组件获取半径，与移动任务保持一致
    const ACharacter* OwnerChar = Cast<ACharacter>(Owner);
    const ACharacter* TargetChar = Cast<ACharacter>(Target);
    const float OwnerRadius = (OwnerChar && OwnerChar->GetCapsuleComponent())
        ? OwnerChar->GetCapsuleComponent()->GetScaledCapsuleRadius()
        : Owner->GetSimpleCollisionRadius();
    const float TargetRadius = (TargetChar && TargetChar->GetCapsuleComponent())
        ? TargetChar->GetCapsuleComponent()->GetScaledCapsuleRadius()
        : Target->GetSimpleCollisionRadius();

    const float CenterDistance = FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation());
    const float EdgeDistance = FMath::Max(0.0f, CenterDistance - OwnerRadius - TargetRadius);
    const float AttackRange = GetSkillAttackRange();
    
    return EdgeDistance <= AttackRange;
}
