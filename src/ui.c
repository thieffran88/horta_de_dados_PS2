/* Horta de Dados PS2 - telas, menus e fluxo de jogo. */
#include <stdio.h>
#include <string.h>
#include "ui.h"
#include "ai.h"
#include "gfx.h"
#include "texts.h"

#define DIRS (BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT)

static const char *CPU_NAMES[MAX_PLAYERS] = { "Seu Zé", "Vó Cida", "Tio Beto", "Dona Lia" };
static const char *VEG_PLURAL[4] = { "", "cenouras", "alfaces", "tomates" };
static const char *VEG_SING[4] = { "", "cenoura", "alface", "tomate" };
static const char *VEG_MARKET[4] = { "", "Cenoura", "Alface", "Tomate" };
static const char *TYPE_NAMES[3] = { "Humano", "CPU Fácil", "CPU Normal" };

/* ------------------------------------------------------------------ */

static void set_msg(Ui *u, Color c, const char *s)
{
    snprintf(u->msg, sizeof(u->msg), "%s", s);
    u->msg_color = c;
    u->msg_timer = 150;
}

static int is_cpu(const Ui *u, int i) { return u->g.p[i].is_cpu; }

static int humans(const Ui *u)
{
    int i, n = 0;
    for (i = 0; i < u->g.num_players; i++) n += !u->g.p[i].is_cpu;
    return n;
}

static Player *cur(Ui *u) { return &u->g.p[u->g.current]; }

static int sel_cost(const Ui *u)
{
    Move m;
    memset(&m, 0, sizeof(m));
    m.die_index = u->pick_sel;
    m.new_value = u->adj_value;
    m.new_color = u->adj_color;
    return move_cost(&u->g, &m);
}

static void fix_place_row(Ui *u)
{
    const Player *p = &u->g.p[u->g.current];
    int r;
    if (u->place_row >= 0 && can_plant_at(p, u->adj_color, u->adj_value, u->place_row)) return;
    u->place_row = -1;
    for (r = 0; r < GARDEN_ROWS; r++)
        if (can_plant_at(p, u->adj_color, u->adj_value, r)) { u->place_row = r; return; }
}

void ui_init(Ui *u, unsigned seed)
{
    int i;
    memset(u, 0, sizeof(*u));
    u->state = ST_TITLE;
    u->setup_players = 2;
    u->setup_type[0] = 0;
    for (i = 1; i < MAX_PLAYERS; i++) u->setup_type[i] = 2;
    u->g.rng = seed ? seed : 1;
    u->view_player = -1;
    u->place_row = -1;
}

static void start_game(Ui *u)
{
    int i, h = 0;
    unsigned seed = plat_seed() ^ (u->frame * 2654435761u);
    for (i = 0; i < MAX_PLAYERS; i++) {
        Player *p = &u->g.p[i];
        p->is_cpu = u->setup_type[i] != 0;
        p->cpu_level = u->setup_type[i] == 1 ? 0 : 1;
        if (p->is_cpu) snprintf(p->name, sizeof(p->name), "%s", CPU_NAMES[i]);
        else snprintf(p->name, sizeof(p->name), "Jogador %d", (++h) & 7);
    }
    game_init(&u->g, u->setup_players, u->setup_adv, seed);
    u->state = ST_ROLL;
    u->timer = 0;
    u->view_player = -1;
    u->sel_active = 0;
    u->msg_timer = 0;
    {
        char b[96];
        snprintf(b, sizeof(b), "%s começa a partida!", u->g.p[u->g.start_player].name);
        set_msg(u, COL_YELLOW, b);
    }
}

static void begin_turn(Ui *u)
{
    u->view_player = -1;
    u->sel_active = 0;
    u->place_row = -1;
    u->timer = 0;
    if (u->pick_sel >= u->g.pool_n) u->pick_sel = u->g.pool_n - 1;
    if (u->pick_sel < 0) u->pick_sel = 0;
    if (is_cpu(u, u->g.current)) {
        ai_choose(&u->g, &u->cpu_move);
        u->state = ST_CPU;
    } else {
        u->state = ST_PICK;
        /* Com mais de um humano, avisa de quem e a vez (passa o controle). */
        u->banner_timer = humans(u) > 1 ? 90 : 45;
    }
}

static void do_move(Ui *u, const Move *m)
{
    char b[128];
    int who = u->g.current;
    int was_last = u->g.last_round;
    Player *p = &u->g.p[who];
    int rows_before = pig_rows_done(p);

    if (!game_apply(&u->g, m)) {
        set_msg(u, COL_BAD, "Jogada inválida.");
        return;
    }
    if (m->feed_pig) {
        snprintf(b, sizeof(b), "%s alimentou o porco (+%d).", p->name, m->new_value);
        if (pig_rows_done(p) > rows_before)
            snprintf(b, sizeof(b), "%s alimentou o porco (+%d) e ganhou um poder!", p->name, m->new_value);
    } else {
        snprintf(b, sizeof(b), "%s plantou %s na coluna %d.", p->name,
                 m->new_color == VEG_LETTUCE ? "uma alface" : (m->new_color == VEG_CARROT ? "uma cenoura" : "um tomate"),
                 m->col + 1);
    }
    set_msg(u, COL_WHITE, b);
    if (!was_last && u->g.last_round) {
        snprintf(b, sizeof(b), "%s ativou o fim de jogo! Esta é a última rodada.", p->name);
        set_msg(u, COL_YELLOW, b);
    }
    u->sel_active = 0;
    if (u->g.phase == PHASE_MARKET) {
        u->state = ST_MARKET;
        u->timer = 0;
        u->pick_sel = 0;
    } else {
        begin_turn(u);
    }
}

