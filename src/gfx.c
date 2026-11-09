/* Horta de Dados PS2 - primitivas de desenho de alto nivel. */
#include <math.h>
#include <string.h>
#include "gfx.h"
#include "game.h"

extern const unsigned char font_small_widths[224];
extern const unsigned char font_big_widths[96];

#define SEG 32
static float s_cos[SEG + 1], s_sin[SEG + 1];
static int s_init;

static void init_tables(void)
{
    int i;
    for (i = 0; i <= SEG; i++) {
        float a = (float)i * 6.2831853f / (float)SEG;
        s_cos[i] = cosf(a);
        s_sin[i] = sinf(a);
    }
    s_init = 1;
}

Color veg_color(int veg)
{
    switch (veg) {
    case VEG_CARROT: return COL_CARROT;
    case VEG_LETTUCE: return COL_LETTUCE;
    case VEG_TOMATO: return COL_TOMATO;
    }
    return COL_PAPER_DARK;
}

Color color_alpha(Color c, int a)
{
    if (a < 0) a = 0;
    if (a > 255) a = 255;
    return (c & 0xFFFFFF00u) | (unsigned)(((c & 0xFF) * (unsigned)a) / 255u);
}

static int seg_for(float r)
{
    return r < 6.0f ? 8 : (r < 14.0f ? 16 : SEG);
}

void gfx_circle(float cx, float cy, float r, Color c)
{
    int i, n = seg_for(r), step;
    if (!s_init) init_tables();
    step = SEG / n;
    for (i = 0; i < SEG; i += step)
        plat_tri(cx, cy, cx + s_cos[i] * r, cy + s_sin[i] * r,
                 cx + s_cos[i + step] * r, cy + s_sin[i + step] * r, c);
}

void gfx_ring(float cx, float cy, float r, float t, Color c)
{
    int i, n = seg_for(r), step;
    float ri = r - t;
    if (!s_init) init_tables();
    step = SEG / n;
    for (i = 0; i < SEG; i += step) {
        float ax = cx + s_cos[i] * r, ay = cy + s_sin[i] * r;
        float bx = cx + s_cos[i + step] * r, by = cy + s_sin[i + step] * r;
        float cx2 = cx + s_cos[i] * ri, cy2 = cy + s_sin[i] * ri;
        float dx = cx + s_cos[i + step] * ri, dy = cy + s_sin[i + step] * ri;
        plat_tri(ax, ay, bx, by, cx2, cy2, c);
        plat_tri(bx, by, dx, dy, cx2, cy2, c);
    }
}

void gfx_line(float x1, float y1, float x2, float y2, float t, Color c)
{
    float dx = x2 - x1, dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    float nx, ny;
    if (len < 0.001f) return;
    nx = -dy / len * t * 0.5f;
    ny = dx / len * t * 0.5f;
    plat_tri(x1 + nx, y1 + ny, x2 + nx, y2 + ny, x1 - nx, y1 - ny, c);
    plat_tri(x2 + nx, y2 + ny, x2 - nx, y2 - ny, x1 - nx, y1 - ny, c);
}

static void quarter(float cx, float cy, float r, int q, Color c)
{
    int i, step = 4;
    int a0 = q * (SEG / 4);
    if (!s_init) init_tables();
    if (r < 6.0f) step = 8;
    for (i = a0; i < a0 + SEG / 4; i += step)
        plat_tri(cx, cy, cx + s_cos[i] * r, cy + s_sin[i] * r,
                 cx + s_cos[i + step] * r, cy + s_sin[i + step] * r, c);
}

void gfx_rrect(float x, float y, float w, float h, float rad, Color c)
{
    if (rad * 2 > w) rad = w / 2;
    if (rad * 2 > h) rad = h / 2;
    if (rad < 1.0f) { plat_rect(x, y, w, h, c); return; }
    plat_rect(x + rad, y, w - 2 * rad, h, c);
    plat_rect(x, y + rad, rad, h - 2 * rad, c);
    plat_rect(x + w - rad, y + rad, rad, h - 2 * rad, c);
    quarter(x + w - rad, y + h - rad, rad, 0, c);
    quarter(x + rad, y + h - rad, rad, 1, c);
    quarter(x + rad, y + rad, rad, 2, c);
    quarter(x + w - rad, y + rad, rad, 3, c);
}

