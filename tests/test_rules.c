/* Testes das regras (rodam no PC: make -f Makefile.host test). */
#include <stdio.h>
#include <string.h>
#include "../src/game.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FALHOU %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static void setup(Game *g, int n, int adv)
{
    memset(g, 0, sizeof(*g));
    game_init(g, n, adv, 42);
    game_roll(g);
}

static Move plant(int die, int value, int color, int row)
{
    Move m; memset(&m, 0, sizeof(m));
    m.die_index = die; m.new_value = value; m.new_color = color; m.row = row; m.col = value - 1;
    return m;
}

static void test_dice_count(void)
{
    Game g;
    setup(&g, 2, 0); CHECK(g.pool_n == 6);
    setup(&g, 3, 0); CHECK(g.pool_n == 9);
    setup(&g, 4, 0); CHECK(g.pool_n == 9);
}

static void test_planting(void)
{
    Game g;
    Player *p;
    Move m;
    setup(&g, 3, 0);
    p = &g.p[g.current];
    /* Primeira cenoura: qualquer linha da coluna. */
    CHECK(can_plant_at(p, VEG_CARROT, 2, 0));
    CHECK(can_plant_at(p, VEG_CARROT, 2, 2));
    p->garden[1][1] = VEG_CARROT;                 /* cenoura na coluna 2 */
    /* Agora so ao lado de outra cenoura. */
    CHECK(can_plant_at(p, VEG_CARROT, 3, 1));     /* direita */
    CHECK(can_plant_at(p, VEG_CARROT, 1, 1));     /* esquerda */
    CHECK(can_plant_at(p, VEG_CARROT, 2, 0));     /* acima */
    CHECK(!can_plant_at(p, VEG_CARROT, 3, 0));    /* diagonal nao */
    CHECK(!can_plant_at(p, VEG_CARROT, 5, 1));    /* longe nao */
    CHECK(!can_plant_at(p, VEG_CARROT, 2, 1));    /* ocupado */
    /* Outra verdura: livre na primeira vez. */
    CHECK(can_plant_at(p, VEG_TOMATO, 5, 1));
    /* Coluna cheia. */
    p->garden[0][3] = VEG_LETTUCE; p->garden[1][3] = VEG_TOMATO; p->garden[2][3] = VEG_TOMATO;
    CHECK(!can_plant(p, VEG_CARROT, 4));
    CHECK(!can_plant(p, VEG_LETTUCE, 4));

    /* game_apply: deve plantar se possivel, nao pode alimentar o porco. */
    setup(&g, 3, 0);
    g.pool[0].color = VEG_LETTUCE; g.pool[0].value = 3;
    memset(&m, 0, sizeof(m));
    m.die_index = 0; m.new_value = 3; m.new_color = VEG_LETTUCE; m.feed_pig = 1;
    CHECK(!move_valid(&g, &m));
    m = plant(0, 3, VEG_LETTUCE, 2);
    CHECK(game_apply(&g, &m));
    CHECK(g.pool_n == 8);
}

static void test_pig(void)
{
    Game g;
    Player *p;
    Move m;
    setup(&g, 3, 0);
    p = &g.p[g.current];
    /* Coluna 6 cheia: o dado 6 vai para o porco. */
    p->garden[0][5] = p->garden[1][5] = p->garden[2][5] = VEG_TOMATO;
    g.pool[0].color = VEG_CARROT; g.pool[0].value = 6;
    memset(&m, 0, sizeof(m));
    m.die_index = 0; m.new_value = 6; m.new_color = VEG_CARROT; m.feed_pig = 1;
    CHECK(move_valid(&g, &m));
    CHECK(game_apply(&g, &m));
    CHECK(p->pig == 6);
    CHECK(pig_rows_done(p) == 1);
    CHECK(powers_available(p) == 1);
}

static void test_powers(void)
{
    Game g;
    Player *p;
    Move m;
    setup(&g, 3, 0);
    p = &g.p[g.current];
    p->pig = 12; /* 2 poderes */
    g.pool[0].color = VEG_CARROT; g.pool[0].value = 4;
    m = plant(0, 6, VEG_CARROT, 0); /* 4 -> 6 custa 2 */
    CHECK(move_cost(&g, &m) == 2);
    CHECK(move_valid(&g, &m));
    m = plant(0, 1, VEG_CARROT, 0); /* 4 -> 1 custa 3 */
    CHECK(!move_valid(&g, &m));
    m = plant(0, 4, VEG_TOMATO, 0); /* troca de cor so no avancado */
    CHECK(!move_valid(&g, &m));
    m = plant(0, 6, VEG_CARROT, 0);
    CHECK(game_apply(&g, &m));
    CHECK(p->powers_used == 2 && powers_available(p) == 0);

    setup(&g, 3, 1);
    p = &g.p[g.current];
    p->pig = 6;
    g.pool[0].color = VEG_CARROT; g.pool[0].value = 4;
    m = plant(0, 4, VEG_TOMATO, 0);
    CHECK(move_valid(&g, &m));
    m = plant(0, 5, VEG_TOMATO, 0);  /* cor + valor = 2 poderes */
    CHECK(!move_valid(&g, &m));
}

