// Fill out your copyright notice in the Description page of Project Settings.


#include "KeycardReader.h"
#include "Components/BoxComponent.h" 
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h" 
#include "Keycard.h"
#include "Door.h"
#include "Materials/MaterialInstanceDynamic.h"

// Sets default values
AKeycardReader::AKeycardReader()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));

	KeycardDetectRegion = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KeycardDetectRegion"));
	KeycardDetectRegion->SetupAttachment(GetRootComponent());
	KeycardDetectRegion->RegisterComponent();

	KeycardDetectRegion->OnComponentBeginOverlap.AddDynamic(this, &AKeycardReader::OnOverlapBegin);


}

// Called when the game starts or when spawned
void AKeycardReader::BeginPlay()
{
	Super::BeginPlay();

	SetReaderMesh();
	UE_LOG(LogTemp, Warning, TEXT("Mats count %d"), ReaderMesh->GetNumMaterials());
	if (!ensure(ReaderMesh->GetMaterial(1))) { return; }
	ActiveMaterialInstance = UMaterialInstanceDynamic::Create(ReaderMesh->GetMaterial(1), this);
	ReaderMesh->SetMaterial(1, ActiveMaterialInstance);

	SetLocked(true);
	UE_LOG(LogTemp, Warning, TEXT("Setup keycard reader"))
}

// Called every frame
void AKeycardReader::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
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
	if (!bDoorLocked) { return; }
	if ( (OtherActor == LinkedKeycard) && (OtherActor != nullptr) && (OtherActor != this) && (OtherComp != nullptr))
	{
		UE_LOG(LogTemp, Warning, TEXT("Keycard touched"))
		SetLocked(false);
	}
}

void AKeycardReader::SetLocked(bool bLocked)
{
	if (!ensure(LinkedDoor)) { return; }
	LinkedDoor->SetLockedState(bLocked);
	bDoorLocked = bLocked;
	ChangeMaterial(bDoorLocked);
}

UStaticMeshComponent* AKeycardReader::SetReaderMesh()
{
	UE_LOG(LogTemp, Warning, TEXT("Setting door mesh"))
	TArray<UStaticMeshComponent*> Meshes;
	GetComponents<UStaticMeshComponent>(Meshes);
	for (UStaticMeshComponent* M : Meshes) { if (M->GetName() == TEXT("KeycardReaderMesh")) { ReaderMesh = M; } }
	if (!ensure(ReaderMesh)) { return nullptr; }
	return ReaderMesh;
}

void AKeycardReader::ChangeMaterial(bool bLocked)
{
	if (!ensure(ReaderMesh)) { return; }
	if (!ensure(ActiveMaterialInstance)) { return; }
	UE_LOG(LogTemp, Warning, TEXT("Change material"))
	//return;
	if (bLocked) { 
		UE_LOG(LogTemp, Warning, TEXT("1"))
		ActiveMaterialInstance->SetScalarParameterValue(TEXT("DoorLocked"), 1); }
	else { 
		UE_LOG(LogTemp, Warning, TEXT("2"))
		ActiveMaterialInstance->SetScalarParameterValue(TEXT("DoorLocked"), 0); }
}

