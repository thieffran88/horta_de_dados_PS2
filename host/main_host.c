/* Executavel de teste no PC.
 *   horta_host shots <dir>   - roda um roteiro de botoes e salva capturas (.ppm)
 *   horta_host sim <n>       - simula n partidas so com CPU e confere invariantes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ui.h"
#include "../src/ai.h"
#include "../src/gfx.h"

void host_set_buttons(unsigned b);
void host_request_shot(const char *path);

static Ui ui;
static char outdir[256];

static void frame(unsigned buttons, const char *shot)
{
    char path[512];
    host_set_buttons(buttons);
    ui_update(&ui, plat_buttons());
    if (shot) {
        snprintf(path, sizeof(path), "%s/%s.ppm", outdir, shot);
        host_request_shot(path);
        plat_frame_begin(COL_BG);
        ui_draw(&ui);
        plat_frame_end();
        {
            extern int host_ntri, host_nrect, host_nglyph;
            printf("captura: %s (estado %d) tri=%d rect=%d glyph=%d\n", path, ui.state, host_ntri, host_nrect, host_nglyph);
        }
    }
}

static void wait(int n) { while (n-- > 0) frame(0, NULL); }
static void press(unsigned b) { frame(b, NULL); frame(b, NULL); frame(0, NULL); frame(0, NULL); }
static void shot(const char *name) { frame(0, name); }

/* avanca ate o estado desejado (ou limite) */
static int until(int state, int limit)
{
    while (limit-- > 0) {
        if (ui.state == state) return 1;
        frame(0, NULL);
    }
    return ui.state == state;
}

static int run_shots(void)
{
    int guard;
    ui_init(&ui, 7);
    wait(30);
    shot("01_titulo");
    press(BTN_CROSS);
    press(BTN_RIGHT);              /* 3 jogadores */
    press(BTN_DOWN); press(BTN_DOWN);
    press(BTN_LEFT);               /* jogador 2: CPU facil */
    press(BTN_DOWN); press(BTN_DOWN);
    press(BTN_RIGHT);              /* modo avancado */
    press(BTN_DOWN);
    shot("02_nova_partida");
    press(BTN_CROSS);
    wait(5);
    shot("03_rolar");

    /* joga ate ser a vez de um humano escolher */
    for (guard = 0; guard < 4000 && ui.state != ST_PICK; guard++) {
        if (ui.state == ST_ROLL && ui.g.phase == PHASE_ROLL && !ui.g.p[ui.g.start_player].is_cpu) press(BTN_CROSS);
        else if (ui.state == ST_MARKET) press(BTN_CROSS);
        else frame(0, NULL);
    }
    wait(10);
    shot("04_vez_humano");
    wait(100);
    press(BTN_RIGHT);
    shot("05_escolher_dado");
    press(BTN_CROSS);
    wait(12);
    shot("06_plantar");
    press(BTN_START);
    shot("06b_pausa");
    press(BTN_DOWN); press(BTN_CROSS);
    if (ui.state != ST_RULES) printf("AVISO: pausa->regras falhou (%d)\n", ui.state);
    press(BTN_CIRCLE);
    if (ui.state != ST_PAUSE) printf("AVISO: regras->pausa falhou (%d)\n", ui.state);
    press(BTN_CIRCLE);
    if (ui.state != ST_PLACE) printf("AVISO: pausa->jogo falhou (%d)\n", ui.state);

    /* joga varias rodadas automaticamente: o humano sempre usa a IA */
    for (guard = 0; guard < 200000 && ui.state != ST_GAMEOVER; guard++) {
        if (ui.state == ST_PLACE) {
            press(BTN_CIRCLE);
        } else if (ui.state == ST_PICK && ui.banner_timer > 0) {
            frame(0, NULL);
        } else if (ui.state == ST_PICK) {
            Move m;
            ai_choose(&ui.g, &m);
            while (ui.pick_sel != m.die_index) press(BTN_RIGHT);
            press(BTN_CROSS);
            while (ui.adj_value < m.new_value) press(BTN_RIGHT);
            while (ui.adj_value > m.new_value) press(BTN_LEFT);
            {
                int k = 0;
                while (ui.adj_color != m.new_color && k++ < 3) press(BTN_TRIANGLE);
            }
            if (!m.feed_pig) while (ui.place_row != m.row) press(BTN_DOWN);
            if (ui.g.round == 4 && ui.g.pool_n <= 6) { shot("07_meio_de_jogo"); }
            press(BTN_CROSS);
        } else if (ui.state == ST_ROLL && ui.g.phase == PHASE_ROLL && !ui.g.p[ui.g.start_player].is_cpu) {
            press(BTN_CROSS);
        } else if (ui.state == ST_MARKET) {
            if (ui.timer == 30 && ui.g.round == 3) shot("08_mercado");
            frame(0, NULL);
        } else if (ui.state == ST_CPU) {
            if (ui.timer == 80 && ui.g.round == 5) shot("09_cpu");
            frame(0, NULL);
        } else frame(0, NULL);
    }
    if (ui.state != ST_GAMEOVER)
        printf("AVISO: partida nao terminou (estado %d, rodada %d, guard %d, msg '%s')\n", ui.state, ui.g.round, guard, ui.msg);
    wait(50);
    shot("10_fim_de_jogo");
    press(BTN_CROSS);
    until(ST_TITLE, 10);
    press(BTN_DOWN);
    press(BTN_CROSS);
    shot("11_regras");
    press(BTN_RIGHT); press(BTN_RIGHT);
    shot("12_regras_plantar");
    return 0;
}

