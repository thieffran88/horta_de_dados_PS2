/* Horta de Dados PS2 - textos em portugues. */
#ifndef HORTA_TEXTS_H
#define HORTA_TEXTS_H

typedef struct { const char *title; const char *body; } RulesPage;

#define TXT_BASIC_DESC \
    "Básico: cada verdura começa valendo 1 ponto e sobe +1 a cada círculo do mercado."
#define TXT_ADV_DESC \
    "Avançado: mercados começam em 0, os poderes do porco também trocam a cor do dado e há " \
    "+5 pontos para quem tiver mais cenouras, alfaces, tomates e círculos no porco."

#define RULES_PAGES 8
static const RulesPage RULES[RULES_PAGES] = {
    { "Objetivo",
      "Em Horta de Dados você escolhe dados para plantar cenouras, alfaces e tomates na sua "
      "horta ou para engordar o seu porco.\n\n"
      "O preço de cada verdura muda ao longo da partida no mercado. Quem tiver mais pontos "
      "quando a colheita chegar vence!\n\n"
      "Dado laranja = cenoura\nDado verde = alface\nDado vermelho = tomate" },
    { "A rodada",
      "O jogador inicial (marcado com o porquinho no placar) rola todos os dados: 9 dados, "
      "ou 6 dados numa partida de 2 jogadores.\n\n"
      "Começando por ele e seguindo a ordem dos jogadores, cada um pega 1 dado da reserva, "
      "até sobrar só 1 dado. Com 2 ou 3 jogadores, alguém acaba pegando um dado a menos.\n\n"
      "Todo dado escolhido precisa ser plantado na horta ou dado ao porco." },
    { "Plantar",
      "O número do dado indica a coluna da horta. Desenhe a verdura da cor do dado em um "
      "espaço livre dessa coluna.\n\n"
      "A primeira verdura de cada tipo pode ir em qualquer espaço livre da coluna. Depois "
      "disso, cada nova verdura daquele tipo precisa ficar encostada (em cima, embaixo, à "
      "esquerda ou à direita) em outra verdura igual.\n\n"
      "Se a coluna estiver cheia, ou se não houver vizinho igual, o dado não pode ser plantado." },
    { "Alimentar o porco",
      "Um dado que não pode ser plantado vai obrigatoriamente para o porco: marque tantos "
      "círculos quanto o valor do dado, da esquerda para a direita e de cima para baixo.\n\n"
      "Você só pode alimentar o porco com um dado que não cabe na horta. Quer engordar o "
      "porco? Escolha um dado que não dá para plantar!\n\n"
      "No fim, o porco vale os pontos ao lado da última linha completa (2, 4, 6, 8, 10, 12 ou 15)." },
    { "Poderes do porco",
      "Cada linha de 6 círculos completa no porco libera um poder (caixinha *). Cada poder "
      "pode ser usado uma vez.\n\n"
      "Um poder muda o valor do dado escolhido em +1 ou -1 (nunca abaixo de 1 nem acima de 6). "
      "Você pode gastar vários poderes no mesmo dado: dois poderes transformam um 4 em 6.\n\n"
      "Modo avançado: cada poder pode, em vez disso, trocar a cor do dado." },
    { "O mercado",
      "Quando sobra só 1 dado na reserva, ninguém o pega. O mercado da cor desse dado sobe um "
      "círculo para todos os jogadores (o número do dado não importa).\n\n"
      "Modo básico: cada verdura vale 1 ponto + 1 por círculo marcado no seu mercado.\n"
      "Modo avançado: os mercados começam vazios, valendo 0.\n\n"
      "Depois disso o próximo jogador vira o inicial e rola os dados de novo." },
    { "Fim de jogo e pontos",
      "A última rodada começa quando alguém enche a horta (18 espaços), marca todos os 42 "
      "círculos do porco, ou quando um mercado chega a 6 círculos. A rodada termina e então:\n\n"
      "Verduras: quantidade de cada tipo x valor no mercado.\n"
      "Laços: 5 pontos por linha completa da horta.\n"
      "Porco: o valor da última linha completa.\n"
      "Avançado: +5 para quem tem mais cenouras, mais alfaces, mais tomates e mais círculos no "
      "porco (empates: todos ganham).\n\n"
      "Desempate: mais verduras plantadas; depois mais círculos no porco." },
    { "Controles",
      "Direcional Esq/Dir: escolher um dado da reserva.\n"
      "X: pegar o dado / plantar / alimentar o porco.\n"
      "Com o dado na mão:\n"
      "   Cima/Baixo: escolher a linha da coluna.\n"
      "   Esq/Dir: gastar um poder do porco (-1 / +1).\n"
      "   Triângulo: gastar um poder para trocar a cor (avançado).\n"
      "   O: devolver o dado e escolher outro.\n"
      "L1/R1/L2/R2/SELECT: ver a horta dos outros jogadores.\n"
      "START: pausa.   Segure X para acelerar a CPU.\n\n"
      "Até 4 jogadores no mesmo controle (passe o controle) ou contra a CPU." },
};

#endif
