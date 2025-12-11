/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/UI/XBLeaderHealthWidget.cpp

/**
 * @file XBLeaderHealthWidget.cpp
 * @brief 将领血量 UI Widget 实现
 * 
 * @note 🔧 修改 - 修复 NativeConstruct 覆盖已设置数据的问题
 */

#include "UI/XBLeaderHealthWidget.h"
#include "Character/XBCharacterBase.h"
#include "GAS/XBAbilitySystemComponent.h"
#include "GAS/XBAttributeSet.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

// 🔧 修改 - 重写 NativeConstruct，在最后刷新数据
void UXBLeaderHealthWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 检查控件是否绑定成功
    if (Text_LeaderName)
    {
        UE_LOG(LogTemp, Log, TEXT("NativeConstruct: Text_LeaderName 存在"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("NativeConstruct: Text_LeaderName 为空!"));
    }

    if (Text_HealthValue)
    {
        UE_LOG(LogTemp, Log, TEXT("NativeConstruct: Text_HealthValue 存在"));
    }

    if (ProgressBar_Health)
    {
        UE_LOG(LogTemp, Log, TEXT("NativeConstruct: ProgressBar_Health 存在"));
    }

    // 🔧 修改 - 如果已经有 Owner，刷新显示数据
    // 否则设置为默认值
    if (OwningLeader.IsValid())
    {
        UE_LOG(LogTemp, Log, TEXT("NativeConstruct: 检测到已有 Owner，执行强制刷新"));
        
        // 清除缓存并强制刷新
        ClearCache();
        ForceRefreshDisplay();
    }
    else
    {
        // 没有 Owner，设置默认值
        if (Text_LeaderName)
        {
            Text_LeaderName->SetText(FText::FromString(TEXT("--")));
        }

        if (Text_HealthValue)
        {
            Text_HealthValue->SetText(FText::FromString(TEXT("0/0")));
        }

        if (ProgressBar_Health)
        {
            ProgressBar_Health->SetPercent(0.0f);
        }
        
        ClearCache();
    }

    UE_LOG(LogTemp, Log, TEXT("NativeConstruct 完成"));
}

void UXBLeaderHealthWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    /*// ✨ 新增 - 如果将领死亡，停止更新
    if (OwningLeader.IsValid() && OwningLeader->IsDead())
    {
        return;
    }*/
    

    if (bAutoUpdate && OwningLeader.IsValid())
    {
        UpdateTimer += InDeltaTime;

        if (UpdateTimer >= UpdateInterval)
        {
            UpdateTimer = 0.0f;
            UpdateHealthFromLeader(false);
        }
    }
}

void UXBLeaderHealthWidget::SetOwningLeader(AXBCharacterBase* InLeader)
{
    OwningLeader = InLeader;
    ClearCache();

    if (InLeader)
    {
        UE_LOG(LogTemp, Log, TEXT("SetOwningLeader: 关联将领 %s, CharacterName: %s"), 
            *InLeader->GetName(),
            InLeader->CharacterName.IsEmpty() ? TEXT("[空]") : *InLeader->CharacterName);
        
        // 🔧 修改 - 只有在 Widget 已经构建完成时才刷新
        // 如果 Widget 还没构建，NativeConstruct 会负责刷新
        if (IsConstructed())
        {
            ForceRefreshDisplay();
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SetOwningLeader: 将领为空"));
    }
}

void UXBLeaderHealthWidget::ClearCache()
{
    CachedCurrentHealth = -1.0f;
    CachedMaxHealth = -1.0f;
    CachedLeaderName.Empty();
    UpdateTimer = 0.0f;
}

void UXBLeaderHealthWidget::RefreshDisplay()
{
    if (!OwningLeader.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("RefreshDisplay: OwningLeader 无效"));
        return;
    }
    /*// ✨ 新增 - 如果将领死亡，不刷新
    if (OwningLeader->IsDead())
    {
        UE_LOG(LogTemp, Log, TEXT("RefreshDisplay: 将领已死亡，跳过刷新"));
        return;
    }*/
    UpdateNameFromLeader();
    UpdateHealthFromLeader(false);
}

