#ifndef HORTA_GFX_H
#define HORTA_GFX_H
#include "plat.h"

/* Paleta */
#define COL_BG         CRGB(46, 92, 40)
#define COL_BG_DARK    CRGB(30, 64, 28)
#define COL_PANEL      CRGB(96, 64, 38)
#define COL_PANEL_DARK CRGB(70, 46, 28)
#define COL_PAPER      CRGB(246, 238, 214)
#define COL_PAPER_DARK CRGB(214, 200, 168)
#define COL_INK        CRGB(60, 40, 24)
#define COL_WHITE      CRGB(255, 255, 255)
#define COL_YELLOW     CRGB(255, 214, 64)
#define COL_SHADOW     CRGBA(0, 0, 0, 110)
#define COL_CARROT     CRGB(240, 130, 30)
#define COL_LETTUCE    CRGB(70, 175, 60)
#define COL_TOMATO     CRGB(214, 44, 40)
#define COL_PIG        CRGB(246, 160, 184)
#define COL_PIG_DARK   CRGB(214, 110, 140)
#define COL_HILITE     CRGB(255, 240, 120)
#define COL_BAD        CRGB(220, 50, 50)

Color veg_color(int veg);
Color color_alpha(Color c, int a);

void gfx_circle(float cx, float cy, float r, Color c);
void gfx_ring(float cx, float cy, float r, float t, Color c);
void gfx_line(float x1, float y1, float x2, float y2, float t, Color c);
void gfx_rrect(float x, float y, float w, float h, float rad, Color c);
void gfx_frame(float x, float y, float w, float h, float t, Color c);
void gfx_panel(float x, float y, float w, float h, Color fill);

/* Texto UTF-8 (Latin-1). Retorna a largura em pixels. */
int  gfx_text_width(int atlas, const char *s);
int  gfx_text(int atlas, float x, float y, const char *s, Color c);
int  gfx_text_shadow(int atlas, float x, float y, const char *s, Color c);
void gfx_text_center(int atlas, float cx, float y, const char *s, Color c);
/* Texto com quebra de linha automatica; retorna a altura usada. */
int  gfx_text_wrap(int atlas, float x, float y, float maxw, float lineh, const char *s, Color c);
int  gfx_line_height(int atlas);

void gfx_die(float x, float y, float size, int veg, int value, int highlight);
void gfx_veg(float cx, float cy, float size, int veg, int alpha);
void gfx_pig(float cx, float cy, float size);
void gfx_ribbon(float cx, float cy, float size, int lit);

#endif