void gfx_frame(float x, float y, float w, float h, float t, Color c)
{
    plat_rect(x, y, w, t, c);
    plat_rect(x, y + h - t, w, t, c);
    plat_rect(x, y + t, t, h - 2 * t, c);
    plat_rect(x + w - t, y + t, t, h - 2 * t, c);
}

void gfx_panel(float x, float y, float w, float h, Color fill)
{
    gfx_rrect(x + 3, y + 4, w, h, 8, COL_SHADOW);
    gfx_rrect(x, y, w, h, 8, COL_PANEL_DARK);
    gfx_rrect(x + 3, y + 3, w - 6, h - 6, 6, fill);
}

/* ---------------- texto ---------------- */

static const char *big_fold =
    /* Latin-1 160..255 -> letra ASCII equivalente (para o atlas grande) */
    "  c       a     "
    "o 23 u   1o    ?"
    "AAAAAAACEEEEIIII"
    "DNOOOOOxOUUUUYPs"
    "aaaaaaaceeeeiiii"
    "dnooooo/ouuuuypy";

static int next_cp(const char **ps)
{
    const unsigned char *s = (const unsigned char *)*ps;
    int cp = s[0];
    if (cp == 0) return 0;
    if (cp < 0x80) { *ps += 1; return cp; }
    if ((cp & 0xE0) == 0xC0 && s[1]) {
        cp = ((cp & 0x1F) << 6) | (s[1] & 0x3F);
        *ps += 2;
        return cp;
    }
    /* sequencias maiores: pula e mostra '?' */
    *ps += 1;
    while ((**ps & 0xC0) == 0x80) *ps += 1;
    return '?';
}

static int glyph_index(int atlas, int cp)
{
    if (atlas == ATLAS_BIG) {
        if (cp >= 160 && cp <= 255) {
            int i = cp - 160;
            cp = i < (int)strlen(big_fold) ? big_fold[i] : '?';
        }
        if (cp < 32 || cp > 127) cp = '?';
        return cp - 32;
    }
    if (cp < 32 || cp > 255) cp = '?';
    return cp - 32;
}

static int glyph_adv(int atlas, int idx)
{
    return atlas == ATLAS_BIG ? font_big_widths[idx] : font_small_widths[idx];
}

int gfx_line_height(int atlas)
{
    return atlas == ATLAS_BIG ? 34 : 17;
}

int gfx_text_width(int atlas, const char *s)
{
    int w = 0, best = 0, cp;
    while ((cp = next_cp(&s)) != 0) {
        if (cp == '\n') { if (w > best) best = w; w = 0; continue; }
        w += glyph_adv(atlas, glyph_index(atlas, cp));
    }
    return w > best ? w : best;
}

int gfx_text(int atlas, float x, float y, const char *s, Color c)
{
    int cw = atlas == ATLAS_BIG ? 32 : 16;
    int ch = atlas == ATLAS_BIG ? 40 : 18;
    float x0 = x, maxx = x;
    int cp;
    while ((cp = next_cp(&s)) != 0) {
        int idx, adv;
        if (cp == '\n') { if (x > maxx) maxx = x; x = x0; y += (float)gfx_line_height(atlas); continue; }
        idx = glyph_index(atlas, cp);
        adv = glyph_adv(atlas, idx);
        if (cp != ' ')
            plat_glyph(atlas, x, y, (float)cw, (float)ch,
                       (float)((idx % 16) * cw), (float)((idx / 16) * ch), (float)cw, (float)ch, c);
        x += (float)adv;
    }
    if (x > maxx) maxx = x;
    return (int)(maxx - x0);
}

int gfx_text_shadow(int atlas, float x, float y, const char *s, Color c)
{
    float o = atlas == ATLAS_BIG ? 3.0f : 2.0f;
    gfx_text(atlas, x + o, y + o, s, CRGBA(0, 0, 0, 150));
    return gfx_text(atlas, x, y, s, c);
}

void gfx_text_center(int atlas, float cx, float y, const char *s, Color c)
{
    float x = cx - (float)gfx_text_width(atlas, s) / 2.0f;
    int lum = (int)(((c >> 24) & 0xFF) * 3 + ((c >> 16) & 0xFF) * 6 + ((c >> 8) & 0xFF)) / 10;
    /* sombra so em texto claro; texto escuro fica limpo */
    if (lum > 110) gfx_text_shadow(atlas, x, y, s, c);
    else gfx_text(atlas, x, y, s, c);
}

