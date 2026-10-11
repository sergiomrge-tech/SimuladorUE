# Roteiro de teste (primeira vez)

1. `git pull`, clique direito em `SimuladorUe.uproject` > **Generate Visual Studio project files**.
2. Abra o projeto e deixe compilar. **Se der erro**, copie o texto do Output/Error List e mande.
3. Crie o nível: File > New Level > **Empty Level**, salve como `Content/Maps/Praia`.
4. Play. Deve aparecer a tela de abertura; aperte **Enter**.

## O que conferir

- [ ] A praia, o mar com ondas e o quiosque aparecem; o céu e o sol também.
- [ ] Andar (WASD), correr (Shift), pular (Espaço), olhar com o mouse.
- [ ] **B** liga o modo construção; a prévia fica verde/vermelha; clique constrói; **X** demole.
- [ ] Hóspedes chegam, andam até as construções e o dinheiro sobe.
- [ ] Bolinha de humor sobre a cabeça dos hóspedes (verde/amarela/vermelha).
- [ ] Olhando para o quiosque: painel com melhorias; **1-4** compra, **5** atendente, **6** caixa, **-/+** preço.
- [ ] Missão aparece no canto; ao cumprir, dá recompensa.
- [ ] **F2/F3** contratam garçom/faxineiro e eles aparecem andando.
- [ ] **F5** salva, **F9** carrega, **P** pausa, **F7** novo jogo.
- [ ] Som de mar e gaivotas; "ding" quando um hóspede paga.
- [ ] Ao subir de nível, a cerca branca se afasta (terreno maior).

## O que me mandar

- Erros de compilação (texto).
- Prints do jogo e do painel da construção.
- Qualquer coisa estranha: hóspede parado, preço absurdo, FPS baixo, sensação de ritmo (rápido/lento).
- Se o mouse não olha ou as teclas não respondem, diga qual.
