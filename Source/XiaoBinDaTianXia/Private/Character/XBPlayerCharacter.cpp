/* --- 完整文件代码 --- */
// Source/XiaoBinDaTianXia/Private/Character/XBPlayerCharacter.cpp

/**
 * @file XBPlayerCharacter.cpp
 * @brief 玩家角色类实现 - 仅包含镜头控制
 */

#include "Character/XBPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Character/Components/XBFormationComponent.h"
#include "Character/Components/XBMagnetFieldComponent.h"
#include "Game/XBGameInstance.h"
#include "GameFramework/SpringArmComponent.h"


// ✨ 新增 - 统一主将显示名称解析，避免 UI 空值导致名称丢失
/**
 * @brief  解析主将显示名称
 * @param  GameConfig 运行时配置数据
 * @param  LeaderData 主将缓存数据
 * @param  ConfigRowName 主将配置行名
 * @return 解析后的主将显示名称
 * @note   详细流程分析: 1) 优先使用 UI 配置的自定义名称 2)
 * 无效则回退到数据表主将名称 3) 再回退到行名字符串
 */
static FString ResolveLeaderDisplayName(const FXBGameConfigData &GameConfig,
                                        const FXBLeaderTableRow &LeaderData,
                                        FName ConfigRowName) {
  // 🔧 修改 - 先过滤空格，避免 UI 仅输入空白导致名称无效
  const FString TrimmedDisplayName =
      GameConfig.LeaderDisplayName.TrimStartAndEnd();
  if (!TrimmedDisplayName.IsEmpty()) {
    return TrimmedDisplayName;
  }

  // 🔧 修改 - UI 名称无效时回退到数据表主将名称，保证展示稳定
  if (!LeaderData.LeaderName.IsEmpty()) {
    return LeaderData.LeaderName.ToString();
  }

  // 🔧 修改 - 若数据表名称也为空，则使用行名作为兜底
  if (!ConfigRowName.IsNone()) {
    return ConfigRowName.ToString();
  }

  return FString();
}

AXBPlayerCharacter::AXBPlayerCharacter() {

  // ========== 弹簧臂配置 ==========
  SpringArmComponent =
      CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
  SpringArmComponent->SetupAttachment(RootComponent);
  SpringArmComponent->TargetArmLength = 1200.0f;
  SpringArmComponent->SetRelativeRotation(FRotator(-50.0f, 0.0f, 0.0f));
  SpringArmComponent->bUsePawnControlRotation = false;
  SpringArmComponent->bInheritPitch = false;
  SpringArmComponent->bInheritYaw = false;
  SpringArmComponent->bInheritRoll = false;
  SpringArmComponent->bEnableCameraLag = true;
  SpringArmComponent->bEnableCameraRotationLag = true;
  SpringArmComponent->CameraLagSpeed = 10.0f;
  SpringArmComponent->CameraRotationLagSpeed = 10.0f;
  SpringArmComponent->bDoCollisionTest = true;
  SpringArmComponent->ProbeSize = 12.0f;
  SpringArmComponent->ProbeChannel = ECC_Camera;

  // ========== 摄像机配置 ==========
  CameraComponent =
      CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
  CameraComponent->SetupAttachment(SpringArmComponent,
                                   USpringArmComponent::SocketName);
  CameraComponent->bUsePawnControlRotation = false;

  // ========== 默认阵营 ==========
  Faction = EXBFaction::Player;
}

/**
 * @brief  玩家主将初始化入口
 * @return 无
 * @note   详细流程分析: 先走父类通用初始化 -> 然后处理镜头初始化
 */
void AXBPlayerCharacter::BeginPlay() {
  if (UXBGameInstance *GameInstance = GetGameInstance<UXBGameInstance>()) {
    // 🔧 修改 - 优先根据配置行名初始化数据表
    ApplyConfigFromGameConfig(GameInstance->GetGameConfig(), true);
  }

  Super::BeginPlay();

  if (UXBGameInstance *GameInstance = GetGameInstance<UXBGameInstance>()) {
    const FXBGameConfigData GameConfig = GameInstance->GetGameConfig();

    const FString ResolvedLeaderName =
        ResolveLeaderDisplayName(GameConfig, CachedLeaderData, ConfigRowName);
    if (!ResolvedLeaderName.IsEmpty()) {
      // 🔧 修改 - UI 名称无效时回退到行名对应数据，确保主将名称稳定
      CharacterName = ResolvedLeaderName;
      CachedLeaderData.LeaderName = FText::FromString(CharacterName);
    }

    // 🔧 修改 - 以配置初始化主将基础倍率与成长参数
    CachedLeaderData.HealthMultiplier = GameConfig.LeaderHealthMultiplier;
    CachedLeaderData.DamageMultiplier = GameConfig.LeaderDamageMultiplier;
    const float InitialScale = FMath::Max(0.01f, GameConfig.LeaderInitialScale);
    GrowthConfigCache.MaxScale =
        FMath::Max(InitialScale, GameConfig.LeaderMaxScale);
    GrowthConfigCache.DamageMultiplierPerSoldier =
        GameConfig.LeaderDamageMultiplierPerSoldier;

    // 🔧 修改 - 以配置初始化主将基础大小，确保出生尺寸一致
    BaseScale = InitialScale;
    CachedLeaderData.Scale = InitialScale;

    // 🔧 修改 - 统一应用运行时配置（倍率/掉落/招募/磁场）
    ApplyRuntimeConfig(GameConfig, true);
  }

  // 初始化镜头
  if (SpringArmComponent) {
    SpringArmComponent->TargetArmLength = 1200.0f;
    SpringArmComponent->SetRelativeRotation(
        FRotator(DefaultCameraPitch, 0.0f, 0.0f));
  }

  // ✨ 新增 - 玩家主将在配置应用完成后开启磁场
  if (MagnetFieldComponent) {
    MagnetFieldComponent->SetFieldEnabled(true);
  }
}

