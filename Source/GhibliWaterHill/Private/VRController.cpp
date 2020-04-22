// Fill out your copyright notice in the Description page of Project Settings.


#include "VRController.h"
#include "MotionControllerComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SplineMeshComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "Engine/StaticMeshActor.h" 

#include "DrawDebugHelpers.h" 

using namespace std;

// Sets default values
AVRController::AVRController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController"));
	SetRootComponent(MotionController);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker"));
	DestinationMarker->SetupAttachment(GetRootComponent());
	DestinationMarker->SetWorldScale3D(DestinationMarkerScale);

	MarkerPoint = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerPoint"));
	MarkerPoint->SetupAttachment(DestinationMarker);

	TeleportPath = CreateDefaultSubobject<USplineComponent>(TEXT("TeleportPath"));
	TeleportPath->SetupAttachment(GetRootComponent());

	PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));

	GrabVolume = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrabVolume"));
	GrabVolume->SetupAttachment(GetRootComponent());

	ControllerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ControllerMesh"));
	ControllerMesh->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AVRController::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AVRController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (bCanHandTeleport() && bCanCheckTeleport) 
	{ 
		bAllowCharacterTeleport = UpdateTeleportationCheck();
	}

	if (bIsGrabbing || bIsFlicking)
	{
		// move object we're holding 
		FVector MoveVector;
		if (ensure(FlickedComponent))
		{
			if (bIsFlicking && FVector::Distance(FlickedComponent->GetComponentLocation(), GetActorLocation()) - GrabbedComponentInitDistance > 100)
			{
				UE_LOG(LogTemp, Warning, TEXT("Special one %f"), FVector::Distance(FlickedComponent->GetComponentLocation(), GetActorLocation()))
				MoveVector = GetActorForwardVector() * FVector::Distance(FlickedComponent->GetComponentLocation(), GetActorLocation());
			}
			else {
				MoveVector = GetActorForwardVector() + GetActorRotation().Vector() * GrabbedComponentInitDistance;
			}
		}
		else
		{
			MoveVector = GetActorForwardVector() + GetActorRotation().Vector() * GrabbedComponentInitDistance;
		}
		
		FRotator MoveRotator = GetActorRotation() - ControllerRotationOnGrab;
		PhysicsHandle->SetTargetLocation(GetActorLocation() + MoveVector);
		PhysicsHandle->SetTargetRotation(GetActorRotation());
	}
	if (Hand == EControllerHand::Left)
	{
		UE_LOG(LogTemp, Warning, TEXT("Rot %s"), *GetActorRotation().ToString())
		FlickHighlight();
	}
}

void AVRController::SetHand(EControllerHand SetHand) {
	Hand = SetHand;
	MotionController->SetTrackingSource(Hand);
	if (Hand == EControllerHand::Left) { ControllerMesh->SetStaticMesh(LeftControllerMesh); }
	else if (Hand == EControllerHand::Right) { ControllerMesh->SetStaticMesh(RightControllerMesh); }
}

EControllerHand AVRController::GetHand() { return Hand; }

bool AVRController::FindTeleportDestination(FVector& Location)
{
	/// Using rotateangleaxis for easiness in teleportation handling (rotates it down from the controller)

	FVector StartLocation = GetActorLocation() + GetActorForwardVector()*5;
	FVector Direction = GetActorForwardVector().RotateAngleAxis(15, GetActorRightVector());

	FPredictProjectilePathResult Result;
	FPredictProjectilePathParams Params(TeleportProjectileRadius,
		StartLocation,
		Direction* TeleportProjectileSpeed,
		TeleportSimulationTime,
		ECollisionChannel::ECC_Visibility,
		this);
	//Params.DrawDebugType = EDrawDebugTrace::ForOneFrame;
	Params.bTraceComplex = true; // to stop it not showing teleport places due to weird collisions in the map
	Params.SimFrequency = TeleportSimulationFrequency; // dictates smoothness of arc
	bool bHit = UGameplayStatics::PredictProjectilePath(this, Params, Result);
	/// Draw teleport curve, annoying its here though could move it
	UpdateSpline(Result);
	/// We want to make sure we are also allowed to teleport there
	UNavigationSystemV1* UNavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!ensure(UNavSystem)) { return false; }
	FNavLocation NavLocation;
	bool bValidDestination = UNavSystem->ProjectPointToNavigation(FVector(Result.HitResult.Location), NavLocation, TeleportNavExtent);
	Location = NavLocation.Location;

	return bHit && bValidDestination;
}

