/* Horta de Dados PS2 - textos em portugues, ingles e espanhol. */
#ifndef HORTA_LANG_H
#define HORTA_LANG_H

enum { LANG_PT = 0, LANG_EN, LANG_ES, NUM_LANGS };

enum {
    S_TAGLINE, S_MENU_NEW, S_MENU_RULES, S_MENU_LANG,
    S_LANG_PROMPT, S_HINT_CHOOSE, S_CONFIRM, S_BACK,
    S_SETUP_TITLE, S_PLAYERS, S_PLAYER_N, S_MODE, S_BASIC, S_ADVANCED,
    S_HUMAN, S_CPU_EASY, S_CPU_NORMAL, S_START, S_BASIC_DESC, S_ADV_DESC, S_HINT_SETUP,
    S_ROUND_MODE, S_LAST_ROUND, S_TURN, S_WILL_ROLL, S_PRESS_ROLL,
    S_MARKET, S_WORTH, S_CANT_PLANT, S_VIEWING, S_PIG, S_POWERS, S_USING,
    S_H_ROLL, S_H_ROLLING, S_H_PICK, S_H_PLACE, S_H_PLACE_ADV, S_H_CPU, S_H_MARKET,
    S_YOUR_TURN, S_PASS_PAD, S_LEFTOVER, S_MARKET_UP, S_MARKET_FULL, S_COLOR_ONLY, S_MARKET_MSG,
    S_STARTS, S_FED, S_FED_POWER, S_PLANTED, S_TRIGGER, S_INVALID,
    S_NO_POWERS_ENOUGH, S_NO_POWERS, S_COLOR_ADV_ONLY, S_NO_POWER_COLOR, S_ONLY_FEED,
    S_PAUSE, S_CONTINUE, S_QUIT, S_GAME_OVER,
    S_CARROTS, S_LETTUCES, S_TOMATOES, S_RIBBONS, S_PIG_ROW, S_BONUS, S_TOTAL,
    S_WON, S_TIE, S_AND, S_BACK_TITLE, S_PAGE, S_RULES_HINT, S_PTS,
    S_COUNT
};

typedef struct { const char *title; const char *body; } RulesPage;
#define RULES_PAGES 8

extern int g_lang;
const char *T(int id);
const char *lang_name(int lang);
const char *veg_obj(int veg);     /* "uma cenoura" */
const char *veg_plural(int veg);  /* "cenouras" */
const char *veg_sing(int veg);    /* "cenoura" */
const char *veg_market(int veg);  /* "Cenoura" (titulo do mercado) */
const char *cpu_name(int i);
const RulesPage *rules_page(int page);

#endif
