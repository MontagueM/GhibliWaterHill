// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KeycardReader.generated.h"

UCLASS()
class GHIBLIWATERHILL_API AKeycardReader : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AKeycardReader();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere)
	class ADoor* LinkedDoor;

	UPROPERTY(EditAnywhere)
	class AKeycard* LinkedKeycard;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* KeycardDetectRegion;

	UPROPERTY(EditDefaultsOnly)
	class UMaterialInterface* DoorActivatedMaterial;

	UPROPERTY(EditDefaultsOnly)
	class UMaterialInterface* DoorDisabledMaterial;

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void LockDoor();
	void ActivateDoor();
};
