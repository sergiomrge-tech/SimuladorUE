#include "BuildingActor.h"
#include "ResortVisuals.h"
#include "ResortEconomy.h"
#include "ResortGameMode.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

ABuildingActor::ABuildingActor()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

UStaticMeshComponent* ABuildingActor::Part(const TCHAR* Shape, const FVector& Loc, const FVector& SizeCm,
	const FLinearColor& Color, bool bBlocking, const FRotator& Rot)
{
	const bool bRealCollision = bBlocking && Mode != EBuildingMode::Ghost;
	UStaticMeshComponent* Comp = ResortVisuals::AddPart(this, GetRootComponent(), Shape, Loc, SizeCm, Rot, Color, bRealCollision);
	if (Comp != nullptr)
	{
		Parts.Add(Comp);
	}
	return Comp;
}

void ABuildingActor::InitFromDef(const FBuildingDef& InDef, EBuildingMode InMode)
{
	Def = InDef;
	Mode = InMode;
	BuildVisuals();

	if (Mode == EBuildingMode::Placed)
	{
		if (UResortEconomy* Economy = AResortGameMode::GetEconomy(this))
		{
			Economy->RegisterBuilding(this);
		}
	}
}

void ABuildingActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Mode == EBuildingMode::Placed)
	{
		if (UResortEconomy* Economy = AResortGameMode::GetEconomy(this))
		{
			Economy->UnregisterBuilding(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ABuildingActor::SetGhostValid(bool bValid)
{
	const FLinearColor Color = bValid ? FLinearColor(0.1f, 0.85f, 0.25f) : FLinearColor(0.95f, 0.15f, 0.1f);
	for (UStaticMeshComponent* Comp : Parts)
	{
		if (Comp == nullptr)
		{
			continue;
		}
		if (UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(Comp->GetMaterial(0)))
		{
			Mid->SetVectorParameterValue(TEXT("Color"), Color);
		}
	}
}

bool ABuildingActor::TryReserve()
{
	if (Occupants >= Def.Capacity)
	{
		return false;
	}
	++Occupants;
	return true;
}

void ABuildingActor::Release()
{
	Occupants = FMath::Max(0, Occupants - 1);
}

FVector ABuildingActor::GetFootprintHalf() const
{
	return FVector(Def.Size.X * 0.5f + 20.f, Def.Size.Y * 0.5f + 20.f, Def.Size.Z * 0.5f + 15.f);
}

void ABuildingActor::BuildVisuals()
{
	const FVector S = Def.Size;
	const FLinearColor C = Def.Color;
	const FLinearColor RoofColor(0.45f, 0.18f, 0.12f);
	const FLinearColor DarkColor(0.1f, 0.07f, 0.05f);

	if (Def.Id == TEXT("umbrella"))
	{
		// Mastro + copa em cone + mesinha
		Part(TEXT("Cylinder"), FVector(0.f, 0.f, 110.f), FVector(6.f, 6.f, 220.f), FLinearColor(0.9f, 0.9f, 0.88f));
		Part(TEXT("Cone"), FVector(0.f, 0.f, 200.f), FVector(S.X, S.Y, 60.f), C, false);
		Part(TEXT("Cylinder"), FVector(0.f, 0.f, 70.f), FVector(80.f, 80.f, 4.f), FLinearColor(0.85f, 0.7f, 0.5f), false);
	}
	else if (Def.Id == TEXT("lounger"))
	{
		// Espreguicadeira: assento + encosto inclinado
		Part(TEXT("Cube"), FVector(20.f, 0.f, 28.f), FVector(140.f, S.Y, 10.f), C);
		Part(TEXT("Cube"), FVector(-60.f, 0.f, 46.f), FVector(70.f, S.Y, 8.f), C, true, FRotator(-30.f, 0.f, 0.f));
		Part(TEXT("Cube"), FVector(-35.f, 0.f, 12.f), FVector(8.f, S.Y - 10.f, 24.f), FLinearColor(0.6f, 0.6f, 0.6f), false);
	}
	else if (Def.Id == TEXT("pool"))
	{
		// Bordas brancas + lamina de agua
		const FLinearColor Rim(0.92f, 0.92f, 0.88f);
		const float T = 30.f;
		Part(TEXT("Cube"), FVector(S.X * 0.5f - T * 0.5f, 0.f, S.Z * 0.5f), FVector(T, S.Y, S.Z), Rim);
		Part(TEXT("Cube"), FVector(-S.X * 0.5f + T * 0.5f, 0.f, S.Z * 0.5f), FVector(T, S.Y, S.Z), Rim);
		Part(TEXT("Cube"), FVector(0.f, S.Y * 0.5f - T * 0.5f, S.Z * 0.5f), FVector(S.X, T, S.Z), Rim);
		Part(TEXT("Cube"), FVector(0.f, -S.Y * 0.5f + T * 0.5f, S.Z * 0.5f), FVector(S.X, T, S.Z), Rim);
		Part(TEXT("Cube"), FVector(0.f, 0.f, S.Z * 0.6f), FVector(S.X - 2.f * T, S.Y - 2.f * T, 6.f), FLinearColor(0.05f, 0.55f, 0.75f), false);
	}
	else if (Def.Id == TEXT("volley"))
	{
		// Quadra de areia + dois postes + rede
		Part(TEXT("Cube"), FVector(0.f, 0.f, 2.f), FVector(S.X, S.Y, 4.f), C, false);
		Part(TEXT("Cylinder"), FVector(0.f, S.Y * 0.5f, 115.f), FVector(8.f, 8.f, 230.f), FLinearColor(0.85f, 0.85f, 0.85f));
		Part(TEXT("Cylinder"), FVector(0.f, -S.Y * 0.5f, 115.f), FVector(8.f, 8.f, 230.f), FLinearColor(0.85f, 0.85f, 0.85f));
		Part(TEXT("Cube"), FVector(0.f, 0.f, 195.f), FVector(2.f, S.Y, 70.f), FLinearColor(0.95f, 0.95f, 0.95f), false);
	}
	else if (Def.Id == TEXT("palm"))
	{
		// Palmeira de enfeite: tronco + folhas caidas
		Part(TEXT("Cylinder"), FVector(0.f, 0.f, S.Z * 0.5f), FVector(35.f, 35.f, S.Z), FLinearColor(0.35f, 0.24f, 0.14f));
		const int32 LeafCount = 7;
		for (int32 i = 0; i < LeafCount; ++i)
		{
			const float Yaw = 360.f * i / LeafCount;
			const FVector Dir = FRotator(0.f, Yaw, 0.f).Vector();
			Part(TEXT("Cube"), FVector(Dir.X * 110.f, Dir.Y * 110.f, S.Z - 10.f), FVector(260.f, 40.f, 6.f),
				FLinearColor(0.08f, 0.4f, 0.1f), false, FRotator(-20.f, Yaw, 0.f));
		}
		Part(TEXT("Sphere"), FVector(0.f, 0.f, S.Z), FVector(35.f, 35.f, 35.f), FLinearColor(0.3f, 0.2f, 0.1f), false);
	}
	else
	{
		// Construcao padrao: corpo + telhado + porta (a porta fica no lado +X)
		Part(TEXT("Cube"), FVector(0.f, 0.f, S.Z * 0.5f), S, C);
		Part(TEXT("Cube"), FVector(0.f, 0.f, S.Z + 8.f), FVector(S.X + 60.f, S.Y + 60.f, 16.f), RoofColor, false);
		const float DoorHeight = static_cast<float>(FMath::Min(210.0, S.Z * 0.75));
		Part(TEXT("Cube"), FVector(S.X * 0.5f + 1.f, 0.f, DoorHeight * 0.5f), FVector(6.f, 90.f, DoorHeight), DarkColor, false);

		// Predios altos ganham "andares" (faixas escuras) para ler como hotel
		if (S.Z > 400.f)
		{
			for (float Z = 300.f; Z < S.Z - 50.f; Z += 300.f)
			{
				Part(TEXT("Cube"), FVector(0.f, 0.f, Z), FVector(S.X + 6.f, S.Y + 6.f, 10.f), DarkColor, false);
			}
		}
	}
}
