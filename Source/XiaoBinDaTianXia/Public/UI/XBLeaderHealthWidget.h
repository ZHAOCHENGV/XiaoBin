/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Public/UI/XBLeaderHealthWidget.h

/**
 * @file XBLeaderHealthWidget.h
 * @brief 将领血量 UI Widget
 * 
 * @note 🔧 修改 - 添加名称缓存，修复名称不显示问题
 */

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "XBLeaderHealthWidget.generated.h"

class AXBCharacterBase;
class UTextBlock;
class UProgressBar;

/**
 * @brief 将领血量 Widget
 */
UCLASS()
class XIAOBINDATIANXIA_API UXBLeaderHealthWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // ==================== 初始化 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|UI", meta = (DisplayName = "设置关联将领"))
    void SetOwningLeader(AXBCharacterBase* InLeader);

    UFUNCTION(BlueprintCallable, Category = "XB|UI", meta = (DisplayName = "获取关联将领"))
    AXBCharacterBase* GetOwningLeader() const { return OwningLeader.Get(); }

    // ==================== 更新接口 ====================

    UFUNCTION(BlueprintCallable, Category = "XB|UI", meta = (DisplayName = "刷新UI显示"))
    void RefreshDisplay();

    UFUNCTION(BlueprintCallable, Category = "XB|UI", meta = (DisplayName = "强制刷新UI"))
    void ForceRefreshDisplay();

    UFUNCTION(BlueprintCallable, Category = "XB|UI", meta = (DisplayName = "更新血量显示"))
    void UpdateHealthDisplay(float CurrentHealth, float MaxHealth);

    UFUNCTION(BlueprintCallable, Category = "XB|UI", meta = (DisplayName = "更新名称显示"))
    void UpdateNameDisplay(const FText& LeaderName);

    // ==================== 格式化工具 ====================

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "XB|UI", meta = (DisplayName = "格式化血量数值"))
    static FString FormatHealthValue(float HealthValue);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "XB|UI", meta = (DisplayName = "格式化血量显示"))
    static FString FormatHealthDisplay(float CurrentHealth, float MaxHealth);

    UFUNCTION(BlueprintCallable, Category = "XB|UI", meta = (DisplayName = "清除缓存"))
    void ClearCache();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    void UpdateHealthFromLeader(bool bForceUpdate = false);
    void UpdateNameFromLeader();

protected:
    // ==================== UI 控件绑定 ====================
    // 🔧 修改 - 使用 BindWidgetOptional 允许控件不存在

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "XB|UI")
    TObjectPtr<UTextBlock> Text_LeaderName;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "XB|UI")
    TObjectPtr<UTextBlock> Text_HealthValue;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "XB|UI")
    TObjectPtr<UProgressBar> ProgressBar_Health;

    // ==================== 配置 ====================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|UI|配置", meta = (DisplayName = "启用自动更新"))
    bool bAutoUpdate = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XB|UI|配置", meta = (DisplayName = "更新间隔", ClampMin = "0.0"))
    float UpdateInterval = 0.1f;

    // ==================== 运行时数据 ====================

    UPROPERTY(BlueprintReadOnly, Category = "XB|UI")
    TWeakObjectPtr<AXBCharacterBase> OwningLeader;

    UPROPERTY(BlueprintReadOnly, Category = "XB|UI")
    float CachedCurrentHealth = -1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "XB|UI")
    float CachedMaxHealth = -1.0f;

    // ✨ 新增 - 名称缓存
    UPROPERTY(BlueprintReadOnly, Category = "XB|UI")
    FString CachedLeaderName;

private:
    float UpdateTimer = 0.0f;
};