static int s_pause_return = ST_TITLE;

static void enter_pause(Ui *u)
{
    s_pause_return = u->state;
    u->prev_state = u->state;
    u->pause_sel = 0;
    u->state = ST_PAUSE;
}

static void cycle_view(Ui *u, int dir)
{
    int n = u->g.num_players;
    int v = u->view_player < 0 ? u->g.current : u->view_player;
    v = (v + dir + n) % n;
    u->view_player = v == u->g.current ? -1 : v;
}

/* ------------------------------------------------------------------ */
/* Atualizacao por estado                                              */

static void upd_title(Ui *u)
{
    if (u->pressed & (BTN_UP | BTN_DOWN)) u->menu_sel ^= 1;
    if (u->pressed & (BTN_CROSS | BTN_START)) {
        if (u->menu_sel == 0) { u->state = ST_SETUP; u->setup_sel = 0; }
        else { u->prev_state = ST_TITLE; u->rules_page = 0; u->state = ST_RULES; }
    }
}

static void upd_setup(Ui *u)
{
    int rows = u->setup_players + 3; /* jogadores, N tipos, modo, comecar */
    int dir = 0;
    if (u->pressed & BTN_UP) u->setup_sel = (u->setup_sel + rows - 1) % rows;
    if (u->pressed & BTN_DOWN) u->setup_sel = (u->setup_sel + 1) % rows;
    if (u->pressed & BTN_LEFT) dir = -1;
    if (u->pressed & BTN_RIGHT) dir = 1;
    if ((u->pressed & BTN_CROSS) && u->setup_sel != rows - 1) dir = 1;
    if (dir) {
        if (u->setup_sel == 0) {
            u->setup_players += dir;
            if (u->setup_players < 2) u->setup_players = MAX_PLAYERS;
            if (u->setup_players > MAX_PLAYERS) u->setup_players = 2;
        } else if (u->setup_sel <= u->setup_players) {
            int i = u->setup_sel - 1;
            u->setup_type[i] = (u->setup_type[i] + dir + 3) % 3;
        } else if (u->setup_sel == u->setup_players + 1) {
            u->setup_adv ^= 1;
        }
    }
    if (u->setup_sel >= u->setup_players + 3) u->setup_sel = u->setup_players + 2;
    if ((u->pressed & (BTN_CROSS | BTN_START)) && u->setup_sel == u->setup_players + 2) start_game(u);
    else if (u->pressed & BTN_START) start_game(u);
    if (u->pressed & BTN_CIRCLE) u->state = ST_TITLE;
}

static void upd_rules(Ui *u)
{
    if (u->pressed & (BTN_RIGHT | BTN_R1 | BTN_CROSS)) {
        if (u->rules_page < RULES_PAGES - 1) u->rules_page++;
        else if (u->pressed & BTN_CROSS) u->state = u->prev_state;
    }
    if (u->pressed & (BTN_LEFT | BTN_L1)) if (u->rules_page > 0) u->rules_page--;
    if (u->pressed & (BTN_CIRCLE | BTN_START)) u->state = u->prev_state;
}

static void upd_roll(Ui *u)
{
    Game *g = &u->g;
    if (g->phase == PHASE_ROLL) {
        int cpu = is_cpu(u, g->start_player);
        u->timer++;
        if ((cpu && u->timer > 50) || (!cpu && (u->pressed & BTN_CROSS))) {
            game_roll(g);
            u->timer = 0;
            u->pick_sel = 0;
        }
        if (!cpu && (u->pressed & BTN_START)) enter_pause(u);
        return;
    }
    /* animacao da rolagem */
    if (++u->timer >= 50) begin_turn(u);
}

static void upd_pick(Ui *u)
{
    Game *g = &u->g;
    if (u->banner_timer > 0) {
        u->banner_timer--;
        if (u->pressed) { u->banner_timer = 0; u->pressed &= ~(BTN_CROSS | BTN_CIRCLE); }
    }
    if (u->pressed & BTN_LEFT) u->pick_sel = (u->pick_sel + g->pool_n - 1) % g->pool_n;
    if (u->pressed & BTN_RIGHT) u->pick_sel = (u->pick_sel + 1) % g->pool_n;
    if (u->pressed & (BTN_L2 | BTN_L1)) cycle_view(u, -1);
    if (u->pressed & (BTN_R2 | BTN_R1 | BTN_SELECT)) cycle_view(u, 1);
    if (u->pressed & BTN_CROSS) {
        u->view_player = -1;
        u->adj_value = g->pool[u->pick_sel].value;
        u->adj_color = g->pool[u->pick_sel].color;
        u->place_row = -1;
        fix_place_row(u);
        u->sel_active = 1;
        u->state = ST_PLACE;
    }
    if (u->pressed & BTN_START) enter_pause(u);
}

