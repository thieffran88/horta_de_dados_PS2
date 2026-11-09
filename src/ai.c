/* Horta de Dados PS2 - jogador controlado pelo console (CPU). */
#include <string.h>
#include "ai.h"

static int count_veg(const Player *p, int veg)
{
    int r, c, n = 0;
    for (r = 0; r < GARDEN_ROWS; r++)
        for (c = 0; c < GARDEN_COLS; c++)
            n += p->garden[r][c] == veg;
    return n;
}

/* Quantas casas livres vizinhas existem para cada tipo de verdura:
 * mede a "fronteira" disponivel para plantios futuros. */
static int frontier(const Player *p, int veg)
{
    int r, c, n = 0;
    for (r = 0; r < GARDEN_ROWS; r++)
        for (c = 0; c < GARDEN_COLS; c++)
            if (p->garden[r][c] == VEG_NONE && can_plant_at(p, veg, c + 1, r)) n++;
    return n;
}

static int empty_cells(const Player *p)
{
    int r, c, n = 0;
    for (r = 0; r < GARDEN_ROWS; r++)
        for (c = 0; c < GARDEN_COLS; c++)
            n += p->garden[r][c] == VEG_NONE;
    return n;
}

static float eval_move(const Game *g, const Move *m)
{
    const Player *cur = &g->p[g->current];
    Player p = *cur;
    float s = 0.0f;
    int cost = move_cost(g, m);
    int rounds_left_guess;

    /* Estimativa grosseira de quanto falta para acabar. */
    rounds_left_guess = empty_cells(cur) / 2;
    if (rounds_left_guess > 6) rounds_left_guess = 6;

    if (m->feed_pig) {
        int before = pig_rows_done(&p);
        int after;
        p.pig += m->new_value;
        if (p.pig > PIG_CIRCLES) p.pig = PIG_CIRCLES;
        after = pig_rows_done(&p);
        s += 0.35f * (float)(p.pig - cur->pig);
        if (after > before) {
            int vb = before > 0 ? PIG_ROW_VALUES[before - 1] : 0;
            s += (float)(PIG_ROW_VALUES[after - 1] - vb);
            s += 1.2f * (float)(after - before); /* poderes novos */
        }
    } else {
        int veg = m->new_color, r, c, full = 1;
        float mv = (float)veg_value(g, veg);
        int fb, fa;
        /* valor esperado: mercado atual + um pouco de crescimento futuro */
        s += mv + 0.35f * (float)rounds_left_guess;
        if (g->advanced) {
            /* bonus de maioria */
            int mine = count_veg(&p, veg) + 1, best = 0, i;
            for (i = 0; i < g->num_players; i++)
                if (i != g->current) { int x = count_veg(&g->p[i], veg); if (x > best) best = x; }
            if (mine == best + 1 || mine == best) s += 1.5f;
        }
        fb = frontier(&p, veg);
        p.garden[m->row][m->col] = (unsigned char)veg;
        fa = frontier(&p, veg);
        s += 0.25f * (float)(fa - fb);
        for (c = 0; c < GARDEN_COLS; c++) if (p.garden[m->row][c] == VEG_NONE) full = 0;
        if (full) s += (float)RIBBON_POINTS;
        else {
            int filled = 0;
            for (c = 0; c < GARDEN_COLS; c++) filled += p.garden[m->row][c] != VEG_NONE;
            s += 0.3f * (float)filled;
        }
        /* primeira verdura de um tipo: prefere a linha do meio (mais vizinhos) */
        if (!garden_has(cur, veg) && m->row == 1) s += 0.6f;
        (void)r;
    }
    s -= 1.1f * (float)cost;
    return s;
}

void ai_choose(const Game *g, Move *best)
{
    const Player *p = &g->p[g->current];
    int avail = powers_available(p);
    float best_s = -1e9f;
    int d, col, val, row, found = 0;
    Move m;

    memset(best, 0, sizeof(*best));
    for (d = 0; d < g->pool_n; d++) {
        for (col = VEG_CARROT; col <= VEG_TOMATO; col++) {
            if (col != g->pool[d].color && !g->advanced) continue;
            for (val = 1; val <= 6; val++) {
                memset(&m, 0, sizeof(m));
                m.die_index = d;
                m.new_color = col;
                m.new_value = val;
                if (move_cost(g, &m) > avail) continue;
                if (can_plant(p, col, val)) {
                    for (row = 0; row < GARDEN_ROWS; row++) {
                        m.feed_pig = 0;
                        m.row = row;
                        m.col = val - 1;
                        if (!move_valid(g, &m)) continue;
                        {
                            float s = eval_move(g, &m);
                            if (p->cpu_level == 0) s += (float)(game_rand((Game *)g) % 300) / 100.0f;
                            if (!found || s > best_s) { best_s = s; *best = m; found = 1; }
                        }
                    }
                } else {
                    m.feed_pig = 1;
                    if (!move_valid(g, &m)) continue;
                    {
                        float s = eval_move(g, &m);
                        if (p->cpu_level == 0) s += (float)(game_rand((Game *)g) % 300) / 100.0f;
                        if (!found || s > best_s) { best_s = s; *best = m; found = 1; }
                    }
                }
            }
        }
    }
}
