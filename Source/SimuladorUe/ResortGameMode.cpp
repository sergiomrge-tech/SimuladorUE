#include "ResortGameMode.h"
#include "BuildingActor.h"
#include "GuestCharacter.h"
#include "ResortCharacter.h"
#include "ResortEconomy.h"
#include "ResortHUD.h"
#include "SimuladorUe.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RandomStream.h"

namespace
{
	// Geometria da praia (cm). A costa fica em X = +500, o mar vai para +X.
	constexpr float SeaEdgeX = 500.f;
	constexpr float BackEdgeX = -3000.f;
	constexpr float SideEdgeY = 3000.f;
	constexpr float GuestSpawnX = -2900.f;
}

AResortGameMode::AResortGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	DefaultPawnClass = AResortCharacter::StaticClass();
	HUDClass = AResortHUD::StaticClass();

	Economy = CreateDefaultSubobject<UResortEconomy>(TEXT("Economy"));
}

UResortEconomy* AResortGameMode::GetEconomy(const UObject* WorldContext)
{
	if (GEngine == nullptr || WorldContext == nullptr)
	{
		return nullptr;
	}
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);
	if (World == nullptr)
	{
		return nullptr;
	}
	AResortGameMode* GameMode = World->GetAuthGameMode<AResortGameMode>();
	return GameMode != nullptr ? GameMode->Economy.Get() : nullptr;
}

void AResortGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	if (Economy != nullptr)
	{
		Economy->Initialize();
	}
}

AActor* AResortGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// O jogador nasce antes do StartPlay: garante o chao e o ponto de partida.
	EnsureWorldBuilt();

	if (PlayerStartActor == nullptr && GetWorld() != nullptr)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		PlayerStartActor = GetWorld()->SpawnActor<APlayerStart>(FVector(-1300.f, 0.f, 100.f), FRotator::ZeroRotator, Params);
	}
	return PlayerStartActor;
}

void AResortGameMode::StartPlay()
{
	EnsureWorldBuilt();
	Super::StartPlay();

	if (Economy != nullptr)
	{
		Economy->PushNotification(
			TEXT("Bem-vindo! Seu quiosque está aberto. Ganhe prestígio para crescer até o resort."),
			FLinearColor(1.f, 0.9f, 0.5f));
	}
}

void AResortGameMode::EnsureWorldBuilt()
{
	if (bWorldBuilt || GetWorld() == nullptr)
	{
		return;
	}
	bWorldBuilt = true;

	BuildBeach();
	BuildAtmosphere();
	SpawnPalms();
	SpawnStartingKiosk();
}

// ------------------------------------------------------------- Mundo

AStaticMeshActor* AResortGameMode::SpawnBox(const FVector& Center, const FVector& SizeCm, const FLinearColor& Color,
	bool bVisible, bool bCollide)
{
	UWorld* World = GetWorld();
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (World == nullptr || Cube == nullptr)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Center, FRotator::ZeroRotator, Params);
	if (Actor == nullptr)
	{
		return nullptr;
	}

	UStaticMeshComponent* Comp = Actor->GetStaticMeshComponent();
	Comp->SetMobility(EComponentMobility::Movable);
	Comp->SetStaticMesh(Cube);
	Actor->SetActorScale3D(SizeCm / 100.f);

	if (bCollide)
	{
		Comp->SetCollisionProfileName(TEXT("BlockAll"));
	}
	else
	{
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	Comp->SetVisibility(bVisible);
	Comp->SetCastShadow(bVisible);

	if (bVisible)
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (Base != nullptr)
		{
			if (UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Base, Actor))
			{
				Mid->SetVectorParameterValue(TEXT("Color"), Color);
				Comp->SetMaterial(0, Mid);
			}
		}
	}
	return Actor;
}