static void upd_place(Ui *u)
{
    Game *g = &u->g;
    Player *p = cur(u);
    int avail = powers_available(p);
    if (u->pressed & (BTN_LEFT | BTN_RIGHT)) {
        int nv = u->adj_value + ((u->pressed & BTN_RIGHT) ? 1 : -1);
        int ov = u->adj_value;
        if (nv >= 1 && nv <= 6) {
            u->adj_value = nv;
            if (sel_cost(u) > avail) {
                u->adj_value = ov;
                set_msg(u, COL_BAD, avail ? "Sem poderes do porco suficientes!" :
                        "Você não tem poderes do porco. Complete uma linha do porco!");
            } else fix_place_row(u);
        }
    }
    if (u->pressed & BTN_TRIANGLE) {
        if (!g->advanced) set_msg(u, COL_BAD, "Trocar a cor só no modo avançado.");
        else {
            int k, oc = u->adj_color;
            for (k = 1; k <= 3; k++) {
                u->adj_color = (oc - 1 + k) % 3 + 1;
                if (sel_cost(u) <= avail) break;
            }
            if (sel_cost(u) > avail) u->adj_color = oc;
            if (u->adj_color == oc && k > 3) set_msg(u, COL_BAD, "Sem poderes do porco para trocar a cor!");
            fix_place_row(u);
        }
    }
    if (u->pressed & (BTN_UP | BTN_DOWN)) {
        int dir = (u->pressed & BTN_DOWN) ? 1 : -1, r = u->place_row, k;
        for (k = 0; k < GARDEN_ROWS; k++) {
            r = (r + dir + GARDEN_ROWS) % GARDEN_ROWS;
            if (can_plant_at(p, u->adj_color, u->adj_value, r)) { u->place_row = r; break; }
        }
    }
    if (u->pressed & (BTN_CROSS | BTN_SQUARE)) {
        Move m;
        memset(&m, 0, sizeof(m));
        m.die_index = u->pick_sel;
        m.new_value = u->adj_value;
        m.new_color = u->adj_color;
        if (can_plant(p, u->adj_color, u->adj_value)) {
            if (u->pressed & BTN_SQUARE) {
                set_msg(u, COL_BAD, "Só dá para alimentar o porco com um dado que não pode ser plantado.");
                return;
            }
            fix_place_row(u);
            m.row = u->place_row;
            m.col = u->adj_value - 1;
        } else {
            m.feed_pig = 1;
        }
        do_move(u, &m);
        return;
    }
    if (u->pressed & BTN_CIRCLE) {
        u->sel_active = 0;
        u->state = ST_PICK;
    }
    if (u->pressed & BTN_START) enter_pause(u);
}

static void upd_cpu(Ui *u)
{
    const Move *m = &u->cpu_move;
    const Die *d = &u->g.pool[m->die_index];
    int speed = (u->held & BTN_CROSS) ? 4 : 1;
    int t;
    u->timer += speed;
    t = u->timer;
    if (u->pressed & BTN_START) { enter_pause(u); return; }
    if (t < 30) {
        /* cursor caminha ate o dado escolhido */
        if ((t / speed) % 6 == 0 && u->pick_sel != m->die_index)
            u->pick_sel += u->pick_sel < m->die_index ? 1 : -1;
        return;
    }
    if (!u->sel_active) {
        u->pick_sel = m->die_index;
        u->sel_active = 1;
        u->adj_value = d->value;
        u->adj_color = d->color;
        u->place_row = -1;
        fix_place_row(u);
    }
    /* aplica os poderes um a um, para ficar visivel */
    if (t >= 45 && (t % 16) < speed) {
        if (u->adj_color != m->new_color) { u->adj_color = m->new_color; fix_place_row(u); return; }
        if (u->adj_value != m->new_value) {
            u->adj_value += u->adj_value < m->new_value ? 1 : -1;
            fix_place_row(u);
            return;
        }
    }
    if (u->adj_color == m->new_color && u->adj_value == m->new_value) {
        if (!m->feed_pig) u->place_row = m->row;
        if (t >= 95) do_move(u, m);
    }
}

static void upd_market(Ui *u)
{
    Game *g = &u->g;
    u->timer++;
    if ((u->timer > 25 && (u->pressed & BTN_CROSS)) || u->timer > 150) {
        int c = g->market_color;
        int full_before = g->market[c - 1] >= MARKET_MAX;
        char b[128];
        game_resolve_market(g);
        if (!full_before) {
            snprintf(b, sizeof(b), "Mercado de %s sobe: cada %s vale %d.", VEG_PLURAL[c], VEG_SING[c], veg_value(g, c));
            set_msg(u, COL_YELLOW, b);
        }
        if (g->phase == PHASE_GAME_OVER) { u->state = ST_GAMEOVER; u->timer = 0; }
        else { u->state = ST_ROLL; u->timer = 0; }
    }
}

static void upd_pause(Ui *u)
{
    if (u->pressed & BTN_UP) u->pause_sel = (u->pause_sel + 2) % 3;
    if (u->pressed & BTN_DOWN) u->pause_sel = (u->pause_sel + 1) % 3;
    if (u->pressed & (BTN_CIRCLE | BTN_START)) { u->state = u->prev_state; return; }
    if (u->pressed & BTN_CROSS) {
        if (u->pause_sel == 0) u->state = u->prev_state;
        else if (u->pause_sel == 1) { u->rules_page = 0; u->state = ST_RULES; u->prev_state = ST_PAUSE; }
        else { u->state = ST_TITLE; u->menu_sel = 0; }
    }
}

