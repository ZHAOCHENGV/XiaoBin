
#include "Utils/XBGameplayTags.h"

FXBGameplayTags FXBGameplayTags::GameplayTags;

// ✨ 新增 - 使用 NativeGameplayTags 宏注册
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Input_Move, "Input.Move", "移动输入");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Input_Look, "Input.Look", "视角输入");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Input_Attack, "Input.Attack", "攻击输入（鼠标左键）");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Input_Skill, "Input.Skill", "技能输入（鼠标右键）");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Input_Dash, "Input.Dash", "冲刺输入（空格键）");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Input_Recall, "Input.Recall", "召回士兵");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Input_CameraRotateLeft, "Input.Camera.RotateLeft", "镜头左旋（Q键）");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Input_CameraRotateRight, "Input.Camera.RotateRight", "镜头右旋（E键）");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Input_CameraReset, "Input.Camera.Reset", "镜头重置（滚轮按下）");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Input_CameraZoom, "Input.Camera.Zoom", "镜头缩放（滚轮）");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Input_SpawnLeader, "Input.SpawnLeader", "生成主将（回车键）");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Combat, "State.Combat", "战斗状态");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Dead, "State.Dead", "死亡状态");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Hidden, "State.Hidden", "隐身状态（草丛中）");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_InBuilding, "State.InBuilding", "在建筑内");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Dashing, "State.Dashing", "冲刺中");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_Attack, "Ability.Attack", "普通攻击");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_Skill, "Ability.Skill", "技能");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_Dash, "Ability.Dash", "冲刺");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_Skill_Roman, "Ability.Skill.Roman", "罗马将领技能");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_Skill_Genghis, "Ability.Skill.Genghis", "成吉思汗 - 箭雨");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_Skill_GuanYu, "Ability.Skill.GuanYu", "关羽 - 下劈斩");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_Skill_Templar, "Ability.Skill.Templar", "圣殿骑士 - 二连斩");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_Skill_Viking, "Ability.Skill.Viking", "维京首领 - 旋转攻击");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_Skill_Persian, "Ability.Skill.Persian", "波斯战神 - 剑气");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_Skill_LiShimin, "Ability.Skill.LiShimin", "李世民 - 范围爆炸");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Attribute_Health, "Attribute.Health", "当前血量");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Attribute_MaxHealth, "Attribute.MaxHealth", "最大血量");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Attribute_Damage, "Attribute.Damage", "伤害值");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Attribute_Scale, "Attribute.Scale", "缩放比例");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Attribute_MoveSpeed, "Attribute.MoveSpeed", "移动速度");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Combat_Enter, "Event.Combat.Enter", "进入战斗事件");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Combat_Exit, "Event.Combat.Exit", "退出战斗事件");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Soldier_Recruited, "Event.Soldier.Recruited", "士兵招募事件");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Soldier_Died, "Event.Soldier.Died", "士兵死亡事件");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Leader_Died, "Event.Leader.Died", "将领死亡事件");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Building_Enter, "Event.Building.Enter", "进入建筑事件");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Building_Exit, "Event.Building.Exit", "离开建筑事件");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Hit, "Event.Hit", "命中事件");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Damage, "Event.Damage", "受伤事件");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Attack_MeleeHit, "Event.Attack.MeleeHit", "近战命中事件");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Faction_Neutral, "Faction.Neutral", "中立阵营");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Faction_Player, "Faction.Player", "玩家阵营");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Faction_Enemy, "Faction.Enemy", "敌方阵营");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_SoldierType_Infantry, "SoldierType.Infantry", "步兵");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_SoldierType_Cavalry, "SoldierType.Cavalry", "骑兵");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_SoldierType_Archer, "SoldierType.Archer", "弓手");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Attack, "GameplayCue.Attack", "攻击特效");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Skill, "GameplayCue.Skill", "技能特效");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Hit, "GameplayCue.Hit", "受击特效");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Death, "GameplayCue.Death", "死亡特效");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Recruit, "GameplayCue.Recruit", "招募特效");

FXBGameplayTags::FXBGameplayTags()
{
    // 🔧 修改 - 统一从 NativeGameplayTags 变量填充结构体字段
    InputTag_Move = TAG_Input_Move;
    InputTag_Look = TAG_Input_Look;
    InputTag_Attack = TAG_Input_Attack;
    InputTag_Skill = TAG_Input_Skill;
    InputTag_Dash = TAG_Input_Dash;
    InputTag_Recall = TAG_Input_Recall;
    InputTag_CameraRotateLeft = TAG_Input_CameraRotateLeft;
    InputTag_CameraRotateRight = TAG_Input_CameraRotateRight;
    InputTag_CameraReset = TAG_Input_CameraReset;
    InputTag_CameraZoom = TAG_Input_CameraZoom;
    InputTag_SpawnLeader = TAG_Input_SpawnLeader;

    State_Combat = TAG_State_Combat;
    State_Dead = TAG_State_Dead;
    State_Hidden = TAG_State_Hidden;
    State_InBuilding = TAG_State_InBuilding;
    State_Dashing = TAG_State_Dashing;

    Ability_Attack = TAG_Ability_Attack;
    Ability_Skill = TAG_Ability_Skill;
    Ability_Dash = TAG_Ability_Dash;
    Ability_Skill_Roman = TAG_Ability_Skill_Roman;
    Ability_Skill_Genghis = TAG_Ability_Skill_Genghis;
    Ability_Skill_GuanYu = TAG_Ability_Skill_GuanYu;
    Ability_Skill_Templar = TAG_Ability_Skill_Templar;
    Ability_Skill_Viking = TAG_Ability_Skill_Viking;
    Ability_Skill_Persian = TAG_Ability_Skill_Persian;
    Ability_Skill_LiShimin = TAG_Ability_Skill_LiShimin;

    Attribute_Health = TAG_Attribute_Health;
    Attribute_MaxHealth = TAG_Attribute_MaxHealth;
    Attribute_Damage = TAG_Attribute_Damage;
    Attribute_Scale = TAG_Attribute_Scale;
    Attribute_MoveSpeed = TAG_Attribute_MoveSpeed;

    Event_Combat_Enter = TAG_Event_Combat_Enter;
    Event_Combat_Exit = TAG_Event_Combat_Exit;
    Event_Soldier_Recruited = TAG_Event_Soldier_Recruited;
    Event_Soldier_Died = TAG_Event_Soldier_Died;
    Event_Leader_Died = TAG_Event_Leader_Died;
    Event_Building_Enter = TAG_Event_Building_Enter;
    Event_Building_Exit = TAG_Event_Building_Exit;
    Event_Hit = TAG_Event_Hit;
    Event_Damage = TAG_Event_Damage;
    Event_Attack_MeleeHit = TAG_Event_Attack_MeleeHit;

    Faction_Neutral = TAG_Faction_Neutral;
    Faction_Player = TAG_Faction_Player;
    Faction_Enemy = TAG_Faction_Enemy;

    SoldierType_Infantry = TAG_SoldierType_Infantry;
    SoldierType_Cavalry = TAG_SoldierType_Cavalry;
    SoldierType_Archer = TAG_SoldierType_Archer;

    GameplayCue_Attack = TAG_GameplayCue_Attack;
    GameplayCue_Skill = TAG_GameplayCue_Skill;
    GameplayCue_Hit = TAG_GameplayCue_Hit;
    GameplayCue_Death = TAG_GameplayCue_Death;
    GameplayCue_Recruit = TAG_GameplayCue_Recruit;
}
