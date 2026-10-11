#include "ResortHUD.h"
#include "ResortCharacter.h"
#include "ResortEconomy.h"
#include "ResortGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"

void AResortHUD::DrawHUD()
{
	Super::DrawHUD();

	UResortEconomy* Economy = AResortGameMode::GetEconomy(this);
	if (Economy == nullptr || Canvas == nullptr || GEngine == nullptr)
	{
		return;
	}

	const float W = Canvas->SizeX;
	const float H = Canvas->SizeY;
	const float S = FMath::Clamp(H / 1080.f, 0.75f, 2.f);
	UFont* Small = GEngine->GetSmallFont();
	UFont* Medium = GEngine->GetMediumFont();
	UFont* Large = GEngine->GetLargeFont();

	const FLinearColor White(1.f, 1.f, 1.f);
	const FLinearColor Gold(1.f, 0.82f, 0.25f);
	const FLinearColor Green(0.6f, 1.f, 0.6f);
	const FLinearColor Dim(0.8f, 0.85f, 0.9f);

	// ------------------------------------------------ Painel principal
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), 20.f * S, 20.f * S, 400.f * S, 240.f * S);

	DrawText(FString::Printf(TEXT("R$ %s"), *FText::AsNumber(FMath::RoundToInt(Economy->GetMoney())).ToString()),
		Green, 36.f * S, 30.f * S, Large, 1.3f * S);

	DrawText(FString::Printf(TEXT("Nível %d: %s"), Economy->GetTier(), *Economy->GetTierName(Economy->GetTier())),
		Gold, 36.f * S, 82.f * S, Medium, 1.0f * S);

	// Barra de prestigio
	{
		const float Start = Economy->GetTierStartPrestige();
		const float Next = Economy->GetNextTierPrestige();
		float Fraction = 1.f;
		FString Label;
		if (Next > 0.f)
		{
			Fraction = FMath::Clamp((Economy->GetPrestige() - Start) / FMath::Max(1.f, Next - Start), 0.f, 1.f);
			Label = FString::Printf(TEXT("Prestígio %d / %d"), FMath::RoundToInt(Economy->GetPrestige()), FMath::RoundToInt(Next));
		}
		else
		{
			Label = FString::Printf(TEXT("Prestígio %d (máximo)"), FMath::RoundToInt(Economy->GetPrestige()));
		}
		const float BarX = 36.f * S;
		const float BarY = 120.f * S;
		const float BarW = 368.f * S;
		DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.18f), BarX, BarY, BarW, 14.f * S);
		DrawRect(FLinearColor(1.f, 0.8f, 0.2f, 0.95f), BarX, BarY, BarW * Fraction, 14.f * S);
		DrawText(Label, White, BarX, BarY + 18.f * S, Small, 1.0f * S);
	}

	{
		const float Hour = Economy->GetHourOfDay();
		const int32 HH = FMath::FloorToInt(Hour);
		const int32 MM = FMath::FloorToInt((Hour - HH) * 60.f);
		DrawText(FString::Printf(TEXT("Dia %d  %02d:%02d   Hóspedes: %d   Hoje: +R$ %d"),
			Economy->GetDay(), HH, MM, Economy->GetGuestCount(), FMath::RoundToInt(Economy->GetTodayEarned())),
			Dim, 36.f * S, 168.f * S, Small, 1.0f * S);
		DrawText(FString::Printf(TEXT("Equipe: %d garçom, %d faxineiro"), Economy->GetWaiters(), Economy->GetCleaners()),
			Dim, 36.f * S, 192.f * S, Small, 1.0f * S);
		DrawText(FString::Printf(TEXT("F2 garçom R$ %d   F3 faxineiro R$ %d"), Economy->GetHireCost(true), Economy->GetHireCost(false)),
			FLinearColor(0.7f, 0.75f, 0.8f), 36.f * S, 214.f * S, Small, 0.9f * S);
	}

	// ------------------------------------------------ Missao e evento do dia
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), 20.f * S, 252.f * S, 400.f * S, Economy->HasMission() ? 84.f * S : 40.f * S);
		DrawText(Economy->HasMission() ? TEXT("MISSÃO") : TEXT("MISSÕES"), Gold, 36.f * S, 258.f * S, Small, 1.0f * S);
		DrawText(Economy->GetMissionText(), White, 36.f * S, 278.f * S, Small, 0.9f * S);
		if (Economy->HasMission())
		{
			int32 Cur = 0, Tgt = 1, Reward = 0;
			Economy->GetMissionProgress(Cur, Tgt, Reward);
			DrawText(FString::Printf(TEXT("%d / %d   Recompensa: R$ %d"), FMath::Min(Cur, Tgt), Tgt, Reward),
				Green, 36.f * S, 302.f * S, Small, 0.9f * S);
		}
		const FString EventName = Economy->GetEventName();
		if (!EventName.IsEmpty())
		{
			DrawText(EventName, FLinearColor(0.6f, 0.9f, 1.f), 36.f * S, (Economy->HasMission() ? 346.f : 300.f) * S, Small, 1.0f * S);
		}
	}

	// ------------------------------------------------ Mira
	DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.85f), W * 0.5f - 2.f, H * 0.5f - 2.f, 4.f, 4.f);

	// ------------------------------------------------ Menu de construcao
	const AResortCharacter* Player = Cast<AResortCharacter>(GetOwningPawn());
	if (Player != nullptr && Player->IsBuildMode())
	{
		const TArray<const FBuildingDef*> List = Economy->GetUnlockedBuildings();
		const float PanelW = 420.f * S;
		const float PanelX = W - PanelW - 20.f * S;
		const float RowH = 26.f * S;
		const float PanelH = 110.f * S + RowH * FMath::Max(1, List.Num());
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), PanelX, 20.f * S, PanelW, PanelH);

		DrawText(TEXT("MODO CONSTRUÇÃO"), Gold, PanelX + 16.f * S, 28.f * S, Medium, 1.0f * S);
		DrawText(TEXT("Q/E ou roda: trocar   R: girar   Clique: construir   X: demolir"),
			Dim, PanelX + 16.f * S, 58.f * S, Small, 0.9f * S);

		float Y = 88.f * S;
		for (int32 i = 0; i < List.Num(); ++i)
		{
			const FBuildingDef* D = List[i];
			const bool bSelected = (i == FMath::Clamp(Player->GetSelectedIndex(), 0, List.Num() - 1));
			const bool bAffordable = Economy->CanAfford(static_cast<float>(D->Cost));
			if (bSelected)
			{
				DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.16f), PanelX + 8.f * S, Y - 2.f * S, PanelW - 16.f * S, RowH);
			}
			const FLinearColor Color = bAffordable ? White : FLinearColor(1.f, 0.5f, 0.45f);
			DrawText(FString::Printf(TEXT("%s%s"), bSelected ? TEXT("> ") : TEXT("   "), *D->DisplayName),
				Color, PanelX + 16.f * S, Y, Small, 1.0f * S);
			DrawText(FString::Printf(TEXT("R$ %d"), D->Cost), Color, PanelX + PanelW - 110.f * S, Y, Small, 1.0f * S);
			Y += RowH;
		}

		const FString& Status = Player->GetStatusText();
		if (!Status.IsEmpty())
		{
			DrawText(Status, FLinearColor(1.f, 0.4f, 0.35f), PanelX + 16.f * S, Y + 10.f * S, Small, 1.0f * S);
		}
		else if (Player->IsPlacementValid())
		{
			DrawText(TEXT("Pode construir aqui"), Green, PanelX + 16.f * S, Y + 10.f * S, Small, 1.0f * S);
		}

		const float Next = Economy->GetNextTierPrestige();
		if (Next > 0.f)
		{
			DrawText(FString::Printf(TEXT("Próximo nível (%s) com %d de prestígio"),
				*Economy->GetTierName(Economy->GetTier() + 1), FMath::RoundToInt(Next)),
				Dim, PanelX + 16.f * S, Y + 34.f * S, Small, 0.9f * S);
		}
	}

	// ------------------------------------------------ Gerenciamento da construcao sob a mira
	if (Player != nullptr && !Player->IsBuildMode() && Player->GetFocusedBuilding() != nullptr)
	{
		const ABuildingActor* B = Player->GetFocusedBuilding();
		const FBuildingDef& BD = B->GetDef();
		const TArray<const FUpgradeDef*> Ups = Economy->GetUpgradesFor(BD);
		const float PanelW = 470.f * S;
		const float PanelX = W - PanelW - 20.f * S;
		const float RowH = 26.f * S;
		const float PanelH = 200.f * S + RowH * (Ups.Num() + 5);
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), PanelX, 20.f * S, PanelW, PanelH);

		DrawText(B->GetTitle(), Gold, PanelX + 16.f * S, 28.f * S, Medium, 1.0f * S);
		const bool bLodging = BD.Category == TEXT("lodging");
		DrawText(FString::Printf(TEXT("Estrelas: %d de 5"), B->GetStars()), Gold, PanelX + PanelW - 150.f * S, 32.f * S, Small, 1.0f * S);
		DrawText(FString::Printf(TEXT("%s: R$ %d   Manutenção: R$ %d/dia"),
			bLodging ? TEXT("Diária") : TEXT("Renda por visita"),
			FMath::RoundToInt(B->GetIncome()), FMath::RoundToInt(B->GetUpkeep())),
			Green, PanelX + 16.f * S, 58.f * S, Small, 1.0f * S);
		DrawText(FString::Printf(TEXT("Investido em melhorias: R$ %d   Ocupação: %d/%d"),
			B->GetInvested(), B->GetOccupants(), B->GetCapacity()),
			Dim, PanelX + 16.f * S, 82.f * S, Small, 0.95f * S);

		float Y = 112.f * S;
		DrawText(FString::Printf(TEXT("Preço: %d%%  (- / + para mudar; caro rende mais por visita, mas cansa o hóspede)"),
			FMath::RoundToInt(B->GetPriceMultiplier() * 100.f)), Dim, PanelX + 16.f * S, Y, Small, 0.85f * S);
		Y += RowH;
		if (B->IsLodging())
		{
			FString Rooms = TEXT("Quartos (nível 0-3): ");
			for (int32 r = 0; r < B->GetRoomCount(); ++r)
			{
				Rooms += FString::Printf(TEXT("%d "), B->GetRoomLevel(r));
			}
			DrawText(Rooms, Dim, PanelX + 16.f * S, Y, Small, 0.9f * S);
			Y += RowH;
			const int32 RoomCost = B->GetNextRoomCost();
			DrawText(TEXT("[7] Reformar um quarto (+30% na diária dele)"), RoomCost >= 0 ? White : Dim, PanelX + 16.f * S, Y, Small, 1.0f * S);
			DrawText(RoomCost >= 0 ? FString::Printf(TEXT("R$ %d"), RoomCost) : FString(TEXT("máximo")),
				RoomCost >= 0 ? White : Dim, PanelX + PanelW - 120.f * S, Y, Small, 1.0f * S);
			Y += RowH;
		}
		if (B->IsFoodShop())
		{
			FString Menu = TEXT("Vende: água de coco");
			for (const FString& UId : B->GetUpgradeIds())
			{
				if (const FUpgradeDef* UD = Economy->FindUpgradeDef(UId))
				{
					Menu += TEXT(", ") + UD->Name.ToLower();
				}
			}
			DrawText(Menu, Dim, PanelX + 16.f * S, Y, Small, 0.9f * S);
			Y += RowH;
		}
		for (int32 i = 0; i < Ups.Num(); ++i)
		{
			const FUpgradeDef& U = *Ups[i];
			const bool bOwned = B->HasUpgrade(U.Id);
			const int32 Cost = U.CostFor(BD);
			const bool bAfford = Economy->CanAfford(static_cast<float>(Cost));
			const FLinearColor C = bOwned ? Green : (bAfford ? White : FLinearColor(1.f, 0.5f, 0.45f));
			DrawText(FString::Printf(TEXT("[%d] %s  +%d%% renda"), i + 1, *U.Name, FMath::RoundToInt(U.IncomeBonus * 100.f)),
				C, PanelX + 16.f * S, Y, Small, 1.0f * S);
			DrawText(bOwned ? FString(TEXT("instalado")) : FString::Printf(TEXT("R$ %d"), Cost),
				C, PanelX + PanelW - 120.f * S, Y, Small, 1.0f * S);
			Y += RowH;
		}
		if (B->IsFoodShop())
		{
			const bool bCanHire = B->CanHireAttendant();
			DrawText(FString::Printf(TEXT("[5] Atendente (%d/%d)"), B->GetAttendants(), B->GetDef().Capacity),
				bCanHire ? White : Dim, PanelX + 16.f * S, Y, Small, 1.0f * S);
			DrawText(bCanHire ? FString::Printf(TEXT("R$ %d"), B->GetAttendantCost()) : FString(TEXT("completo")),
				bCanHire ? White : Dim, PanelX + PanelW - 120.f * S, Y, Small, 1.0f * S);
			Y += RowH;
			DrawText(TEXT("[6] Caixa registradora (-30% tempo)"), B->HasRegister() ? Green : White, PanelX + 16.f * S, Y, Small, 1.0f * S);
			DrawText(B->HasRegister() ? FString(TEXT("instalado")) : FString::Printf(TEXT("R$ %d"), B->GetRegisterCost()),
				B->HasRegister() ? Green : White, PanelX + PanelW - 120.f * S, Y, Small, 1.0f * S);
			Y += RowH;
		}
		if (Ups.Num() > 0)
		{
			DrawText(TEXT("Aperte o número para comprar a melhoria"), Dim, PanelX + 16.f * S, Y + 8.f * S, Small, 0.9f * S);
		}
	}

	// ------------------------------------------------ Tela de abertura
	if (Player != nullptr && Player->IsTitleScreen())
	{
		DrawRect(FLinearColor(0.02f, 0.12f, 0.2f, 0.88f), 0.f, 0.f, W, H);
		DrawText(TEXT("SIMULADOR DE RESORT"), Gold, W * 0.5f - 330.f * S, H * 0.5f - 230.f * S, Large, 2.0f * S);
		DrawText(TEXT("Comece com um quiosque na areia e cresça até um resort completo."), White,
			W * 0.5f - 330.f * S, H * 0.5f - 130.f * S, Medium, 1.0f * S);
		DrawText(TEXT("1) Aperte B para construir.  2) Olhe para uma construção e use 1-7 para melhorá-la."), Dim,
			W * 0.5f - 330.f * S, H * 0.5f - 70.f * S, Small, 1.0f * S);
		DrawText(TEXT("3) Ganhe prestígio com hóspedes felizes para liberar novos níveis e ampliar o terreno."), Dim,
			W * 0.5f - 330.f * S, H * 0.5f - 40.f * S, Small, 1.0f * S);
		DrawText(TEXT("WASD mover   Shift correr   P pausa   F5 salvar   F9 carregar   F7 novo jogo"), Dim,
			W * 0.5f - 330.f * S, H * 0.5f - 10.f * S, Small, 1.0f * S);
		DrawText(TEXT("Aperte ENTER para começar"), Green, W * 0.5f - 150.f * S, H * 0.5f + 70.f * S, Medium, 1.2f * S);
		return;
	}

	// ------------------------------------------------ Pausa
	if (GetWorld() != nullptr && GetWorld()->IsPaused())
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), 0.f, 0.f, W, H);
		DrawText(TEXT("PAUSADO"), Gold, W * 0.5f - 90.f * S, H * 0.5f - 120.f * S, Large, 1.6f * S);
		DrawText(TEXT("P: continuar   F5: salvar   F9: carregar   F7: novo jogo"), White, W * 0.5f - 270.f * S, H * 0.5f - 40.f * S, Medium, 1.0f * S);
		DrawText(TEXT("WASD mover   B construir   1-4 melhorias (olhando a construção)   F2/F3 contratar"), Dim, W * 0.5f - 330.f * S, H * 0.5f, Small, 1.0f * S);
	}

	// ------------------------------------------------ Avisos
	{
		float Y = H - 60.f * S;
		const TArray<FResortNotification>& Notes = Economy->GetNotifications();
		for (int32 i = Notes.Num() - 1; i >= 0; --i)
		{
			FLinearColor C = Notes[i].Color;
			C.A = FMath::Clamp(Notes[i].TimeLeft, 0.f, 1.f);
			DrawText(Notes[i].Text, C, 24.f * S, Y, Medium, 1.0f * S);
			Y -= 30.f * S;
		}
	}

	// ------------------------------------------------ Dicas
	{
		const UWorld* World = GetWorld();
		const bool bFirstSeconds = World != nullptr && World->GetTimeSeconds() < 30.f;
		const bool bBuilding = Player != nullptr && Player->IsBuildMode();
		FString Hint = TEXT("WASD mover   Shift correr   B: construir   F5: salvar   F9: carregar");
		if (bFirstSeconds && !bBuilding)
		{
			Hint = TEXT("Aperte B para construir mais coisas na praia. WASD mover, Shift correr.");
		}
		DrawText(Hint, FLinearColor(1.f, 1.f, 1.f, 0.85f), W - 560.f * S, H - 40.f * S, Small, 1.0f * S);
	}
}
