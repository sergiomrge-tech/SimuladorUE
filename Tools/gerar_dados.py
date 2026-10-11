#!/usr/bin/env python3
"""Gera Content/Data/Buildings.json e Content/Data/Guests.json.

Edite os numeros aqui (ou direto nos JSONs) para balancear o jogo e rode:
    python Tools/gerar_dados.py

O JSON sai em ASCII puro (acentos viram \\uXXXX), o que evita problemas de
codificacao ao ler o arquivo dentro da Unreal.

Categorias de construcao: food, shade, entertainment, lodging, wellness
Niveis do resort: 1 a 20 (a cada nivel uma construcao nova; fases a cada 4 niveis)
Medidas em cm.
"""
import json
from pathlib import Path

BUILDINGS = [
    # id, nome, categoria, nivel minimo (1-20), custo, renda/visita, segundos de uso, capacidade,
    # satisfacao, manutencao/dia, tamanho (x,y,z), cor (r,g,b)
    ("kiosk", "Quiosque", "food", 1, 700, 14, 4, 3, 0.12, 8, (300, 260, 260), (0.78, 0.52, 0.28)),
    ("umbrella", "Guarda-sol com Mesa", "shade", 1, 120, 4, 14, 4, 0.10, 1, (240, 240, 230), (0.95, 0.25, 0.20)),
    ("lounger", "Espreguiçadeira", "shade", 1, 90, 6, 20, 1, 0.15, 1, (200, 70, 55), (0.95, 0.95, 0.90)),
    ("ice_cream", "Sorveteria", "food", 2, 900, 11, 3, 3, 0.10, 10, (220, 200, 230), (0.98, 0.72, 0.80)),
    ("volley", "Quadra de Vôlei", "entertainment", 3, 1800, 9, 18, 6, 0.22, 6, (900, 500, 230), (0.95, 0.88, 0.70)),
    ("beach_bar", "Bar de Praia", "food", 4, 2200, 26, 5, 4, 0.15, 18, (420, 300, 280), (0.20, 0.62, 0.70)),
    ("massage", "Tenda de Massagem", "wellness", 5, 3200, 30, 14, 2, 0.20, 14, (320, 300, 240), (0.90, 0.82, 0.68)),
    ("shower", "Chuveirão e Vestiário", "shade", 6, 3800, 8, 6, 6, 0.18, 12, (300, 250, 260), (0.60, 0.72, 0.80)),
    ("surf", "Escola de Surf", "entertainment", 7, 6500, 45, 22, 5, 0.28, 30, (450, 300, 220), (0.15, 0.65, 0.65)),
    ("pool", "Piscina", "entertainment", 8, 9000, 24, 20, 10, 0.25, 40, (700, 450, 40), (0.05, 0.55, 0.75)),
    ("restaurant", "Restaurante", "food", 9, 14000, 60, 9, 12, 0.22, 70, (800, 600, 320), (0.90, 0.82, 0.65)),
    ("jet_ski", "Aluguel de Jet Ski", "entertainment", 10, 20000, 90, 22, 4, 0.30, 60, (500, 350, 180), (0.95, 0.55, 0.15)),
    ("cabin", "Chalé", "lodging", 11, 30000, 170, 25, 2, 0.30, 90, (450, 400, 320), (0.70, 0.50, 0.32)),
    ("spa", "Spa", "wellness", 12, 45000, 120, 18, 4, 0.30, 130, (700, 600, 320), (0.75, 0.85, 0.80)),
    ("pizzeria", "Pizzaria", "food", 13, 40000, 80, 8, 14, 0.22, 110, (700, 500, 300), (0.85, 0.40, 0.25)),
    ("gym", "Academia", "wellness", 14, 70000, 70, 20, 6, 0.25, 150, (800, 500, 320), (0.55, 0.60, 0.68)),
    ("tennis", "Quadra de Tênis", "entertainment", 15, 90000, 95, 24, 4, 0.32, 140, (1100, 600, 60), (0.30, 0.60, 0.35)),
    ("hotel", "Bloco do Resort", "lodging", 16, 220000, 520, 30, 20, 0.40, 500, (1400, 700, 900), (0.96, 0.93, 0.86)),
    ("pool_bar", "Bar da Piscina", "food", 17, 150000, 110, 6, 8, 0.25, 220, (600, 450, 300), (0.25, 0.70, 0.75)),
    ("suite", "Suíte Presidencial", "lodging", 18, 450000, 800, 30, 2, 0.50, 400, (1000, 800, 420), (0.97, 0.94, 0.88)),
    ("marina", "Marina e Iate Clube", "entertainment", 19, 900000, 300, 28, 8, 0.45, 700, (1600, 900, 500), (0.90, 0.92, 0.95)),
    ("stage", "Palco de Shows", "entertainment", 20, 1500000, 400, 35, 30, 0.50, 1000, (1500, 1000, 500), (0.55, 0.25, 0.60)),
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

# Arquivo opcional do usuario (nao precisa editar este script): Tools/assets.json.
# Veja Tools/assets.exemplo.json. Chaves: "buildings", "guests", "staff".
def _load_user_assets():
    path = Path(__file__).resolve().parent / "assets.json"
    if not path.exists():
        return
    data = json.loads(path.read_text(encoding="utf-8"))
    for bid, fields in data.get("buildings", {}).items():
        MESH_OVERRIDES[bid] = fields
    for gid, fields in data.get("guests", {}).items():
        GUEST_MODELS[gid] = fields
    for role, fields in data.get("staff", {}).items():
        STAFF_MODELS[role] = fields
    print(f"assets.json lido: {len(data.get('buildings', {}))} construcoes, "
          f"{len(data.get('guests', {}))} hospedes, {len(data.get('staff', {}))} funcionarios")


GUESTS = [
    # id, nome, nivel minimo, orcamento min, max, velocidade, visitas min, max, peso,
    # categoria preferida, cor da camisa
    ("turista", "Turista", 1, 40, 110, 220, 2, 3, 5, "shade", (0.20, 0.50, 0.90)),
    ("mochileiro", "Mochileiro", 1, 20, 60, 230, 1, 2, 4, "food", (0.35, 0.65, 0.35)),
    ("familia", "Família", 5, 90, 260, 200, 3, 4, 3, "entertainment", (0.90, 0.60, 0.20)),
    ("amigos", "Grupo de Amigos", 5, 60, 200, 240, 3, 5, 3, "food", (0.85, 0.25, 0.45)),
    ("influencer", "Influenciador", 8, 200, 700, 240, 3, 5, 2, "entertainment", (0.95, 0.45, 0.70)),
    ("casal", "Casal", 9, 150, 500, 190, 3, 4, 2, "wellness", (0.30, 0.75, 0.55)),
    ("vip", "Hóspede VIP", 13, 400, 1500, 180, 3, 5, 1, "lodging", (0.95, 0.85, 0.30)),
    ("celebridade", "Celebridade", 18, 1500, 5000, 170, 3, 4, 0.5, "lodging", (0.85, 0.2, 0.85)),
]


def main():
    _load_user_assets()
    out_dir = Path(__file__).resolve().parent.parent / "Content" / "Data"
    out_dir.mkdir(parents=True, exist_ok=True)

    buildings = []
    for (bid, name, cat, tier, cost, income, secs, cap, sat, upkeep, size, color) in BUILDINGS:
        buildings.append({
            "Id": bid,
            "DisplayName": name,
            "Category": cat,
            "MinLevel": tier,
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
            "MinLevel": tier,
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