static int run_sim(int n)
{
    static Game g;
    int k, i, rounds = 0, trig_garden = 0, trig_pig = 0, trig_market = 0;
    long tot = 0;
    for (k = 0; k < n; k++) {
        int np = 2 + k % 3, guard = 0;
        memset(&g, 0, sizeof(g));
        for (i = 0; i < MAX_PLAYERS; i++) { g.p[i].is_cpu = 1; g.p[i].cpu_level = k & 1; }
        game_init(&g, np, (k / 3) & 1, 1000u + (unsigned)k);
        while (g.phase != PHASE_GAME_OVER && guard++ < 1000) {
            if (g.phase == PHASE_ROLL) game_roll(&g);
            else if (g.phase == PHASE_PICK) {
                Move m;
                ai_choose(&g, &m);
                if (!game_apply(&g, &m)) { printf("ERRO: jogada da IA invalida (partida %d)\n", k); return 1; }
            } else if (g.phase == PHASE_MARKET) game_resolve_market(&g);
        }
        if (g.phase != PHASE_GAME_OVER) { printf("ERRO: partida %d nao terminou\n", k); return 1; }
        {
            Score s[MAX_PLAYERS];
            int m;
            game_score(&g, s);
            for (i = 0; i < np; i++) tot += s[i].total;
            for (m = 0; m < 3; m++) if (g.market[m] >= MARKET_MAX) { trig_market++; break; }
            for (i = 0; i < np; i++) {
                if (g.p[i].pig >= PIG_CIRCLES) { trig_pig++; break; }
            }
            for (i = 0; i < np; i++) {
                int r, c, full = 1;
                for (r = 0; r < 3; r++) for (c = 0; c < 6; c++) if (!g.p[i].garden[r][c]) full = 0;
                if (full) { trig_garden++; break; }
            }
            rounds += g.round;
        }
    }
    printf("%d partidas ok. rodadas medias %.1f, pontos medios %.1f\n", n, (double)rounds / n,
           (double)tot / (n * 3.0));
    printf("fim por horta cheia: %d, porco cheio: %d, mercado cheio: %d\n", trig_garden, trig_pig, trig_market);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc >= 3 && !strcmp(argv[1], "shots")) {
        snprintf(outdir, sizeof(outdir), "%s", argv[2]);
        return run_shots();
    }
    if (argc >= 3 && !strcmp(argv[1], "sim")) return run_sim(atoi(argv[2]));
    fprintf(stderr, "uso: %s shots <dir> | sim <n>\n", argv[0]);
    return 2;
}
