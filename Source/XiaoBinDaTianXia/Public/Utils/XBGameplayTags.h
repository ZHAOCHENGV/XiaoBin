#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * 项目 GameplayTags 集中定义
 * 使用原生 GameplayTags 而非 NativeGameplayTags 宏以保持兼容性
 */
struct XIAOBINDATIANXIA_API FXBGameplayTags
{
public:
    static const FXBGameplayTags& Get() { return GameplayTags; }
    static void InitializeNativeTags();

    // ============ 输入 Tags ============
    FGameplayTag InputTag_Move;
    FGameplayTag InputTag_Look;
    FGameplayTag InputTag_Attack;
    FGameplayTag InputTag_Skill;
    FGameplayTag InputTag_Dash;
    FGameplayTag InputTag_Recall;
    FGameplayTag InputTag_CameraRotateLeft;
    FGameplayTag InputTag_CameraRotateRight;
    FGameplayTag InputTag_CameraReset;
    FGameplayTag InputTag_CameraZoom;

    // ============ 状态 Tags ============
    FGameplayTag State_Combat;
    FGameplayTag State_Dead;
    FGameplayTag State_Hidden;
    FGameplayTag State_InBuilding;
    FGameplayTag State_Dashing;

    // ============ 技能 Tags ============
    FGameplayTag Ability_Attack;
    FGameplayTag Ability_Skill;
    FGameplayTag Ability_Dash;
    
    // 将领专属技能
    FGameplayTag Ability_Skill_Roman;           // 罗马将领 - 圣殿骑士技能
    FGameplayTag Ability_Skill_Genghis;         // 成吉思汗 - 箭雨
    FGameplayTag Ability_Skill_GuanYu;          // 关羽 - 下劈斩
    FGameplayTag Ability_Skill_Templar;         // 圣殿骑士 - 二连斩
    FGameplayTag Ability_Skill_Viking;          // 维京首领 - 旋转攻击
    FGameplayTag Ability_Skill_Persian;         // 波斯战神 - 剑气
    FGameplayTag Ability_Skill_LiShimin;        // 李世民 - 范围爆炸

    // ============ 属性 Tags ============
    FGameplayTag Attribute_Health;
    FGameplayTag Attribute_MaxHealth;
    FGameplayTag Attribute_Damage;
    FGameplayTag Attribute_Scale;
    FGameplayTag Attribute_MoveSpeed;

    // ============ 事件 Tags ============
    FGameplayTag Event_Combat_Enter;
    FGameplayTag Event_Combat_Exit;
    FGameplayTag Event_Soldier_Recruited;
    FGameplayTag Event_Soldier_Died;
    FGameplayTag Event_Leader_Died;
    FGameplayTag Event_Building_Enter;
    FGameplayTag Event_Building_Exit;
    FGameplayTag Event_Hit;
    FGameplayTag Event_Damage;
    FGameplayTag Event_Attack_MeleeHit;

    // ============ 阵营 Tags ============
    FGameplayTag Faction_Neutral;
    FGameplayTag Faction_Player;
    FGameplayTag Faction_Enemy;

    // ============ 兵种 Tags ============
    FGameplayTag SoldierType_Infantry;
    FGameplayTag SoldierType_Cavalry;
    FGameplayTag SoldierType_Archer;

    // ============ Cue Tags ============
    FGameplayTag GameplayCue_Attack;
    FGameplayTag GameplayCue_Skill;
    FGameplayTag GameplayCue_Hit;
    FGameplayTag GameplayCue_Death;
    FGameplayTag GameplayCue_Recruit;

private:
    static FXBGameplayTags GameplayTags;
};
