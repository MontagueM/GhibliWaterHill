// Fill out your copyright notice in the Description page of Project Settings.


#include "Lever.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
ALever::ALever()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ALever::BeginPlay()
{
	Super::BeginPlay();
	SetLeverMesh();
	//InitialRodRotation = RodMesh->GetComponentRotation(); <-- this doesn't work, provides incorrect init
}

// Called every frame
void ALever::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//UE_LOG(LogTemp, Warning, TEXT("A %s %s"), *InitialRodRotation.ToString(), *RodMesh->GetComponentRotation().ToString())
}

UStaticMeshComponent* ALever::SetLeverMesh()
{
	TArray<UStaticMeshComponent*> Meshes;
	GetComponents<UStaticMeshComponent>(Meshes);
	for (UStaticMeshComponent* M : Meshes) { if (M->GetName() == TEXT("Rod")) { RodMesh = M; } }
	if (!ensure(RodMesh)) { return nullptr; }
	return RodMesh;
}


float ALever::GetLeverRotationPercentage()
{
	if (!ensure(RodMesh)) { return 0; }
	float RodRotation = RodMesh->GetComponentRotation().Pitch - InitialRodRotation.Pitch;
	float Scale = RodRotation / RodRotationMaxScale;
	//UE_LOG(LogTemp, Warning, TEXT("Percentage %f"), RodMesh->GetComponentRotation().Pitch)
	if (abs(Scale) < 0.05) { Scale = 0; }
	return FMath::Clamp(abs(Scale), 0.f, 1.f);
}