// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "XBCharacterBase.h"
#include "XBDummyCharacter.generated.h"

UCLASS()
class XIAOBINDATIANXIA_API AXBDummyCharacter : public AXBCharacterBase
{
	GENERATED_BODY()

public:

	AXBDummyCharacter();

protected:

	virtual void BeginPlay() override;

public:

	virtual void Tick(float DeltaTime) override;

	/**
	 * @brief  初始化主将数据（假人）
	 * @return 无
	 * @note   详细流程分析: 使用父类通用初始化，默认从 Actor 内部与数据表读取
	 */
	virtual void InitializeLeaderData() override;


};
