#!/usr/bin/env python3
"""Gera Content/Data/Buildings.json e Content/Data/Guests.json.

Edite os numeros aqui (ou direto nos JSONs) para balancear o jogo e rode:
    python Tools/gerar_dados.py

O JSON sai em ASCII puro (acentos viram \\uXXXX), o que evita problemas de
codificacao ao ler o arquivo dentro da Unreal.

Categorias de construcao: food, shade, entertainment, lodging, wellness
Niveis: 1 Quiosque, 2 Barraca de Praia, 3 Beach Club, 4 Pousada, 5 Resort
Medidas em cm.
"""
import json
from pathlib import Path

BUILDINGS = [
    # id, nome, categoria, nivel, custo, renda/visita, segundos de uso, capacidade,
    # satisfacao, manutencao/dia, tamanho (x,y,z), cor (r,g,b)
    ("kiosk", "Quiosque", "food", 1, 700, 14, 4, 3, 0.12, 8, (300, 260, 260), (0.78, 0.52, 0.28)),
    ("umbrella", "Guarda-sol com Mesa", "shade", 1, 120, 4, 14, 4, 0.10, 1, (240, 240, 230), (0.95, 0.25, 0.20)),
    ("lounger", "Espreguiçadeira", "shade", 1, 90, 6, 20, 1, 0.15, 1, (200, 70, 55), (0.95, 0.95, 0.90)),
    ("beach_bar", "Bar de Praia", "food", 2, 1400, 26, 5, 4, 0.15, 18, (420, 300, 280), (0.20, 0.62, 0.70)),
    ("volley", "Quadra de Vôlei", "entertainment", 2, 1800, 9, 18, 6, 0.22, 6, (900, 500, 230), (0.95, 0.88, 0.70)),
    ("pool", "Piscina", "entertainment", 3, 4800, 16, 20, 10, 0.25, 25, (700, 450, 40), (0.05, 0.55, 0.75)),
    ("restaurant", "Restaurante", "food", 3, 6500, 48, 9, 12, 0.22, 45, (800, 600, 320), (0.90, 0.82, 0.65)),
    ("cabin", "Chalé", "lodging", 4, 9500, 140, 25, 2, 0.30, 40, (450, 400, 320), (0.70, 0.50, 0.32)),
    ("spa", "Spa", "wellness", 4, 14000, 90, 18, 4, 0.30, 70, (700, 600, 320), (0.75, 0.85, 0.80)),
    ("hotel", "Bloco do Resort", "lodging", 5, 42000, 420, 30, 20, 0.40, 220, (1400, 700, 900), (0.96, 0.93, 0.86)),
]

# Modelos 3D finais (opcional). Depois de importar/baixar um asset na Unreal, aponte aqui:
#   "kiosk": {"Mesh": "/Game/Fab/Praia/SM_Quiosque.SM_Quiosque", "MeshScale": 1.0,
#             "MeshYaw": 0, "MeshOffset": [0, 0, 0]},
# O caminho aparece no Content Browser (botao direito no asset > Copy Reference).
# Se o modelo nao for achado, o jogo usa as formas basicas.
MESH_OVERRIDES = {}

# Modelos 3D de pessoas (opcional). Aponte para um SkeletalMesh e, se quiser animacao,
# para o Animation Blueprint (note o "_C" no final). Exemplo:
#   "attendant": {"Mesh": "/Game/Fab/Pessoas/SK_Atendente.SK_Atendente",
#                 "AnimClass": "/Game/Fab/Pessoas/ABP_Pessoa.ABP_Pessoa_C",
#                 "MeshScale": 1.0, "MeshYaw": -90, "MeshOffsetZ": 0},
# Funcoes: attendant (atendente da loja), waiter (garcom), cleaner (faxineiro).
STAFF_MODELS = {}

# O mesmo vale para hospedes: GUEST_MODELS["turista"] = {...mesmos campos...}
GUEST_MODELS = {}

