/* Horta de Dados PS2 - motor de regras. */
#include <string.h>
#include "game.h"

const int PIG_ROW_VALUES[PIG_ROWS] = { 2, 4, 6, 8, 10, 12, 15 };

unsigned int game_rand(Game *g)
{
    /* xorshift32 */
    unsigned int x = g->rng ? g->rng : 0x9E3779B9u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g->rng = x;
    return x;
}

void game_init(Game *g, int num_players, int advanced, unsigned int seed)
{
    int i;
    Player keep[MAX_PLAYERS];
    memcpy(keep, g->p, sizeof(keep)); /* nomes / cpu vem do menu */
    memset(g, 0, sizeof(*g));
    if (num_players < 2) num_players = 2;
    if (num_players > MAX_PLAYERS) num_players = MAX_PLAYERS;
    g->num_players = num_players;
    g->advanced = advanced ? 1 : 0;
    g->rng = seed ? seed : 12345u;
    for (i = 0; i < MAX_PLAYERS; i++) {
        memcpy(g->p[i].name, keep[i].name, sizeof(g->p[i].name));
        g->p[i].name[sizeof(g->p[i].name) - 1] = 0;
        g->p[i].is_cpu = keep[i].is_cpu;
        g->p[i].cpu_level = keep[i].cpu_level;
    }
    /* 2 jogadores: 2 dados de cada cor; senao 3 de cada cor. */
    g->num_dice = (num_players == 2) ? 6 : 9;
    g->round = 1;
    g->start_player = (int)(game_rand(g) % (unsigned)num_players);
    g->current = g->start_player;
    g->phase = PHASE_ROLL;
}

void game_roll(Game *g)
{
    int i, per = g->num_dice / NUM_VEG;
    if (g->phase != PHASE_ROLL) return;
    g->pool_n = g->num_dice;
    for (i = 0; i < g->num_dice; i++) {
        g->pool[i].color = (unsigned char)(VEG_CARROT + i / per);
        g->pool[i].value = (unsigned char)(1 + game_rand(g) % 6);
    }
    g->current = g->start_player;
    g->phase = PHASE_PICK;
}

int veg_value(const Game *g, int veg)
{
    if (veg < 1 || veg > NUM_VEG) return 0;
    /* Basico: cada verdura ja vale 1. Avancado: mercados comecam vazios. */
    return g->market[veg - 1] + (g->advanced ? 0 : 1);
}

int garden_has(const Player *p, int veg)
{
    int r, c;
    for (r = 0; r < GARDEN_ROWS; r++)
        for (c = 0; c < GARDEN_COLS; c++)
            if (p->garden[r][c] == veg) return 1;
    return 0;
}

int can_plant_at(const Player *p, int veg, int value, int row)
{
    int col = value - 1;
    if (veg < 1 || veg > NUM_VEG || value < 1 || value > 6) return 0;
    if (row < 0 || row >= GARDEN_ROWS) return 0;
    if (p->garden[row][col] != VEG_NONE) return 0;
    if (!garden_has(p, veg)) return 1; /* primeira verdura deste tipo */
    /* Ja existe: precisa ser ortogonalmente adjacente a uma igual. */
    if (row > 0 && p->garden[row - 1][col] == veg) return 1;
    if (row < GARDEN_ROWS - 1 && p->garden[row + 1][col] == veg) return 1;
    if (col > 0 && p->garden[row][col - 1] == veg) return 1;
    if (col < GARDEN_COLS - 1 && p->garden[row][col + 1] == veg) return 1;
    return 0;
}

int can_plant(const Player *p, int veg, int value)
{
    int r;
    for (r = 0; r < GARDEN_ROWS; r++)
        if (can_plant_at(p, veg, value, r)) return 1;
    return 0;
}

int pig_rows_done(const Player *p)
{
    return p->pig / PIG_COLS;
}

int powers_available(const Player *p)
{
    return pig_rows_done(p) - p->powers_used;
}

int move_cost(const Game *g, const Move *m)
{
    const Die *d;
    int cost;
    if (m->die_index < 0 || m->die_index >= g->pool_n) return 99;
    d = &g->pool[m->die_index];
    cost = m->new_value - d->value;
    if (cost < 0) cost = -cost;
    if (m->new_color != d->color) cost++;
    return cost;
}

int move_valid(const Game *g, const Move *m)
{
    const Player *p;
    const Die *d;
    if (g->phase != PHASE_PICK) return 0;
    if (m->die_index < 0 || m->die_index >= g->pool_n) return 0;
    p = &g->p[g->current];
    d = &g->pool[m->die_index];
    if (m->new_value < 1 || m->new_value > 6) return 0;
    if (m->new_color < 1 || m->new_color > NUM_VEG) return 0;
    if (m->new_color != d->color && !g->advanced) return 0;
    if (move_cost(g, m) > powers_available(p)) return 0;
    if (m->feed_pig) {
        /* So pode alimentar o porco se o dado nao puder ser plantado. */
        return !can_plant(p, m->new_color, m->new_value);
    }
    if (m->col != m->new_value - 1) return 0;
    return can_plant_at(p, m->new_color, m->new_value, m->row);
}