int gfx_text_wrap(int atlas, float x, float y, float maxw, float lineh, const char *s, Color c)
{
    char line[256], word[128];
    int ll = 0, lines = 0;
    const char *p = s;
    line[0] = 0;
    while (1) {
        int wl = 0, nl = 0;
        while (*p == ' ') p++;
        while (*p && *p != ' ' && *p != '\n' && wl < (int)sizeof(word) - 1) word[wl++] = *p++;
        word[wl] = 0;
        if (*p == '\n') { nl = 1; p++; }
        if (wl > 0) {
            char test[256];
            int tl = 0;
            memcpy(test, line, (size_t)ll);
            tl = ll;
            if (tl > 0) test[tl++] = ' ';
            if (tl + wl >= (int)sizeof(test) - 1) wl = (int)sizeof(test) - 2 - tl;
            memcpy(test + tl, word, (size_t)wl);
            tl += wl;
            test[tl] = 0;
            if (ll > 0 && (float)gfx_text_width(atlas, test) > maxw) {
                gfx_text(atlas, x, y + lines * lineh, line, c);
                lines++;
                memcpy(line, word, (size_t)wl + 1);
                ll = wl;
            } else {
                memcpy(line, test, (size_t)tl + 1);
                ll = tl;
            }
        }
        if (nl || !*p) {
            if (ll > 0 || nl) {
                gfx_text(atlas, x, y + lines * lineh, line, c);
                lines++;
            }
            ll = 0;
            line[0] = 0;
            if (!*p) break;
        }
    }
    return (int)(lines * lineh);
}

/* ---------------- icones ---------------- */