static void test_market_and_round(void)
{
    Game g;
    int guard = 0, start;
    setup(&g, 3, 0);
    start = g.start_player;
    while (g.phase == PHASE_PICK && guard++ < 20) {
        Move m; int r, ok = 0;
        Player *p = &g.p[g.current];
        memset(&m, 0, sizeof(m));
        m.die_index = 0; m.new_value = g.pool[0].value; m.new_color = g.pool[0].color;
        if (can_plant(p, m.new_color, m.new_value)) {
            for (r = 0; r < 3 && !ok; r++) { m.row = r; m.col = m.new_value - 1; ok = move_valid(&g, &m); }
        } else m.feed_pig = 1;
        CHECK(game_apply(&g, &m));
    }
    CHECK(g.phase == PHASE_MARKET);
    CHECK(g.pool_n == 1);
    {
        int c = g.pool[0].color;
        CHECK(veg_value(&g, c) == 1);
        game_resolve_market(&g);
        CHECK(g.market[c - 1] == 1);
        CHECK(veg_value(&g, c) == 2);
    }
    CHECK(g.phase == PHASE_ROLL);
    CHECK(g.start_player == (start + 1) % 3);
    CHECK(g.round == 2);
}

static void test_scoring(void)
{
    Game g;
    Score s[MAX_PLAYERS];
    int w[MAX_PLAYERS], c;
    setup(&g, 2, 0);
    /* Exemplo do manual: 5 alfaces, mercado de alface com 3 circulos.
     * No modo basico isto vale 1 + 3 = 4 por alface. */
    g.market[VEG_LETTUCE - 1] = 3;
    for (c = 0; c < 5; c++) g.p[0].garden[0][c] = VEG_LETTUCE;
    g.p[0].garden[0][5] = VEG_CARROT;      /* linha completa: laco */
    g.p[0].pig = 13;                       /* 2 linhas: 4 pontos */
    game_score(&g, s);
    CHECK(s[0].veg_points[VEG_LETTUCE - 1] == 20);
    CHECK(s[0].veg_points[VEG_CARROT - 1] == 1);
    CHECK(s[0].ribbon_points == 5);
    CHECK(s[0].pig_points == 4);
    CHECK(s[0].total == 30);
    CHECK(s[1].total == 0);
    game_winners(&g, s, w);
    CHECK(w[0] && !w[1]);

    /* Avancado: mercados comecam em 0, bonus de maioria. */
    setup(&g, 2, 1);
    g.market[VEG_LETTUCE - 1] = 3;
    for (c = 0; c < 5; c++) g.p[0].garden[0][c] = VEG_LETTUCE;
    g.p[1].garden[0][0] = VEG_TOMATO;
    g.p[0].pig = 7; g.p[1].pig = 7;
    game_score(&g, s);
    CHECK(s[0].veg_points[VEG_LETTUCE - 1] == 15); /* 5 x 3 = 15, como no manual */
    CHECK(s[0].bonus_points == 10);                /* alfaces + porco (empate) */
    CHECK(s[1].bonus_points == 10);                /* tomates + porco (empate) */

    /* Desempate por verduras plantadas. */
    setup(&g, 2, 0);
    g.p[0].garden[0][0] = VEG_CARROT;              /* 1 ponto */
    g.p[1].pig = 6;                                /* 2 pontos */
    g.p[0].garden[1][0] = VEG_CARROT;              /* agora 2 x 1 = 2 */
    game_score(&g, s);
    CHECK(s[0].total == s[1].total);
    game_winners(&g, s, w);
    CHECK(w[0] && !w[1]);
}

static void test_end_triggers(void)
{
    Game g;
    Move m;
    int r, c;
    setup(&g, 2, 0);
    /* Horta quase cheia: plantar o ultimo espaco ativa o fim. */
    for (r = 0; r < 3; r++) for (c = 0; c < 6; c++) g.p[g.current].garden[r][c] = VEG_TOMATO;
    g.p[g.current].garden[2][2] = VEG_NONE;
    g.pool[0].color = VEG_TOMATO; g.pool[0].value = 3;
    m = plant(0, 3, VEG_TOMATO, 2);
    CHECK(game_apply(&g, &m));
    CHECK(g.last_round == 1);
    CHECK(g.phase == PHASE_PICK);   /* a rodada continua ate o fim */

    /* Mercado cheio encerra apos a rodada. */
    setup(&g, 2, 0);
    g.market[0] = 5;
    g.phase = PHASE_MARKET; g.market_color = VEG_CARROT; g.pool_n = 1;
    game_resolve_market(&g);
    CHECK(g.phase == PHASE_GAME_OVER);
}

int main(void)
{
    test_dice_count();
    test_planting();
    test_pig();
    test_powers();
    test_market_and_round();
    test_scoring();
    test_end_triggers();
    if (fails) { printf("%d falha(s)\n", fails); return 1; }
    printf("todos os testes de regras passaram\n");
    return 0;
}
