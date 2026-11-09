# Horta de Dados PS2

Jogo de dados e horta para **PlayStation 2** (homebrew), de 2 a 4 jogadores, humanos ou CPU.
Role os dados, plante cenouras, alfaces e tomates, engorde o seu porco e fique de olho no
mercado: o preço das verduras muda a cada rodada.

![Tela título](docs/01_titulo.png)
![Partida](docs/09_cpu.png)
![Fim de jogo](docs/10_fim_de_jogo.png)

> As capturas acima saem do renderizador de testes no PC (`make -f Makefile.host shots`), que
> desenha o mesmo código de interface do jogo.

## Como jogar

| Botão | Ação |
| --- | --- |
| Direcional Esq/Dir | escolher um dado da reserva |
| X | pegar o dado / plantar / alimentar o porco |
| Cima/Baixo (com o dado na mão) | escolher a linha da coluna |
| Esq/Dir (com o dado na mão) | gastar um poder do porco: valor -1 / +1 |
| Triângulo (com o dado na mão) | gastar um poder para trocar a cor (modo avançado) |
| O | devolver o dado e escolher outro |
| L1/R1/L2/R2/SELECT | ver a horta dos outros jogadores |
| START | pausa (com "Como jogar" e "Sair") |
| Segurar X | acelera a jogada da CPU |

Vários humanos jogam no mesmo controle (o jogo avisa quando passar o controle). O controle da
porta 2 também funciona, e o analógico esquerdo serve de direcional.

### Regras

- **Rodada:** o jogador inicial rola todos os dados (9; ou 6, sendo 2 de cada cor, com 2
  jogadores). Na ordem, cada um pega 1 dado até sobrar só 1.
- **Plantar:** a cor do dado é a verdura (laranja = cenoura, verde = alface, vermelho = tomate) e o
  número é a coluna da horta (3 linhas x 6 colunas). A primeira verdura de cada tipo vai em
  qualquer espaço livre da coluna; as seguintes precisam encostar (ortogonalmente) em outra igual.
- **Porco:** um dado que não pode ser plantado vai para o porco: marca tantos círculos quanto o
  valor do dado. Só dá para alimentar o porco com dados que não podem ser plantados.
- **Poderes do porco:** cada linha de 6 círculos completa dá um poder de uso único: muda o valor do
  dado em ±1 (vários poderes podem ser somados). No modo avançado, um poder também pode trocar a cor.
- **Mercado:** o dado que sobra na reserva faz o mercado da sua cor subir um círculo para todos.
  No modo básico cada verdura vale 1 + círculos; no avançado os mercados começam em 0.
- **Fim:** quando alguém enche a horta, completa o porco (42 círculos) ou um mercado chega a 6
  círculos, a rodada atual termina e a partida acaba.
- **Pontos:** verduras x valor no mercado; 5 por linha completa da horta; o porco vale o número
  ao lado da última linha completa (2, 4, 6, 8, 10, 12, 15). No modo avançado, +5 para quem tiver
  mais cenouras, mais alfaces, mais tomates e mais círculos no porco (empates: todos ganham).
  Desempate: mais verduras plantadas, depois mais círculos no porco.

Tudo isso também está no menu **Como jogar**, dentro do jogo.

## Rodar

- **Emulador (PCSX2):** *Sistema → Iniciar ELF* (ou "Run ELF") e escolha `horta_de_dados.elf`;
  ou abra `horta_de_dados.iso` como um disco. O PCSX2 precisa de uma BIOS do seu próprio console.
- **PS2 de verdade:** copie o `horta_de_dados.elf` para um pendrive e abra pelo wLaunchELF
  (uLaunchELF), ou adicione como aplicativo no OPL. A ISO gravada em CD só dá boot em console
  desbloqueado (ex.: com FreeMcBoot/modchip).

O jogo usa NTSC 640x448 e se centraliza automaticamente em consoles PAL.

## Compilar

Precisa do [PS2SDK](https://github.com/ps2dev/ps2dev) com gsKit. O jeito mais fácil é a imagem
Docker oficial:

```sh
# com make (a imagem não traz make; instale com apk)
docker run --rm -v "$PWD":/src -w /src ps2dev/ps2dev:latest \
    sh -c "apk add --no-cache make && make"

# ou sem make
docker run --rm -v "$PWD":/src -w /src ps2dev/ps2dev:latest sh build_ps2.sh
```

Isso gera `horta_de_dados.elf`. `make iso` gera uma ISO (precisa de `mkisofs`/`genisoimage`).
O GitHub Actions (`.github/workflows/build.yml`) compila o ELF e a ISO a cada push e deixa os
arquivos como artefatos do workflow.

### Testes no PC

O motor de regras e a interface são C puro, separados da camada do PS2, então rodam no PC:

```sh
make -f Makefile.host test    # testes de regras + 300 partidas simuladas só com CPU
make -f Makefile.host shots   # capturas de tela (.ppm) em build-host/shots/
```

## Estrutura

| Arquivo | Conteúdo |
| --- | --- |
| `src/game.c` | regras: dados, plantio, porco, poderes, mercado, fim de jogo, pontuação |
| `src/ai.c` | jogadores CPU (Fácil e Normal) |
| `src/ui.c` | telas, menus, fluxo da partida |
| `src/gfx.c` | desenho: texto, dados, verduras, porco, painéis |
| `src/texts.h` | textos das regras (português) |
| `src/plat_ps2.c`, `src/main_ps2.c` | PS2: vídeo com gsKit, controles com libpad |
| `src/font_data.c` | fonte gerada por `tools/genfont.py` (DejaVu Sans Bold) |
| `host/` | plataforma de testes no PC (renderizador por software) |
| `tests/test_rules.c` | testes das regras |

## Créditos

- Regras adaptadas do jogo de tabuleiro *Harvest Dice* (design de Danny Devine, Grey Fox Games).
  Este é um projeto de fã, sem afiliação; nome, arte e textos do jogo são próprios.
- Fonte: DejaVu Sans Bold (licença Bitstream Vera/DejaVu).
- Feito com PS2SDK e gsKit (ps2dev).