void AResortGameMode::BuildBeach()
{
	// Areia: grande, com o topo em Z = 0
	const float SandLengthX = SeaEdgeX - (-20000.f);
	SpawnBox(FVector((SeaEdgeX + (-20000.f)) * 0.5f, 0.f, -25.f), FVector(SandLengthX, 40000.f, 50.f),
		FLinearColor(0.93f, 0.8f, 0.58f), true, true);

	// Mar: superficie 15 cm abaixo da areia, sem colisao (paredes invisiveis seguram o jogador)
	SpawnBox(FVector(SeaEdgeX + 15000.f, 0.f, -45.f), FVector(30000.f, 40000.f, 60.f),
		FLinearColor(0.02f, 0.38f, 0.5f), true, false);

	// Paredes invisiveis: limitam a area jogavel (3500 x 6000 cm)
	const float WallHeight = 600.f;
	const float WallZ = WallHeight * 0.5f;
	const float WallThickness = 40.f;
	const float LengthX = SeaEdgeX - BackEdgeX;
	const float CenterX = (SeaEdgeX + BackEdgeX) * 0.5f;
	SpawnBox(FVector(SeaEdgeX + WallThickness * 0.5f, 0.f, WallZ), FVector(WallThickness, SideEdgeY * 2.f + 80.f, WallHeight), FLinearColor::White, false, true);
	SpawnBox(FVector(BackEdgeX - WallThickness * 0.5f, 0.f, WallZ), FVector(WallThickness, SideEdgeY * 2.f + 80.f, WallHeight), FLinearColor::White, false, true);
	SpawnBox(FVector(CenterX, SideEdgeY + WallThickness * 0.5f, WallZ), FVector(LengthX + 80.f, WallThickness, WallHeight), FLinearColor::White, false, true);
	SpawnBox(FVector(CenterX, -SideEdgeY - WallThickness * 0.5f, WallZ), FVector(LengthX + 80.f, WallThickness, WallHeight), FLinearColor::White, false, true);
}

void AResortGameMode::BuildAtmosphere()
{
	UWorld* World = GetWorld();
	AActor* Holder = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (Holder == nullptr)
	{
		return;
	}

	USceneComponent* Root = NewObject<USceneComponent>(Holder, TEXT("AtmosphereRoot"));
	Holder->SetRootComponent(Root);
	Root->RegisterComponent();

	// Sol (luz direcional que alimenta a atmosfera)
	SunComponent = NewObject<UDirectionalLightComponent>(Holder, TEXT("Sun"));
	SunComponent->SetMobility(EComponentMobility::Movable);
	SunComponent->SetupAttachment(Root);
	SunComponent->bUsedAsAtmosphereSunLight = true;
	SunComponent->SetIntensity(9.f);
	SunComponent->SetRelativeRotation(FRotator(-45.f, 40.f, 0.f));
	SunComponent->RegisterComponent();

	// Atmosfera do ceu
	USkyAtmosphereComponent* SkyAtmosphere = NewObject<USkyAtmosphereComponent>(Holder, TEXT("SkyAtmosphere"));
	SkyAtmosphere->SetupAttachment(Root);
	SkyAtmosphere->RegisterComponent();

	// Luz do ceu captura a atmosfera em tempo real (necessario para o Lumen iluminar bem)
	USkyLightComponent* SkyLight = NewObject<USkyLightComponent>(Holder, TEXT("SkyLight"));
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->SetupAttachment(Root);
	SkyLight->bRealTimeCapture = true;
	SkyLight->SourceType = SLS_CapturedScene;
	SkyLight->RegisterComponent();

	// Neblina leve para dar profundidade ao horizonte
	UExponentialHeightFogComponent* Fog = NewObject<UExponentialHeightFogComponent>(Holder, TEXT("Fog"));
	Fog->SetupAttachment(Root);
	Fog->SetFogDensity(0.004f);
	Fog->RegisterComponent();

	// Pos-processamento "realista estilizado": cores mais vivas, contraste suave, brilho e vinheta leve
	UPostProcessComponent* PostProcess = NewObject<UPostProcessComponent>(Holder, TEXT("PostProcess"));
	PostProcess->SetupAttachment(Root);
	PostProcess->bUnbound = true;
	PostProcess->BlendWeight = 1.f;
	FPostProcessSettings& Settings = PostProcess->Settings;
	Settings.bOverride_ColorSaturation = true;
	Settings.ColorSaturation = FVector4(1.15f, 1.15f, 1.15f, 1.f);
	Settings.bOverride_ColorContrast = true;
	Settings.ColorContrast = FVector4(1.06f, 1.06f, 1.06f, 1.f);
	Settings.bOverride_BloomIntensity = true;
	Settings.BloomIntensity = 0.7f;
	Settings.bOverride_VignetteIntensity = true;
	Settings.VignetteIntensity = 0.3f;
	PostProcess->RegisterComponent();

	UpdateSun();
}

void AResortGameMode::SpawnPalms()
{
	UWorld* World = GetWorld();
	FRandomStream Random(7);

	auto SpawnPalm = [&](const FVector& Location)
	{
		FBuildingDef Palm;
		Palm.Id = TEXT("palm");
		Palm.DisplayName = TEXT("Palmeira");
		Palm.Category = TEXT("decor");
		Palm.Size = FVector(40.f, 40.f, Random.FRandRange(420.f, 560.f));

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ABuildingActor* Actor = World->SpawnActor<ABuildingActor>(
			ABuildingActor::StaticClass(), Location, FRotator(0.f, Random.FRandRange(0.f, 360.f), 0.f), Params);
		if (Actor != nullptr)
		{
			Actor->InitFromDef(Palm, EBuildingMode::Decor);
		}
	};

	// Fileira no fundo da praia
	for (float Y = -2700.f; Y <= 2700.f; Y += 450.f)
	{
		SpawnPalm(FVector(-2750.f + Random.FRandRange(-120.f, 120.f), Y + Random.FRandRange(-100.f, 100.f), 0.f));
	}
	// Algumas perto do mar
	for (float Y = -2400.f; Y <= 2400.f; Y += 1200.f)
	{
		SpawnPalm(FVector(380.f + Random.FRandRange(-40.f, 40.f), Y + Random.FRandRange(-150.f, 150.f), 0.f));
	}
}

