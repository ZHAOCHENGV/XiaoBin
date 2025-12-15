// Source/XiaoBinDaTianXia/Public/Core/XBCollisionChannels.h

/**
 * @file XBCollisionChannels.h
 * @brief 项目自定义碰撞通道定义
 * 
 * @note 🔧 修改 - 修正通道编号以匹配编辑器配置
 * 
 * @note 编辑器中 Object Channels 顺序：
 *       1. SoftCollision  -> ECC_GameTraceChannel1
 *       2. 障碍物         -> ECC_GameTraceChannel2
 *       3. Soldier        -> ECC_GameTraceChannel3
 *       4. Leader         -> ECC_GameTraceChannel4
 */

#pragma once

#include "Engine/EngineTypes.h"

namespace XBCollision
{
    /** 
     * @brief 士兵碰撞通道
     * @note 🔧 修改 - 对应编辑器中第3个自定义通道
     */
    constexpr ECollisionChannel Soldier = ECC_GameTraceChannel3;

    /** 
     * @brief 将领碰撞通道
     * @note 🔧 修改 - 对应编辑器中第4个自定义通道
     */
    constexpr ECollisionChannel Leader = ECC_GameTraceChannel4;

    /** 
     * @brief 将领碰撞预设名称
     */
    const FName LeaderPresetName = FName("LeaderPawn");

    /** 
     * @brief 士兵碰撞预设名称
     */
    const FName SoldierPresetName = FName("SoldierPawn");
    
    // ✨ 新增 - 其他已存在的通道（备用）
    /** @brief 软碰撞通道 */
    constexpr ECollisionChannel SoftCollision = ECC_GameTraceChannel1;
    
    /** @brief 障碍物通道 */
    constexpr ECollisionChannel Obstacle = ECC_GameTraceChannel2;
}
