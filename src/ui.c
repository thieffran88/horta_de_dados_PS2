/* Horta de Dados PS2 - telas, menus e fluxo de jogo. */
#include <stdio.h>
#include <string.h>
#include "ui.h"
#include "ai.h"
#include "gfx.h"
#include "lang.h"
#include "audio.h"

#define DIRS (BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT)

/* ---- layout do tabuleiro (640x448) ---- */
#define POOL_X 16
#define POOL_Y 40
#define MKT_X 424
#define MKT_Y 40
#define GARDEN_X 16
#define GARDEN_Y 110
#define GRID_PX 28.0f
#define GRID_PY 156.0f
#define PIG_X 424
#define PIG_Y 170
#define BOARD_PX 430.0f
#define BOARD_PY 214.0f
#define BOARD_SC 0.86f
#define HINT_Y 334
#define SCORE_Y 366
/* rolagem: cada dado gira com um pequeno atraso e "cai" com um brilho */
#define ROLL_LAND(i) (22 + (i) * 3)
#define ROLL_FRAMES 66

/* ------------------------------------------------------------------ */

static void set_msg(Ui *u, Color c, const char *s)
{
    snprintf(u->msg, sizeof(u->msg), "%s", s);
    u->msg_color = c;
    u->msg_timer = 150;
    if (c == COL_BAD) audio_sfx(SFX_ERROR);
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
    u->state = ST_SPLASH;        /* arte de abertura, depois a escolha do idioma */
    u->lang_sel = LANG_PT;
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
    char b[96];
    for (i = 0; i < MAX_PLAYERS; i++) {
        Player *p = &u->g.p[i];
        p->is_cpu = u->setup_type[i] != 0;
        p->cpu_level = u->setup_type[i] == 1 ? 0 : 1;
        if (p->is_cpu) snprintf(p->name, sizeof(p->name), "%s", cpu_name(i));
        else snprintf(p->name, sizeof(p->name), T(S_PLAYER_N), (++h) & 7);
    }
    game_init(&u->g, u->setup_players, u->setup_adv, seed);
    u->state = ST_ROLL;
    u->timer = 0;
    u->view_player = -1;
    u->sel_active = 0;
    u->msg_timer = 0;
    snprintf(b, sizeof(b), T(S_STARTS), u->g.p[u->g.start_player].name);
    set_msg(u, COL_YELLOW, b);
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
        u->banner_timer = humans(u) > 1 ? 90 : 60; /* mostra qual controle usar */
        audio_sfx(SFX_CLUCK);
    }
}

static void do_move(Ui *u, const Move *m)
{
    char b[128];
    int who = u->g.current;
    int was_last = u->g.last_round;
    Player *p = &u->g.p[who];
    int rows_before = pig_rows_done(p);
    int r, c, full_before = 0, full_after = 0;
    for (r = 0; r < GARDEN_ROWS; r++) {
        int f = 1;
        for (c = 0; c < GARDEN_COLS; c++) if (!p->garden[r][c]) f = 0;
        full_before += f;
    }

    if (!game_apply(&u->g, m)) {
        set_msg(u, COL_BAD, T(S_INVALID));
        return;
    }
    for (r = 0; r < GARDEN_ROWS; r++) {
        int f = 1;
        for (c = 0; c < GARDEN_COLS; c++) if (!p->garden[r][c]) f = 0;
        full_after += f;
    }
    audio_sfx(m->feed_pig ? SFX_OINK : SFX_PLANT);
    if (pig_rows_done(p) > rows_before) audio_sfx(SFX_POWER);
    if (full_after > full_before) audio_sfx(SFX_RIBBON);
    if (!was_last && u->g.last_round) audio_sfx(SFX_MOO);
    if (m->feed_pig)
        snprintf(b, sizeof(b), T(pig_rows_done(p) > rows_before ? S_FED_POWER : S_FED), p->name, m->new_value);
    else
        snprintf(b, sizeof(b), T(S_PLANTED), p->name, veg_obj(m->new_color), m->col + 1);
    set_msg(u, COL_WHITE, b);
    if (!was_last && u->g.last_round) {
        snprintf(b, sizeof(b), T(S_TRIGGER), p->name);
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

static void upd_lang(Ui *u)
{
    if (u->pressed & BTN_UP) u->lang_sel = (u->lang_sel + NUM_LANGS - 1) % NUM_LANGS;
    if (u->pressed & BTN_DOWN) u->lang_sel = (u->lang_sel + 1) % NUM_LANGS;
    if (u->pressed & (BTN_CROSS | BTN_START)) {
        g_lang = u->lang_sel;
        u->state = ST_TITLE;
        u->menu_sel = 0;
    }
}

static void upd_splash(Ui *u)
{
    if (u->frame > 20 && (u->pressed & (BTN_START | BTN_CROSS))) {
        u->lang_sel = g_lang;
        u->state = ST_LANG;
    }
}

static void upd_title(Ui *u)
{
    if (u->pressed & BTN_UP) u->menu_sel = (u->menu_sel + 2) % 3;
    if (u->pressed & BTN_DOWN) u->menu_sel = (u->menu_sel + 1) % 3;
    if (u->pressed & (BTN_CROSS | BTN_START)) {
        if (u->menu_sel == 0) { u->state = ST_SETUP; u->setup_sel = 0; }
        else if (u->menu_sel == 1) { u->prev_state = ST_TITLE; u->rules_page = 0; u->state = ST_RULES; }
        else { u->lang_sel = g_lang; u->state = ST_LANG; }
    }
    if (u->pressed & BTN_CIRCLE) { u->lang_sel = g_lang; u->state = ST_LANG; }
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
            audio_sfx(SFX_ROLL);
            u->timer = 0;
            u->pick_sel = 0;
        }
        if (!cpu && (u->pressed & BTN_START)) enter_pause(u);
        return;
    }
    if (++u->timer == ROLL_LAND(g->pool_n - 1)) audio_sfx(SFX_LAND);
    if (u->timer >= ROLL_FRAMES) begin_turn(u); /* animacao da rolagem */
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
                set_msg(u, COL_BAD, T(avail ? S_NO_POWERS_ENOUGH : S_NO_POWERS));
            } else fix_place_row(u);
        }
    }
    if (u->pressed & BTN_TRIANGLE) {
        if (!g->advanced) set_msg(u, COL_BAD, T(S_COLOR_ADV_ONLY));
        else {
            int k, oc = u->adj_color;
            for (k = 1; k <= 3; k++) {
                u->adj_color = (oc - 1 + k) % 3 + 1;
                if (sel_cost(u) <= avail) break;
            }
            if (sel_cost(u) > avail) u->adj_color = oc;
            if (u->adj_color == oc && k > 3) set_msg(u, COL_BAD, T(S_NO_POWER_COLOR));
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
            if (u->pressed & BTN_SQUARE) { set_msg(u, COL_BAD, T(S_ONLY_FEED)); return; }
            fix_place_row(u);
            m.row = u->place_row;
            m.col = u->adj_value - 1;
        } else {
            m.feed_pig = 1;
        }
        do_move(u, &m);
        return;
    }
    if (u->pressed & BTN_CIRCLE) { u->sel_active = 0; u->state = ST_PICK; }
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
            snprintf(b, sizeof(b), T(S_MARKET_MSG), veg_plural(c), veg_sing(c), veg_value(g, c));
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