static const signed char PIPS[7][7][2] = {
    { {0} },
    { {0, 0}, {99} },
    { {-1, -1}, {1, 1}, {99} },
    { {-1, -1}, {0, 0}, {1, 1}, {99} },
    { {-1, -1}, {1, -1}, {-1, 1}, {1, 1}, {99} },
    { {-1, -1}, {1, -1}, {0, 0}, {-1, 1}, {1, 1}, {99} },
    { {-1, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {1, 1}, {99} },
};

void gfx_die(float x, float y, float size, int veg, int value, int highlight)
{
    Color body = veg ? veg_color(veg) : COL_PAPER;
    Color pip = veg ? COL_WHITE : COL_INK;
    float r = size * 0.18f, k = size * 0.26f, pr = size * 0.09f;
    float cx = x + size / 2, cy = y + size / 2;
    int i;
    if (highlight) gfx_rrect(x - 4, y - 4, size + 8, size + 8, r + 3, COL_HILITE);
    gfx_rrect(x + 2, y + 3, size, size, r, COL_SHADOW);
    gfx_rrect(x, y, size, size, r, CRGB(40, 30, 20));
    gfx_rrect(x + 2, y + 2, size - 4, size - 4, r - 1, body);
    if (value < 1 || value > 6) return;
    for (i = 0; PIPS[value][i][0] != 99; i++)
        gfx_circle(cx + PIPS[value][i][0] * k, cy + PIPS[value][i][1] * k, pr, pip);
}

void gfx_veg(float cx, float cy, float s, int veg, int alpha)
{
    if (veg == VEG_CARROT) {
        Color o = color_alpha(COL_CARROT, alpha), g = color_alpha(CRGB(60, 150, 50), alpha);
        Color d = color_alpha(CRGB(190, 90, 20), alpha);
        plat_tri(cx - s * 0.10f, cy - s * 0.32f, cx - s * 0.22f, cy - s * 0.55f, cx + s * 0.02f, cy - s * 0.30f, g);
        plat_tri(cx + s * 0.10f, cy - s * 0.32f, cx + s * 0.22f, cy - s * 0.55f, cx - s * 0.02f, cy - s * 0.30f, g);
        plat_tri(cx - s * 0.04f, cy - s * 0.30f, cx, cy - s * 0.60f, cx + s * 0.04f, cy - s * 0.30f, g);
        plat_tri(cx - s * 0.24f, cy - s * 0.32f, cx + s * 0.24f, cy - s * 0.32f, cx, cy + s * 0.48f, o);
        gfx_line(cx - s * 0.10f, cy - s * 0.10f, cx + s * 0.02f, cy - s * 0.12f, 2, d);
        gfx_line(cx - s * 0.04f, cy + s * 0.10f, cx + s * 0.08f, cy + s * 0.08f, 2, d);
    } else if (veg == VEG_LETTUCE) {
        Color a = color_alpha(CRGB(50, 140, 45), alpha), b = color_alpha(COL_LETTUCE, alpha);
        Color c = color_alpha(CRGB(150, 220, 110), alpha);
        gfx_circle(cx - s * 0.18f, cy + s * 0.05f, s * 0.26f, a);
        gfx_circle(cx + s * 0.18f, cy + s * 0.05f, s * 0.26f, a);
        gfx_circle(cx, cy - s * 0.10f, s * 0.30f, b);
        gfx_circle(cx, cy + s * 0.12f, s * 0.28f, b);
        gfx_circle(cx, cy, s * 0.16f, c);
        gfx_line(cx, cy - s * 0.12f, cx, cy + s * 0.30f, 2, color_alpha(CRGB(90, 170, 70), alpha));
    } else if (veg == VEG_TOMATO) {
        Color r = color_alpha(COL_TOMATO, alpha), h = color_alpha(CRGB(250, 120, 110), alpha);
        Color g = color_alpha(CRGB(50, 140, 45), alpha);
        gfx_circle(cx, cy + s * 0.06f, s * 0.40f, r);
        gfx_circle(cx - s * 0.16f, cy - s * 0.06f, s * 0.08f, h);
        plat_tri(cx - s * 0.26f, cy - s * 0.26f, cx + s * 0.26f, cy - s * 0.26f, cx, cy - s * 0.14f, g);
        plat_tri(cx - s * 0.10f, cy - s * 0.40f, cx + s * 0.10f, cy - s * 0.40f, cx, cy - s * 0.10f, g);
        plat_rect(cx - s * 0.03f, cy - s * 0.50f, s * 0.06f, s * 0.16f, g);
    }
}

void gfx_pig(float cx, float cy, float s)
{
    Color ink = CRGB(90, 40, 50);
    plat_tri(cx - s * 0.42f, cy - s * 0.20f, cx - s * 0.30f, cy - s * 0.62f, cx - s * 0.08f, cy - s * 0.36f, COL_PIG_DARK);
    plat_tri(cx + s * 0.42f, cy - s * 0.20f, cx + s * 0.30f, cy - s * 0.62f, cx + s * 0.08f, cy - s * 0.36f, COL_PIG_DARK);
    gfx_circle(cx, cy, s * 0.48f, COL_PIG);
    gfx_circle(cx, cy + s * 0.12f, s * 0.20f, COL_PIG_DARK);
    gfx_circle(cx - s * 0.08f, cy + s * 0.12f, s * 0.05f, ink);
    gfx_circle(cx + s * 0.08f, cy + s * 0.12f, s * 0.05f, ink);
    gfx_circle(cx - s * 0.18f, cy - s * 0.14f, s * 0.06f, ink);
    gfx_circle(cx + s * 0.18f, cy - s * 0.14f, s * 0.06f, ink);
}

void gfx_ribbon(float cx, float cy, float s, int lit)
{
    Color c = lit ? CRGB(60, 150, 230) : CRGB(150, 140, 120);
    Color c2 = lit ? CRGB(40, 110, 190) : CRGB(120, 110, 95);
    plat_tri(cx - s * 0.30f, cy, cx - s * 0.05f, cy, cx - s * 0.35f, cy + s * 0.60f, c2);
    plat_tri(cx + s * 0.30f, cy, cx + s * 0.05f, cy, cx + s * 0.35f, cy + s * 0.60f, c2);
    gfx_circle(cx, cy, s * 0.40f, c);
    gfx_circle(cx, cy, s * 0.30f, lit ? CRGB(240, 250, 255) : CRGB(200, 192, 170));
    gfx_text_center(ATLAS_SMALL, cx, cy - 9, "5", lit ? CRGB(30, 90, 170) : CRGB(110, 100, 90));
}
