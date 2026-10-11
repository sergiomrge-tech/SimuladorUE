#include "ResortVisuals.h"
#include "SimuladorUe.h"
#include "ResortTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"
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

bool ResortVisuals::ApplyCharacterModel(ACharacter* Character, const FCharacterModel& Model)
{
	if (Character == nullptr || !Model.IsSet() || Character->GetMesh() == nullptr)
	{
		return false;
	}
	USkeletalMesh* Skeletal = LoadObject<USkeletalMesh>(nullptr, *Model.Mesh);
	if (Skeletal == nullptr)
	{
		UE_LOG(LogResort, Warning, TEXT("Modelo de personagem nao encontrado: %s (usando boneco basico)"), *Model.Mesh);
		return false;
	}

	USkeletalMeshComponent* Mesh = Character->GetMesh();
	Mesh->SetSkeletalMesh(Skeletal);
	if (!Model.AnimClass.IsEmpty())
	{
		if (UClass* Anim = LoadClass<UAnimInstance>(nullptr, *Model.AnimClass))
		{
			Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			Mesh->SetAnimInstanceClass(Anim);
		}
		else
		{
			UE_LOG(LogResort, Warning, TEXT("Animation Blueprint nao encontrado: %s"), *Model.AnimClass);
		}
	}
	const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, -HalfHeight + Model.OffsetZ));
	Mesh->SetRelativeRotation(FRotator(0.f, Model.Yaw, 0.f));
	Mesh->SetRelativeScale3D(FVector(Model.Scale));
	Mesh->SetVisibility(true);
	return true;
}

bool ResortVisuals::AddStaticCharacter(AActor* Owner, USceneComponent* Parent, const FCharacterModel& Model,
	const FVector& LocalLocation, float LocalYaw)
{
	if (Owner == nullptr || Parent == nullptr || !Model.IsSet())
	{
		return false;
	}
	USkeletalMesh* Skeletal = LoadObject<USkeletalMesh>(nullptr, *Model.Mesh);
	if (Skeletal == nullptr)
	{
		UE_LOG(LogResort, Warning, TEXT("Modelo de funcionario nao encontrado: %s (usando boneco basico)"), *Model.Mesh);
		return false;
	}
	USkeletalMeshComponent* Comp = NewObject<USkeletalMeshComponent>(Owner);
	Comp->SetSkeletalMesh(Skeletal);
	Comp->SetMobility(EComponentMobility::Movable);
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (!Model.AnimClass.IsEmpty())
	{
		if (UClass* Anim = LoadClass<UAnimInstance>(nullptr, *Model.AnimClass))
		{
			Comp->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			Comp->SetAnimInstanceClass(Anim);
		}
	}
	Comp->SetupAttachment(Parent);
	Comp->SetRelativeLocation(LocalLocation + FVector(0.f, 0.f, Model.OffsetZ));
	Comp->SetRelativeRotation(FRotator(0.f, LocalYaw + Model.Yaw, 0.f));
	Comp->SetRelativeScale3D(FVector(Model.Scale));
	Comp->RegisterComponent();
	return true;
}