/* Sons de interface: compara o estado antes/depois de cada quadro. */
typedef struct {
    int state, menu, lang, setup, pause, page, pick, row, view, players, adv, value, color, sel, pool, types;
} UiSnap;

static void snap(const Ui *u, UiSnap *s)
{
    s->state = u->state; s->menu = u->menu_sel; s->lang = u->lang_sel; s->setup = u->setup_sel;
    s->pause = u->pause_sel; s->page = u->rules_page; s->pick = u->pick_sel; s->row = u->place_row;
    s->view = u->view_player; s->players = u->setup_players; s->adv = u->setup_adv;
    s->value = u->adj_value; s->color = u->adj_color; s->sel = u->sel_active; s->pool = u->g.pool_n;
    s->types = u->setup_type[0] | (u->setup_type[1] << 2) | (u->setup_type[2] << 4) | (u->setup_type[3] << 6);
}

static void ui_sounds(const Ui *u, const UiSnap *a, const UiSnap *b)
{
    if (a->state != b->state) {
        int from = a->state, to = b->state;
        if (b->pool != a->pool && to != ST_MARKET) return; /* jogada: o som ja saiu em do_move */
        if (to == ST_MARKET) audio_sfx(SFX_MARKET);
        else if (to == ST_GAMEOVER) audio_sfx(SFX_WIN);
        else if (from == ST_SPLASH) audio_sfx(SFX_ROOSTER);
        else if (from == ST_SETUP && to == ST_ROLL) audio_sfx(SFX_ROOSTER);
        else if (to == ST_ROLL || to == ST_CPU || from == ST_ROLL || from == ST_MARKET || from == ST_CPU) return;
        else if ((from == ST_PLACE && to == ST_PICK) || (from == ST_SETUP && to == ST_TITLE) ||
                 from == ST_PAUSE || from == ST_RULES) audio_sfx(SFX_BACK);
        else audio_sfx(SFX_SELECT);
        return;
    }
    if (b->sel && a->sel && (a->value != b->value || a->color != b->color)) { audio_sfx(SFX_POWER_USE); return; }
    if (a->menu != b->menu || a->lang != b->lang || a->setup != b->setup || a->pause != b->pause ||
        a->page != b->page || (a->pick != b->pick && u->state == ST_PICK) || a->row != b->row ||
        a->view != b->view || a->players != b->players || a->adv != b->adv || a->types != b->types)
        audio_sfx(SFX_TICK);
}

/* Musica de cada tela: fazendinha animada nos menus, tema calmo durante a partida. */
static void ui_music(const Ui *u)
{
    switch (u->state) {
    case ST_ROLL: case ST_PICK: case ST_PLACE: case ST_CPU: case ST_MARKET: case ST_PAUSE:
        audio_music(MUS_GAME);
        break;
    case ST_RULES:
        if (u->prev_state != ST_PAUSE) audio_music(MUS_TITLE);
        break;
    case ST_GAMEOVER:
        audio_music(u->timer > 240 ? MUS_TITLE : -1); /* silencio para a fanfarra */
        break;
    default:
        audio_music(MUS_TITLE);
    }
}

static void ui_update_inner(Ui *u, unsigned pad1, unsigned pad2);

void ui_update(Ui *u, unsigned pad1, unsigned pad2)
{
    UiSnap before, after;
    snap(u, &before);
    ui_update_inner(u, pad1, pad2);
    snap(u, &after);
    ui_sounds(u, &before, &after);
    ui_music(u);
}

