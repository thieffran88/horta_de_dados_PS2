/* Plataforma "host" (PC): rasterizador por software para testes e capturas.
 * Nao faz parte do jogo de PS2; serve para validar regras e layout sem console. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/plat.h"

extern const unsigned char font_small_pixels[];
extern const unsigned char font_big_pixels[];

static unsigned char fb[SCREEN_H][SCREEN_W][3];
static unsigned host_buttons;
static const char *shot_path;
int host_ntri, host_nrect, host_nglyph;

void host_set_buttons(unsigned b) { host_buttons = b; }
void host_request_shot(const char *path) { shot_path = path; }

int plat_init(void) { return 0; }
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

void plat_glyph(int atlas, float x, float y, float w, float h,
                float u, float v, float uw, float vh, Color c)
{
    const unsigned char *px = atlas == ATLAS_BIG ? font_big_pixels : font_small_pixels;
    int aw = atlas == ATLAS_BIG ? 512 : 256;
    int x0 = (int)(x + 0.5f), y0 = (int)(y + 0.5f), i, j;
    int dw = (int)(w + 0.5f), dh = (int)(h + 0.5f);
    host_nglyph++;
    for (j = 0; j < dh; j++)
        for (i = 0; i < dw; i++) {
            int tu = (int)(u + (float)i * uw / w), tv = (int)(v + (float)j * vh / h);
            int idx = tv * aw + tu;
            int a4 = (px[idx >> 1] >> ((idx & 1) * 4)) & 15;
            if (a4) blend(x0 + i, y0 + j, c, a4 * 17);
        }
}
