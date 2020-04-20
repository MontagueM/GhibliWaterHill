// Fill out your copyright notice in the Description page of Project Settings.


#include "Door.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
ADoor::ADoor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ADoor::BeginPlay()
{
	Super::BeginPlay();
	SetDoorMesh();
}

// Called every frame
void ADoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

UStaticMeshComponent* ADoor::SetDoorMesh()
{
	UE_LOG(LogTemp, Warning, TEXT("Setting door mesh"))
	DoorMesh = FindComponentByClass<UStaticMeshComponent>();
	if (!ensure(DoorMesh)) { return nullptr; }
	UE_LOG(LogTemp, Warning, TEXT("Door %s"), *DoorMesh->GetName())
	return DoorMesh;
}

void ADoor::LockDoor()
{
	UE_LOG(LogTemp, Warning, TEXT("Trying to lock"))
	if (!ensure(DoorMesh->GetBodyInstance())) { return; }
	DoorMesh->BodyInstance.bLockXRotation = true;
	DoorMesh->BodyInstance.bLockYRotation = true;
	DoorMesh->BodyInstance.bLockZRotation = true;
	UE_LOG(LogTemp, Warning, TEXT("Locked door"))
}

void ADoor::UnlockDoor()
{
	UE_LOG(LogTemp, Warning, TEXT("Trying to unlock door"))
	if (!ensure(DoorMesh)) { return; }
	DoorMesh->BodyInstance.bLockRotation = false;
	UE_LOG(LogTemp, Warning, TEXT("Unlocked door"))
}