void AVRController::UpdateSpline(FPredictProjectilePathResult Result)
{
	// to hide left over spline components
	for (USplineMeshComponent* u : TeleportMeshObjects) // TODO refactor
	{
		u->SetVisibility(false);
	}
	TeleportPath->ClearSplinePoints(true);
	for (int i = 0; i < Result.PathData.Num(); i++)
	{
		if (Result.PathData.Num() > TeleportMeshObjects.Num()) // only add if we need to add another one
		{

			SplineMesh = NewObject<USplineMeshComponent>(this);
			SplineMesh->SetStaticMesh(TeleportArcMesh);
			SplineMesh->SetMaterial(0, TeleportArcMaterial);
			SplineMesh->RegisterComponent();
			TeleportMeshObjects.Add(SplineMesh);
		}
		TeleportPath->AddSplinePoint(Result.PathData[i].Location, ESplineCoordinateSpace::Local, ESplinePointType::Curve);

		/// Orienting the meshes
		if (i > 0)
		{
			SplineMesh = TeleportMeshObjects[i - 1];
			FVector LocationStart, TangentStart, LocationEnd, TangentEnd;
			TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i - 1, LocationStart, TangentStart);
			TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i, LocationEnd, TangentEnd);
			SplineMesh->SetStartAndEnd(LocationStart, TangentStart, LocationEnd, TangentEnd);
			TeleportMeshObjects[i - 1]->SetVisibility(true);
			if (i == Result.PathData.Num() - 1)
			{
				MarkerPoint->SetWorldLocation(LocationEnd);
			}
		}
	}
	MarkerPoint->SetVisibility(true);
}

bool AVRController::UpdateTeleportationCheck()
{
	/// Destination for teleport
	FVector TeleportLocation;
	bool bTeleportDestinationExists = FindTeleportDestination(TeleportLocation);
	if (bTeleportDestinationExists && bCanHandTeleport() && bCanCheckTeleport)
	{
		FCollisionQueryParams TraceParams(FName(TEXT("Trace")), false, GetOwner());
		/// Ray-cast out to reach distance
		FHitResult Hit;
		GetWorld()->LineTraceSingleByObjectType(OUT Hit,
			TeleportLocation,
			TeleportLocation + FVector(0, 0, -200),
			FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic),
			TraceParams
		);
		if (Hit.bBlockingHit) { DestinationMarker->SetWorldLocation(Hit.Location); }
		else { DestinationMarker->SetWorldLocation(TeleportLocation); }

		DestinationMarker->SetWorldRotation(FRotator::ZeroRotator);
		DestinationMarker->SetVisibility(true);
		return true;
	}
	else
	{
		DestinationMarker->SetVisibility(false);
		return false;
	}
}

bool AVRController::bCanHandTeleport()
{
	if (Hand == TeleportHand ) { return true; }

	return false;
}

bool AVRController::bCanHandMove()
{
	if (Hand == TeleportHand) { return false; }

	return true;
}

void AVRController::SetCanCheckTeleport(bool bCheck)
{
	bCanCheckTeleport = bCheck;
	DestinationMarker->SetVisibility(false);
	MarkerPoint->SetVisibility(false);
	for (USplineMeshComponent* u : TeleportMeshObjects)
	{
		u->SetVisibility(false);
	}
	TeleportPath->ClearSplinePoints(true);
}

void AVRController::DetectGrabStyle()
{
	if (ComponentToFlick || FlickedComponent) { TryFlick(); }
	else { TryGrab(); }
}

bool AVRController::bGoodFlickRotation()
{
	FRotator Rotation = GetActorRotation();
	// TODO we can deal with the magic numbers here at least for now
	if (Hand == EControllerHand::Left) // since we need to change it/flip it based on the hand
	{
		if (( Rotation.Pitch < 15 && Rotation.Pitch > -45 ) && (Rotation.Roll < 120 && Rotation.Roll > 50))
		{
			UE_LOG(LogTemp, Warning, TEXT("Trying to highlight"))
				return true;
		}
	}
	return false;
}

