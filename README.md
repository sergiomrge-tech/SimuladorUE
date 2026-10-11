# SimuladorUE

Simulador de resort em **primeira pessoa**, feito em **Unreal Engine 5.8 (C++)**.
Você começa com um quiosque numa faixa de praia e vai crescendo até um resort completo.

> **Estado atual: protótipo 1 (jogabilidade).** O visual usa formas básicas como
> provisório. A arte final (realista e estilizada, sem low poly) entra depois, com
> assets do Fab/Quixel, Nanite, Lumen e materiais estilizados.

## Como rodar (Windows)

1. Instale a Unreal Engine 5.8 e o Visual Studio 2022 com as cargas
   **"Desenvolvimento de jogos com C++"** e o **Windows SDK**.
2. Baixe o projeto (`git clone` ou ZIP).
3. Clique com o botão direito em `SimuladorUe.uproject` → **Switch Unreal Engine version…**
   (escolha a 5.8) → depois **Generate Visual Studio project files**.
4. Dê dois cliques em `SimuladorUe.uproject`. A Unreal pergunta se quer compilar: **Sim**.
5. Dentro do editor: **File → New Level → Empty Level** e salve em
   `Content/Maps/Praia` (o nome `Praia` é o que o `DefaultEngine.ini` espera).
6. Aperte **Play**. Todo o cenário é montado por código.

Se algo falhar na compilação, copie a mensagem de erro e me mande.

## Controles

| Tecla | Ação |
|---|---|
| WASD / mouse | andar / olhar |
| Shift / Espaço | correr / pular |
| **B** | liga/desliga o modo construção |
| Q / E ou roda do mouse | troca a construção selecionada |
| R | gira a construção em 90° |
| Clique esquerdo | constrói |
| X | demole a construção na mira (devolve 50%) |
| F5 / F9 | salva / carrega o jogo (também salva sozinho a cada dia) |
| F7 | novo jogo (apaga o save) |
| 1-4 | compra melhoria da construção na mira |
| F2 / F3 | contrata garçom / faxineiro |
| P | pausa |

O save fica em `Saved/resort_save.json`. Ao abrir o jogo, ele continua de onde parou.

## Como o jogo funciona

- Uma bolinha sobre a cabeça do hóspede mostra o humor (verde feliz, amarelo ok, vermelho irritado); filas longas geram reclamações.
- **Hospedes** chegam pela entrada da praia, escolhem construções que podem pagar,
  usam, pagam e vão embora. Satisfação alta vira **prestígio**.
- **Melhorias:** olhe para uma construção (fora do modo construção) e aperte **1-4** para
  instalar melhorias (cardápio, cozinha, cama, armário, frigobar, sauna...). Cada uma custa
  uma parte do valor da construção, aumenta a renda (nos quartos, a diária), a satisfação e a
  manutenção. Uma esfera dourada no telhado marca cada melhoria. Os dados ficam em
  `Content/Data/Upgrades.json`.
- **Quiosque e comida:** vende água de coco de base; melhorias adicionam bebidas, lanches, petiscos e
  drinks ao cardápio. Com a mira no quiosque, **5** contrata atendente (mais clientes ao mesmo tempo;
  o primeiro é o dono, de graça) e **6** instala o caixa registradora (atendimento 30% mais rápido).
- **Equipe:** garçons (+6% de renda em comida, hospedagem e spa) e faxineiros (+satisfação por visita),
  até 5 de cada, com salário diário.
- **Estrelas:** cada melhoria sobe a construção em 1 estrela (até 5); hóspedes ricos preferem as de mais estrelas.
- **Prestígio** sobe o **nível** do resort e libera novas construções e hóspedes mais ricos:
  1 Quiosque → 2 Barraca de Praia → 3 Beach Club → 4 Pousada → 5 Resort.
- Cada construção tem **manutenção diária**. Sem dinheiro para pagar, o prestígio cai.
- O dia dura 4 minutos reais (das 8h às 20h) e o sol se move no céu.

## Balanceamento (sem mexer em código)

Os números ficam em `Content/Data/Buildings.json` e `Content/Data/Guests.json`.
Para editar com acentos facilmente, altere `Tools/gerar_dados.py` e rode:

```
python Tools/gerar_dados.py
```

## Trocar uma forma básica por um modelo 3D final

1. Importe ou adicione o modelo ao projeto (Fab, ou arraste o arquivo no Content Browser).
2. Botão direito no asset → **Copy Reference** e cole o caminho em `MESH_OVERRIDES`
   dentro de `Tools/gerar_dados.py` (há um exemplo comentado).
3. Rode `python Tools/gerar_dados.py`. Ajuste `MeshScale`, `MeshYaw` e `MeshOffset`
   até encaixar na caixa de colisão (`Size`).

Se o modelo não for encontrado, o jogo volta às formas básicas e avisa no Output Log.

## Estrutura

```
Source/SimuladorUe/
  ResortGameMode   monta praia, mar, céu/sol, palmeiras, quiosque inicial; gera hóspedes
  ResortCharacter  jogador em 1ª pessoa + modo construção (entrada criada por código)
  BuildingActor    uma construção (visual provisório montado por formas básicas)
  GuestCharacter   hóspede (escolhe, anda, usa, paga, vai embora)
  ResortEconomy    dinheiro, prestígio, nível, relógio, lista de construções
  ResortHUD        interface desenhada no Canvas (sem UMG)
  ResortData       leitura dos JSON em Content/Data
Content/Data/      Buildings.json, Guests.json
Config/            renderização (Lumen/Nanite), canal de colisão, entrada
```

## Próximos passos

- Visual final: assets do Fab/Quixel, água com o plugin Water, materiais estilizados,
  pós-processo com contorno suave.
- Mais hóspedes com personalidade, filas e reclamações.
- Menu inicial, sons e música.
- Pacote (build) para Windows.
