// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Lever.generated.h"

UCLASS()
class GHIBLIWATERHILL_API ALever : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALever();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	float GetLeverRotationPercentage();

private:
	class UStaticMeshComponent* SetLeverMesh();

	UStaticMeshComponent* RodMesh = nullptr;

	FRotator InitialRodRotation = FRotator(-22.5, -90, 0);
	float RodRotationMaxScale = 22.5;
};
