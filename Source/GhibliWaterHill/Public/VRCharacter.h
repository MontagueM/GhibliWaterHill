// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VRCharacter.generated.h"

UCLASS()
class GHIBLIWATERHILL_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVRCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY()
	class UCameraComponent* Camera = nullptr;
	UPROPERTY()
	class USceneComponent* VRRoot = nullptr;
	class AVRController* LeftController = nullptr;
	class AVRController* RightController = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AVRController> HandControllerClass;
	UPROPERTY(EditDefaultsOnly)
	float TeleportBlinkTime = 1;
	UPROPERTY(EditDefaultsOnly)
	float TeleportTime = 0.1;

	class APlayerCameraManager* PlayerCameraManager = nullptr;

private:
	void MoveForward(float Scale);
	void MoveRight(float Scale);
	void BeginTeleport();
	void EndTeleport();
	void FadeOutFromTeleport();
	void UpdateActionMapping(FName ActionName, FKey OldKey, FKey NewKey);
	AVRController* GetTeleportController();

};
