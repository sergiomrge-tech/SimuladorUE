# Guia de arte final (assets)

O jogo já funciona com formas básicas. Para trocar por arte realista e estilizada, siga os passos.
Nada aqui exige mexer em código: você só aponta os arquivos.

## 1. Onde achar (precisa de login na Epic)

- **Fab** (fab.com): procure pacotes com licença que permita uso em jogo. Boas buscas:
  "beach props", "tropical palm", "beach bar", "kiosk", "sun lounger", "beach umbrella",
  "casual characters", "swimsuit characters", "hotel building modular", "swimming pool".
- **Quixel Megascans** (dentro do Fab): materiais de **areia** e rochas.
- Dica de licença: se o pacote não permite redistribuição, mantenha o repositório do jogo **privado**.

## 2. Importar na Unreal

1. No Fab, clique em "Add to project" e escolha este projeto (`SimuladorUe`).
2. Os arquivos aparecem no Content Browser (por exemplo, `Content/Fab/...`).
3. Clique com o botão direito no asset e escolha **Copy Reference**. Isso copia o caminho, como
   `/Game/Fab/Praia/SM_Quiosque.SM_Quiosque`.

## 3. Ligar ao jogo

1. Copie `Tools/assets.exemplo.json` para `Tools/assets.json`.
2. Troque os caminhos pelos seus (os que você copiou com Copy Reference).
3. Rode `python Tools/gerar_dados.py`.
4. Aperte Play. Se algo estiver torto, ajuste:
   - **Construções** (`buildings`): `MeshScale` (tamanho), `MeshYaw` (giro) e `MeshOffset` (posição).
     A porta do modelo deve ficar virada para o lado +X; use `MeshYaw` (90, 180, -90) até acertar.
   - **Pessoas** (`guests` e `staff`): `MeshScale`, `MeshYaw` (padrão -90) e `MeshOffsetZ` (altura dos pés).
     Use o Animation Blueprint do pacote em `AnimClass` (com `_C` no final) para andar com animação.

Funções de funcionários: `attendant` (atendente da loja), `waiter` (garçom), `cleaner` (faxineiro).
Hóspedes: `turista`, `familia`, `amigos`, `casal`, `vip`.
Construções: `kiosk`, `umbrella`, `lounger`, `beach_bar`, `volley`, `pool`, `restaurant`, `cabin`, `spa`, `hotel`.

Se um caminho estiver errado, o jogo usa a forma básica e escreve um aviso no Output Log.

## 4. Água e areia

- **Água:** ative o plugin **Water** (Edit > Plugins). Depois me peça para ligar o oceano ao jogo.
- **Areia:** ao importar um material de areia do Quixel, me mande o caminho do material; eu aplico no chão.

## 5. Estilizado

Com os assets no lugar, dá para suavizar o visual com pós-processo (contorno leve, cores vivas).
O jogo já tem Lumen, sombras virtuais e Nanite ligados em `Config/DefaultEngine.ini`.
