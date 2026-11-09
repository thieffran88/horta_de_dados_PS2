#ifndef HORTA_UI_H
#define HORTA_UI_H
#include "game.h"
#include "plat.h"

enum {
    ST_TITLE = 0, ST_SETUP, ST_RULES, ST_ROLL, ST_PICK, ST_PLACE, ST_CPU,
    ST_MARKET, ST_GAMEOVER, ST_PAUSE, ST_LANG
};

typedef struct {
    int state, prev_state;
    Game g;
    unsigned frame;

    /* menus */
    int menu_sel;
    int setup_sel;
    int setup_players;
    int setup_type[MAX_PLAYERS]; /* 0 humano, 1 CPU facil, 2 CPU normal */
    int setup_adv;
    int rules_page;
    int pause_sel;
    int lang_sel;

    /* turno */
    int pick_sel;
    int adj_value, adj_color;
    int place_row;
    int timer;
    int view_player;
    Move cpu_move;
    int banner_timer;
    int sel_active;     /* um dado foi escolhido (mostrar previa) */

    char msg[128];
    int msg_timer;
    Color msg_color;

    /* entrada */
    unsigned held, pressed;
    int repeat_timer;
} Ui;

void ui_init(Ui *u, unsigned seed);
void ui_update(Ui *u, unsigned buttons);
void ui_draw(Ui *u);

#endif
