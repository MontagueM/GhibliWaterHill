// Fill out your copyright notice in the Description page of Project Settings.


#include "VRController.h"
#include "MotionControllerComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SplineMeshComponent.h"

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

	if (bCanHandTeleport() && bCanCheckTeleport) { UpdateTeleportationCheck(); }
		
}

void AVRController::SetHand(EControllerHand SetHand) {
	Hand = SetHand;
	MotionController->SetTrackingSource(Hand);
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
		Direction * TeleportProjectileSpeed,
		TeleportSimulationTime,
		ECollisionChannel::ECC_Visibility,
		this);
	//Params.DrawDebugType = EDrawDebugTrace::ForOneFrame;
	Params.bTraceComplex = true; // to stop it not showing teleport places due to weird collisions in the map
	Params.SimFrequency = TeleportSimulationFrequency; // dictates smoothness of arc
	bool bHit = UGameplayStatics::PredictProjectilePath(this, Params, Result);
	/// Draw teleport curve
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

void AVRController::UpdateTeleportationCheck()
{
	//UE_LOG(LogTemp, Warning, TEXT("Updating teleportation"))
	/// Destination for teleport
	FVector TeleportLocation;
	bool CanTeleport = FindTeleportDestination(TeleportLocation);
	if (CanTeleport && bCanHandTeleport() && bCanCheckTeleport)
	{
		DestinationMarker->SetWorldLocation(TeleportLocation);
		DestinationMarker->SetWorldRotation(FRotator::ZeroRotator);
		DestinationMarker->SetVisibility(true);
	}
	else
	{
		DestinationMarker->SetVisibility(false);
	}
}

bool AVRController::bCanHandTeleport()
{
	if (Hand == TeleportHand) { return true; }
	return false;
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