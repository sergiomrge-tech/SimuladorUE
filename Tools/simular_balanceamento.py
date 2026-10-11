#!/usr/bin/env python3
"""Simula a economia do jogo (sem a Unreal) para conferir o balanceamento.

Le Content/Data/*.json, reproduz as formulas do codigo C++ (chegada de hospedes, escolha de
construcao, pagamento, satisfacao, prestigio, manutencao) e joga com um "jogador automatico"
que sempre compra a melhor opcao que consegue pagar.

Uso:  python Tools/simular_balanceamento.py calibrar [K_PRESTIGE]   (acha os limites dos 20 niveis)
      python Tools/simular_balanceamento.py [dias] [sementes] [K_PRESTIGE] [SPAWN_PRESTIGE] [limites, ex.: 0,40,120,300,700]
"""
import json
import random
import sys
from pathlib import Path

DATA = Path(__file__).resolve().parent.parent / "Content" / "Data"
DAY = 240.0
THRESH = [0, 25, 96, 243, 435, 725, 1050, 1395, 1757, 2176, 2602, 3076, 3526, 4029, 4532, 5059, 5588, 6141, 6730, 7317]
TRAVEL = 7.0          # segundos medios andando ate a construcao
QUEUE_PATIENCE = 8.0  # igual ao codigo
MAX_AGE = 240.0
# Constantes de balanceamento (iguais as do codigo C++)
LIMITS = {"hotel": 3, "spa": 1, "restaurant": 2, "pool": 1, "gym": 1, "suite": 3, "jet_ski": 2, "massage": 2,
          "ice_cream": 2, "surf": 2, "tennis": 2, "marina": 1, "stage": 1, "pizzeria": 2, "pool_bar": 2, "shower": 2, "cabin": 6}
K_PRESTIGE = 0.25     # prestigio por ponto de satisfacao acima de 0.45 (ResortEconomy::OnGuestLeft)
SPAWN_PRESTIGE = 300.0 # divisor do prestigio na chegada de hospedes (ResortGameMode::Tick)
THRESH = THRESH


def load():
    b = json.loads((DATA / "Buildings.json").read_text())["Buildings"]
    g = json.loads((DATA / "Guests.json").read_text())["Guests"]
    u = json.loads((DATA / "Upgrades.json").read_text())["Upgrades"]
    return b, g, u


class Building:
    def __init__(self, d):
        self.d = d
        self.ups = []
        self.attendants = 1
        self.register = False
        self.price = 1.0
        self.rooms = [0] * d["Capacity"] if d["Category"] == "lodging" else []
        self.busy = []   # lista de (fim, indice_quarto)

    @property
    def food(self):
        return self.d["Category"] == "food"

    def cap(self):
        return min(self.d["Capacity"], 1 + self.attendants) if self.food else self.d["Capacity"]

    def service(self):
        return self.d["ServiceSeconds"] * (0.7 if self.register else 1.0)

    def income(self, waiters, room=-1):
        inc = sum(u["IncomeBonus"] for u in self.ups)
        enclosed = self.d["Category"] in ("food", "lodging", "wellness")
        mult = 1 + 0.06 * waiters if enclosed else 1.0
        room_mult = 1 + 0.3 * (self.rooms[room] if 0 <= room < len(self.rooms) else 0)
        return self.d["IncomePerVisit"] * (1 + inc) * mult * room_mult * self.price

    def sat(self, cleaners):
        s = self.d["Satisfaction"] + sum(u["SatisfactionBonus"] for u in self.ups)
        return s + 0.012 * cleaners - (self.price - 1) * 0.12

    def upkeep(self):
        extra = 15 * max(0, self.attendants - 1) if self.food else 0
        return self.d["UpkeepPerDay"] * (1 + sum(u["UpkeepFactor"] for u in self.ups)) + extra

    def stars(self):
        return min(5, 1 + len(self.ups))


class Guest:
    def __init__(self, gd, t):
        self.g = gd
        self.budget = random.uniform(gd["BudgetMin"], gd["BudgetMax"])
        self.visits = random.randint(gd["MinVisits"], gd["MaxVisits"])
        self.sat = 0.5
        self.visited = set()
        self.t0 = t
        self.state = "pick"
        self.timer = 0.0
        self.target = None
        self.room = -1


def tier_of(prestige):
    """Nivel (1 a 20) para o prestigio atual."""
    t = 1
    for i in range(1, 20):
        if prestige >= THRESH[i]:
            t = i + 1
    return t


