// Fill out your copyright notice in the Description page of Project Settings.


#include "Public/Character/XBDummyCharacter.h"





AXBDummyCharacter::AXBDummyCharacter()
{
	
	PrimaryActorTick.bCanEverTick = true;


	
}


/**
 * @brief  假人初始化入口
 * @return 无
 * @note   详细流程分析: 复用父类通用初始化逻辑
 */
void AXBDummyCharacter::BeginPlay()
{
	Super::BeginPlay();
}


void AXBDummyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

/**
 * @brief  初始化主将数据（假人）
 * @return 无
 * @note   详细流程分析: 默认从 Actor 内部配置与数据表初始化
 */
void AXBDummyCharacter::InitializeLeaderData()
{
	// 🔧 修改 - 假人仅使用父类通用初始化
	Super::InitializeLeaderData();
}