void AResortGameMode::SpawnStartingKiosk()
{
	UWorld* World = GetWorld();
	if (World == nullptr || Economy == nullptr)
	{
		return;
	}
	const FBuildingDef* Def = Economy->FindBuildingDef(TEXT("kiosk"));
	if (Def == nullptr)
	{
		UE_LOG(LogResort, Warning, TEXT("Definicao 'kiosk' nao encontrada: sem quiosque inicial."));
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// A porta fica no lado +X; yaw 180 vira a porta para quem chega pela entrada da praia.
	ABuildingActor* Kiosk = World->SpawnActor<ABuildingActor>(
		ABuildingActor::StaticClass(), FVector(-300.f, 0.f, 0.f), FRotator(0.f, 180.f, 0.f), Params);
	if (Kiosk != nullptr)
	{
		Kiosk->InitFromDef(*Def, EBuildingMode::Placed);
	}
}

// ------------------------------------------------------------- Partida

void AResortGameMode::SpawnGuest()
{
	UWorld* World = GetWorld();
	if (World == nullptr || Economy == nullptr)
	{
		return;
	}

	// Sorteia o tipo de hospede entre os liberados no nivel atual
	const int32 Tier = Economy->GetTier();
	TArray<const FGuestDef*> Pool;
	float Total = 0.f;
	for (const FGuestDef& D : Economy->GetGuestDefs())
	{
		if (D.MinTier <= Tier)
		{
			Pool.Add(&D);
			Total += D.Weight;
		}
	}
	if (Pool.Num() == 0)
	{
		return;
	}

	float Roll = FMath::FRandRange(0.f, Total);
	const FGuestDef* Chosen = Pool.Last();
	for (const FGuestDef* D : Pool)
	{
		Roll -= D->Weight;
		if (Roll <= 0.f)
		{
			Chosen = D;
			break;
		}
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AGuestCharacter* Guest = World->SpawnActor<AGuestCharacter>(
		AGuestCharacter::StaticClass(),
		FVector(GuestSpawnX, FMath::FRandRange(-1500.f, 1500.f), 120.f),
		FRotator::ZeroRotator, Params);
	if (Guest != nullptr)
	{
		Guest->InitGuest(*Chosen);
	}
}

void AResortGameMode::UpdateSun()
{
	if (SunComponent == nullptr || Economy == nullptr)
	{
		return;
	}

	// Manha (sol baixo) -> meio-dia (sol alto) -> fim de tarde (sol baixo do outro lado)
	const float Fraction = Economy->GetDayFraction();
	const float Pitch = -(15.f + Fraction * 150.f);
	SunComponent->SetWorldRotation(FRotator(Pitch, 40.f, 0.f));

	// Luz mais quente nas pontas do dia
	const float Edge = FMath::Abs(Fraction - 0.5f) * 2.f;
	const float Warm = Edge * Edge;
	const FLinearColor Noon(1.f, 0.96f, 0.9f);
	const FLinearColor Golden(1.f, 0.62f, 0.35f);
	SunComponent->SetLightColor(Noon + (Golden - Noon) * Warm);
}

void AResortGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Economy == nullptr)
	{
		return;
	}

	Economy->Tick(DeltaSeconds);

	SunUpdateTimer += DeltaSeconds;
	if (SunUpdateTimer >= 0.2f)
	{
		SunUpdateTimer = 0.f;
		UpdateSun();
	}

	// Chegada de hospedes: mais construcoes e prestigio atraem mais gente
	const int32 BuildingCount = Economy->GetBuildingCount();
	const int32 MaxGuests = FMath::Min(8 + 3 * BuildingCount, 60);
	if (Economy->GetGuestCount() < MaxGuests)
	{
		const float Interval = FMath::Clamp(14.f / (1.f + 0.35f * BuildingCount + Economy->GetPrestige() / 60.f), 2.f, 14.f);
		GuestSpawnTimer += DeltaSeconds;
		if (GuestSpawnTimer >= Interval)
		{
			GuestSpawnTimer = 0.f;
			SpawnGuest();
		}
	}
}