int player_triggers_end(const Player *p)
{
    int r, c, full = 1;
    for (r = 0; r < GARDEN_ROWS; r++)
        for (c = 0; c < GARDEN_COLS; c++)
            if (p->garden[r][c] == VEG_NONE) full = 0;
    return full || p->pig >= PIG_CIRCLES;
}

int game_apply(Game *g, const Move *m)
{
    Player *p;
    int i;
    if (!move_valid(g, m)) return 0;
    p = &g->p[g->current];
    p->powers_used += move_cost(g, m);

    if (m->feed_pig) {
        /* Marca circulos iguais ao valor do dado, esquerda->direita, cima->baixo. */
        p->pig += m->new_value;
        if (p->pig > PIG_CIRCLES) p->pig = PIG_CIRCLES;
    } else {
        p->garden[m->row][m->col] = (unsigned char)m->new_color;
    }

    for (i = m->die_index; i < g->pool_n - 1; i++) g->pool[i] = g->pool[i + 1];
    g->pool_n--;

    if (player_triggers_end(p)) g->last_round = 1;

    g->current = (g->current + 1) % g->num_players;
    if (g->pool_n <= 1) {
        g->phase = PHASE_MARKET;
        g->market_color = g->pool[0].color;
    }
    return 1;
}

void game_resolve_market(Game *g)
{
    int m;
    if (g->phase != PHASE_MARKET) return;
    m = g->market_color - 1;
    if (m >= 0 && m < NUM_VEG && g->market[m] < MARKET_MAX) g->market[m]++;
    if (m >= 0 && m < NUM_VEG && g->market[m] >= MARKET_MAX) g->last_round = 1;
    g->pool_n = 0;

    if (g->last_round) {
        g->phase = PHASE_GAME_OVER;
        return;
    }
    g->round++;
    g->start_player = (g->start_player + 1) % g->num_players;
    g->current = g->start_player;
    g->phase = PHASE_ROLL;
}

void game_score(const Game *g, Score out[MAX_PLAYERS])
{
    int i, v, r, c;
    memset(out, 0, sizeof(Score) * MAX_PLAYERS);
    for (i = 0; i < g->num_players; i++) {
        const Player *p = &g->p[i];
        Score *s = &out[i];
        for (r = 0; r < GARDEN_ROWS; r++) {
            int full = 1;
            for (c = 0; c < GARDEN_COLS; c++) {
                int veg = p->garden[r][c];
                if (veg == VEG_NONE) full = 0;
                else { s->veg_count[veg - 1]++; s->planted++; }
            }
            if (full) s->ribbons++;
        }
        for (v = 0; v < NUM_VEG; v++) {
            s->veg_points[v] = s->veg_count[v] * veg_value(g, v + 1);
            s->total += s->veg_points[v];
        }
        s->ribbon_points = s->ribbons * RIBBON_POINTS;
        s->total += s->ribbon_points;
        r = pig_rows_done(p);
        s->pig_points = r > 0 ? PIG_ROW_VALUES[r - 1] : 0;
        s->total += s->pig_points;
    }
    if (g->advanced) {
        /* +5 para quem tiver mais de cada verdura e mais circulos no porco.
         * Empates: todos os empatados recebem. */
        for (v = 0; v <= NUM_VEG; v++) {
            int best = 0;
            for (i = 0; i < g->num_players; i++) {
                int x = v < NUM_VEG ? out[i].veg_count[v] : g->p[i].pig;
                if (x > best) best = x;
            }
            if (best == 0) continue;
            for (i = 0; i < g->num_players; i++) {
                int x = v < NUM_VEG ? out[i].veg_count[v] : g->p[i].pig;
                if (x == best) { out[i].bonus_points += BONUS_POINTS; out[i].total += BONUS_POINTS; }
            }
        }
    }
}

void game_winners(const Game *g, const Score s[MAX_PLAYERS], int winners[MAX_PLAYERS])
{
    int i, best;
    for (i = 0; i < MAX_PLAYERS; i++) winners[i] = i < g->num_players;
    /* 1) mais pontos */
    best = -1;
    for (i = 0; i < g->num_players; i++) if (winners[i] && s[i].total > best) best = s[i].total;
    for (i = 0; i < g->num_players; i++) if (s[i].total != best) winners[i] = 0;
    /* 2) mais verduras plantadas */
    best = -1;
    for (i = 0; i < g->num_players; i++) if (winners[i] && s[i].planted > best) best = s[i].planted;
    for (i = 0; i < g->num_players; i++) if (winners[i] && s[i].planted != best) winners[i] = 0;
    /* 3) mais circulos no porco */
    best = -1;
    for (i = 0; i < g->num_players; i++) if (winners[i] && g->p[i].pig > best) best = g->p[i].pig;
    for (i = 0; i < g->num_players; i++) if (winners[i] && g->p[i].pig != best) winners[i] = 0;
    /* 4) ainda empatados: dividem a vitoria */
}
