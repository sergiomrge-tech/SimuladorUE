#include "ResortGameMode.h"
#include "BuildingActor.h"
#include "GuestCharacter.h"
#include "StaffCharacter.h"
#include "ResortAudio.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
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
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

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

	// Continua o jogo salvo, se houver
	if (LoadResort(true) && Economy != nullptr)
	{
		LastAutosaveDay = Economy->GetDay();
		Economy->PushNotification(TEXT("Jogo carregado. (F5 salva, F9 recarrega, F7 novo jogo)"), FLinearColor(0.6f, 1.f, 0.6f));
	}
}

namespace
{
	FString SavePath()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("resort_save.json"));
	}
}

void AResortGameMode::SaveResort(bool bQuiet)
{
	if (Economy == nullptr)
	{
		return;
	}

	TArray<TSharedPtr<FJsonValue>> List;
	for (const TWeakObjectPtr<ABuildingActor>& P : Economy->GetBuildings())
	{
		if (!P.IsValid())
		{
			continue;
		}
		TSharedRef<FJsonObject> B = MakeShared<FJsonObject>();
		B->SetStringField(TEXT("Id"), P->GetDef().Id);
		B->SetNumberField(TEXT("X"), P->GetActorLocation().X);
		B->SetNumberField(TEXT("Y"), P->GetActorLocation().Y);
		B->SetNumberField(TEXT("Yaw"), P->GetActorRotation().Yaw);
		TArray<TSharedPtr<FJsonValue>> Ups;
		for (const FString& UId : P->GetUpgradeIds())
		{
			Ups.Add(MakeShared<FJsonValueString>(UId));
		}
		B->SetArrayField(TEXT("Upgrades"), Ups);
		B->SetNumberField(TEXT("Attendants"), P->GetAttendants());
		B->SetNumberField(TEXT("Price"), P->GetPriceMultiplier());
		TArray<TSharedPtr<FJsonValue>> Rooms;
		for (int32 RL : P->GetRoomLevels())
		{
			Rooms.Add(MakeShared<FJsonValueNumber>(RL));
		}
		B->SetArrayField(TEXT("Rooms"), Rooms);
		B->SetBoolField(TEXT("Register"), P->HasRegister());
		List.Add(MakeShared<FJsonValueObject>(B));
	}

	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("Money"), Economy->GetMoney());
	Root->SetNumberField(TEXT("Prestige"), Economy->GetPrestige());
	Root->SetNumberField(TEXT("Day"), Economy->GetDay());
	Root->SetNumberField(TEXT("DayClock"), Economy->GetDayClock());
	Root->SetNumberField(TEXT("Mission"), Economy->GetMissionIndex());
	Root->SetNumberField(TEXT("GuestsServed"), Economy->GetGuestsServed());
	Root->SetNumberField(TEXT("Waiters"), Economy->GetWaiters());
	Root->SetNumberField(TEXT("Cleaners"), Economy->GetCleaners());
	Root->SetArrayField(TEXT("Buildings"), List);

	FString Text;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Text);
	FJsonSerializer::Serialize(Root, Writer);

	const bool bOk = FFileHelper::SaveStringToFile(Text, *SavePath());
	if (!bQuiet || !bOk)
	{
		Economy->PushNotification(bOk ? TEXT("Jogo salvo.") : TEXT("Não foi possível salvar o jogo."),
			bOk ? FLinearColor(0.6f, 1.f, 0.6f) : FLinearColor(1.f, 0.4f, 0.3f));
	}
}

