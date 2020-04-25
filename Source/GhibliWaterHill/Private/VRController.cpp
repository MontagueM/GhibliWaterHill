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

	FlickRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FlickRoot"));
	FlickRoot->SetupAttachment(GetRootComponent());

	FlickVolume = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlickVolume"));
	FlickVolume->SetupAttachment(FlickRoot);

	DebugMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugMesh"));
	DebugMesh->SetupAttachment(GetRootComponent());
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
		//	UE_LOG(LogTemp, Warning, TEXT("The %d"), RegisteredSplineComponent->GetNumberOfSplinePoints())
		//}
		//else
		//{
		//	UE_LOG(LogTemp, Warning, TEXT("Spline is nullptr"))
		//}


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
	UpdateSpline(PathPointDataToFVector(Result.PathData), TeleportPath);

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

TArray<FVector> AVRController::PathPointDataToFVector(TArray<FPredictProjectilePathPointData> PathData)
{
	TArray<FVector> OutArray;
	for (FPredictProjectilePathPointData Point : PathData) { OutArray.Add(Point.Location); }
	return OutArray;
}

void AVRController::UpdateSpline(TArray<FVector> PathData, USplineComponent* PathToUpdate)
{
	// to hide left over spline components
	ModifySplinePoints(PathToUpdate, true, true);
	for (int i = 0; i < PathData.Num(); i++)
	{
		if (PathData.Num() > MeshObjects.Num()) // only add if we need to add another one
		{

			SplineMesh = NewObject<USplineMeshComponent>(this);
			SplineMesh->SetStaticMesh(TeleportArcMesh);
			SplineMesh->SetMaterial(0, TeleportArcMaterial);
			SplineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			SplineMesh->RegisterComponent();
			MeshObjects.Add(SplineMesh);
		}
		PathToUpdate->AddSplinePoint(PathData[i], ESplineCoordinateSpace::Local, ESplinePointType::Curve);
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
				if (i == PathData.Num() - 1)
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
		return true && !bGoodFlickRotation(); // We only want to allow teleporting if not trying to flick
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
	if (ComponentCurrentlyFlicking || bHoldingFlick) { ReleaseFlick(); }
	else { ReleaseGrab(); }
}

bool AVRController::bGoodFlickRotation()
{
	FRotator Rotation = GetActorRotation();
	// TODO we can deal with the magic numbers here at least for now
	if (Hand == EControllerHand::Left) // since we need to change it/flip it based on the hand
	{
		if (( Rotation.Pitch < 25 && Rotation.Pitch > -45 ) && (Rotation.Roll < 120 && Rotation.Roll > 15))
		{
			//UE_LOG(LogTemp, Warning, TEXT("Trying to highlight"))
			return true;
		}
	}
	return false;
}

void AVRController::FlickHighlight()
{
	if (bGoodFlickRotation() && !bHoldingFlick && !ComponentCurrentlyFlicking)
	{
		UE_LOG(LogTemp, Warning, TEXT("Trying to find object to flick"))
		/// Ray-cast out to reach distance
		FVector StartLocation = GetActorLocation();
		HandFlickDirection = GetActorUpVector().RotateAngleAxis(100, GetActorRightVector()).RotateAngleAxis(0, GetActorUpVector()).RotateAngleAxis(0, GetActorForwardVector());


		FPredictProjectilePathResult FlickResult;
		bool bHit = ProjectilePathingUpdate(FlickResult,
			TeleportProjectileRadius,
			StartLocation,
			HandFlickDirection,
			TeleportProjectileSpeed*1,
			TeleportSimulationTime*2,
			ECollisionChannel::ECC_PhysicsBody);
		

		// Getting a larger area to detect objects
		FlickRoot->SetWorldRotation(HandFlickDirection.Rotation());
		TArray<UPrimitiveComponent*> PotentialFlickComponents;
		TArray<float> Distances;
		GetOverlappingComponents(PotentialFlickComponents);

		for (UPrimitiveComponent* Comp : PotentialFlickComponents)
		{
			if (ensure(Comp) && Comp->IsSimulatingPhysics())
			{
				Distances.Add(FVector::Distance(Comp->GetComponentLocation(), FlickVolume->GetComponentLocation()));
			}
		}
		int32 MinIndex = 0;
		float MinValue = 0;
		UPrimitiveComponent* Component = nullptr;
		if (Distances.Num() > 0)
		{
			UKismetMathLibrary::MinOfFloatArray(Distances, MinIndex, MinValue);
			Component = PotentialFlickComponents[MinIndex];
		}
		else { Component = FlickResult.HitResult.GetComponent(); }
		bOnOldComponent = true;
		if (RegisteredFlickComponent != Component ||
			(FVector::Distance(RegisteredControllerLocation, GetActorLocation()) > 1 && RegisteredControllerLocation != FVector::ZeroVector))
		{
			bOnOldComponent = false;
			if (!bHoldingFlick)
			{
				ResetRegisteredComponents();
			}
		}
		if (bHit && Component != ControllerMesh && Component->IsSimulatingPhysics() && !ComponentCurrentlyFlicking )
		{
			UE_LOG(LogTemp, Warning, TEXT("Found object to flick %s"), *Component->GetName())
			RegisteredFlickComponent = Component;
			RegisteredFlickComponent->SetRenderCustomDepth(true);
			RegisteredControllerLocation = GetActorLocation();

			UpdateFlickSpline(HandFlickDirection);
			UE_LOG(LogTemp, Warning, TEXT("2"))
		}
		else 
		{ 
			ModifySplinePoints(FlickPath, true, true);
			bOnOldComponent = false;
		}
	}
}

void AVRController::UpdateFlickSpline(FVector Direction)
{
	FVector Vec1 = GetActorLocation();
	FVector Vec2 = RegisteredFlickComponent->GetComponentLocation();
	float DirectionAngle = acos(FVector::DotProduct(Direction, Vec2 - Vec1) / (Direction.Size() * (Vec2 - Vec1).Size()));
	UE_LOG(LogTemp, Warning, TEXT("Angle %f"), DirectionAngle)
		float CpMultiplier = 100 * pow(DirectionAngle / (15 * PI / 180), 2); // Remove magic number and deal with angle going down
	UE_LOG(LogTemp, Warning, TEXT("3"))
		FVector Cp1 = Vec1 - GetActorRightVector() * CpMultiplier;
	FVector Cp2 = Vec2 - GetActorRightVector() * CpMultiplier;
	//DebugMesh->SetWorldLocation(FVector::ZeroVector);
	FVector ControlPoints[4] = { Vec1,
	Cp1,
	Cp2,
	Vec2 };
	int32 NumPoints = 100;
	TArray<FVector> OutPoints;
	UE_LOG(LogTemp, Warning, TEXT("5"))
		FVector::EvaluateBezier(ControlPoints, NumPoints, OutPoints);
	UpdateSpline(OutPoints, FlickPath);
	ModifySplinePoints(FlickPath, true, false); // DO hide points, DO NOT remove them
	RegisteredSplineComponent = FlickPath;
}

void AVRController::TryFlick()
{ 
	/*
	Given highlighted component

	*/
	//UE_LOG(LogTemp, Warning, TEXT("Trying to flick"))
	bHoldingFlick = true;
	UpdateFlickSpline(HandFlickDirection);
	ModifySplinePoints(FlickPath, false, false);
	UE_LOG(LogTemp, Warning, TEXT("Hodl"))
	if (RegisteredFlickComponent && !bIsGrabbing)
	{
		//UE_LOG(LogTemp, Warning, TEXT("1"))
		if (bUpVelocityForFlick())
		{
			//UE_LOG(LogTemp, Warning, TEXT("WOOOOOOOOOOOOOOOOOOOOO"))
			ComponentCurrentlyFlicking = RegisteredFlickComponent; // TODO figure this stuff out, need to smoothly move from 0 to 1
			RegisteredFlickComponent = nullptr;
			ModifySplinePoints(FlickPath, true, false); // we only want to hide the spline points
			ComponentCurrentlyFlicking->SetRenderCustomDepth(false);
			StartComponentFling.Broadcast(RegisteredSplineComponent, ComponentCurrentlyFlicking);
		}
		else
		{ 
			ComponentCurrentlyFlicking = nullptr;
			//UE_LOG(LogTemp, Warning, TEXT("We have a component highlighted, ready to pull!"))
		}
	}
}

bool AVRController::bUpVelocityForFlick()
{
	FVector Velocity = ControllerMesh->GetPhysicsAngularVelocityInDegrees();
	return (abs(Velocity.X) > 125 && abs(Velocity.Y) > 125); // TODO mess with this
}

void AVRController::ReleaseFlick()
{
	// I think this method is causing the dissapearances ie release flick and it removes the spline so stops flying
	// Stop when flicking component reaches end of spline (alpha = 1)
	bHoldingFlick = false;
	//UE_LOG(LogTemp, Warning, TEXT("Resetting! 1"))
	//ComponentCurrentlyFlicking = nullptr; Not sure if we want this, wtf do i do
}

void AVRController::ResetRegisteredComponents()
{
	if (ComponentCurrentlyFlicking) 
	{ 
		ComponentCurrentlyFlicking->SetRenderCustomDepth(false);
	}
	if (RegisteredFlickComponent) { RegisteredFlickComponent->SetRenderCustomDepth(false); }
	ComponentCurrentlyFlicking = nullptr;
	RegisteredFlickComponent = nullptr;
	//UE_LOG(LogTemp, Warning, TEXT("0"))
	RegisteredSplineComponent = nullptr;
	RegisteredControllerLocation = FVector::ZeroVector;
}

void AVRController::ModifySplinePoints(USplineComponent* PathToUpdate, bool bHidePoints, bool bClear)
{
	for (USplineMeshComponent* u : MeshObjects)
	{
		u->SetVisibility(!bHidePoints);
	}
	if (bClear) { PathToUpdate->ClearSplinePoints(true); }
	//UE_LOG(LogTemp, Error, TEXT("ClearSplinePoints"))
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