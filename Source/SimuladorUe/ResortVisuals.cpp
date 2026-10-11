#include "ResortVisuals.h"
#include "SimuladorUe.h"
#include "ResortTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

UStaticMeshComponent* ResortVisuals::AddPart(
	AActor* Owner,
	USceneComponent* Parent,
	const TCHAR* Shape,
	const FVector& LocalLocation,
	const FVector& SizeCm,
	const FRotator& LocalRotation,
	const FLinearColor& Color,
	bool bBlocking)
{
	if (Owner == nullptr || Parent == nullptr)
	{
		return nullptr;
	}

	const FString MeshPath = FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Shape, Shape);
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
	if (Mesh == nullptr)
	{
		UE_LOG(LogResort, Warning, TEXT("Forma basica nao encontrada: %s"), *MeshPath);
		return nullptr;
	}

	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Owner);
	Comp->SetStaticMesh(Mesh);
	Comp->SetMobility(EComponentMobility::Movable);
	Comp->SetCanEverAffectNavigation(false);
	Comp->SetupAttachment(Parent);
	Comp->SetRelativeLocation(LocalLocation);
	Comp->SetRelativeRotation(LocalRotation);
	Comp->SetRelativeScale3D(SizeCm / 100.f);

	if (bBlocking)
	{
		Comp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Comp->SetCollisionObjectType(ECC_Building);
		Comp->SetCollisionResponseToAllChannels(ECR_Block);
	}
	else
	{
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BaseMaterial != nullptr)
	{
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(BaseMaterial, Owner);
		if (Mid != nullptr)
		{
			Mid->SetVectorParameterValue(TEXT("Color"), Color);
			Comp->SetMaterial(0, Mid);
		}
	}

	Comp->RegisterComponent();
	return Comp;
}