bool AResortGameMode::LoadResort(bool bQuiet)
{
	UWorld* World = GetWorld();
	FString Text;
	if (World == nullptr || Economy == nullptr || !FFileHelper::LoadFileToString(Text, *SavePath()))
	{
		if (!bQuiet && Economy != nullptr)
		{
			Economy->PushNotification(TEXT("Nenhum jogo salvo encontrado."), FLinearColor(1.f, 0.7f, 0.3f));
		}
		return false;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogResort, Warning, TEXT("Save invalido: %s"), *SavePath());
		return false;
	}

	// Remove o que existe agora (construcoes e hospedes)
	TArray<ABuildingActor*> OldBuildings;
	for (const TWeakObjectPtr<ABuildingActor>& P : Economy->GetBuildings())
	{
		if (P.IsValid())
		{
			OldBuildings.Add(P.Get());
		}
	}
	for (ABuildingActor* B : OldBuildings)
	{
		B->Destroy();
	}
	TArray<AGuestCharacter*> OldGuests;
	for (TActorIterator<AGuestCharacter> It(World); It; ++It)
	{
		OldGuests.Add(*It);
	}
	for (AGuestCharacter* G : OldGuests)
	{
		G->Destroy();
	}

	double Money = 1000.0, Prestige = 0.0, Day = 1.0, Clock = 0.0;
	Root->TryGetNumberField(TEXT("Money"), Money);
	Root->TryGetNumberField(TEXT("Prestige"), Prestige);
	Root->TryGetNumberField(TEXT("Day"), Day);
	Root->TryGetNumberField(TEXT("DayClock"), Clock);
	Economy->SetState(static_cast<float>(Money), static_cast<float>(Prestige), static_cast<int32>(Day), static_cast<float>(Clock));

	double MissionSaved = 0.0, ServedSaved = 0.0;
	Root->TryGetNumberField(TEXT("Mission"), MissionSaved);
	Root->TryGetNumberField(TEXT("GuestsServed"), ServedSaved);
	Economy->SetMissionState(static_cast<int32>(MissionSaved), static_cast<int32>(ServedSaved));
	double WaitersSaved = 0.0, CleanersSaved = 0.0;
	Root->TryGetNumberField(TEXT("Waiters"), WaitersSaved);
	Root->TryGetNumberField(TEXT("Cleaners"), CleanersSaved);
	Economy->SetStaff(static_cast<int32>(WaitersSaved), static_cast<int32>(CleanersSaved));

	const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
	if (Root->TryGetArrayField(TEXT("Buildings"), Arr) && Arr != nullptr)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		for (const TSharedPtr<FJsonValue>& V : *Arr)
		{
			const TSharedPtr<FJsonObject> O = V.IsValid() ? V->AsObject() : nullptr;
			FString Id;
			double X = 0.0, Y = 0.0, Yaw = 0.0;
			if (!O.IsValid() || !O->TryGetStringField(TEXT("Id"), Id))
			{
				continue;
			}
			O->TryGetNumberField(TEXT("X"), X);
			O->TryGetNumberField(TEXT("Y"), Y);
			O->TryGetNumberField(TEXT("Yaw"), Yaw);
			const FBuildingDef* Def = Economy->FindBuildingDef(Id);
			if (Def == nullptr)
			{
				continue;
			}
			ABuildingActor* B = World->SpawnActor<ABuildingActor>(
				ABuildingActor::StaticClass(), FVector(X, Y, 0.0), FRotator(0.0, Yaw, 0.0), Params);
			if (B != nullptr)
			{
				B->InitFromDef(*Def, EBuildingMode::Placed);
				double Price = 1.0;
				if (O->TryGetNumberField(TEXT("Price"), Price))
				{
					B->SetPriceMultiplier(static_cast<float>(Price));
				}
				const TArray<TSharedPtr<FJsonValue>>* RoomArr = nullptr;
				if (O->TryGetArrayField(TEXT("Rooms"), RoomArr) && RoomArr != nullptr)
				{
					TArray<int32> Levels;
					for (const TSharedPtr<FJsonValue>& RV : *RoomArr)
					{
						Levels.Add(static_cast<int32>(RV->AsNumber()));
					}
					B->SetRoomLevels(Levels);
				}
				double Att = 1.0;
				bool bReg = false;
				O->TryGetNumberField(TEXT("Attendants"), Att);
				O->TryGetBoolField(TEXT("Register"), bReg);
				if (B->IsFoodShop())
				{
					B->SetAttendants(static_cast<int32>(Att));
					if (bReg)
					{
						B->InstallRegister();
					}
				}
				const TArray<TSharedPtr<FJsonValue>>* Ups = nullptr;
				if (O->TryGetArrayField(TEXT("Upgrades"), Ups) && Ups != nullptr)
				{
					for (const TSharedPtr<FJsonValue>& UV : *Ups)
					{
						if (const FUpgradeDef* U = Economy->FindUpgradeDef(UV->AsString()))
						{
							B->ApplyUpgrade(*U);
						}
					}
				}
			}
		}
	}

	if (!bQuiet)
	{
		Economy->PushNotification(TEXT("Jogo carregado."), FLinearColor(0.6f, 1.f, 0.6f));
	}
	return true;
}

