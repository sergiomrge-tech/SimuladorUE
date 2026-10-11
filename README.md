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

## Como o jogo funciona

- **Hospedes** chegam pela entrada da praia, escolhem construções que podem pagar,
  usam, pagam e vão embora. Satisfação alta vira **prestígio**.
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
- Salvar/carregar o jogo, menu inicial, sons e música.
- Pacote (build) para Windows.
