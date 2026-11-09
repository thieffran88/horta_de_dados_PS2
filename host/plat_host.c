/* Plataforma "host" (PC): rasterizador por software para testes e capturas.
 * Nao faz parte do jogo de PS2; serve para validar regras e layout sem console. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/plat.h"

extern const unsigned char font_small_pixels[];

/* atlas RGBA expandidos a partir da paleta */
static unsigned char *atlas_rgba[NUM_ATLAS];
static int atlas_w[NUM_ATLAS] = { 256, SPRITES_W, BUBBLE_W, DICE_W, SPLASH_W };

static unsigned char *expand(const unsigned char *bin, int w, int h)
{
    unsigned char *out = malloc((size_t)w * h * 4);
    const unsigned char *pal = bin, *idx = bin + 1024;
    int i;
    for (i = 0; i < w * h; i++) memcpy(out + i * 4, pal + idx[i] * 4, 4);
    return out;
}

static unsigned char fb[SCREEN_H][SCREEN_W][3];
static unsigned host_buttons;
static const char *shot_path;
int host_ntri, host_nrect, host_nglyph;

void host_set_buttons(unsigned b) { host_buttons = b; }
void host_request_shot(const char *path) { shot_path = path; }

int plat_init(void)
{
    if (!atlas_rgba[ATLAS_SPRITES]) {
        atlas_rgba[ATLAS_SPRITES] = expand(asset_sprites, SPRITES_W, SPRITES_H);
        atlas_rgba[ATLAS_BUBBLE] = expand(asset_bubble, BUBBLE_W, BUBBLE_H_TEX);
        atlas_rgba[ATLAS_DICE] = expand(asset_dice, DICE_W, DICE_H);
        atlas_rgba[ATLAS_SPLASH] = expand(asset_splash, SPLASH_W, SPLASH_H);
    }
    return 0;
}
unsigned plat_buttons(void) { return host_buttons; }
unsigned plat_seed(void) { return 20261109u; }

static void blend(int x, int y, Color c, int extra_a)
{
    int a = (int)(c & 0xFF) * extra_a / 255;
    unsigned char *p;
    if (x < 0 || y < 0 || x >= SCREEN_W || y >= SCREEN_H || a <= 0) return;
    p = fb[y][x];
    p[0] = (unsigned char)((((c >> 24) & 0xFF) * a + p[0] * (255 - a)) / 255);
    p[1] = (unsigned char)((((c >> 16) & 0xFF) * a + p[1] * (255 - a)) / 255);
    p[2] = (unsigned char)((((c >> 8) & 0xFF) * a + p[2] * (255 - a)) / 255);
}

void plat_frame_begin(Color clear)
{
    host_ntri = host_nrect = host_nglyph = 0;
    int x, y;
    for (y = 0; y < SCREEN_H; y++)
        for (x = 0; x < SCREEN_W; x++) {
            fb[y][x][0] = (clear >> 24) & 0xFF;
            fb[y][x][1] = (clear >> 16) & 0xFF;
            fb[y][x][2] = (clear >> 8) & 0xFF;
        }
}

void plat_frame_end(void)
{
    FILE *f;
    if (!shot_path) return;
    f = fopen(shot_path, "wb");
    if (f) {
        fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
        fwrite(fb, 1, sizeof(fb), f);
        fclose(f);
    }
    shot_path = NULL;
}

/* Mesma convencao do GS: pixel coberto se o centro (x+0.5) esta dentro. */
void plat_rect(float x, float y, float w, float h, Color c)
{
    host_nrect++;
    int x0 = (int)(x + 0.5f), y0 = (int)(y + 0.5f), x1 = (int)(x + w + 0.5f), y1 = (int)(y + h + 0.5f);
    int i, j;
    for (j = y0; j < y1; j++)
        for (i = x0; i < x1; i++) blend(i, j, c, 255);
}