void AResortGameMode::NewResort()
{
	IFileManager::Get().Delete(*SavePath());
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
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

	// Mar em faixas (raso -> fundo) para dar profundidade, e espuma na beira
	auto AddWave = [this](AStaticMeshActor* Actor, const FVector& Size, float Phase, int32 Kind)
	{
		if (Actor != nullptr)
		{
			FWavePart P;
			P.Actor = Actor;
			P.Base = Actor->GetActorLocation();
			P.BaseSize = Size;
			P.Phase = Phase;
			P.Kind = Kind;
			WaveParts.Add(P);
		}
	};
	{
		const FVector ShallowSize(2000.f, 40000.f, 4.f);
		const FVector DeepSize(3500.f, 40000.f, 4.f);
		AddWave(SpawnBox(FVector(SeaEdgeX + 1000.f, 0.f, -13.f), ShallowSize, FLinearColor(0.15f, 0.7f, 0.7f), true, false), ShallowSize, 0.f, 0);
		AddWave(SpawnBox(FVector(SeaEdgeX + 3000.f + 250.f, 0.f, -15.f), DeepSize, FLinearColor(0.05f, 0.5f, 0.6f), true, false), DeepSize, 1.7f, 0);

		// Espuma da beira que avanca e recua
		const FVector FoamSize(80.f, 40000.f, 4.f);
		AddWave(SpawnBox(FVector(SeaEdgeX + 40.f, 0.f, -11.f), FoamSize, FLinearColor(0.95f, 0.98f, 0.98f), true, false), FoamSize, 0.f, 1);

		// Cristas de onda que viajam do fundo ate a praia
		for (int32 i = 0; i < 4; ++i)
		{
			const FVector CrestSize(40.f, 40000.f, 6.f);
			AddWave(SpawnBox(FVector(SeaEdgeX + 800.f, 0.f, -9.f), CrestSize, FLinearColor(0.9f, 0.97f, 0.97f), true, false), CrestSize, i * 2.f, 2);
		}

		// Areia molhada escura que acompanha a onda subindo na praia
		const FVector WetSize(100.f, 6000.f, 2.f);
		AddWave(SpawnBox(FVector(SeaEdgeX - 50.f, 0.f, 1.5f), WetSize, FLinearColor(0.62f, 0.5f, 0.34f), true, false), WetSize, 0.f, 3);
	}

	// Areia molhada perto da agua e passarela de madeira da entrada ate o quiosque (so visual)
	SpawnBox(FVector(325.f, 0.f, 1.f), FVector(350.f, 6000.f, 2.f), FLinearColor(0.78f, 0.66f, 0.45f), true, false);
	SpawnBox(FVector(-1725.f, 0.f, 2.f), FVector(2450.f, 220.f, 4.f), FLinearColor(0.55f, 0.38f, 0.22f), true, false);

	BuildSandDetail();

	// Paredes invisiveis: limitam a area jogavel (3500 x 6000 cm)
	const float WallHeight = 600.f;
	const float WallZ = WallHeight * 0.5f;
	const float WallThickness = 40.f;
	const float LengthX = SeaEdgeX - BackEdgeX;
	const float CenterX = (SeaEdgeX + BackEdgeX) * 0.5f;
	SpawnBox(FVector(SeaEdgeX + WallThickness * 0.5f, 0.f, WallZ), FVector(WallThickness, SideEdgeY * 2.f + 80.f, WallHeight), FLinearColor::White, false, true);
	SpawnBox(FVector(BackEdgeX - WallThickness * 0.5f, 0.f, WallZ), FVector(WallThickness, SideEdgeY * 2.f + 80.f, WallHeight), FLinearColor::White, false, true);
	// As paredes laterais e as cercas acompanham o terreno liberado (UpdatePlot as posiciona)
	WallLeft = SpawnBox(FVector(CenterX, SideEdgeY, WallZ), FVector(LengthX + 80.f, WallThickness, WallHeight), FLinearColor::White, false, true);
	WallRight = SpawnBox(FVector(CenterX, -SideEdgeY, WallZ), FVector(LengthX + 80.f, WallThickness, WallHeight), FLinearColor::White, false, true);
	FenceLeft = SpawnBox(FVector(CenterX, SideEdgeY, 18.f), FVector(LengthX, 8.f, 36.f), FLinearColor(0.95f, 0.93f, 0.85f), true, false);
	FenceRight = SpawnBox(FVector(CenterX, -SideEdgeY, 18.f), FVector(LengthX, 8.f, 36.f), FLinearColor(0.95f, 0.93f, 0.85f), true, false);
	UpdatePlot(false);
}

