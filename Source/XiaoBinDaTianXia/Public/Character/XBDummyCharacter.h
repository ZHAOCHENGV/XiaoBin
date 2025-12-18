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


};
