// Fill out your copyright notice in the Description page of Project Settings.


#include "Building/XBBuildingBase.h"


// Sets default values
AXBBuildingBase::AXBBuildingBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AXBBuildingBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AXBBuildingBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

