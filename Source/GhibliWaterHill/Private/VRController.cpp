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
#include "Kismet/KismetMathLibrary.h" 

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

	FlickPath = CreateDefaultSubobject<USplineComponent>(TEXT("FlickPath"));
	FlickPath->SetupAttachment(GetRootComponent());

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

	if (bIsGrabbing)
	{
		// move object we're holding 
		FVector MoveVector = GetActorForwardVector() + GetActorRotation().Vector() * GrabbedComponentInitDistance;
		FRotator MoveRotator = GetActorRotation() - ControllerRotationOnGrab;
		PhysicsHandle->SetTargetLocation(GetActorLocation() + MoveVector);
		PhysicsHandle->SetTargetRotation(GetActorRotation());
	}
	if (Hand == EControllerHand::Left)
	{
		//if (RegisteredSplineComponent)
		//{
		//	UE_LOG(LogTemp, Warning, TEXT("Spline %d"), RegisteredSplineComponent->GetNumberOfSplinePoints())
		//}
		//else
		//{
		//	UE_LOG(LogTemp, Warning, TEXT("Spline not available"))
		//}
		//UE_LOG(LogTemp, Warning, TEXT("Up vel %s"), *ControllerMesh->GetComponentVelocity().ToString())
		FlickHighlight();
	}
}

void AVRController::SetHand(EControllerHand SetHand) {
	Hand = SetHand;
	MotionController->SetTrackingSource(Hand);
	if (Hand == EControllerHand::Left) 
	{ 
		ControllerMesh->SetStaticMesh(LeftControllerMesh);
		GrabVolume->SetWorldScale3D(FVector(0.22, 0.22, 0.38)/2); // TODO remove magic numbers
		GrabVolume->SetRelativeLocation(FVector(7.261321, 5.595972, 0.0));
	}
	else if (Hand == EControllerHand::Right) 
	{ 
		ControllerMesh->SetStaticMesh(RightControllerMesh);
		GrabVolume->SetWorldScale3D(FVector(0.22, 0.22, 0.38)/2);
		GrabVolume->SetRelativeLocation(FVector(7.261321, -8.009752, 0.0));
	}
}

EControllerHand AVRController::GetHand() { return Hand; }

bool AVRController::FindTeleportDestination(FVector& Location)
{
	/// Using rotateangleaxis for easiness in teleportation handling (rotates it down from the controller)

	FVector StartLocation = GetActorLocation() + GetActorForwardVector()*5;
	FVector Direction = GetActorForwardVector().RotateAngleAxis(15, GetActorRightVector());

	FPredictProjectilePathResult Result;
	bool bHit = ProjectilePathingUpdate(Result,
		TeleportProjectileRadius,
		StartLocation,
		Direction,
		TeleportProjectileSpeed,
		TeleportSimulationTime,
		ECollisionChannel::ECC_Visibility);
	UpdateSpline(Result, TeleportPath);

	/// We want to make sure we are also allowed to teleport there
	UNavigationSystemV1* UNavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!ensure(UNavSystem)) { return false; }
	FNavLocation NavLocation;
	bool bValidDestination = UNavSystem->ProjectPointToNavigation(FVector(Result.HitResult.Location), NavLocation, TeleportNavExtent);
	Location = NavLocation.Location;

	return bHit && bValidDestination;
}

bool AVRController::ProjectilePathingUpdate(FPredictProjectilePathResult& Result, float ProjectileRadius, FVector StartLocation, FVector Direction, float ProjectileSpeed, float SimulationTime, ECollisionChannel CollisionChannel)
{
	FPredictProjectilePathParams Params(ProjectileRadius,
		StartLocation,
		Direction * ProjectileSpeed,
		SimulationTime,
		CollisionChannel,
		this);
	//Params.DrawDebugType = EDrawDebugTrace::ForOneFrame;
	Params.bTraceComplex = true; // to stop it not showing teleport places due to weird collisions in the map
	Params.SimFrequency = TeleportSimulationFrequency; // dictates smoothness of arc
	bool bHit = UGameplayStatics::PredictProjectilePath(this, Params, Result);
	/// Draw teleport curve, annoying its here though could move it
	return bHit;
}

void AVRController::UpdateSpline(FPredictProjectilePathResult Result, USplineComponent* PathToUpdate)
{
	// to hide left over spline components
	for (USplineMeshComponent* u : MeshObjects)
	{
		u->SetVisibility(false);
	}
	PathToUpdate->ClearSplinePoints(true);


	for (int i = 0; i < Result.PathData.Num(); i++)
	{
		if (Result.PathData.Num() > MeshObjects.Num()) // only add if we need to add another one
		{

			SplineMesh = NewObject<USplineMeshComponent>(this);
			SplineMesh->SetStaticMesh(TeleportArcMesh);
			SplineMesh->SetMaterial(0, TeleportArcMaterial);
			SplineMesh->RegisterComponent();
			MeshObjects.Add(SplineMesh);
		}
		PathToUpdate->AddSplinePoint(Result.PathData[i].Location, ESplineCoordinateSpace::Local, ESplinePointType::Curve);

		/// Orienting the meshes
		if (i > 0)
		{
			SplineMesh = MeshObjects[i - 1];
			FVector LocationStart, TangentStart, LocationEnd, TangentEnd;
			PathToUpdate->GetLocalLocationAndTangentAtSplinePoint(i - 1, LocationStart, TangentStart);
			PathToUpdate->GetLocalLocationAndTangentAtSplinePoint(i, LocationEnd, TangentEnd);
			SplineMesh->SetStartAndEnd(LocationStart, TangentStart, LocationEnd, TangentEnd);
			MeshObjects[i - 1]->SetVisibility(true);
			if (bCanCheckTeleport)
			{
				if (i == Result.PathData.Num() - 1)
				{
					MarkerPoint->SetWorldLocation(LocationEnd);
				}
			}
		}
	}
	if (bCanCheckTeleport) { MarkerPoint->SetVisibility(true); }
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
	for (USplineMeshComponent* u : MeshObjects)
	{
		u->SetVisibility(false);
	}
	TeleportPath->ClearSplinePoints(true);
}

