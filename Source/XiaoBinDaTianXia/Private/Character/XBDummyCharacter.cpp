// Fill out your copyright notice in the Description page of Project Settings.


#include "Public/Character/XBDummyCharacter.h"


// Sets default values
AXBDummyCharacter::AXBDummyCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AXBDummyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AXBDummyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AXBDummyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

