#ifndef HORTA_GFX_H
#define HORTA_GFX_H
#include "plat.h"
#include "sprites.h"

/* Paleta */
#define COL_BG         CRGB(78, 150, 62)
#define COL_BG_STRIPE  CRGB(70, 140, 56)
#define COL_WOOD       CRGB(150, 88, 44)
#define COL_WOOD_DARK  CRGB(96, 52, 24)
#define COL_WOOD_LIGHT CRGB(186, 118, 62)
#define COL_PAPER      CRGB(255, 240, 206)
#define COL_PAPER_DARK CRGB(222, 196, 150)
#define COL_INK        CRGB(84, 44, 20)
#define COL_WHITE      CRGB(255, 255, 255)
#define COL_YELLOW     CRGB(255, 214, 64)
#define COL_SHADOW     CRGBA(0, 0, 0, 90)
#define COL_CARROT     CRGB(240, 130, 30)
#define COL_LETTUCE    CRGB(70, 175, 60)
#define COL_TOMATO     CRGB(214, 44, 40)
#define COL_PIG        CRGB(255, 170, 196)
#define COL_PIG_DARK   CRGB(214, 100, 136)
#define COL_HILITE     CRGB(255, 226, 60)
#define COL_BAD        CRGB(220, 50, 50)
#define COL_GREEN_TXT  CRGB(40, 120, 40)

Color veg_color(int veg);
Color color_alpha(Color c, int a);

void gfx_circle(float cx, float cy, float r, Color c);
void gfx_ring(float cx, float cy, float r, float t, Color c);
void gfx_line(float x1, float y1, float x2, float y2, float t, Color c);
void gfx_rrect(float x, float y, float w, float h, float rad, Color c);
void gfx_frame(float x, float y, float w, float h, float t, Color c);
void gfx_panel(float x, float y, float w, float h);         /* painel de madeira */
void gfx_card(float x, float y, float w, float h, Color border); /* cartao creme */

/* Texto pequeno (UTF-8 / Latin-1). Retorna a largura em pixels. */
int  gfx_text_width(const char *s);
int  gfx_text(float x, float y, const char *s, Color c);
int  gfx_text_shadow(float x, float y, const char *s, Color c);
void gfx_text_center(float cx, float y, const char *s, Color c);
int  gfx_text_wrap(float x, float y, float maxw, float lineh, const char *s, Color c);

/* Letras "bolha" (titulos). h = altura em pixels. */
int  gfx_big_width(const char *s, float h);
void gfx_big(float x, float y, const char *s, float h);
void gfx_big_center(float cx, float y, const char *s, float h);

/* Sprites do atlas */
void gfx_spr(int id, float x, float y);
void gfx_spr_s(int id, float x, float y, float w, float h, int alpha);
void gfx_spr_c(int id, float cx, float cy, float scale);
void gfx_button(float x, float y, float w, float h, int selected);
void gfx_hintbar(float x, float y, float w, const char *s);

void gfx_die(float x, float y, float size, int veg, int value, int highlight);
void gfx_veg(float cx, float cy, float size, int veg, int alpha);
/* Dado da arte (cor = verdura). frame 0..4 girando, DICE_STILL parado. Centro em (cx, cy). */
void gfx_dice(float cx, float cy, int veg, int value, int frame, float scale, int alpha);
void gfx_pig(float cx, float cy, float size);
void gfx_ribbon(float cx, float cy, int lit);

#endif
