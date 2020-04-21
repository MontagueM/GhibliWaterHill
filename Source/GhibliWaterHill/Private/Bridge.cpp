// Fill out your copyright notice in the Description page of Project Settings.


#include "Bridge.h"
#include "Lever.h"

// Sets default values
ABridge::ABridge()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ABridge::BeginPlay()
{
	Super::BeginPlay();
	
	//InitRotation = GetActorRotation();
}

// Called every frame
void ABridge::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!ensure(LinkedLever)) { return; }

	UE_LOG(LogTemp, Warning, TEXT("A %f"), LinkedLever->GetLeverRotationPercentage())
	SetActorRotation(InitRotation + FRotator(90, 0, 0) * LinkedLever->GetLeverRotationPercentage());
}