void AResortGameMode::BuildSandDetail()
{
	// Areia mais "real": manchas de tons diferentes, marcas de onda (ondulacoes), conchas e pedras.
	FRandomStream Rand(20241011);
	const FLinearColor Dark(0.84f, 0.7f, 0.48f);
	const FLinearColor Light(0.98f, 0.88f, 0.67f);

	for (int32 i = 0; i < 140; ++i)
	{
		const float X = Rand.FRandRange(BackEdgeX + 100.f, SeaEdgeX - 100.f);
		const float Y = Rand.FRandRange(-SideEdgeY + 100.f, SideEdgeY - 100.f);
		const float Z = 0.6f + 0.004f * i;
		AStaticMeshActor* Patch = SpawnBox(FVector(X, Y, Z),
			FVector(Rand.FRandRange(150.f, 500.f), Rand.FRandRange(150.f, 500.f), 1.2f),
			FMath::Lerp(Dark, Light, Rand.FRand()), true, false);
		if (Patch != nullptr)
		{
			Patch->SetActorRotation(FRotator(0.f, Rand.FRandRange(0.f, 180.f), 0.f));
		}
	}

	// Ondulacoes paralelas a beira do mar, mais fortes perto da agua
	for (int32 i = 0; i < 90; ++i)
	{
		const float X = Rand.FRandRange(-600.f, SeaEdgeX - 120.f);
		const float Y = Rand.FRandRange(-SideEdgeY + 200.f, SideEdgeY - 200.f);
		AStaticMeshActor* Ripple = SpawnBox(FVector(X, Y, 0.9f),
			FVector(Rand.FRandRange(60.f, 140.f), Rand.FRandRange(250.f, 700.f), 1.f),
			FLinearColor(0.8f, 0.66f, 0.45f), true, false);
		if (Ripple != nullptr)
		{
			Ripple->SetActorRotation(FRotator(0.f, Rand.FRandRange(-10.f, 10.f), 0.f));
		}
	}

	// Conchas pequenas
	for (int32 i = 0; i < 50; ++i)
	{
		const bool bPink = Rand.FRand() > 0.5f;
		AStaticMeshActor* Shell = SpawnBox(
			FVector(Rand.FRandRange(-300.f, SeaEdgeX - 60.f), Rand.FRandRange(-SideEdgeY + 100.f, SideEdgeY - 100.f), 1.8f),
			FVector(Rand.FRandRange(5.f, 9.f), Rand.FRandRange(4.f, 7.f), 3.f),
			bPink ? FLinearColor(0.95f, 0.7f, 0.68f) : FLinearColor(0.97f, 0.95f, 0.9f), true, false);
		if (Shell != nullptr)
		{
			Shell->SetActorRotation(FRotator(0.f, Rand.FRandRange(0.f, 360.f), 0.f));
		}
	}

	// Pedras nas pontas da praia
	for (int32 i = 0; i < 8; ++i)
	{
		const float Side = (i % 2 == 0) ? 1.f : -1.f;
		const float S = Rand.FRandRange(60.f, 130.f);
		AStaticMeshActor* Rock = SpawnBox(
			FVector(Rand.FRandRange(-2600.f, 400.f), Side * Rand.FRandRange(2500.f, 2900.f), S * 0.3f),
			FVector(S, S * Rand.FRandRange(0.7f, 1.2f), S * 0.6f),
			FLinearColor(0.38f, 0.37f, 0.36f) * Rand.FRandRange(0.8f, 1.2f), true, false);
		if (Rock != nullptr)
		{
			Rock->SetActorRotation(FRotator(Rand.FRandRange(-8.f, 8.f), Rand.FRandRange(0.f, 360.f), Rand.FRandRange(-8.f, 8.f)));
		}
	}
}