static float edge(float ax, float ay, float bx, float by, float px, float py)
{
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

void plat_tri(float x1, float y1, float x2, float y2, float x3, float y3, Color c)
{
    float minx = x1, maxx = x1, miny = y1, maxy = y1, area;
    int i, j;
    if (x2 < minx) minx = x2;
    if (x3 < minx) minx = x3;
    if (x2 > maxx) maxx = x2;
    if (x3 > maxx) maxx = x3;
    if (y2 < miny) miny = y2;
    if (y3 < miny) miny = y3;
    if (y2 > maxy) maxy = y2;
    if (y3 > maxy) maxy = y3;
    host_ntri++;
    area = edge(x1, y1, x2, y2, x3, y3);
    if (area == 0) return;
    for (j = (int)miny; j <= (int)maxy; j++)
        for (i = (int)minx; i <= (int)maxx; i++) {
            float px = (float)i + 0.5f, py = (float)j + 0.5f;
            float w0 = edge(x2, y2, x3, y3, px, py);
            float w1 = edge(x3, y3, x1, y1, px, py);
            float w2 = edge(x1, y1, x2, y2, px, py);
            if (area < 0) { w0 = -w0; w1 = -w1; w2 = -w2; }
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) blend(i, j, c, 255);
        }
}

static void blend_rgb(int x, int y, int r, int g, int b, int a)
{
    unsigned char *p;
    if (x < 0 || y < 0 || x >= SCREEN_W || y >= SCREEN_H || a <= 0) return;
    p = fb[y][x];
    p[0] = (unsigned char)((r * a + p[0] * (255 - a)) / 255);
    p[1] = (unsigned char)((g * a + p[1] * (255 - a)) / 255);
    p[2] = (unsigned char)((b * a + p[2] * (255 - a)) / 255);
}

/* amostragem bilinear (como GS_FILTER_LINEAR), limitada ao retangulo de origem */
static void sample(int atlas, float u, float v, float u0, float v0, float u1, float v1, float out[4])
{
    const unsigned char *t = atlas_rgba[atlas];
    int W = atlas_w[atlas], k, i;
    float fu, fv;
    int x0, y0;
    u -= 0.5f; v -= 0.5f;
    if (u < u0) u = u0;
    if (v < v0) v = v0;
    if (u > u1 - 1) u = u1 - 1;
    if (v > v1 - 1) v = v1 - 1;
    x0 = (int)u; y0 = (int)v;
    fu = u - x0; fv = v - y0;
    for (k = 0; k < 4; k++) out[k] = 0;
    for (i = 0; i < 4; i++) {
        int xx = x0 + (i & 1), yy = y0 + (i >> 1);
        float wgt = ((i & 1) ? fu : 1 - fu) * ((i >> 1) ? fv : 1 - fv);
        const unsigned char *px;
        if (xx >= (int)u1) xx = (int)u1 - 1;
        if (yy >= (int)v1) yy = (int)v1 - 1;
        px = t + ((size_t)yy * W + xx) * 4;
        /* pre-multiplicado para nao "sangrar" a cor de pixels transparentes */
        out[0] += px[0] * px[3] / 255.0f * wgt;
        out[1] += px[1] * px[3] / 255.0f * wgt;
        out[2] += px[2] * px[3] / 255.0f * wgt;
        out[3] += px[3] * wgt;
    }
}

void plat_sprite(int atlas, float x, float y, float w, float h,
                 float u, float v, float uw, float vh, Color c)
{
    int x0 = (int)(x + 0.5f), y0 = (int)(y + 0.5f), i, j;
    int x1 = (int)(x + w + 0.5f), y1 = (int)(y + h + 0.5f);
    host_nglyph++;
    if (atlas == ATLAS_FONT) {
        for (j = y0; j < y1; j++)
            for (i = x0; i < x1; i++) {
                int tu = (int)(u + ((float)i + 0.5f - x) * uw / w), tv = (int)(v + ((float)j + 0.5f - y) * vh / h);
                int idx = tv * 256 + tu;
                int a4 = (font_small_pixels[idx >> 1] >> ((idx & 1) * 4)) & 15;
                if (a4) blend(i, j, c, a4 * 17);
            }
        return;
    }
    for (j = y0; j < y1; j++)
        for (i = x0; i < x1; i++) {
            float s[4], a;
            sample(atlas, u + ((float)i + 0.5f - x) * uw / w, v + ((float)j + 0.5f - y) * vh / h,
                   u, v, u + uw, v + vh, s);
            a = s[3] * (float)(c & 0xFF) / 255.0f;
            if (a < 1.0f || s[3] <= 0) continue;
            blend_rgb(i, j, (int)(s[0] / s[3] * 255.0f * ((c >> 24) & 0xFF) / 255.0f),
                      (int)(s[1] / s[3] * 255.0f * ((c >> 16) & 0xFF) / 255.0f),
                      (int)(s[2] / s[3] * 255.0f * ((c >> 8) & 0xFF) / 255.0f), (int)a);
        }
}