int ui_player_pad(int player) { return player == 0 ? 0 : 1; }

/* Quem pode agir agora: -1 = qualquer controle; 0/1 = so aquele controle. */
static int input_owner(const Ui *u)
{
    const Game *g = &u->g;
    switch (u->state) {
    case ST_PICK: case ST_PLACE:
        return ui_player_pad(g->current);
    case ST_ROLL:
        if (g->phase == PHASE_ROLL && !g->p[g->start_player].is_cpu) return ui_player_pad(g->start_player);
        return -1;
    }
    return -1;
}

static void ui_update_inner(Ui *u, unsigned pad1, unsigned pad2)
{
    unsigned pads[2];
    int i, owner;
    pads[0] = pad1;
    pads[1] = pad2;
    u->frame++;
    for (i = 0; i < 2; i++) {
        unsigned b = pads[i];
        u->pad_pressed[i] = b & ~u->pad_held[i];
        if (u->pad_pressed[i] & DIRS) u->pad_repeat[i] = 18;
        else if (b & DIRS & u->pad_held[i]) {
            if (--u->pad_repeat[i] <= 0) { u->pad_pressed[i] |= b & DIRS; u->pad_repeat[i] = 6; }
        }
        u->pad_held[i] = b;
    }
    owner = input_owner(u);
    if (owner < 0) {
        u->pressed = u->pad_pressed[0] | u->pad_pressed[1];
        u->held = u->pad_held[0] | u->pad_held[1];
    } else {
        u->pressed = u->pad_pressed[owner];
        u->held = u->pad_held[owner];
    }
    if (u->msg_timer > 0) u->msg_timer--;

    switch (u->state) {
    case ST_SPLASH: upd_splash(u); break;
    case ST_LANG: upd_lang(u); break;
    case ST_TITLE: upd_title(u); break;
    case ST_SETUP: upd_setup(u); break;
    case ST_RULES:
        if (u->prev_state == ST_PAUSE) {
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
    case ST_PAUSE: upd_pause(u); break;
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

static void draw_background(const Ui *u, int deco)
{
    int i;
    (void)u;
    for (i = 0; i < SCREEN_H; i += 40)
        plat_rect(0, (float)i, SCREEN_W, 20, COL_BG_STRIPE);
    if (deco) {
        for (i = 0; i < 6; i++)
            gfx_spr(i % 2 ? SPR_GRASS2 : SPR_GRASS, -10.0f + (float)i * 118.0f, i % 2 ? 424.0f : 418.0f);
        gfx_spr(SPR_FLOWER, 6, 384);
        gfx_spr(SPR_FLOWER2, 604, 374);
        gfx_spr(SPR_LEAF1, 8, 8);
        gfx_spr(SPR_LEAF2, 608, 8);
    }
}

static void draw_header(const Ui *u)
{
    const Game *g = &u->g;
    char b[96];
    plat_rect(0, 0, SCREEN_W, 36, COL_WOOD_DARK);
    plat_rect(0, 0, SCREEN_W, 32, COL_WOOD);
    plat_rect(0, 4, SCREEN_W, 2, CRGBA(255, 220, 160, 50));
    plat_rect(0, 18, SCREEN_W, 2, CRGBA(120, 66, 30, 110));
    gfx_big(12, 5, "HORTA DE DADOS", 22);
    snprintf(b, sizeof(b), T(S_ROUND_MODE), g->round, T(g->advanced ? S_ADVANCED : S_BASIC));
    gfx_text_center(340, 8, b, COL_WHITE);
    if (g->last_round) {
        if ((u->frame / 20) % 2 == 0)
            gfx_text_shadow(624 - gfx_text_width(T(S_LAST_ROUND)), 8, T(S_LAST_ROUND), COL_YELLOW);
    } else {
        snprintf(b, sizeof(b), T(S_TURN), g->p[g->current].name);
        gfx_text_shadow(624 - gfx_text_width(b), 8, b, COL_WHITE);
    }
}

static void draw_pool(const Ui *u)
{
    const Game *g = &u->g;
    int i;
    gfx_panel(POOL_X, POOL_Y, 400, 64);
    if (u->state == ST_ROLL && g->phase == PHASE_ROLL) {
        char b[96];
        snprintf(b, sizeof(b), T(is_cpu(u, g->start_player) ? S_WILL_ROLL : S_PRESS_ROLL), g->p[g->start_player].name,
                 ui_player_pad(g->start_player) + 1);
        /* tres dados "chacoalhando" na mao antes de rolar */
        for (i = 0; i < 3; i++)
            gfx_dice(POOL_X + 26.0f + (float)i * 14.0f, POOL_Y + 32.0f + (float)((u->frame / 6 + (unsigned)i) % 2) * 2.0f,
                     i + 1, 1 + (int)((u->frame / 9 + (unsigned)i * 2) % 6), (int)((u->frame / 4 + (unsigned)i) % 5), 0.8f, 255);
        gfx_text_center(POOL_X + 232, POOL_Y + 23, b, (u->frame / 20) % 2 ? COL_YELLOW : COL_WHITE);
        return;
    }
    for (i = 0; i < g->pool_n; i++) {
        float cx = POOL_X + 31.0f + (float)i * 43.0f, cy = POOL_Y + 32.0f;
        int v = g->pool[i].value, c = g->pool[i].color, frame = DICE_STILL, alpha = 255;
        if (u->state == ST_ROLL) {
            int t = u->timer - i * 2, land = ROLL_LAND(i);
            if (t < 0) continue;                          /* ainda nao foi lancado */
            if (u->timer < land) {
                /* girando: entra pela esquerda quicando */
                float k = (float)(land - u->timer) / (float)(land - i * 2);
                frame = (u->timer / 3 + i) % 5;
                cx -= k * 60.0f;
                cy -= (float)((u->timer / 3 + i) % 2) * 5.0f + k * 6.0f;
            } else if (u->timer < land + 14) {
                gfx_spr(SPR_BURST, cx + 12, cy - 26);
            }
        }
        if ((u->state == ST_PICK || u->state == ST_PLACE || u->state == ST_CPU) && i == u->pick_sel) {
            gfx_rrect(cx - 21, cy - 23, 42, 44, 10, COL_HILITE);
            if (u->sel_active) { cy -= 4; v = u->adj_value; c = u->adj_color; }
        } else if (u->state == ST_MARKET && (u->frame / 15) % 2) {
            gfx_rrect(cx - 21, cy - 23, 42, 44, 10, COL_HILITE);
        }
        if (u->sel_active && i != u->pick_sel) alpha = 120;
        gfx_dice(cx, cy, c, v, frame, 1.0f, alpha);
    }
}

static void draw_market(const Ui *u)
{
    const Game *g = &u->g;
    int k;
    char b[32];
    gfx_panel(MKT_X, MKT_Y, 200, 124);
    gfx_text_center(MKT_X + 100, MKT_Y + 4, T(S_MARKET), COL_YELLOW);
    for (k = 0; k < NUM_VEG; k++) {
        int j, veg = k + 1, id = SPR_CRATE1 + k;
        float cx = MKT_X + 34.0f + (float)k * 66.0f;
        float x0 = cx - SPRITES[id].w / 2.0f, y0 = MKT_Y + 24.0f;
        gfx_spr(id, x0, y0);
        for (j = 0; j < MARKET_MAX; j++) {
            float x = x0 + CRATE_CIRC[j][0], y = y0 + CRATE_CIRC[j][1];
            int ghost = u->state == ST_MARKET && g->market_color == veg && j == g->market[k];
            if (j < g->market[k]) {
                gfx_circle(x, y, 5.5f, veg_color(veg));
                gfx_line(x - 3.5f, y - 3.5f, x + 3.5f, y + 3.5f, 2, COL_WHITE);
                gfx_line(x - 3.5f, y + 3.5f, x + 3.5f, y - 3.5f, 2, COL_WHITE);
            } else if (ghost && (u->frame / 10) % 2) {
                gfx_circle(x, y, 5.5f, COL_HILITE);
            }
        }
        gfx_veg(x0 + SPRITES[id].w - 6, y0 + 6, 20, veg, 255);
        snprintf(b, sizeof(b), T(S_WORTH), veg_value(g, veg));
        gfx_text_center(cx, MKT_Y + 100, b, COL_WHITE);
    }
}

static int viewed(const Ui *u) { return u->view_player >= 0 ? u->view_player : u->g.current; }

static void cell_rect(int r, int c, float *x, float *y, float *w, float *h)
{
    *x = GRID_PX + GRID_X[c] + 2;
    *y = GRID_PY + GRID_Y[r] + 2;
    *w = (float)(GRID_X[c + 1] - GRID_X[c]) - 4;
    *h = (float)(GRID_Y[r + 1] - GRID_Y[r]) - 4;
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

    gfx_panel(GARDEN_X, GARDEN_Y, 400, 218);
    gfx_spr(SPR_GRID, GRID_PX, GRID_PY);
    for (c = 0; c < GARDEN_COLS; c++) {
        float cx = GRID_PX + (GRID_X[c] + GRID_X[c + 1]) / 2.0f;
        int id = SPR_SIGN1 + c;
        int hl = preview && c == u->adj_value - 1;
        float bob = hl ? (float)((u->frame / 8) % 2) * -2.0f : 0.0f;
        if (hl) gfx_rrect(cx - 21, GARDEN_Y + 2, 42, 44, 8, COL_HILITE);
        gfx_spr(id, cx - SPRITES[id].w / 2.0f, GARDEN_Y + 5 + bob);
    }
    for (r = 0; r < GARDEN_ROWS; r++) {
        int full = 1;
        for (c = 0; c < GARDEN_COLS; c++) {
            float x, y, w, h;
            int veg = p->garden[r][c];
            cell_rect(r, c, &x, &y, &w, &h);
            if (veg == VEG_NONE) full = 0;
            if (preview && c == u->adj_value - 1)
                gfx_rrect(x, y, w, h, 6, plantable ? CRGBA(255, 240, 120, 120) : CRGBA(240, 90, 70, 110));
            if (veg) gfx_veg(x + w / 2, y + h / 2, 42, veg, 255);
            if (plantable && c == u->adj_value - 1 && veg == VEG_NONE &&
                can_plant_at(p, u->adj_color, u->adj_value, r)) {
                if (r == u->place_row) {
                    gfx_frame(x - 1, y - 1, w + 2, h + 2, 4, CRGB(255, 190, 0));
                    gfx_veg(x + w / 2, y + h / 2, 42, u->adj_color, pulse(u, 110, 230));
                } else {
                    gfx_frame(x + 2, y + 2, w - 4, h - 4, 2, CRGBA(230, 160, 0, pulse(u, 80, 220)));
                }
            }
        }
        gfx_ribbon(GRID_PX + 344 + 22, GRID_PY + (GRID_Y[r] + GRID_Y[r + 1]) / 2.0f, full);
    }
    if (preview && !plantable) {
        gfx_rrect(36, GRID_PY + 120, 360, 30, 8, CRGBA(150, 30, 20, 230));
        snprintf(b, sizeof(b), T(S_CANT_PLANT), u->adj_value);
        gfx_text_center(216, GRID_PY + 126, b, COL_WHITE);
    }
    if (pi != g->current) {
        gfx_rrect(70, GRID_PY - 4, 292, 26, 8, CRGBA(30, 80, 170, 235));
        snprintf(b, sizeof(b), T(S_VIEWING), p->name);
        gfx_text_center(216, GRID_PY, b, COL_WHITE);
    }
}

static void draw_pig(const Ui *u)
{
    const Game *g = &u->g;
    int pi = viewed(u);
    const Player *p = &g->p[pi];
    int i, rows = pig_rows_done(p);
    int sel_here = u->sel_active && pi == g->current;
    int preview = sel_here && !can_plant(p, u->adj_color, u->adj_value);
    int ghost_to = preview ? p->pig + u->adj_value : p->pig;
    int using = sel_here ? sel_cost(u) : 0;
    int left = powers_available(p) - using;
    float bw = SPRITES[SPR_PIGBOARD].w * BOARD_SC, bh = SPRITES[SPR_PIGBOARD].h * BOARD_SC;
    char b[64];

    gfx_panel(PIG_X, PIG_Y, 200, 158);
    gfx_spr(SPR_PIG, PIG_X + 6, PIG_Y + 4);
    gfx_text_shadow(PIG_X + 66, PIG_Y + 5, T(S_PIG), COL_PIG);
    snprintf(b, sizeof(b), "%d/%d", p->pig, PIG_CIRCLES);
    gfx_text_shadow(PIG_X + 192 - gfx_text_width(b), PIG_Y + 5, b, CRGB(255, 236, 210));
    snprintf(b, sizeof(b), T(S_POWERS), left);
    gfx_text_shadow(PIG_X + 66, PIG_Y + 23, b, left > 0 ? COL_YELLOW : CRGB(220, 200, 180));

    gfx_spr_s(SPR_PIGBOARD, BOARD_PX, BOARD_PY, bw, bh, 255);
    for (i = 0; i < PIG_CIRCLES; i++) {
        float x = BOARD_PX + PIG_CIRC[i][0] * BOARD_SC, y = BOARD_PY + PIG_CIRC[i][1] * BOARD_SC;
        if (i < p->pig) {
            gfx_line(x - 4, y - 4, x + 4, y + 4, 2.5f, COL_INK);
            gfx_line(x - 4, y + 4, x + 4, y - 4, 2.5f, COL_INK);
        } else if (i < ghost_to && (u->frame / 8) % 2) {
            gfx_circle(x, y, 5.0f, COL_PIG_DARK);
        }
    }
    for (i = 0; i < PIG_ROWS; i++) {
        float x = BOARD_PX + PIG_BOX[i][0] * BOARD_SC, y = BOARD_PY + PIG_BOX[i][1] * BOARD_SC;
        int done = i < rows, used = i < p->powers_used + using;
        if (done && !used) gfx_ring(x, y, 8.5f, 2.5f, color_alpha(CRGB(60, 220, 60), pulse(u, 150, 255)));
        if (done && used) {
            gfx_line(x - 5, y - 5, x + 5, y + 5, 2.5f, COL_INK);
            gfx_line(x - 5, y + 5, x + 5, y - 5, 2.5f, COL_INK);
        }
        if (i == rows - 1) gfx_ring(x + 20, y, 9.5f, 2.0f, COL_YELLOW);
    }
    /* icones dos poderes ao lado do tabuleiro */
    {
        float ix = BOARD_PX + bw + 6;
        gfx_spr_s(SPR_POW_VALUE, ix, BOARD_PY + 6, 44, 44.0f * SPRITES[SPR_POW_VALUE].h / SPRITES[SPR_POW_VALUE].w,
                  left > 0 ? 255 : 110);
        if (g->advanced)
            gfx_spr_s(SPR_POW_COLOR, ix + 4, BOARD_PY + 36, 36, 36.0f * SPRITES[SPR_POW_COLOR].h / SPRITES[SPR_POW_COLOR].w,
                      left > 0 ? 255 : 110);
        if (using) {
            snprintf(b, sizeof(b), T(S_USING), using);
            gfx_text_center(ix + 22, BOARD_PY + 76, b, CRGB(255, 190, 150));
        }
    }
}

static const char *hint_text(const Ui *u)
{
    const Game *g = &u->g;
    switch (u->state) {
    case ST_ROLL: return T(g->phase == PHASE_ROLL && !is_cpu(u, g->start_player) ? S_H_ROLL : S_H_ROLLING);
    case ST_PICK: return T(S_H_PICK);
    case ST_PLACE: return T(g->advanced ? S_H_PLACE_ADV : S_H_PLACE);
    case ST_CPU: return T(S_H_CPU);
    case ST_MARKET: return T(S_H_MARKET);
    }
    return "";
}

static void draw_status(const Ui *u)
{
    if (u->msg_timer > 0 && u->msg[0]) {
        int a = u->msg_timer > 25 ? 255 : u->msg_timer * 10;
        gfx_rrect(18, HINT_Y + 3, 608, 26, 13, COL_SHADOW);
        gfx_rrect(16, HINT_Y, 608, 26, 13, COL_WOOD_DARK);
        gfx_rrect(18, HINT_Y + 2, 604, 22, 11, CRGB(120, 64, 28));
        gfx_text_center(320, HINT_Y + 4, u->msg, color_alpha(u->msg_color, a));
    } else {
        gfx_hintbar(16, HINT_Y, 608, hint_text(u));
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
        float x = 16.0f + (float)i * (w + 8.0f), y = SCORE_Y;
        int is_cur = i == g->current && u->state != ST_MARKET && u->state != ST_ROLL;
        Color border = is_cur ? COL_YELLOW : (i == viewed(u) ? CRGB(90, 150, 240) : COL_WOOD_DARK);
        gfx_card(x, y, w, 76, border);
        gfx_text(x + 9, y + 5, p->name, COL_INK);
        if (p->is_cpu) gfx_text(x + 13 + gfx_text_width(p->name), y + 5, "CPU", CRGB(120, 110, 170));
        else {
            /* controle deste jogador: C1 = so o jogador 1, C2 = os demais */
            char cb[8];
            snprintf(cb, sizeof(cb), "C%d", ui_player_pad(i) + 1);
            gfx_text(x + 13 + gfx_text_width(p->name), y + 5, cb, CRGB(50, 110, 200));
        }
        if (i == g->start_player) gfx_spr(SPR_PIGLET, x + w - 34, y + 3);
        for (k = 0; k < NUM_VEG; k++) {
            float ix = x + 9.0f + (float)k * (w - 18.0f) / 3.0f;
            gfx_veg(ix + 9, y + 35, 20, k + 1, 255);
            snprintf(b, sizeof(b), "%d", s[i].veg_count[k]);
            gfx_text(ix + 21, y + 26, b, COL_INK);
        }
        gfx_spr(SPR_PIGLET, x + 6, y + 49);
        snprintf(b, sizeof(b), "%d  * %d", p->pig, powers_available(p));
        gfx_text(x + 36, y + 52, b, COL_INK);
        snprintf(b, sizeof(b), "%d %s", s[i].total, T(S_PTS));
        gfx_text(x + w - 9 - gfx_text_width(b), y + 52, b, COL_GREEN_TXT);
    }
}

static void draw_wood_popup(float x, float y, float w, float h)
{
    gfx_rrect(x + 4, y + 6, w, h, 14, CRGBA(0, 0, 0, 120));
    gfx_rrect(x, y, w, h, 14, COL_WOOD_DARK);
    gfx_rrect(x + 4, y + 4, w - 8, h - 8, 11, COL_WOOD);
    gfx_rrect(x + 10, y + 10, w - 20, h - 20, 8, CRGB(130, 74, 34));
}

static void draw_board(Ui *u)
{
    draw_background(u, 0);
    draw_header(u);
    draw_pool(u);
    draw_market(u);
    draw_garden(u);
    draw_pig(u);
    draw_status(u);
    draw_scores(u);

    if (u->state == ST_PICK && u->banner_timer > 0) {
        draw_wood_popup(130, 158, 380, 110);
        gfx_spr(SPR_M_PIG, 136, 166);
        gfx_big_center(360, 172, T(S_YOUR_TURN), 34);
        gfx_text_center(360, 214, u->g.p[u->g.current].name, COL_WHITE);
        {
            char pb[96];
            snprintf(pb, sizeof(pb), T(S_PASS_PAD), ui_player_pad(u->g.current) + 1);
            gfx_text_center(360, 236, pb, CRGB(255, 230, 190));
        }
    }
    if (u->state == ST_MARKET) {
        const Game *g = &u->g;
        int c = g->market_color;
        char b[128];
        draw_wood_popup(110, 146, 420, 136);
        gfx_text_center(320, 160, T(S_LEFTOVER), COL_WHITE);
        gfx_dice(278, 204, c, g->pool[0].value, DICE_STILL, 1.0f, 255);
        gfx_spr(SPR_ARROW, 310, 192);
        gfx_spr(SPR_CRATE1 + c - 1, 338, 176);
        if (g->market[c - 1] < MARKET_MAX)
            snprintf(b, sizeof(b), T(S_MARKET_UP), veg_market(c), veg_value(g, c) + 1);
        else
            snprintf(b, sizeof(b), T(S_MARKET_FULL), veg_plural(c));
        gfx_text_center(320, 244, b, COL_YELLOW);
        gfx_text_center(320, 261, T(S_COLOR_ONLY), CRGB(230, 200, 170));
    }
}

static void draw_menu_button(float x, float y, float w, int icon, const char *label, int sel)
{
    gfx_button(x, y, w, 40, sel);
    if (icon >= 0) gfx_spr_c(icon, x + 34, y + 20, 0.95f);
    gfx_text_center(x + w / 2 + (icon >= 0 ? 14 : 0), y + 11, label, COL_INK);
}

/* Arte de abertura em tela cheia (o logotipo ja faz parte dela). */
static void draw_splash_bg(int dim)
{
    plat_sprite(ATLAS_SPLASH, 0, 0, SPLASH_W, SPLASH_H, 0, 0, SPLASH_W, SPLASH_H, COL_WHITE);
    if (dim) {
        /* escurece abaixo do logotipo para o menu se destacar */
        int k;
        for (k = 0; k < 8; k++) plat_rect(0, 140.0f + (float)k * 4.0f, SCREEN_W, 4, CRGBA(0, 0, 0, k * 10));
        plat_rect(0, 172, SCREEN_W, SCREEN_H - 172, CRGBA(0, 0, 0, 80));
    }
}

static void draw_splash(Ui *u)
{
    draw_splash_bg(0);
    if ((u->frame / 30) % 2 == 0) {
        gfx_rrect(232, 400, 176, 34, 17, CRGBA(0, 0, 0, 120));
        gfx_rrect(228, 396, 176, 34, 17, COL_WOOD_DARK);
        gfx_rrect(231, 399, 170, 28, 14, CRGB(166, 92, 40));
        gfx_text_center(316, 404, "{ST}  START  /  {X}", COL_YELLOW);
    }
}

static void draw_menu_panel(void)
{
    gfx_panel(130, 160, 380, 196);
}

static void draw_lang(Ui *u)
{
    int i;
    draw_splash_bg(1);
    draw_menu_panel();
    gfx_text_center(320, 170, "Idioma  ·  Language", COL_YELLOW);
    for (i = 0; i < NUM_LANGS; i++)
        draw_menu_button(196, 196.0f + (float)i * 50.0f, 248, -1, lang_name(i), u->lang_sel == i);
    gfx_hintbar(190, 404, 260, "{U}{D}  ·  {X}  OK");
}

static void draw_title(Ui *u)
{
    static const int icons[3] = { SPR_I_DICE, SPR_I_BOOK, SPR_I_GEAR };
    static const int labels[3] = { S_MENU_NEW, S_MENU_RULES, S_MENU_LANG };
    char b[96];
    int i;
    draw_splash_bg(1);
    draw_menu_panel();
    gfx_text_center(320, 170, T(S_TAGLINE), COL_YELLOW);
    for (i = 0; i < 3; i++)
        draw_menu_button(196, 196.0f + (float)i * 50.0f, 248, icons[i], T(labels[i]), u->menu_sel == i);
    snprintf(b, sizeof(b), "%s     {X} %s", T(S_HINT_CHOOSE), T(S_CONFIRM));
    gfx_hintbar(150, 404, 340, b);
}

static void draw_setup(Ui *u)
{
    int rows = u->setup_players + 3, i;
    char b[128];
    draw_background(u, 1);
    gfx_panel(90, 10, 460, 390);
    gfx_big_center(320, 20, T(S_SETUP_TITLE), 36);
    for (i = 0; i < rows; i++) {
        float y = 72.0f + (float)i * 35.0f;
        int sel = u->setup_sel == i;
        const char *label = NULL, *val = NULL;
        char lb[32];
        if (i == 0) { label = T(S_PLAYERS); snprintf(b, sizeof(b), "%d", u->setup_players); val = b; }
        else if (i <= u->setup_players) {
            static const int tn[3] = { S_HUMAN, S_CPU_EASY, S_CPU_NORMAL };
            snprintf(lb, sizeof(lb), T(S_PLAYER_N), i);
            label = lb;
            val = T(tn[u->setup_type[i - 1]]);
        } else if (i == u->setup_players + 1) { label = T(S_MODE); val = T(u->setup_adv ? S_ADVANCED : S_BASIC); }
        gfx_button(120, y, 400, 32, sel);
        if (label) {
            char vb[48];
            if (i >= 1 && i <= u->setup_players) {
                if (u->setup_type[i - 1] == 0) gfx_spr(SPR_PIGLET, 132, y + 6);
                else gfx_spr_c(SPR_I_GEAR, 144, y + 16, 0.7f);
            } else if (i == 0) gfx_spr_c(SPR_I_DICE, 144, y + 16, 0.7f);
            else gfx_spr_c(SPR_I_BOOK, 144, y + 16, 0.65f);
            gfx_text(166, y + 7, label, COL_INK);
            snprintf(vb, sizeof(vb), "{L} %.30s {R}", val);
            gfx_text(504 - gfx_text_width(vb), y + 7, vb, COL_INK);
        } else {
            gfx_text_center(320, y + 7, T(S_START), COL_INK);
        }
    }
    gfx_text_wrap(124, 72.0f + (float)rows * 35.0f + 4, 392, 17,
                  T(u->setup_adv ? S_ADV_DESC : S_BASIC_DESC), CRGB(255, 236, 200));
    snprintf(b, sizeof(b), "%s    {X} %s    %s", T(S_HINT_SETUP), T(S_CONFIRM), T(S_BACK));
    gfx_hintbar(40, 408, 560, b);
}

static void draw_rules(Ui *u)
{
    static const int deco[RULES_PAGES] = { SPR_M_LETTUCE, SPR_I_DICE, SPR_SIGN3, SPR_PIG, SPR_POW_VALUE,
                                           SPR_CRATE2, SPR_I_TROPHY, SPR_B_CRO };
    const RulesPage *rp = rules_page(u->rules_page);
    char b[64];
    int d = deco[u->rules_page], k;
    float sc;
    draw_background(u, 1);
    gfx_card(40, 14, 560, 388, COL_WOOD_DARK);
    gfx_big_center(320, 26, rp->title, 30);
    sc = 40.0f / (float)(SPRITES[d].h > SPRITES[d].w ? SPRITES[d].h : SPRITES[d].w);
    gfx_spr_c(d, 78, 44, sc);
    gfx_spr_c(d, 562, 44, sc);
    gfx_text_wrap(64, 78, 512, 19, rp->body, COL_INK);
    snprintf(b, sizeof(b), T(S_PAGE), u->rules_page + 1, RULES_PAGES);
    gfx_text(64, 376, b, CRGB(140, 100, 70));
    for (k = 0; k < RULES_PAGES; k++)
        gfx_circle(300.0f + (float)k * 14.0f, 384, k == u->rules_page ? 5.0f : 3.5f,
                   k == u->rules_page ? COL_CARROT : COL_PAPER_DARK);
    gfx_hintbar(130, 410, 380, T(S_RULES_HINT));
}

static void draw_pause(Ui *u)
{
    static const int items[3] = { S_CONTINUE, S_MENU_RULES, S_QUIT };
    static const int icons[3] = { SPR_I_DICE, SPR_I_BOOK, SPR_PIGLET };
    int i;
    plat_rect(0, 0, SCREEN_W, SCREEN_H, CRGBA(0, 0, 0, 150));
    gfx_panel(180, 96, 280, 236);
    gfx_big_center(320, 108, T(S_PAUSE), 38);
    for (i = 0; i < 3; i++)
        draw_menu_button(196, 166.0f + (float)i * 50.0f, 248, icons[i], T(items[i]), u->pause_sel == i);
}

static void draw_gameover(Ui *u)
{
    const Game *g = &u->g;
    Score s[MAX_PLAYERS];
    int win[MAX_PLAYERS];
    int n = g->num_players, i, r, nw = 0;
    static const int labels[] = { S_CARROTS, S_LETTUCES, S_TOMATOES, S_RIBBONS, S_PIG_ROW, S_BONUS, S_TOTAL };
    int nrows = g->advanced ? 7 : 6;
    float colw = 420.0f / (float)n;
    char b[160];

    game_score(g, s);
    game_winners(g, s, win);
    draw_background(u, 1);
    gfx_card(24, 8, 592, 360, COL_WOOD_DARK);
    gfx_big_center(320, 16, T(S_GAME_OVER), 36);

    for (i = 0; i < n; i++) {
        float x = 180.0f + (float)i * colw;
        if (win[i]) gfx_rrect(x + 2, 60, colw - 4, 28 + (float)nrows * 30 + 4, 8, CRGB(255, 232, 140));
        gfx_text_center(x + colw / 2, 64, g->p[i].name, COL_INK);
    }
    for (r = 0; r < nrows; r++) {
        int row = (!g->advanced && r == 5) ? 6 : r;
        float y = 94.0f + (float)r * 30.0f;
        plat_rect(44, y - 4, 552, 2, COL_PAPER_DARK);
        if (row < 3) gfx_veg(60, y + 10, 20, row + 1, 255);
        else if (row == 3) gfx_spr_c(SPR_RIBBON_S, 60, y + 10, 1.0f);
        else if (row == 4) gfx_spr_c(SPR_PIGLET, 60, y + 10, 0.9f);
        else if (row == 5) gfx_spr_c(SPR_ARROW, 60, y + 10, 0.9f);
        else gfx_spr_c(SPR_I_TROPHY, 60, y + 10, 0.7f);
        gfx_text(78, y + 2, T(labels[row]), row == 6 ? COL_CARROT : COL_INK);
        for (i = 0; i < n; i++) {
            float x = 180.0f + (float)i * colw + colw / 2;
            switch (row) {
            case 0: case 1: case 2:
                snprintf(b, sizeof(b), "%d×%d = %d", s[i].veg_count[row], veg_value(g, row + 1), s[i].veg_points[row]);
                break;
            case 3: snprintf(b, sizeof(b), "%d×5 = %d", s[i].ribbons, s[i].ribbon_points); break;
            case 4: snprintf(b, sizeof(b), "%d", s[i].pig_points); break;
            case 5: snprintf(b, sizeof(b), "%d", s[i].bonus_points); break;
            default: snprintf(b, sizeof(b), "%d", s[i].total); break;
            }
            gfx_text(x - (float)gfx_text_width(b) / 2, y + 2, b, row == 6 ? COL_GREEN_TXT : COL_INK);
        }
    }
    for (i = 0; i < n; i++) if (win[i]) nw++;
    if (nw == 1) {
        for (i = 0; i < n; i++) if (win[i]) snprintf(b, sizeof(b), T(S_WON), g->p[i].name);
    } else {
        int k = 0;
        size_t L;
        snprintf(b, sizeof(b), "%s", T(S_TIE));
        for (i = 0; i < n; i++) if (win[i]) {
            L = strlen(b);
            snprintf(b + L, sizeof(b) - L, "%s%s", k++ ? T(S_AND) : "", g->p[i].name);
        }
    }
    draw_wood_popup(60, 318, 520, 62);
    gfx_spr(SPR_TROPHY_L, 76, 327);
    gfx_spr(SPR_TROPHY_L, 524, 327);
    if (nw == 1 && gfx_big_width(b, 30) < 420) gfx_big_center(320, 334, b, 30);
    else gfx_text_center(320, 340, b, COL_YELLOW);
    if (u->timer > 40) gfx_hintbar(190, 404, 260, T(S_BACK_TITLE));
}

void ui_draw(Ui *u)
{
    switch (u->state) {
    case ST_SPLASH: draw_splash(u); break;
    case ST_LANG: draw_lang(u); break;
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