UPGRADES = [
    # id, nome, categoria, construcao (ou "" para toda a categoria), custo (x custo da
    # construcao), bonus de renda, bonus de satisfacao, aumento de manutencao (fracao)
    # Comida: cada melhoria e um item novo no cardapio (agua de coco ja vem de base)
    ("bebidas", "Bebidas e Sucos", "food", "", 0.15, 0.25, 0.02, 0.05),
    ("lanches", "Lanches", "food", "", 0.25, 0.35, 0.03, 0.08),
    ("petiscos", "Petiscos e Porções", "food", "", 0.30, 0.40, 0.03, 0.10),
    ("drinks", "Drinks e Caipirinhas", "food", "", 0.40, 0.45, 0.02, 0.12),
    ("cushions", "Almofadas e Toalhas", "shade", "", 0.50, 0.20, 0.05, 0.10),
    ("cooler", "Frigobar Portátil", "shade", "", 0.80, 0.30, 0.03, 0.15),
    ("equipment", "Equipamentos Novos", "entertainment", "", 0.30, 0.20, 0.05, 0.08),
    ("lights", "Iluminação e Som", "entertainment", "", 0.25, 0.15, 0.04, 0.06),
    ("bed", "Cama Premium", "lodging", "", 0.30, 0.30, 0.05, 0.06),
    ("wardrobe", "Armário Embutido", "lodging", "", 0.18, 0.12, 0.03, 0.04),
    ("minibar", "Frigobar e Cafeteira", "lodging", "", 0.22, 0.20, 0.03, 0.08),
    ("tv", "TV e Wi-Fi", "lodging", "", 0.15, 0.15, 0.04, 0.05),
    ("massage", "Massagem Premium", "wellness", "", 0.35, 0.25, 0.04, 0.10),
    ("aroma", "Aromaterapia", "wellness", "", 0.20, 0.12, 0.06, 0.05),
    ("sauna", "Sauna", "wellness", "", 0.45, 0.30, 0.03, 0.15),
]

GUESTS = [
    # id, nome, nivel minimo, orcamento min, max, velocidade, visitas min, max, peso,
    # categoria preferida, cor da camisa
    ("turista", "Turista", 1, 40, 110, 220, 2, 3, 5, "shade", (0.20, 0.50, 0.90)),
    ("familia", "Família", 2, 90, 260, 200, 3, 4, 3, "entertainment", (0.90, 0.60, 0.20)),
    ("amigos", "Grupo de Amigos", 2, 60, 200, 240, 3, 5, 3, "food", (0.85, 0.25, 0.45)),
    ("casal", "Casal", 3, 150, 500, 190, 3, 4, 2, "wellness", (0.30, 0.75, 0.55)),
    ("vip", "Hóspede VIP", 4, 400, 1500, 180, 3, 5, 1, "lodging", (0.95, 0.85, 0.30)),
]


def main():
    out_dir = Path(__file__).resolve().parent.parent / "Content" / "Data"
    out_dir.mkdir(parents=True, exist_ok=True)

    buildings = []
    for (bid, name, cat, tier, cost, income, secs, cap, sat, upkeep, size, color) in BUILDINGS:
        buildings.append({
            "Id": bid,
            "DisplayName": name,
            "Category": cat,
            "Tier": tier,
            "Cost": cost,
            "IncomePerVisit": income,
            "ServiceSeconds": secs,
            "Capacity": cap,
            "Satisfaction": sat,
            "UpkeepPerDay": upkeep,
            "Size": list(size),
            "Color": list(color),
        })
        buildings[-1].update(MESH_OVERRIDES.get(bid, {}))

    guests = []
    for (gid, name, tier, bmin, bmax, speed, vmin, vmax, weight, pref, shirt) in GUESTS:
        guests.append({
            "Id": gid,
            "DisplayName": name,
            "MinTier": tier,
            "BudgetMin": bmin,
            "BudgetMax": bmax,
            "WalkSpeed": speed,
            "MinVisits": vmin,
            "MaxVisits": vmax,
            "Weight": weight,
            "PreferredCategory": pref,
            "ShirtColor": list(shirt),
        })
        guests[-1].update(GUEST_MODELS.get(gid, {}))

    upgrades = []
    for (uid, name, cat, bid, cf, inc, sat, upk) in UPGRADES:
        u = {"Id": uid, "Name": name, "Category": cat, "CostFactor": cf,
             "IncomeBonus": inc, "SatisfactionBonus": sat, "UpkeepFactor": upk}
        if bid:
            u["BuildingId"] = bid
        upgrades.append(u)
    (out_dir / "Upgrades.json").write_text(
        json.dumps({"Upgrades": upgrades}, indent=2) + "\n", encoding="utf-8")

    staff = [dict(Id=role, **fields) for role, fields in STAFF_MODELS.items()]
    (out_dir / "Staff.json").write_text(
        json.dumps({"Staff": staff}, indent=2) + "\n", encoding="utf-8")

    # ensure_ascii=True (padrao): acentos viram \uXXXX
    (out_dir / "Buildings.json").write_text(
        json.dumps({"Buildings": buildings}, indent=2) + "\n", encoding="utf-8")
    (out_dir / "Guests.json").write_text(
        json.dumps({"Guests": guests}, indent=2) + "\n", encoding="utf-8")
    print(f"OK: {len(buildings)} construcoes e {len(guests)} tipos de hospede em {out_dir}")


if __name__ == "__main__":
    main()
