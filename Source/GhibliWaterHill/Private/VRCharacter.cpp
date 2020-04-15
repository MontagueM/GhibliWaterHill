// Fill out your copyright notice in the Description page of Project Settings.


#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "Runtime/HeadMountedDisplay/Public/HeadMountedDisplayFunctionLibrary.h"
#include "Components/InputComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "VRController.h"

// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();

	// so that we aren't in the floor
	UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Floor);
	// TODO
	// we want to spawn the specific class here (BP)
	LeftController = GetWorld()->SpawnActor<AVRController>(HandControllerClass);
	if (!ensure(LeftController)) { return; }
	LeftController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
	LeftController->SetOwner(this);
	LeftController->SetHand(EControllerHand::Left);

	RightController = GetWorld()->SpawnActor<AVRController>(HandControllerClass);
	if (!ensure(RightController)) { return; }
	RightController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
	RightController->SetOwner(this);
	RightController->SetHand(EControllerHand::Right);
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	/*
	Explanation:
	The error is that the actor doesn't move when the camera does. Hence, we move the actor in the direction of the camera movement.
	However, since the camera is a child of the main actor, the camera also gets pushed forwards. Hence, we need to remove that offset
	only for the camera so that is stays in place.
	*/
	FVector NewCameraOffset = Camera->GetComponentLocation() - GetActorLocation();
	NewCameraOffset.Z = 0; // We don't want to be pushing the component up or down. Without this you fall through the component
	AddActorWorldOffset(NewCameraOffset);
	VRRoot->AddWorldOffset(-NewCameraOffset);

}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Forward"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Right"), this, &AVRCharacter::MoveRight);
	PlayerInputComponent->BindAction(TEXT("Teleport"), IE_Released, this, &AVRCharacter::BeginTeleport);

}

void AVRCharacter::MoveForward(float Scale)
{
	AddMovementInput(Camera->GetForwardVector(), Scale);
}

void AVRCharacter::MoveRight(float Scale)
{
	AddMovementInput(Camera->GetRightVector(), Scale);
}

void AVRCharacter::BeginTeleport()
{
	// Fade out
	PlayerCameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	PlayerCameraManager->StartCameraFade(0, 1, TeleportBlinkTime / 2, FLinearColor::Black, false, true); // last needs to be true otherwise flashes white
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AVRCharacter::EndTeleport, TeleportBlinkTime / 2);
}

void AVRCharacter::EndTeleport()
{
	// TODO
	PlayerCameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	FVector TeleportLocation;
	GetTeleportController()->FindTeleportDestination(TeleportLocation); // could be more efficient to simply grab the position as usual instead of recalculating it all
	SetActorLocation(TeleportLocation + FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight())); // Capsule added to stop teleporting into floor
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AVRCharacter::FadeOutFromTeleport, TeleportTime);
}

void AVRCharacter::FadeOutFromTeleport()
{
	PlayerCameraManager->StartCameraFade(1, 0, TeleportBlinkTime / 2, FLinearColor::Black, false, true);
}

AVRController* AVRCharacter::GetTeleportController()
{
	if (LeftController->bCanHandTeleport()) { return LeftController; }
	else { return RightController; }
}