float AResortGameMode::PlotHalfYForTier(int32 Tier)
{
	static const float Halves[5] = { 900.f, 1500.f, 2100.f, 2600.f, 3000.f };
	return Halves[FMath::Clamp(Tier, 1, 5) - 1];
}

float AResortGameMode::GetPlotHalfY(const UObject* WorldContext)
{
	const UResortEconomy* Eco = GetEconomy(WorldContext);
	return PlotHalfYForTier(Eco != nullptr ? Eco->GetTier() : 1);
}

void AResortGameMode::UpdatePlot(bool bAnnounce)
{
	if (Economy == nullptr)
	{
		return;
	}
	const int32 Tier = Economy->GetTier();
	if (Tier == PlotTier)
	{
		return;
	}
	PlotTier = Tier;
	const float Half = PlotHalfYForTier(Tier);
	const float Thickness = 40.f;
	if (AStaticMeshActor* A = WallLeft.Get())
	{
		A->SetActorLocation(FVector(A->GetActorLocation().X, Half + Thickness * 0.5f, A->GetActorLocation().Z));
	}
	if (AStaticMeshActor* A = WallRight.Get())
	{
		A->SetActorLocation(FVector(A->GetActorLocation().X, -Half - Thickness * 0.5f, A->GetActorLocation().Z));
	}
	if (AStaticMeshActor* A = FenceLeft.Get())
	{
		A->SetActorLocation(FVector(A->GetActorLocation().X, Half, A->GetActorLocation().Z));
	}
	if (AStaticMeshActor* A = FenceRight.Get())
	{
		A->SetActorLocation(FVector(A->GetActorLocation().X, -Half, A->GetActorLocation().Z));
	}
	if (bAnnounce)
	{
		Economy->PushNotification(
			FString::Printf(TEXT("Terreno ampliado! Agora a praia tem %d m de largura."), FMath::RoundToInt(Half * 2.f / 100.f)),
			FLinearColor(0.6f, 1.f, 0.9f));
	}
}

void AResortGameMode::SyncStaff()
{
	UWorld* World = GetWorld();
	if (World == nullptr || Economy == nullptr)
	{
		return;
	}

	auto Sync = [this, World](TArray<TWeakObjectPtr<AStaffCharacter>>& List, int32 Wanted, EStaffRole Role)
	{
		List.RemoveAll([](const TWeakObjectPtr<AStaffCharacter>& P) { return !P.IsValid(); });
		while (List.Num() > Wanted)
		{
			if (AStaffCharacter* Extra = List.Last().Get())
			{
				Extra->Destroy();
			}
			List.Pop();
		}
		while (List.Num() < Wanted)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AStaffCharacter* Staff = World->SpawnActor<AStaffCharacter>(
				FVector(-1500.f + FMath::FRandRange(-100.f, 100.f), FMath::FRandRange(-200.f, 200.f), 120.f),
				FRotator::ZeroRotator, Params);
			if (Staff == nullptr)
			{
				break;
			}
			Staff->InitStaff(Role);
			List.Add(Staff);
		}
	};
	Sync(WaiterActors, Economy->GetWaiters(), EStaffRole::Waiter);
	Sync(CleanerActors, Economy->GetCleaners(), EStaffRole::Cleaner);
}

