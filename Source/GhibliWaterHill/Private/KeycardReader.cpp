// Fill out your copyright notice in the Description page of Project Settings.


#include "KeycardReader.h"
#include "Components/BoxComponent.h" 
#include "Components/StaticMeshComponent.h"
#include "Keycard.h"
#include "Door.h"

// Sets default values
AKeycardReader::AKeycardReader()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* SceneComp = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	SceneComp->SetupAttachment(GetRootComponent());

	KeycardDetectRegion = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KeycardDetectRegion"));
	KeycardDetectRegion->SetupAttachment(GetRootComponent());

	//KeycardDetectRegion->OnComponentBeginOverlap.AddDynamic(this, &AKeycardReader::OnOverlapBegin);
}

// Called when the game starts or when spawned
void AKeycardReader::BeginPlay()
{
	Super::BeginPlay();

	LockDoor();
	UE_LOG(LogTemp, Warning, TEXT("Setup keycard reader"))
}

// Called every frame
void AKeycardReader::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UE_LOG(LogTemp, Warning, TEXT("1"))
	if (!ensure(LinkedKeycard)) { return; }
	UE_LOG(LogTemp, Warning, TEXT("2"))
	TArray<UPrimitiveComponent*> OverlappingComponents;
	KeycardDetectRegion->GetOverlappingComponents(OverlappingComponents);
	UE_LOG(LogTemp, Warning, TEXT("3"))
	if (OverlappingComponents.Num() == 0) { return; }
	UE_LOG(LogTemp, Warning, TEXT("4444"))
	UPrimitiveComponent* CheckComponent = OverlappingComponents[0];
	if (CheckComponent == LinkedKeycard->FindComponentByClass<UPrimitiveComponent>())
	{ 
		UE_LOG(LogTemp, Warning, TEXT("Touched1"))
		ActivateDoor(); 
	}
}

void AKeycardReader::OnOverlapBegin(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("Touched"))
	if (!ensure(LinkedKeycard)) { return; }
	if ( (OtherActor == LinkedKeycard->GetParentActor()) && (OtherActor != nullptr) && (OtherActor != this) && (OtherComp != nullptr))
	{
		UE_LOG(LogTemp, Warning, TEXT("Keycard touched"))
		ActivateDoor();
	}
}

void AKeycardReader::LockDoor()
{
	if (!ensure(LinkedDoor)) { return; }
	UE_LOG(LogTemp, Warning, TEXT("Locking door now"))
	LinkedDoor->LockDoor();
}

void AKeycardReader::ActivateDoor()
{
	if (!ensure(LinkedDoor)) { return; }
	UE_LOG(LogTemp, Warning, TEXT("Unlocking door now"))
	LinkedDoor->UnlockDoor();
}

