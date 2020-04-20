// Fill out your copyright notice in the Description page of Project Settings.


#include "Door.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h" 

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

void ADoor::SetLockedState(bool Locked)
{
	TArray<UPhysicsConstraintComponent*> Constraints;
	GetComponents<UPhysicsConstraintComponent>(Constraints);
	for (UPhysicsConstraintComponent* Constraint : Constraints)
	{
		if (!ensure(Constraint)) { break; }
		if (Locked) { Constraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 90); }
		else { Constraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 90); }
	}
}