void AResortGameMode::UpdateWaves(float DeltaSeconds)
{
	WaveTime += DeltaSeconds;
	for (const FWavePart& P : WaveParts)
	{
		AStaticMeshActor* A = P.Actor.Get();
		if (A == nullptr)
		{
			continue;
		}
		FVector Loc = P.Base;
		FVector Scale(1.f, 1.f, 1.f);
		switch (P.Kind)
		{
		case 0: // mar sobe e desce de leve
			Loc.Z += 2.f * FMath::Sin(WaveTime * 1.3f + P.Phase);
			break;
		case 1: // espuma avanca e recua
			Loc.X += 70.f * FMath::Sin(WaveTime * 0.9f + P.Phase);
			break;
		case 2: // crista viaja do fundo ate a beira e recomeca
		{
			const float F = FMath::Frac((WaveTime + P.Phase) / 8.f);
			Loc.X = SeaEdgeX + 900.f * (1.f - F) + 20.f;
			Loc.Z += 3.f * FMath::Sin(F * PI);
			Scale.X = 0.7f + 1.3f * F;
			Scale.Z = 0.4f + 1.0f * FMath::Sin(F * PI);
			break;
		}
		default: // areia molhada avanca com a espuma
		{
			const float Reach = 60.f + 70.f * (0.5f + 0.5f * FMath::Sin(WaveTime * 0.9f + P.Phase));
			Scale.X = Reach / P.BaseSize.X;
			Loc.X = SeaEdgeX - Reach * 0.5f;
			break;
		}
		}
		A->SetActorLocation(Loc);
		A->SetActorScale3D(FVector(P.BaseSize.X * Scale.X, P.BaseSize.Y * Scale.Y, P.BaseSize.Z * Scale.Z) / 100.0);
	}
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
	FogComp = Fog;
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
		FVector(GuestSpawnX, FMath::FRandRange(-1.f, 1.f) * FMath::Min(1500.f, GetPlotHalfY(this) - 150.f), 120.f),
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

	// Dia de chuva: sol fraco e neblina
	const bool bRain = Economy->GetEvent() == EResortEvent::Rain;
	SunComponent->SetIntensity(bRain ? 3.f : 9.f);
	if (FogComp != nullptr)
	{
		FogComp->SetFogDensity(bRain ? 0.02f : 0.004f);
	}
}

void AResortGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Economy == nullptr)
	{
		return;
	}

	Economy->Tick(DeltaSeconds);
	UpdateWaves(DeltaSeconds);
	UpdatePlot(true);

	// Som do mar (continuo) e gaivotas de vez em quando
	AudioTimer -= DeltaSeconds;
	if (AudioTimer <= 0.f)
	{
		AudioTimer = 0.5f;
		if (SeaWave == nullptr && GetWorld() != nullptr && GetWorld()->GetNetMode() != NM_DedicatedServer)
		{
			SeaWave = ResortAudio::CreateSeaAmbience();
			SeaAudio = UGameplayStatics::SpawnSound2D(this, SeaWave, 0.5f);
		}
		ResortAudio::KeepSeaAmbienceFed(SeaWave);
	}
	GullTimer -= DeltaSeconds;
	if (GullTimer <= 0.f)
	{
		GullTimer = FMath::FRandRange(14.f, 30.f);
		ResortAudio::Play(this, EResortSound::Gull, 0.25f);
	}

	StaffSyncTimer -= DeltaSeconds;
	if (StaffSyncTimer <= 0.f)
	{
		StaffSyncTimer = 1.f;
		SyncStaff();
	}

	// Salva sozinho a cada novo dia
	if (Economy->GetDay() != LastAutosaveDay)
	{
		LastAutosaveDay = Economy->GetDay();
		SaveResort(true);
	}

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
		const float Interval = FMath::Clamp(14.f / (1.f + 0.35f * BuildingCount + Economy->GetPrestige() / 300.f), 2.f, 14.f) / Economy->GetSpawnMultiplier();
		GuestSpawnTimer += DeltaSeconds;
		if (GuestSpawnTimer >= Interval)
		{
			GuestSpawnTimer = 0.f;
			SpawnGuest();
		}
	}
}
