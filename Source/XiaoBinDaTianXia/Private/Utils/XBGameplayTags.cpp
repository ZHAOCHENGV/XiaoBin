
#include "Utils/XBGameplayTags.h"
#include "GameplayTagsManager.h"

FXBGameplayTags FXBGameplayTags::GameplayTags;

void FXBGameplayTags::InitializeNativeTags()
{
    UGameplayTagsManager& Manager = UGameplayTagsManager::Get();

    // ============ 输入 Tags ============
    GameplayTags.InputTag_Move = Manager.AddNativeGameplayTag(
        FName("Input.Move"), 
        FString("移动输入"));
    
    GameplayTags.InputTag_Look = Manager.AddNativeGameplayTag(
        FName("Input.Look"), 
        FString("视角输入"));
    
    GameplayTags.InputTag_Attack = Manager.AddNativeGameplayTag(
        FName("Input.Attack"), 
        FString("攻击输入（鼠标左键）"));
    
    GameplayTags.InputTag_Skill = Manager.AddNativeGameplayTag(
        FName("Input.Skill"), 
        FString("技能输入（鼠标右键）"));
    
    GameplayTags.InputTag_Dash = Manager.AddNativeGameplayTag(
        FName("Input.Dash"), 
        FString("冲刺输入（空格键）"));
    
    GameplayTags.InputTag_Recall = Manager.AddNativeGameplayTag(
        FName("Input.Recall"), 
        FString("召回士兵"));
    
    GameplayTags.InputTag_CameraRotateLeft = Manager.AddNativeGameplayTag(
        FName("Input.Camera.RotateLeft"), 
        FString("镜头左旋（Q键）"));
    
    GameplayTags.InputTag_CameraRotateRight = Manager.AddNativeGameplayTag(
        FName("Input.Camera.RotateRight"), 
        FString("镜头右旋（E键）"));
    
    GameplayTags.InputTag_CameraReset = Manager.AddNativeGameplayTag(
        FName("Input.Camera.Reset"), 
        FString("镜头重置（滚轮按下）"));
    
    GameplayTags.InputTag_CameraZoom = Manager.AddNativeGameplayTag(
        FName("Input.Camera.Zoom"), 
        FString("镜头缩放（滚轮）"));

    // ============ 状态 Tags ============
    GameplayTags.State_Combat = Manager.AddNativeGameplayTag(
        FName("State.Combat"), 
        FString("战斗状态"));
    
    GameplayTags.State_Dead = Manager.AddNativeGameplayTag(
        FName("State.Dead"), 
        FString("死亡状态"));
    
    GameplayTags.State_Hidden = Manager.AddNativeGameplayTag(
        FName("State.Hidden"), 
        FString("隐身状态（草丛中）"));
    
    GameplayTags.State_InBuilding = Manager.AddNativeGameplayTag(
        FName("State.InBuilding"), 
        FString("在建筑内"));
    
    GameplayTags.State_Dashing = Manager.AddNativeGameplayTag(
        FName("State.Dashing"), 
        FString("冲刺中"));

    // ============ 技能 Tags ============
    GameplayTags.Ability_Attack = Manager.AddNativeGameplayTag(
        FName("Ability.Attack"), 
        FString("普通攻击"));
    
    GameplayTags.Ability_Skill = Manager.AddNativeGameplayTag(
        FName("Ability.Skill"), 
        FString("技能"));
    
    GameplayTags.Ability_Dash = Manager.AddNativeGameplayTag(
        FName("Ability.Dash"), 
        FString("冲刺"));

    // 将领专属技能
    GameplayTags.Ability_Skill_Roman = Manager.AddNativeGameplayTag(
        FName("Ability.Skill.Roman"), 
        FString("罗马将领技能"));
    
    GameplayTags.Ability_Skill_Genghis = Manager.AddNativeGameplayTag(
        FName("Ability.Skill.Genghis"), 
        FString("成吉思汗 - 箭雨"));
    
    GameplayTags.Ability_Skill_GuanYu = Manager.AddNativeGameplayTag(
        FName("Ability.Skill.GuanYu"), 
        FString("关羽 - 下劈斩"));
    
    GameplayTags.Ability_Skill_Templar = Manager.AddNativeGameplayTag(
        FName("Ability.Skill.Templar"), 
        FString("圣殿骑士 - 二连斩"));
    
    GameplayTags.Ability_Skill_Viking = Manager.AddNativeGameplayTag(
        FName("Ability.Skill.Viking"), 
        FString("维京首领 - 旋转攻击"));
    
    GameplayTags.Ability_Skill_Persian = Manager.AddNativeGameplayTag(
        FName("Ability.Skill.Persian"), 
        FString("波斯战神 - 剑气"));
    
    GameplayTags.Ability_Skill_LiShimin = Manager.AddNativeGameplayTag(
        FName("Ability.Skill.LiShimin"), 
        FString("李世民 - 范围爆炸"));

    // ============ 属性 Tags ============
    GameplayTags.Attribute_Health = Manager.AddNativeGameplayTag(
        FName("Attribute.Health"), 
        FString("当前血量"));
    
    GameplayTags.Attribute_MaxHealth = Manager.AddNativeGameplayTag(
        FName("Attribute.MaxHealth"), 
        FString("最大血量"));
    
    GameplayTags.Attribute_Damage = Manager.AddNativeGameplayTag(
        FName("Attribute.Damage"), 
        FString("伤害值"));
    
    GameplayTags.Attribute_Scale = Manager.AddNativeGameplayTag(
        FName("Attribute.Scale"), 
        FString("缩放比例"));
    
    GameplayTags.Attribute_MoveSpeed = Manager.AddNativeGameplayTag(
        FName("Attribute.MoveSpeed"), 
        FString("移动速度"));

    // ============ 事件 Tags ============
    GameplayTags.Event_Combat_Enter = Manager.AddNativeGameplayTag(
        FName("Event.Combat.Enter"), 
        FString("进入战斗事件"));
    
    GameplayTags.Event_Combat_Exit = Manager.AddNativeGameplayTag(
        FName("Event.Combat.Exit"), 
        FString("退出战斗事件"));
    
    GameplayTags.Event_Soldier_Recruited = Manager.AddNativeGameplayTag(
        FName("Event.Soldier.Recruited"), 
        FString("士兵招募事件"));
    
    GameplayTags.Event_Soldier_Died = Manager.AddNativeGameplayTag(
        FName("Event.Soldier.Died"), 
        FString("士兵死亡事件"));
    
    GameplayTags.Event_Leader_Died = Manager.AddNativeGameplayTag(
        FName("Event.Leader.Died"), 
        FString("将领死亡事件"));
    
    GameplayTags.Event_Building_Enter = Manager.AddNativeGameplayTag(
        FName("Event.Building.Enter"), 
        FString("进入建筑事件"));
    
    GameplayTags.Event_Building_Exit = Manager.AddNativeGameplayTag(
        FName("Event.Building.Exit"), 
        FString("离开建筑事件"));
    
    GameplayTags.Event_Hit = Manager.AddNativeGameplayTag(
        FName("Event.Hit"), 
        FString("命中事件"));
    
    GameplayTags.Event_Damage = Manager.AddNativeGameplayTag(
        FName("Event.Damage"), 
        FString("受伤事件"));

    GameplayTags.Event_Attack_MeleeHit = Manager.AddNativeGameplayTag(
        FName("Event.Attack.MeleeHit"),
        FString("近战命中事件"));

    // ============ 阵营 Tags ============
    GameplayTags.Faction_Neutral = Manager.AddNativeGameplayTag(
        FName("Faction.Neutral"), 
        FString("中立阵营"));
    
    GameplayTags.Faction_Player = Manager.AddNativeGameplayTag(
        FName("Faction.Player"), 
        FString("玩家阵营"));
    
    GameplayTags.Faction_Enemy = Manager.AddNativeGameplayTag(
        FName("Faction.Enemy"), 
        FString("敌方阵营"));

    // ============ 兵种 Tags ============
    GameplayTags.SoldierType_Infantry = Manager.AddNativeGameplayTag(
        FName("SoldierType.Infantry"), 
        FString("步兵"));
    
    GameplayTags.SoldierType_Cavalry = Manager.AddNativeGameplayTag(
        FName("SoldierType.Cavalry"), 
        FString("骑兵"));
    
    GameplayTags.SoldierType_Archer = Manager.AddNativeGameplayTag(
        FName("SoldierType.Archer"), 
        FString("弓手"));

    // ============ Cue Tags ============
    GameplayTags.GameplayCue_Attack = Manager.AddNativeGameplayTag(
        FName("GameplayCue.Attack"), 
        FString("攻击特效"));
    
    GameplayTags.GameplayCue_Skill = Manager.AddNativeGameplayTag(
        FName("GameplayCue.Skill"), 
        FString("技能特效"));
    
    GameplayTags.GameplayCue_Hit = Manager.AddNativeGameplayTag(
        FName("GameplayCue.Hit"), 
        FString("受击特效"));
    
    GameplayTags.GameplayCue_Death = Manager.AddNativeGameplayTag(
        FName("GameplayCue.Death"), 
        FString("死亡特效"));
    
    GameplayTags.GameplayCue_Recruit = Manager.AddNativeGameplayTag(
        FName("GameplayCue.Recruit"), 
        FString("招募特效"));
}