/**
 * @brief  初始化主将数据（玩家）
 * @return 无
 * @note   详细流程分析: 调用通用初始化 -> 若存在外部配置则执行配置初始化
 */
void AXBPlayerCharacter::InitializeLeaderData() {
  FXBGameConfigData ExternalConfig;
  if (GetExternalInitConfig(ExternalConfig)) {
    // 🔧 修改 - 玩家主将优先使用配置界面数据初始化
    ApplyConfigFromGameConfig(ExternalConfig, true);
    return;
  }

  Super::InitializeLeaderData();
}

/**
 * @brief  获取外部初始化配置（玩家从配置界面）
 * @param  OutConfig 输出配置
 * @return 是否存在外部配置
 * @note   详细流程分析: 从 GameInstance 获取配置
 */
bool AXBPlayerCharacter::GetExternalInitConfig(
    FXBGameConfigData &OutConfig) const {
  if (const UXBGameInstance *GameInstance =
          GetGameInstance<UXBGameInstance>()) {
    OutConfig = GameInstance->GetGameConfig();
    return true;
  }

  return false;
}

/**
 * @brief  从配置数据初始化玩家主将
 * @param  GameConfig 配置数据
 * @param  bApplyInitialSoldiers 是否生成初始士兵
 * @return 无
 * @note   详细流程分析: 行名/名称/倍率/成长参数 -> 缩放 -> 属性刷新 ->
 * 运行时配置
 */
void AXBPlayerCharacter::ApplyConfigFromGameConfig(
    const FXBGameConfigData &GameConfig, bool bApplyInitialSoldiers) {
  // 🔧 修改 - 仅玩家主将应用配置行名
  if (!GameConfig.LeaderConfigRowName.IsNone()) {
    ConfigRowName = GameConfig.LeaderConfigRowName;
  }

  if (ConfigDataTable && !ConfigRowName.IsNone()) {
    InitializeFromDataTable(ConfigDataTable, ConfigRowName);
  }

  const FString ResolvedLeaderName =
      ResolveLeaderDisplayName(GameConfig, CachedLeaderData, ConfigRowName);
  if (!ResolvedLeaderName.IsEmpty()) {
    // 🔧 修改 - UI 名称无效时回退到行名对应数据，确保主将名称稳定
    CharacterName = ResolvedLeaderName;
    CachedLeaderData.LeaderName = FText::FromString(CharacterName);
  }

  // 🔧 修改 - 以配置初始化主将基础倍率与成长参数
  CachedLeaderData.HealthMultiplier = GameConfig.LeaderHealthMultiplier;
  CachedLeaderData.DamageMultiplier = GameConfig.LeaderDamageMultiplier;
  const float InitialScale = FMath::Max(0.01f, GameConfig.LeaderInitialScale);
  GrowthConfigCache.MaxScale =
      FMath::Max(InitialScale, GameConfig.LeaderMaxScale);
  GrowthConfigCache.DamageMultiplierPerSoldier =
      GameConfig.LeaderDamageMultiplierPerSoldier;

  // 🔧 修改 - 以配置初始化主将基础大小，确保出生尺寸一致
  BaseScale = InitialScale;
  CachedLeaderData.Scale = InitialScale;

  // 🔧 修改 - 重新应用初始属性，确保倍率写入属性集
  ApplyInitialAttributes();
  UpdateLeaderScale();
  UpdateDamageMultiplier();

  // 🔧 修改 - 统一应用运行时配置（倍率/掉落/招募/磁场）
  ApplyRuntimeConfig(GameConfig, bApplyInitialSoldiers);
}

void AXBPlayerCharacter::SetupPlayerInputComponent(
    UInputComponent *PlayerInputComponent) {
  Super::SetupPlayerInputComponent(PlayerInputComponent);
  // 输入绑定在 PlayerController 中完成
}

// ==================== 镜头控制实现 ====================

void AXBPlayerCharacter::SetCameraDistance(float NewDistance) {
  if (SpringArmComponent) {
    SpringArmComponent->TargetArmLength = NewDistance;
  }
}

float AXBPlayerCharacter::GetCameraDistance() const {
  if (SpringArmComponent) {
    return SpringArmComponent->TargetArmLength;
  }
  return 0.0f;
}

void AXBPlayerCharacter::SetCameraYawOffset(float YawOffset) {
  CurrentCameraYawOffset = YawOffset;

  if (SpringArmComponent) {
    FRotator CurrentRotation = SpringArmComponent->GetRelativeRotation();
    CurrentRotation.Yaw = YawOffset;
    SpringArmComponent->SetRelativeRotation(CurrentRotation);
  }
}
