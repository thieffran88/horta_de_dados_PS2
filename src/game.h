/*
 * Horta de Dados PS2 - motor de regras (C puro, sem dependencias de plataforma).
 *
 * Folha de cada jogador:
 *   - horta de 3 linhas x 6 colunas (coluna = valor do dado);
 *   - porco com 7 linhas de 6 circulos (valores 2,4,6,8,10,12,15);
 *   - mercados (cenoura, alface, tomate) com 6 circulos cada. Como todos os
 *     jogadores marcam o mesmo circulo, os mercados sao guardados uma vez so.
 */
#ifndef HORTA_GAME_H
#define HORTA_GAME_H

#define MAX_PLAYERS   4
#define MAX_DICE      9
#define GARDEN_ROWS   3
#define GARDEN_COLS   6
#define PIG_ROWS      7
#define PIG_COLS      6
#define PIG_CIRCLES   (PIG_ROWS * PIG_COLS)
#define MARKET_MAX    6
#define RIBBON_POINTS 5
#define BONUS_POINTS  5

/* Cores dos dados / tipos de verdura */
enum { VEG_NONE = 0, VEG_CARROT = 1, VEG_LETTUCE = 2, VEG_TOMATO = 3 };
#define NUM_VEG 3

typedef struct {
    unsigned char color; /* VEG_CARROT / VEG_LETTUCE / VEG_TOMATO */
    unsigned char value; /* 1..6 */
} Die;

typedef struct {
    char name[16];
    int is_cpu;
    int cpu_level;                              /* 0 = facil, 1 = normal */
    unsigned char garden[GARDEN_ROWS][GARDEN_COLS];
    int pig;                                    /* circulos marcados 0..42 */
    int powers_used;                            /* poderes do porco ja gastos */
} Player;

typedef struct {
    int veg_count[NUM_VEG];
    int veg_points[NUM_VEG];
    int ribbons;        /* linhas completas */
    int ribbon_points;
    int pig_points;
    int bonus_points;   /* so no modo avancado */
    int total;
    int planted;        /* desempate 1 */
} Score;

enum {
    PHASE_ROLL = 0,   /* jogador inicial deve rolar os dados */
    PHASE_PICK,       /* jogadores escolhem dados */
    PHASE_MARKET,     /* resta um dado: mercado sobe */
    PHASE_GAME_OVER
};

typedef struct {
    int num_players;
    int advanced;
    Player p[MAX_PLAYERS];
    int market[NUM_VEG];            /* circulos marcados em cada mercado */

    int num_dice;                   /* 6 (2 jogadores) ou 9 */
    Die pool[MAX_DICE];
    int pool_n;

    int phase;
    int round;
    int start_player;
    int current;
    int last_round;                 /* fim de jogo ativado nesta rodada */
    int market_color;               /* cor do ultimo dado (fase MARKET) */

    unsigned int rng;
} Game;

/* Uma jogada completa: dado escolhido + poderes + destino. */
typedef struct {
    int die_index;      /* indice no pool */
    int new_value;      /* valor depois dos poderes */
    int new_color;      /* cor depois dos poderes (avancado) */
    int feed_pig;       /* 1 = alimentar o porco */
    int row, col;       /* destino na horta se plantar */
} Move;

void game_init(Game *g, int num_players, int advanced, unsigned int seed);
unsigned int game_rand(Game *g);

void game_roll(Game *g);                 /* PHASE_ROLL -> PHASE_PICK */

/* Consultas de regra */
int  veg_value(const Game *g, int veg);  /* valor atual de mercado */
int  garden_has(const Player *p, int veg);
int  can_plant_at(const Player *p, int veg, int value, int row);
int  can_plant(const Player *p, int veg, int value);
int  pig_rows_done(const Player *p);
int  powers_available(const Player *p);
int  move_cost(const Game *g, const Move *m);
int  move_valid(const Game *g, const Move *m);

/* Executa a jogada do jogador atual. Retorna 0 se invalida. */
int  game_apply(Game *g, const Move *m);

/* PHASE_MARKET -> proxima rodada (PHASE_ROLL) ou PHASE_GAME_OVER */
void game_resolve_market(Game *g);

int  player_triggers_end(const Player *p);

void game_score(const Game *g, Score out[MAX_PLAYERS]);
/* Preenche winners[] com 1 para vencedores (empates compartilham). */
void game_winners(const Game *g, const Score s[MAX_PLAYERS], int winners[MAX_PLAYERS]);

extern const int PIG_ROW_VALUES[PIG_ROWS];

#endif