void ui_update(Ui *u, unsigned b)
{
    u->frame++;
    u->pressed = b & ~u->held;
    if (u->pressed & DIRS) u->repeat_timer = 18;
    else if (b & DIRS & u->held) {
        if (--u->repeat_timer <= 0) { u->pressed |= b & DIRS; u->repeat_timer = 6; }
    }
    u->held = b;
    if (u->msg_timer > 0) u->msg_timer--;

    switch (u->state) {
    case ST_TITLE: upd_title(u); break;
    case ST_SETUP: upd_setup(u); break;
    case ST_RULES:
        if (u->prev_state == ST_PAUSE) {
            /* volta para a pausa, que lembra de onde veio */
            int before = u->state;
            upd_rules(u);
            if (u->state != before) u->prev_state = s_pause_return;
        } else upd_rules(u);
        break;
    case ST_ROLL: upd_roll(u); break;
    case ST_PICK: upd_pick(u); break;
    case ST_PLACE: upd_place(u); break;
    case ST_CPU: upd_cpu(u); break;
    case ST_MARKET: upd_market(u); break;
    case ST_GAMEOVER:
        u->timer++;
        if (u->timer > 40 && (u->pressed & (BTN_CROSS | BTN_START))) { u->state = ST_TITLE; u->menu_sel = 0; }
        break;
    case ST_PAUSE:
        upd_pause(u);
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Desenho                                                             */

static int pulse(const Ui *u, int lo, int hi)
{
    int t = (int)(u->frame % 60);
    int k = t < 30 ? t : 60 - t;
    return lo + (hi - lo) * k / 30;
}

static void draw_background(const Ui *u)
{
    int i;
    (void)u;
    for (i = 0; i < SCREEN_H; i += 32)
        plat_rect(0, (float)i, SCREEN_W, 16, CRGB(42, 86, 36));
}

static void draw_header(const Ui *u)
{
    const Game *g = &u->g;
    char b[96];
    plat_rect(0, 0, SCREEN_W, 32, CRGB(60, 38, 22));
    plat_rect(0, 30, SCREEN_W, 3, CRGB(40, 24, 14));
    gfx_veg(28, 17, 22, VEG_CARROT, 255);
    {
        int w = gfx_text_shadow(ATLAS_SMALL, 44, 7, "Horta de Dados", COL_YELLOW);
        gfx_text_shadow(ATLAS_SMALL, 50.0f + (float)w, 7, "PS2", CRGB(140, 200, 255));
    }
    snprintf(b, sizeof(b), "Rodada %d  ·  Modo %s", g->round, g->advanced ? "Avançado" : "Básico");
    gfx_text_center(ATLAS_SMALL, 340, 7, b, COL_WHITE);
    if (g->last_round && (u->frame / 20) % 2 == 0)
        gfx_text_shadow(ATLAS_SMALL, 504, 7, "ÚLTIMA RODADA!", COL_BAD);
    else if (!g->last_round) {
        snprintf(b, sizeof(b), "Vez: %s", g->p[g->current].name);
        gfx_text_shadow(ATLAS_SMALL, 624 - gfx_text_width(ATLAS_SMALL, b), 7, b, COL_WHITE);
    }
}

static void draw_pool(const Ui *u)
{
    const Game *g = &u->g;
    int i;
    gfx_panel(16, 38, 400, 64, COL_PANEL);
    if (u->state == ST_ROLL && g->phase == PHASE_ROLL) {
        char b[96];
        if (is_cpu(u, g->start_player))
            snprintf(b, sizeof(b), "%s vai rolar os dados...", g->p[g->start_player].name);
        else
            snprintf(b, sizeof(b), "%s: aperte X para rolar os dados!", g->p[g->start_player].name);
        gfx_text_center(ATLAS_SMALL, 216, 61, b, (u->frame / 20) % 2 ? COL_YELLOW : COL_WHITE);
        return;
    }
    if (g->pool_n == 0) return;
    for (i = 0; i < g->pool_n; i++) {
        float x = 28.0f + (float)i * 43.0f, y = 50.0f;
        int v = g->pool[i].value, c = g->pool[i].color, hl = 0;
        if (u->state == ST_ROLL) {
            /* rolando: valores e posicoes "pulando" */
            v = 1 + (int)((u->frame / 3 + (unsigned)i * 7) % 6);
            y += (float)(((u->frame + (unsigned)i * 5) % 8) < 4 ? -3 : 1);
            if (u->timer > 40) v = g->pool[i].value;
        }
        if ((u->state == ST_PICK || u->state == ST_PLACE || u->state == ST_CPU) && i == u->pick_sel) {
            hl = 1;
            if (u->sel_active) { y -= 4; v = u->adj_value; c = u->adj_color; }
        }
        if (u->state == ST_MARKET) hl = (u->frame / 15) % 2;
        gfx_die(x, y, 38, c, v, hl);
        if (u->sel_active && i != u->pick_sel) plat_rect(x - 2, y - 2, 42, 42, CRGBA(70, 46, 28, 140));
    }
}

static void draw_market(const Ui *u)
{
    const Game *g = &u->g;
    int k;
    gfx_panel(424, 38, 200, 120, COL_PANEL);
    gfx_text_center(ATLAS_SMALL, 524, 42, "Mercado", COL_YELLOW);
    for (k = 0; k < NUM_VEG; k++) {
        float cx = 460.0f + (float)k * 64.0f;
        int j, veg = k + 1;
        char b[32];
        gfx_rrect(cx - 28, 60, 56, 92, 6, CRGB(120, 82, 48));
        gfx_veg(cx, 76, 26, veg, 255);
        for (j = 0; j < MARKET_MAX; j++) {
            float x = cx + (j % 2 ? 9.0f : -9.0f), y = 98.0f + (float)(j / 2) * 15.0f;
            int marked = j < g->market[k];
            int ghost = u->state == ST_MARKET && g->market_color == veg && j == g->market[k];
            gfx_circle(x, y, 6.5f, COL_PAPER);
            if (marked) {
                gfx_circle(x, y, 5.0f, veg_color(veg));
                gfx_line(x - 3, y - 3, x + 3, y + 3, 2, COL_WHITE);
                gfx_line(x - 3, y + 3, x + 3, y - 3, 2, COL_WHITE);
            } else if (ghost && (u->frame / 10) % 2) {
                gfx_circle(x, y, 5.0f, COL_HILITE);
            }
        }
        snprintf(b, sizeof(b), "vale %d", veg_value(g, veg));
        gfx_text_center(ATLAS_SMALL, cx, 131, b, COL_WHITE);
    }
}

static int viewed(const Ui *u)
{
    return u->view_player >= 0 ? u->view_player : u->g.current;
}

static void draw_garden(const Ui *u)
{
    const Game *g = &u->g;
    int pi = viewed(u);
    const Player *p = &g->p[pi];
    int r, c;
    int preview = u->sel_active && pi == g->current;
    int plantable = preview && can_plant(p, u->adj_color, u->adj_value);
    char b[96];

    gfx_panel(16, 108, 400, 212, COL_PANEL);
    for (c = 0; c < GARDEN_COLS; c++) {
        float x = 32.0f + (float)c * 56.0f;
        int hl = preview && c == u->adj_value - 1;
        gfx_die(x + 15, 115, 26, 0, c + 1, hl);
    }
    for (r = 0; r < GARDEN_ROWS; r++) {
        int full = 1;
        for (c = 0; c < GARDEN_COLS; c++) {
            float x = 32.0f + (float)c * 56.0f, y = 148.0f + (float)r * 54.0f;
            int veg = p->garden[r][c];
            Color bg = COL_PAPER;
            if (veg == VEG_NONE) full = 0;
            if (preview && c == u->adj_value - 1) bg = plantable ? CRGB(255, 248, 200) : CRGB(250, 205, 195);
            gfx_rrect(x + 2, y + 2, 52, 50, 6, bg);
            if (veg) gfx_veg(x + 28, y + 27, 40, veg, 255);
            if (preview && plantable && c == u->adj_value - 1 && veg == VEG_NONE &&
                can_plant_at(p, u->adj_color, u->adj_value, r)) {
                if (r == u->place_row) {
                    gfx_frame(x, y, 56, 54, 4, CRGB(255, 200, 0));
                    gfx_veg(x + 28, y + 27, 40, u->adj_color, pulse(u, 110, 220));
                } else {
                    gfx_frame(x + 3, y + 3, 50, 48, 2, CRGBA(230, 170, 0, pulse(u, 80, 200)));
                }
            }
        }
        gfx_ribbon(392, 166.0f + (float)r * 54.0f, 28, full);
    }
    if (preview && !plantable) {
        gfx_rrect(40, 282, 352, 32, 6, CRGBA(120, 20, 20, 220));
        snprintf(b, sizeof(b), "Não dá para plantar! X: alimentar o porco (+%d)", u->adj_value);
        gfx_text_center(ATLAS_SMALL, 216, 289, b, COL_WHITE);
    }
    if (pi != g->current) {
        gfx_rrect(80, 110, 272, 24, 6, CRGBA(20, 60, 140, 230));
        snprintf(b, sizeof(b), "Vendo: horta de %s", p->name);
        gfx_text_center(ATLAS_SMALL, 216, 113, b, COL_WHITE);
    }
}

static void draw_pig(const Ui *u)
{
    const Game *g = &u->g;
    int pi = viewed(u);
    const Player *p = &g->p[pi];
    int i, j, rows = pig_rows_done(p);
    int preview = u->sel_active && pi == g->current && !can_plant(p, u->adj_color, u->adj_value);
    int ghost_to = preview ? p->pig + u->adj_value : p->pig;
    int using = (u->sel_active && pi == g->current) ? sel_cost(u) : 0;
    char b[64];

    gfx_panel(424, 164, 200, 156, COL_PANEL);
    gfx_pig(446, 186, 30);
    gfx_text_shadow(ATLAS_SMALL, 468, 170, "Porco", COL_PIG);
    snprintf(b, sizeof(b), "Poderes: %d", powers_available(p) - using);
    gfx_text_shadow(ATLAS_SMALL, 530, 170, b, powers_available(p) - using > 0 ? COL_YELLOW : CRGB(200, 190, 170));
    if (using) {
        snprintf(b, sizeof(b), "(usando %d)", using);
        gfx_text_shadow(ATLAS_SMALL, 530, 186, b, CRGB(255, 160, 120));
    } else {
        snprintf(b, sizeof(b), "%d/%d", p->pig, PIG_CIRCLES);
        gfx_text_shadow(ATLAS_SMALL, 468, 186, b, CRGB(230, 220, 200));
    }
    for (i = 0; i < PIG_ROWS; i++) {
        float y = 214.0f + (float)i * 15.0f;
        int done = i < rows, used = i < p->powers_used + using;
        for (j = 0; j < PIG_COLS; j++) {
            float x = 442.0f + (float)j * 15.0f;
            int idx = i * PIG_COLS + j;
            gfx_circle(x, y, 6.5f, COL_PAPER);
            if (idx < p->pig) {
                gfx_line(x - 4, y - 4, x + 4, y + 4, 2, COL_INK);
                gfx_line(x - 4, y + 4, x + 4, y - 4, 2, COL_INK);
            } else if (idx < ghost_to && (u->frame / 8) % 2) {
                gfx_circle(x, y, 5.0f, COL_PIG_DARK);
            }
        }
        /* caixa do poder */
        gfx_rrect(533, y - 6, 13, 13, 2, done ? (used ? CRGB(120, 110, 100) : CRGB(80, 170, 80)) : CRGB(150, 130, 110));
        gfx_text(ATLAS_SMALL, 535, y - 9, "*", done && !used ? COL_PIG : CRGB(200, 190, 180));
        if (done && used) gfx_line(533, y + 6, 546, y - 6, 2, COL_INK);
        snprintf(b, sizeof(b), "%d", PIG_ROW_VALUES[i]);
        gfx_text(ATLAS_SMALL, 556, y - 9, b, i == rows - 1 ? COL_YELLOW : CRGB(230, 220, 200));
        if (i == rows - 1) gfx_text(ATLAS_SMALL, 578, y - 9, "pts", COL_YELLOW);
    }
}

static const char *hint_text(const Ui *u)
{
    const Game *g = &u->g;
    switch (u->state) {
    case ST_ROLL: return g->phase == PHASE_ROLL && !is_cpu(u, g->start_player) ? "X: rolar os dados   START: pausa" : "Rolando...";
    case ST_PICK: return "Esq/Dir: dado   X: pegar   L/R: ver hortas   START: pausa";
    case ST_PLACE:
        if (g->advanced) return "X: ok  O: voltar  Cima/Baixo: linha  Esq/Dir: valor  Tri: cor";
        return "X: confirmar   O: voltar   Cima/Baixo: linha   Esq/Dir: poder";
    case ST_CPU: return "A CPU está pensando...  (segure X para acelerar)";
    case ST_MARKET: return "X: continuar";
    }
    return "";
}

static void draw_status(const Ui *u)
{
    gfx_rrect(16, 326, 608, 24, 6, CRGBA(20, 40, 16, 220));
    if (u->msg_timer > 0 && u->msg[0]) {
        int a = u->msg_timer > 25 ? 255 : u->msg_timer * 10;
        gfx_text_center(ATLAS_SMALL, 320, 329, u->msg, color_alpha(u->msg_color, a));
    } else {
        gfx_text_center(ATLAS_SMALL, 320, 329, hint_text(u), CRGB(220, 230, 200));
    }
}

static void draw_scores(const Ui *u)
{
    const Game *g = &u->g;
    Score s[MAX_PLAYERS];
    int n = g->num_players, i, k;
    float w = (608.0f - (float)(n - 1) * 8.0f) / (float)n;
    char b[64];
    game_score(g, s);
    for (i = 0; i < n; i++) {
        const Player *p = &g->p[i];
        float x = 16.0f + (float)i * (w + 8.0f), y = 356.0f;
        int is_cur = i == g->current && u->state != ST_MARKET && u->state != ST_ROLL;
        if (is_cur) gfx_rrect(x - 3, y - 3, w + 6, 88, 9, COL_YELLOW);
        else if (i == viewed(u)) gfx_rrect(x - 3, y - 3, w + 6, 88, 9, CRGB(90, 150, 240));
        gfx_rrect(x, y, w, 82, 7, COL_PAPER);
        gfx_text(ATLAS_SMALL, x + 8, y + 4, p->name, COL_INK);
        if (p->is_cpu) gfx_text(ATLAS_SMALL, x + 12 + gfx_text_width(ATLAS_SMALL, p->name), y + 4, "CPU", CRGB(120, 120, 160));
        if (i == g->start_player) gfx_pig(x + w - 14, y + 13, 18);
        for (k = 0; k < NUM_VEG; k++) {
            float ix = x + 8.0f + (float)k * (w - 16.0f) / 3.0f;
            gfx_veg(ix + 8, y + 32, 18, k + 1, 255);
            snprintf(b, sizeof(b), "%d", s[i].veg_count[k]);
            gfx_text(ATLAS_SMALL, ix + 19, y + 23, b, COL_INK);
        }
        gfx_pig(x + 16, y + 55, 16);
        snprintf(b, sizeof(b), "%d  * %d", p->pig, powers_available(p));
        gfx_text(ATLAS_SMALL, x + 28, y + 46, b, COL_INK);
        snprintf(b, sizeof(b), "%d pts", s[i].total);
        gfx_text(ATLAS_SMALL, x + w - 8 - gfx_text_width(ATLAS_SMALL, b), y + 62, b, CRGB(30, 110, 40));
    }
}

static void draw_banner(const Ui *u, const char *line1, const char *line2)
{
    int a = u->banner_timer > 20 ? 230 : u->banner_timer * 11;
    gfx_rrect(110, 170, 420, 96, 12, CRGBA(30, 20, 10, a));
    gfx_text_center(ATLAS_BIG, 320, 184, line1, color_alpha(COL_YELLOW, a + 25));
    if (line2) gfx_text_center(ATLAS_SMALL, 320, 230, line2, color_alpha(COL_WHITE, a + 25));
}

static void draw_board(Ui *u)
{
    draw_background(u);
    draw_header(u);
    draw_pool(u);
    draw_market(u);
    draw_garden(u);
    draw_pig(u);
    draw_status(u);
    draw_scores(u);

    if (u->state == ST_PICK && u->banner_timer > 0) {
        char b[64];
        snprintf(b, sizeof(b), "Vez de %s", u->g.p[u->g.current].name);
        draw_banner(u, b, humans(u) > 1 ? "Passe o controle! Aperte qualquer botão." : NULL);
    }
    if (u->state == ST_MARKET) {
        const Game *g = &u->g;
        int c = g->market_color;
        char b[128];
        gfx_rrect(124, 154, 400, 120, 12, COL_SHADOW);
        gfx_rrect(120, 150, 400, 120, 12, CRGB(40, 28, 16));
        gfx_text_center(ATLAS_SMALL, 320, 160, "Sobrou um dado na reserva:", COL_WHITE);
        gfx_die(300, 182, 40, c, g->pool[0].value, 0);
        if (g->market[c - 1] < MARKET_MAX)
            snprintf(b, sizeof(b), "Mercado de %s sobe! Agora vale %d.", VEG_MARKET[c], veg_value(g, c) + 1);
        else
            snprintf(b, sizeof(b), "O mercado de %s já está cheio.", VEG_PLURAL[c]);
        gfx_text_center(ATLAS_SMALL, 320, 230, b, COL_YELLOW);
        gfx_text_center(ATLAS_SMALL, 320, 248, "(só a cor importa)", CRGB(200, 190, 170));
    }
}

static void draw_title(Ui *u)
{
    int i;
    static const char *items[2] = { "Novo jogo", "Como jogar" };
    draw_background(u);
    for (i = 0; i < 6; i++) {
        float x = 70.0f + (float)i * 100.0f;
        float bob = (float)(((u->frame / 4 + (unsigned)i * 3) % 16) < 8 ? 0 : 3);
        gfx_veg(x, 52 + bob, 40, i % 3 + 1, 255);
        gfx_veg(x + 30, 404 - bob, 40, (i + 1) % 3 + 1, 255);
    }
    gfx_panel(90, 92, 460, 140, COL_PANEL);
    gfx_text_center(ATLAS_BIG, 320, 112, "HORTA DE DADOS", COL_YELLOW);
    gfx_rrect(270, 156, 100, 30, 8, CRGB(30, 60, 160));
    gfx_text_center(ATLAS_SMALL, 320, 162, "PS2", COL_WHITE);
    gfx_text_center(ATLAS_SMALL, 320, 198, "Role, plante, alimente o porco e venda bem!", CRGB(240, 230, 210));
    for (i = 0; i < 3; i++) {
        unsigned v = (u->frame / 20 + (unsigned)i * 2) % 6 + 1;
        gfx_die(36.0f + (float)i * 6.0f, 110.0f + (float)i * 44.0f, 34, i + 1, (int)v, 0);
        gfx_die(568.0f - (float)i * 6.0f, 110.0f + (float)i * 44.0f, 34, 3 - i, (int)(7 - v), 0);
    }
    for (i = 0; i < 2; i++) {
        float y = 260.0f + (float)i * 40.0f;
        int sel = u->menu_sel == i;
        gfx_rrect(220, y, 200, 32, 10, sel ? COL_YELLOW : CRGBA(0, 0, 0, 120));
        gfx_text_center(ATLAS_SMALL, 320, y + 7, items[i], sel ? COL_INK : COL_WHITE);
    }
    gfx_text_center(ATLAS_SMALL, 320, 350, "Direcional: escolher    X: confirmar", CRGB(220, 230, 200));
}

static void draw_setup(Ui *u)
{
    int rows = u->setup_players + 3, i;
    char b[96];
    draw_background(u);
    gfx_panel(100, 18, 440, 412, COL_PANEL);
    gfx_text_center(ATLAS_BIG, 320, 30, "Nova partida", COL_YELLOW);
    for (i = 0; i < rows; i++) {
        float y = 84.0f + (float)i * 36.0f;
        int sel = u->setup_sel == i;
        const char *label, *val;
        if (i == 0) { label = "Jogadores"; snprintf(b, sizeof(b), "%d", u->setup_players); val = b; }
        else if (i <= u->setup_players) {
            static char lb[32];
            snprintf(lb, sizeof(lb), "Jogador %d", i);
            label = lb;
            val = TYPE_NAMES[u->setup_type[i - 1]];
        } else if (i == u->setup_players + 1) { label = "Modo"; val = u->setup_adv ? "Avançado" : "Básico"; }
        else { label = NULL; val = NULL; }
        gfx_rrect(130, y, 380, 32, 8, sel ? COL_YELLOW : CRGBA(0, 0, 0, 90));
        if (label) {
            gfx_text(ATLAS_SMALL, 146, y + 7, label, sel ? COL_INK : COL_WHITE);
            char vb[48];
            snprintf(vb, sizeof(vb), "<  %.30s  >", val);
            gfx_text(ATLAS_SMALL, 494 - gfx_text_width(ATLAS_SMALL, vb), y + 7, vb, sel ? COL_INK : COL_WHITE);
        } else {
            gfx_text_center(ATLAS_SMALL, 320, y + 7, "Começar!", sel ? COL_INK : COL_WHITE);
        }
    }
    gfx_text_wrap(ATLAS_SMALL, 130, 84.0f + (float)rows * 36.0f + 6, 380, 17,
                  u->setup_adv ? TXT_ADV_DESC : TXT_BASIC_DESC, CRGB(230, 220, 200));
    gfx_text_center(ATLAS_SMALL, 320, 404, "Cima/Baixo: opção   Esq/Dir: mudar   X: confirmar   O: voltar", CRGB(220, 230, 200));
}

static void draw_rules(Ui *u)
{
    char b[48];
    draw_background(u);
    gfx_panel(40, 24, 560, 400, COL_PAPER);
    gfx_text_center(ATLAS_BIG, 320, 36, RULES[u->rules_page].title, COL_CARROT);
    gfx_text_wrap(ATLAS_SMALL, 64, 92, 512, 19, RULES[u->rules_page].body, COL_INK);
    snprintf(b, sizeof(b), "Página %d/%d", u->rules_page + 1, RULES_PAGES);
    gfx_text(ATLAS_SMALL, 64, 392, b, CRGB(120, 100, 80));
    gfx_text(ATLAS_SMALL, 300, 392, "Esq/Dir: páginas   O: voltar", CRGB(120, 100, 80));
}

static void draw_pause(Ui *u)
{
    static const char *items[3] = { "Continuar", "Como jogar", "Sair para o título" };
    int i;
    plat_rect(0, 0, SCREEN_W, SCREEN_H, CRGBA(0, 0, 0, 150));
    gfx_panel(190, 120, 260, 200, COL_PANEL);
    gfx_text_center(ATLAS_BIG, 320, 132, "Pausa", COL_YELLOW);
    for (i = 0; i < 3; i++) {
        float y = 190.0f + (float)i * 40.0f;
        int sel = u->pause_sel == i;
        gfx_rrect(220, y, 200, 32, 10, sel ? COL_YELLOW : CRGBA(0, 0, 0, 120));
        gfx_text_center(ATLAS_SMALL, 320, y + 7, items[i], sel ? COL_INK : COL_WHITE);
    }
}

static void draw_gameover(Ui *u)
{
    const Game *g = &u->g;
    Score s[MAX_PLAYERS];
    int win[MAX_PLAYERS];
    int n = g->num_players, i, r, nw = 0;
    static const char *labels[] = { "Cenouras", "Alfaces", "Tomates", "Laços", "Porco", "Bônus", "TOTAL" };
    int nrows = g->advanced ? 7 : 6;
    float colw = 440.0f / (float)n;
    char b[160];

    game_score(g, s);
    game_winners(g, s, win);
    draw_background(u);
    gfx_panel(24, 16, 592, 416, COL_PAPER);
    gfx_text_center(ATLAS_BIG, 320, 26, "Fim de Jogo!", COL_CARROT);

    for (i = 0; i < n; i++) {
        float x = 168.0f + (float)i * colw;
        if (win[i]) gfx_rrect(x + 2, 74, colw - 4, 30 + (float)nrows * 30 + 6, 8, CRGB(255, 236, 150));
        gfx_text_center(ATLAS_SMALL, x + colw / 2, 80, g->p[i].name, COL_INK);
    }
    for (r = 0; r < nrows; r++) {
        int row = (!g->advanced && r == 5) ? 6 : r;
        float y = 110.0f + (float)r * 30.0f;
        plat_rect(44, y - 4, 552, 2, COL_PAPER_DARK);
        gfx_text(ATLAS_SMALL, 52, y + 2, labels[row], row == 6 ? COL_CARROT : COL_INK);
        if (row < 3) gfx_veg(140, y + 10, 20, row + 1, 255);
        if (row == 4) gfx_pig(140, y + 11, 20);
        for (i = 0; i < n; i++) {
            float x = 168.0f + (float)i * colw + colw / 2;
            switch (row) {
            case 0: case 1: case 2:
                snprintf(b, sizeof(b), "%d×%d = %d", s[i].veg_count[row], veg_value(g, row + 1), s[i].veg_points[row]);
                break;
            case 3: snprintf(b, sizeof(b), "%d×5 = %d", s[i].ribbons, s[i].ribbon_points); break;
            case 4: snprintf(b, sizeof(b), "%d", s[i].pig_points); break;
            case 5: snprintf(b, sizeof(b), "%d", s[i].bonus_points); break;
            default: snprintf(b, sizeof(b), "%d", s[i].total); break;
            }
            gfx_text(ATLAS_SMALL, x - (float)gfx_text_width(ATLAS_SMALL, b) / 2, y + 2, b,
                     row == 6 ? CRGB(30, 110, 40) : COL_INK);
        }
    }
    b[0] = 0;
    for (i = 0; i < n; i++) if (win[i]) nw++;
    if (nw == 1) {
        for (i = 0; i < n; i++) if (win[i]) snprintf(b, sizeof(b), "%s venceu!", g->p[i].name);
    } else {
        int k = 0;
        size_t L;
        snprintf(b, sizeof(b), "Empate! Vitória dividida: ");
        for (i = 0; i < n; i++) if (win[i]) {
            L = strlen(b);
            snprintf(b + L, sizeof(b) - L, "%s%s", k++ ? " e " : "", g->p[i].name);
        }
    }
    gfx_rrect(80, 330, 480, 44, 10, CRGB(40, 110, 40));
    gfx_text_center(n == 1 || nw == 1 ? ATLAS_BIG : ATLAS_SMALL, 320, nw == 1 ? 333 : 343, b, COL_YELLOW);
    if (u->timer > 40) gfx_text_center(ATLAS_SMALL, 320, 392, "X: voltar ao título", COL_INK);
}

void ui_draw(Ui *u)
{
    switch (u->state) {
    case ST_TITLE: draw_title(u); break;
    case ST_SETUP: draw_setup(u); break;
    case ST_RULES: draw_rules(u); break;
    case ST_GAMEOVER: draw_gameover(u); break;
    case ST_PAUSE:
        if (s_pause_return == ST_TITLE) draw_title(u);
        else draw_board(u);
        draw_pause(u);
        break;
    default: draw_board(u); break;
    }
}