void AVRController::DetectGrabStyle()
{
	if (RegisteredFlickComponent && !ComponentCurrentlyFlicking) { TryFlick(); }
	else if (!bHoldingFlick) { TryGrab(); }
}

void AVRController::DetectReleaseStyle()
{
	if (ComponentCurrentlyFlicking) { ReleaseFlick(); }
	else { ReleaseGrab(); }
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

	if (bGoodFlickRotation() && !bHoldingFlick)
	{
		UE_LOG(LogTemp, Warning, TEXT("Trying to find object to flick"))
		/// Ray-cast out to reach distance
		FVector StartLocation = GetActorLocation();
		FVector Direction = GetActorUpVector().RotateAngleAxis(140, GetActorRightVector()).RotateAngleAxis(-30, GetActorUpVector()).RotateAngleAxis(-30, GetActorForwardVector());
		FPredictProjectilePathResult FlickResult;
		bool bHit = ProjectilePathingUpdate(FlickResult,
			TeleportProjectileRadius,
			StartLocation,
			Direction,
			TeleportProjectileSpeed*2,
			TeleportSimulationTime*2,
			ECollisionChannel::ECC_PhysicsBody);

		//DrawDebugLine(GetWorld(),
		//GetActorLocation(),
		//GetActorLocation() + GetActorForwardVector() * 1000,
		//FColor::Red);
		//DrawDebugLine(GetWorld(),
		//GetActorLocation(),
		//GetActorLocation() + GetActorUpVector() * 1000,
		//FColor::Blue);
		//DrawDebugLine(GetWorld(),
		//GetActorLocation(),
		//GetActorLocation() + GetActorRightVector() * 1000,
		//FColor::Yellow);

		UPrimitiveComponent* Component = FlickResult.HitResult.GetComponent();
		UpdateSpline(FlickResult, FlickPath);
		bool bNew = false;
		if (RegisteredFlickComponent != Component ||
			(FVector::Distance(RegisteredControllerLocation, GetActorLocation()) > 1 && RegisteredControllerLocation != FVector::ZeroVector))
		{
			//UE_LOG(LogTemp, Warning, TEXT("Resetting! 2"))
			ResetRegisteredComponents();
			bNew = true;
		}
		if (bHit && Component != ControllerMesh && Component->IsSimulatingPhysics())
		{
			//UE_LOG(LogTemp, Warning, TEXT("Found object to flick %s"), *FlickResult.HitResult.GetActor()->GetName())
			RegisteredFlickComponent = Component;
			RegisteredFlickComponent->SetRenderCustomDepth(true);
			RegisteredSplineComponent = FlickPath;
			RegisteredControllerLocation = GetActorLocation();
			// highlight component

		}
		else 
		{ 
			FlickPath->ClearSplinePoints(true);
			//UE_LOG(LogTemp, Warning, TEXT("Resetting! 3"))
		}
	}
}

void AVRController::TryFlick()
{ 
	/*
	Given highlighted component

	*/
	//UE_LOG(LogTemp, Warning, TEXT("Trying to flick"))
	bHoldingFlick = true;
	if (RegisteredFlickComponent && !bIsGrabbing)
	{
		//UE_LOG(LogTemp, Warning, TEXT("1"))
		if (bUpVelocityForFlick())
		{
			bCanShowGrabSpline = false;
			//UE_LOG(LogTemp, Warning, TEXT("WOOOOOOOOOOOOOOOOOOOOO"))
			ComponentCurrentlyFlicking = RegisteredFlickComponent; // TODO figure this stuff out, need to smoothly move from 0 to 1
			RegisteredFlickComponent = nullptr;
			StartComponentFling.Broadcast(RegisteredSplineComponent, ComponentCurrentlyFlicking);
		}
		else
		{ 
			ComponentCurrentlyFlicking = nullptr;
			bCanShowGrabSpline = true;
			//UE_LOG(LogTemp, Warning, TEXT("We have a component highlighted, ready to pull!"))
		}
	}
}

bool AVRController::bUpVelocityForFlick()
{
	FVector Velocity = ControllerMesh->GetPhysicsAngularVelocityInDegrees();
	return (abs(Velocity.X) > 50 && abs(Velocity.Y) > 50);
}

void AVRController::ReleaseFlick()
{
	ComponentCurrentlyFlicking = nullptr;
	bHoldingFlick = false;
	//UE_LOG(LogTemp, Warning, TEXT("Resetting! 1"))
	ResetRegisteredComponents();
}

void AVRController::ResetRegisteredComponents()
{
	if (RegisteredFlickComponent) { RegisteredFlickComponent->SetRenderCustomDepth(false); }
	RegisteredFlickComponent = nullptr;
	RegisteredSplineComponent = nullptr;
	RegisteredControllerLocation = FVector::ZeroVector;
}

void AVRController::TryGrab()
{
	/*
	Get collision area around controller for grabbing
	If any objects, get closest to controller
	Use PhysicsHandle or socket
	*/
	UE_LOG(LogTemp, Warning, TEXT("Trying to grab"))
	if (bIsGrabbing) { return; }

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