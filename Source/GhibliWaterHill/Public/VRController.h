// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VRController.generated.h"

UCLASS()
class GHIBLIWATERHILL_API AVRController : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AVRController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere)
		EControllerHand Hand;
	UPROPERTY(VisibleAnywhere)
		class UMotionControllerComponent* MotionController;

	UFUNCTION(BlueprintCallable)
		EControllerHand GetHand();

	void SetHand(EControllerHand Hand);

private:
	UPROPERTY()
		class USplineMeshComponent* SplineMesh = nullptr;
	UPROPERTY(VisibleAnywhere)
		class UStaticMeshComponent* DestinationMarker = nullptr;
	UPROPERTY(VisibleAnywhere)
		class USplineComponent* TeleportPath = nullptr;

	UPROPERTY(EditDefaultsOnly)
		float TeleportProjectileSpeed = 800;
	UPROPERTY(EditDefaultsOnly)
		float TeleportProjectileRadius = 10;
	UPROPERTY(EditDefaultsOnly)
		float TeleportSimulationTime = 5;
	UPROPERTY(EditDefaultsOnly)
		FVector TeleportNavExtent = FVector(100, 100, 100);
	UPROPERTY(EditDefaultsOnly)
		class UStaticMesh* TeleportArcMesh;
	UPROPERTY(EditDefaultsOnly)
		class UMaterialInterface* TeleportArcMaterial;
	UPROPERTY() // need this for proper garbage collection
		TArray<class USplineMeshComponent*> TeleportMeshObjects;

private:
	bool FindTeleportDestination(FVector& Location);
	void UpdateSpline(struct FPredictProjectilePathResult Result);
	void UpdateTeleportation();
};
