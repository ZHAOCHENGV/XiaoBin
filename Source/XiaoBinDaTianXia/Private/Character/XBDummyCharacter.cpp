// Fill out your copyright notice in the Description page of Project Settings.


#include "Public/Character/XBDummyCharacter.h"





AXBDummyCharacter::AXBDummyCharacter()
{
	
	PrimaryActorTick.bCanEverTick = true;


	
}


void AXBDummyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}


void AXBDummyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