def simulate(days, seed, verbose=False):
    random.seed(seed)
    bdefs, gdefs, udefs = load()
    money, prestige = 1000.0, 0.0
    waiters = cleaners = 0
    blds = []
    kiosk = next(b for b in bdefs if b["Id"] == "kiosk")
    blds.append(Building(kiosk))
    guests = []
    spawn_timer = 0.0
    t = 0.0
    reach = {}
    today = 0.0
    log = []
    next_buy = 0.0
    bankrupt_days = 0

    while t < days * DAY:
        dt = 1.0
        t += dt
        tier = tier_of(prestige)
        if tier not in reach:
            reach[tier] = t / DAY

        # chegada de hospedes
        n = len(blds)
        spawn_timer -= dt
        max_guests = min(8 + 3 * n, 20 + 5 * tier)
        if spawn_timer <= 0:
            spawn_timer = min(14, max(2, 14 / (1 + 0.35 * n + prestige / SPAWN_PRESTIGE)))
            if len(guests) < max_guests:
                pool = [g for g in gdefs if g["MinLevel"] <= tier]
                gd = random.choices(pool, weights=[g["Weight"] for g in pool])[0]
                guests.append(Guest(gd, t))

        # hospedes
        for gu in list(guests):
            age = t - gu.t0
            if age > MAX_AGE and gu.state != "leave":
                if gu.state == "use":
                    gu.target.busy = [x for x in gu.target.busy if x[2] is not gu]
                gu.state = "leave"
                gu.timer = 6
            if gu.state == "pick":
                if gu.visits <= 0:
                    gu.state, gu.timer = "leave", 6
                    continue
                cands, ws = [], []
                for b in blds:
                    if b.income(waiters) > gu.budget + 0.01:
                        continue
                    w = 1.0
                    if gu.g["PreferredCategory"] == b.d["Category"]:
                        w *= 2.5
                    w *= 1 + 0.15 * (b.stars() - 1) * (2 if gu.g["BudgetMax"] >= 400 else 1)
                    if b.d["Id"] in gu.visited:
                        w *= 0.25
                    if len(b.busy) >= b.cap():
                        w *= 0.4
                    cands.append(b)
                    ws.append(w)
                if not cands:
                    if not gu.visited:
                        gu.sat = max(0, gu.sat - 0.2)
                    gu.state, gu.timer = "leave", 6
                    continue
                gu.target = random.choices(cands, weights=ws)[0]
                gu.state, gu.timer = "walk", TRAVEL * random.uniform(0.6, 1.4)
            elif gu.state == "walk":
                gu.timer -= dt
                if gu.timer <= 0:
                    gu.state, gu.timer = "wait", 0.0
            elif gu.state == "wait":
                b = gu.target
                if len(b.busy) < b.cap():
                    room = -1
                    if b.rooms:
                        free = [i for i in range(len(b.rooms)) if all(x[1] != i for x in b.busy)]
                        room = max(free, key=lambda i: b.rooms[i])
                    gu.room = room
                    b.busy.append((t, room, gu))
                    gu.state, gu.timer = "use", b.service()
                else:
                    gu.timer += dt
                    if gu.timer > QUEUE_PATIENCE:
                        gu.sat = max(0, gu.sat - 0.15)
                        gu.visits -= 1
                        gu.state = "pick"
            elif gu.state == "use":
                gu.timer -= dt
                if gu.timer <= 0:
                    b = gu.target
                    pay = min(b.income(waiters, gu.room), gu.budget)
                    gu.budget -= pay
                    net = pay * 0.75 if b.food else pay
                    money += net
                    today += net
                    gu.sat = min(1.5, gu.sat + b.sat(cleaners))
                    gu.visited.add(b.d["Id"])
                    b.busy = [x for x in b.busy if x[2] is not gu]
                    gu.visits -= 1
                    gu.state = "pick"
            elif gu.state == "leave":
                gu.timer -= dt
                if gu.timer <= 0:
                    prestige += max(0, gu.sat - 0.45) * K_PRESTIGE
                    guests.remove(gu)

        # fim do dia: manutencao
        if int(t) % int(DAY) == 0:
            upkeep = sum(b.upkeep() for b in blds) + waiters * 25 + cleaners * 20
            if money >= upkeep:
                money -= upkeep
            else:
                money = 0
                prestige = max(0, prestige - 2)
                bankrupt_days += 1
            log.append((int(t // DAY), money, prestige, len(blds), tier_of(prestige), today))
            today = 0.0

        # jogador automatico: compra a melhor opcao a cada 5s
        if t >= next_buy:
            next_buy = t + 5
            reserve = 2 * (sum(b.upkeep() for b in blds) + waiters * 25 + cleaners * 20)
            opts = []
            visits_per_day = 40.0  # estimativa grosseira de uso
            for d in bdefs:
                if d["MinLevel"] > tier:
                    continue
                cnt = sum(1 for b in blds if b.d["Id"] == d["Id"])
                limit = LIMITS.get(d["Id"], 4)
                if cnt >= limit:
                    continue
                gain = d["IncomePerVisit"] * d["Capacity"] / (d["ServiceSeconds"] + TRAVEL) * 3600 / 60 * 0.1
                opts.append(((gain + 6 * d["Satisfaction"] * 100) / max(1, d["Cost"] + 20 * d["UpkeepPerDay"]), d["Cost"], ("build", d)))
            for b in blds:
                for u in udefs:
                    if u["Category"] != b.d["Category"] or u["Id"] in [x["Id"] for x in b.ups] or len(b.ups) >= 4:
                        continue
                    cost = max(10, round(b.d["Cost"] * u["CostFactor"] / 10) * 10)
                    gain = b.d["IncomePerVisit"] * u["IncomeBonus"] * b.d["Capacity"] / (b.d["ServiceSeconds"] + TRAVEL) * 60
                    opts.append((gain / cost * 1.0, cost, ("up", b, u)))
                if b.food and b.attendants < b.d["Capacity"]:
                    gain = b.d["IncomePerVisit"] / (b.d["ServiceSeconds"] + TRAVEL) * 60
                    opts.append((gain / 150 * 0.8, 150, ("att", b)))
            if waiters < 5:
                opts.append((0.03, 300, ("waiter",)))
            if cleaners < 5:
                opts.append((0.02, 250, ("cleaner",)))
            opts = [o for o in opts if money - o[1] >= reserve]
            if opts:
                opts.sort(key=lambda o: -o[0])
                score, cost, act = opts[0]
                money -= cost
                if act[0] == "build":
                    blds.append(Building(act[1]))
                elif act[0] == "up":
                    act[1].ups.append(act[2])
                elif act[0] == "att":
                    act[1].attendants += 1
                elif act[0] == "waiter":
                    waiters += 1
                else:
                    cleaners += 1

    return reach, log, bankrupt_days, blds, waiters, cleaners


def calibrar(k_prestige, fim_dias=330.0, expoente=1.4, iteracoes=7):
    """Ajusta os limites de prestigio dos 20 niveis para o jogo perfeito chegar ao nivel 20 no dia
    `fim_dias` (um humano leva mais). Imprime a lista para colar em ResortEconomy.cpp."""
    global K_PRESTIGE
    K_PRESTIGE = k_prestige
    alvo = [fim_dias * (((k - 1) / 19.0) ** expoente) for k in range(1, 21)]
    horizonte = int(fim_dias * 1.35)
    for it in range(iteracoes):
        reach, log, bd, blds, w, c = simulate(horizonte, 1)
        prest = {row[0]: row[2] for row in log}
        novo = [0.0]
        for k in range(2, 21):
            d = max(1, int(round(alvo[k - 1])))
            while d not in prest and d > 1:
                d -= 1
            v = prest.get(d, novo[-1] + 5)
            novo.append(max(v, novo[-1] + 3))
        # mistura com o anterior para estabilizar
        THRESH[:] = [round(0.5 * a + 0.5 * b) if i else 0 for i, (a, b) in enumerate(zip(THRESH, novo))]
        print(f"iteracao {it + 1}: nivel 20 em {reach.get(20, float('nan')):.0f}  limites {THRESH}", flush=True)
    return THRESH


def main():
    global K_PRESTIGE, SPAWN_PRESTIGE
    if len(sys.argv) > 3:
        K_PRESTIGE = float(sys.argv[3])
    if len(sys.argv) > 4:
        SPAWN_PRESTIGE = float(sys.argv[4])
    if len(sys.argv) > 5:
        THRESH[:] = [float(x) for x in sys.argv[5].split(',')]
    if len(sys.argv) > 1 and sys.argv[1] == "calibrar":
        k = float(sys.argv[2]) if len(sys.argv) > 2 else 0.25
        calibrar(k)
        return
    days = int(sys.argv[1]) if len(sys.argv) > 1 else 40
    seeds = int(sys.argv[2]) if len(sys.argv) > 2 else 5
    agg = {}
    last = None
    brk = 0
    for s in range(seeds):
        reach, log, bd, blds, w, c = simulate(days, s)
        for k, v in reach.items():
            agg.setdefault(k, []).append(v)
        brk += bd
        last = (log, blds, w, c)
    print(f"Simulacao de {days} dias, {seeds} sementes (1 dia = 4 min reais)")
    for t in sorted(agg):
        vals = agg[t]
        d = sum(vals) / len(vals)
        print(f"  Nivel {t}: dia {d:.1f} (~{d * 4 / 60:.1f} h de jogo perfeito; {len(vals)}/{seeds} partidas)")
    print(f"  Dias sem dinheiro para a manutencao (total): {brk}")
    log, blds, w, c = last
    print("  Ultima partida, a cada 5 dias: dia, dinheiro, prestigio, construcoes, nivel, lucro do dia")
    for row in log[max(0, days // 12 - 1)::max(1, days // 12)]:
        print("   ", row[0], round(row[1]), round(row[2]), row[3], row[4], round(row[5]))
    print("  Construcoes finais:", ", ".join(f"{b.d['Id']}({len(b.ups)})" for b in blds), f"| garcons {w}, faxineiros {c}")


if __name__ == "__main__":
    main()