void AVRController::FlickHighlight()
{
	/*
	PER TICK
	If the hand controlled is rotated correctly (depending on hand)
		Get the actor
		Highlight the actor in the world or something like it
	If it isn't rotated correctly
		Set to nullptr
	*/

	if (bGoodFlickRotation())
	{
		UE_LOG(LogTemp, Warning, TEXT("Trying to find object to flick"))
		FCollisionQueryParams TraceParams(FName(TEXT("Trace")), false, GetOwner());
		/// Ray-cast out to reach distance
		FHitResult Hit; // .RotateAngleAxis(-90, FVector(0, 0, 0))
		FVector HandOrientation = GetActorUpVector().RotateAngleAxis(120, GetActorRightVector());
		float Reach = 1000;

		DrawDebugLine(GetWorld(),
			GetActorLocation(),
			GetActorLocation() + HandOrientation * Reach,
			FColor::Black);

		//TraceParams.bDebugQuery = true;
		//DrawDebugLine(GetWorld(),
		//	GetActorLocation(),
		//	GetActorLocation() + GetActorForwardVector() * Reach,
		//	FColor::Red);

		//TraceParams.bDebugQuery = true;
		//DrawDebugLine(GetWorld(),
		//	GetActorLocation(),
		//	GetActorLocation() + GetActorRightVector() * Reach,
		//	FColor::Green);

		//TraceParams.bDebugQuery = true;
		//DrawDebugLine(GetWorld(),
		//	GetActorLocation(),
		//	GetActorLocation() + GetActorUpVector() * Reach,
		//	FColor::Blue);

		GetWorld()->LineTraceSingleByObjectType(OUT Hit,
			GetActorLocation(),
			GetActorLocation() + HandOrientation * Reach,
			FCollisionObjectQueryParams(ECollisionChannel::ECC_PhysicsBody),
			TraceParams
		);
		if (Hit.bBlockingHit)
		{
			UE_LOG(LogTemp, Warning, TEXT("Found object to flick %s"), *Hit.GetComponent()->GetName())
			// highlight component
			ComponentToFlick = Hit.GetComponent();
		}
		else { ComponentToFlick = nullptr; }
	}
}

void AVRController::TryFlick()
{
	/*
	Given highlighted component
	Grab the component with physics handle
	Place at location which (x^2+y^2+z^2)^0.5 but scale not location + a bit
	*/
	UE_LOG(LogTemp, Warning, TEXT("Trying to flick"))
	if (ComponentToFlick && !bIsGrabbing && !bIsFlicking)
	{
		FVector Scale = ComponentToFlick->RelativeScale3D;
		FVector Location = GetActorLocation() + GetActorForwardVector() * Scale.Size();
		Location = GrabVolume->GetComponentLocation() + GetActorForwardVector() * 100;
		UE_LOG(LogTemp, Warning, TEXT("GrabbingGrabbingGrabbingGrabbingg to %s"), *Location.ToString())
		//ComponentToFlick->SetWorldLocation(Location);
		PhysicsHandle->GrabComponentAtLocationWithRotation(ComponentToFlick, NAME_None, Location, GetOwner()->GetActorRotation());
		GrabbedComponentInitDistance = FVector::Distance(GetActorLocation(), Location);
		bIsFlicking = true;
		FlickedComponent = ComponentToFlick;
	}
}

void AVRController::TryGrab()
{
	/*
	Get collision area around controller for grabbing
	If any objects, get closest to controller
	Use PhysicsHandle or socket
	*/
	UE_LOG(LogTemp, Warning, TEXT("Trying to grab"))
	if (bIsGrabbing || bIsFlicking) { return; }

	TArray<UPrimitiveComponent*> OverlappingComponents;
	GrabVolume->GetOverlappingComponents(OverlappingComponents);
	if (OverlappingComponents.Num() == 0) { return; }
	bIsGrabbing = true;
	GrabbedComponent = OverlappingComponents[0];
	UE_LOG(LogTemp, Warning, TEXT("grab"))
	PhysicsHandle->GrabComponentAtLocationWithRotation(GrabbedComponent, NAME_None, GrabbedComponent->GetComponentLocation(), GetOwner()->GetActorRotation());
	GrabbedComponentInitDistance = FVector::Distance(GetActorLocation(), GrabbedComponent->GetComponentLocation());
	ControllerRotationOnGrab = GetActorRotation();
}

void AVRController::ReleaseGrab()
{
	if (bIsGrabbing)
	{
		PhysicsHandle->ReleaseComponent();
		bIsGrabbing = false;
	}
}