void UXBLeaderHealthWidget::ForceRefreshDisplay()
{
    /*if (!OwningLeader.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("ForceRefreshDisplay: OwningLeader 无效"));
        return;
    }*/

    // ✨ 新增 - 如果将领死亡，不刷新
    if (OwningLeader->IsDead())
    {
        UE_LOG(LogTemp, Log, TEXT("ForceRefreshDisplay: 将领已死亡，跳过刷新"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("ForceRefreshDisplay: 开始强制刷新"));

    CachedLeaderName.Empty();
    UpdateNameFromLeader();
    UpdateHealthFromLeader(true);
    
    UE_LOG(LogTemp, Log, TEXT("ForceRefreshDisplay: 强制刷新完成"));
}

void UXBLeaderHealthWidget::UpdateNameFromLeader()
{
    if (!OwningLeader.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("UpdateNameFromLeader: OwningLeader 无效"));
        return;
    }

    AXBCharacterBase* Leader = OwningLeader.Get();
    if (!Leader)
    {
        UE_LOG(LogTemp, Warning, TEXT("UpdateNameFromLeader: Leader 指针为空"));
        return;
    }

    // 获取名称
    FString NameToDisplay;
    
    if (!Leader->CharacterName.IsEmpty())
    {
        NameToDisplay = Leader->CharacterName;
        UE_LOG(LogTemp, Log, TEXT("UpdateNameFromLeader: 使用 CharacterName: %s"), *NameToDisplay);
    }
    else
    {
        // 后备：使用 Actor 名称
        FString ActorName = Leader->GetName();
        
        // 移除蓝图生成的后缀
        int32 UnderscoreIndex;
        if (ActorName.FindLastChar('_', UnderscoreIndex))
        {
            FString Suffix = ActorName.Mid(UnderscoreIndex + 1);
            if (Suffix.IsNumeric() || Suffix.StartsWith(TEXT("C")))
            {
                ActorName = ActorName.Left(UnderscoreIndex);
            }
        }
        
        NameToDisplay = ActorName;
        UE_LOG(LogTemp, Log, TEXT("UpdateNameFromLeader: 使用 Actor 名称: %s"), *NameToDisplay);
    }

    // 检查名称是否变化
    if (NameToDisplay != CachedLeaderName)
    {
        CachedLeaderName = NameToDisplay;
        UpdateNameDisplay(FText::FromString(NameToDisplay));
    }
}

void UXBLeaderHealthWidget::UpdateHealthFromLeader(bool bForceUpdate)
{
    if (!OwningLeader.IsValid())
    {
        return;
    }

    AXBCharacterBase* Leader = OwningLeader.Get();
    UAbilitySystemComponent* ASC = Leader->GetAbilitySystemComponent();

    if (!ASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("UpdateHealthFromLeader: ASC 无效"));
        return;
    }

    float CurrentHealth = ASC->GetNumericAttribute(UXBAttributeSet::GetHealthAttribute());
    float MaxHealth = ASC->GetNumericAttribute(UXBAttributeSet::GetMaxHealthAttribute());

    bool bNeedsUpdate = bForceUpdate;
    
    if (!bNeedsUpdate)
    {
        bNeedsUpdate = !FMath::IsNearlyEqual(CurrentHealth, CachedCurrentHealth, 0.1f) ||
                       !FMath::IsNearlyEqual(MaxHealth, CachedMaxHealth, 0.1f);
    }

    if (bNeedsUpdate)
    {
        CachedCurrentHealth = CurrentHealth;
        CachedMaxHealth = MaxHealth;
        UpdateHealthDisplay(CurrentHealth, MaxHealth);
    }
}

void UXBLeaderHealthWidget::UpdateNameDisplay(const FText& LeaderName)
{
    if (Text_LeaderName)
    {
        Text_LeaderName->SetText(LeaderName);
        UE_LOG(LogTemp, Log, TEXT("UpdateNameDisplay: 设置名称为 '%s'"), *LeaderName.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateNameDisplay: Text_LeaderName 控件为空!"));
    }
}

void UXBLeaderHealthWidget::UpdateHealthDisplay(float CurrentHealth, float MaxHealth)
{
    if (Text_HealthValue)
    {
        FString HealthText = FormatHealthDisplay(CurrentHealth, MaxHealth);
        Text_HealthValue->SetText(FText::FromString(HealthText));
    }

    if (ProgressBar_Health)
    {
        float Percent = (MaxHealth > 0.0f) ? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f) : 0.0f;
        ProgressBar_Health->SetPercent(Percent);
    }
}

FString UXBLeaderHealthWidget::FormatHealthValue(float HealthValue)
{
    HealthValue = FMath::Max(0.0f, HealthValue);

    if (HealthValue < 1000.0f)
    {
        return FString::Printf(TEXT("%d"), FMath::RoundToInt(HealthValue));
    }

    float ValueInK = HealthValue / 1000.0f;
    float RoundedValue = FMath::RoundToFloat(ValueInK * 10.0f) / 10.0f;
    float FractionalPart = FMath::Frac(RoundedValue);
    
    if (FMath::IsNearlyZero(FractionalPart, 0.01f))
    {
        return FString::Printf(TEXT("%dK"), FMath::RoundToInt(RoundedValue));
    }
    else
    {
        return FString::Printf(TEXT("%.1fK"), RoundedValue);
    }
}

FString UXBLeaderHealthWidget::FormatHealthDisplay(float CurrentHealth, float MaxHealth)
{
    FString CurrentStr = FormatHealthValue(CurrentHealth);
    FString MaxStr = FormatHealthValue(MaxHealth);

    return FString::Printf(TEXT("%s/%s"), *CurrentStr, *MaxStr);
}
