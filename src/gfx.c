/* Horta de Dados PS2 - primitivas de desenho de alto nivel. */
#include <math.h>
#include <string.h>
#include "gfx.h"
#include "game.h"

extern const unsigned char font_small_widths[224];

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

/* ---------------- formas ---------------- */

static int seg_for(float r) { return r < 6.0f ? 8 : (r < 14.0f ? 16 : SEG); }

void gfx_circle(float cx, float cy, float r, Color c)
{
    int i, step;
    if (!s_init) init_tables();
    step = SEG / seg_for(r);
    for (i = 0; i < SEG; i += step)
        plat_tri(cx, cy, cx + s_cos[i] * r, cy + s_sin[i] * r,
                 cx + s_cos[i + step] * r, cy + s_sin[i + step] * r, c);
}

void gfx_ring(float cx, float cy, float r, float t, Color c)
{
    int i, step;
    float ri = r - t;
    if (!s_init) init_tables();
    step = SEG / seg_for(r);
    for (i = 0; i < SEG; i += step) {
        float ax = cx + s_cos[i] * r, ay = cy + s_sin[i] * r;
        float bx = cx + s_cos[i + step] * r, by = cy + s_sin[i + step] * r;
        float qx = cx + s_cos[i] * ri, qy = cy + s_sin[i] * ri;
        float dx = cx + s_cos[i + step] * ri, dy = cy + s_sin[i + step] * ri;
        plat_tri(ax, ay, bx, by, qx, qy, c);
        plat_tri(bx, by, dx, dy, qx, qy, c);
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
    int i, step = r < 6.0f ? 8 : 4;
    int a0 = q * (SEG / 4);
    if (!s_init) init_tables();
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

void gfx_panel(float x, float y, float w, float h)
{
    float k;
    gfx_rrect(x + 3, y + 4, w, h, 10, COL_SHADOW);
    gfx_rrect(x, y, w, h, 10, COL_WOOD_DARK);
    gfx_rrect(x + 3, y + 3, w - 6, h - 6, 8, COL_WOOD);
    /* veios da madeira */
    for (k = y + 14; k < y + h - 8; k += 22)
        plat_rect(x + 10, k, w - 20, 2, CRGBA(120, 66, 30, 110));
    gfx_rrect(x + 3, y + 3, w - 6, 5, 3, CRGBA(255, 220, 160, 50));
}

void gfx_card(float x, float y, float w, float h, Color border)
{
    gfx_rrect(x + 2, y + 3, w, h, 9, COL_SHADOW);
    gfx_rrect(x, y, w, h, 9, border);
    gfx_rrect(x + 3, y + 3, w - 6, h - 6, 7, COL_PAPER);
}

/* ---------------- texto pequeno ---------------- */

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
    *ps += 1;
    while ((**ps & 0xC0) == 0x80) *ps += 1;
    return '?';
}

/* Icones de botoes no meio do texto: "{X}", "{ST}", "{L1}"... */
static const struct { const char *name; int spr; } TOKENS[] = {
    { "X", SPR_B_CRO }, { "O", SPR_B_CIR }, { "T", SPR_B_TRI }, { "Q", SPR_B_SQR },
    { "U", SPR_B_UP }, { "D", SPR_B_DOWN }, { "L", SPR_B_LEFT }, { "R", SPR_B_RIGHT },
    { "L1", SPR_B_L1 }, { "L2", SPR_B_L2 }, { "R1", SPR_B_R1 }, { "R2", SPR_B_R2 },
    { "ST", SPR_B_START }, { "SE", SPR_B_SEL }, { "AN", SPR_B_STICK },
};
#define ICON_H 20.0f
static int s_shadow_pass; /* a sombra do texto nao desenha os icones */

/* Se *ps comeca com um token conhecido, avanca e retorna o sprite; senao -1. */
static int next_token(const char **ps)
{
    const char *s = *ps, *e;
    unsigned i;
    if (*s != '{') return -1;
    e = strchr(s, '}');
    if (!e || e - s > 4) return -1;
    for (i = 0; i < sizeof(TOKENS) / sizeof(TOKENS[0]); i++)
        if (strlen(TOKENS[i].name) == (size_t)(e - s - 1) && !strncmp(TOKENS[i].name, s + 1, (size_t)(e - s - 1))) {
            *ps = e + 1;
            return TOKENS[i].spr;
        }
    return -1;
}

static float icon_w(int spr) { return (float)SPRITES[spr].w * ICON_H / (float)SPRITES[spr].h; }

static int glyph_index(int cp)
{
    if (cp < 32 || cp > 255) cp = '?';
    return cp - 32;
}

static void glyph(int idx, float x, float y, float sc, Color c)
{
    plat_sprite(ATLAS_FONT, x, y, 16 * sc, 18 * sc,
                (float)((idx % 16) * 16), (float)((idx / 16) * 18), 16, 18, c);
}

int gfx_text_width(const char *s)
{
    int w = 0, best = 0, cp, tk;
    for (;;) {
        if ((tk = next_token(&s)) >= 0) { w += (int)(icon_w(tk) + 2.5f); continue; }
        if ((cp = next_cp(&s)) == 0) break;
        if (cp == '\n') { if (w > best) best = w; w = 0; continue; }
        w += font_small_widths[glyph_index(cp)];
    }
    return w > best ? w : best;
}

int gfx_text(float x, float y, const char *s, Color c)
{
    float x0 = x, maxx = x;
    int cp, tk;
    for (;;) {
        int idx;
        if ((tk = next_token(&s)) >= 0) {
            float iw = icon_w(tk);
            if (!s_shadow_pass)
                plat_sprite(ATLAS_SPRITES, x, y - 1, iw, ICON_H, SPRITES[tk].x, SPRITES[tk].y,
                            SPRITES[tk].w, SPRITES[tk].h, color_alpha(COL_WHITE, (int)(c & 0xFF)));
            x += (int)(iw + 2.5f);
            continue;
        }
        if ((cp = next_cp(&s)) == 0) break;
        if (cp == '\n') { if (x > maxx) maxx = x; x = x0; y += 17; continue; }
        idx = glyph_index(cp);
        if (cp != ' ') glyph(idx, x, y, 1.0f, c);
        x += (float)font_small_widths[idx];
    }
    if (x > maxx) maxx = x;
    return (int)(maxx - x0);
}

int gfx_text_shadow(float x, float y, const char *s, Color c)
{
    s_shadow_pass = 1;
    gfx_text(x + 1, y + 2, s, CRGBA(40, 20, 8, (c & 0xFF) * 3 / 5));
    s_shadow_pass = 0;
    return gfx_text(x, y, s, c);
}

void gfx_text_center(float cx, float y, const char *s, Color c)
{
    float x = cx - (float)gfx_text_width(s) / 2.0f;
    int lum = (int)(((c >> 24) & 0xFF) * 3 + ((c >> 16) & 0xFF) * 6 + ((c >> 8) & 0xFF)) / 10;
    if (lum > 110) gfx_text_shadow(x, y, s, c);
    else gfx_text(x, y, s, c);
}

int gfx_text_wrap(float x, float y, float maxw, float lineh, const char *s, Color c)
{
    char line[256], word[128];
    int ll = 0, lines = 0;
    const char *p = s;
    line[0] = 0;
    for (;;) {
        int wl = 0, nl = 0;
        while (*p == ' ') p++;
        while (*p && *p != ' ' && *p != '\n' && wl < (int)sizeof(word) - 1) word[wl++] = *p++;
        word[wl] = 0;
        if (*p == '\n') { nl = 1; p++; }
        if (wl > 0) {
            char test[256];
            int tl = ll;
            memcpy(test, line, (size_t)ll);
            if (tl > 0) test[tl++] = ' ';
            if (tl + wl >= (int)sizeof(test) - 1) wl = (int)sizeof(test) - 2 - tl;
            memcpy(test + tl, word, (size_t)wl);
            tl += wl;
            test[tl] = 0;
            if (ll > 0 && (float)gfx_text_width(test) > maxw) {
                gfx_text(x, y + lines * lineh, line, c);
                lines++;
                memcpy(line, word, (size_t)wl + 1);
                ll = wl;
            } else {
                memcpy(line, test, (size_t)tl + 1);
                ll = tl;
            }
        }
        if (nl || !*p) {
            if (ll > 0 || nl) { gfx_text(x, y + lines * lineh, line, c); lines++; }
            ll = 0;
            line[0] = 0;
            if (!*p) break;
        }
    }
    return (int)(lines * lineh);
}

/* ---------------- letras "bolha" ---------------- */

/* Converte para maiuscula e separa o acento (Latin-1). acc: 0 = nenhum,
 * senao o codigo do glifo de acento na fonte pequena. */
static int big_base(int cp, int *acc)
{
    static const char base[] = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYPs";
    static const unsigned char accs[32] = {
        0x60, 0xB4, 0x5E, 0x7E, 0xA8, 0xB0, 0, 0xB8, 0x60, 0xB4, 0x5E, 0xA8, 0x60, 0xB4, 0x5E, 0xA8,
        0, 0x7E, 0x60, 0xB4, 0x5E, 0x7E, 0xA8, 0, 0, 0x60, 0xB4, 0x5E, 0xA8, 0xB4, 0, 0 };
    *acc = 0;
    if (cp >= 'a' && cp <= 'z') return cp - 32;
    if (cp >= 0xE0 && cp <= 0xFE && cp != 0xF7) cp -= 0x20;
    if (cp >= 0xC0 && cp <= 0xDF && cp != 0xD7) {
        *acc = accs[cp - 0xC0];
        return base[cp - 0xC0];
    }
    return cp;
}

static int bubble_idx(int c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= '0' && c <= '9') return 26 + c - '0';
    return -1;
}

static float big_adv(int c, float h)
{
    int bi = bubble_idx(c);
    float sc = h / (float)BUBBLE_H;
    if (c == ' ') return h * 0.32f;
    if (bi >= 0) return (float)BUBBLE[bi].w * sc - h * 0.05f;
    return (float)font_small_widths[glyph_index(c)] * h / 18.0f * 1.05f;
}

int gfx_big_width(const char *s, float h)
{
    float w = 0;
    int cp, acc;
    while ((cp = next_cp(&s)) != 0) w += big_adv(big_base(cp, &acc), h);
    return (int)(w + 0.5f);
}

void gfx_big(float x, float y, const char *s, float h)
{
    static const signed char o[8][2] = { {-2,0},{2,0},{0,-2},{0,2},{-2,-2},{2,2},{-2,2},{2,-2} };
    float sc = h / (float)BUBBLE_H;
    int cp, acc, k;
    while ((cp = next_cp(&s)) != 0) {
        int c = big_base(cp, &acc), bi = bubble_idx(c);
        float adv = big_adv(c, h);
        if (bi >= 0) {
            const SprRect *r = &BUBBLE[bi];
            plat_sprite(ATLAS_BUBBLE, x, y, r->w * sc, r->h * sc, r->x, r->y, r->w, r->h, COL_WHITE);
            if (acc) {
                float gs = h / 22.0f, gx = x + r->w * sc / 2 - 8 * gs;
                float gy = acc == 0xB8 ? y + h * 0.62f : y - h * 0.42f;
                int gi = glyph_index(acc);
                for (k = 0; k < 4; k++)
                    glyph(gi, gx + o[k][0], gy + o[k][1], gs, COL_INK);
                glyph(gi, gx, gy, gs, COL_YELLOW);
            }
        } else if (c != ' ') {
            /* pontuacao: glifo da fonte pequena ampliado, com contorno */
            float gs = h / 18.0f;
            int gi = glyph_index(c);
            for (k = 0; k < 8; k++) glyph(gi, x + o[k][0], y + o[k][1], gs, COL_INK);
            glyph(gi, x, y, gs, COL_YELLOW);
        }
        x += adv;
    }
}

void gfx_big_center(float cx, float y, const char *s, float h)
{
    gfx_big(cx - (float)gfx_big_width(s, h) / 2.0f, y, s, h);
}

/* ---------------- sprites ---------------- */

void gfx_spr_s(int id, float x, float y, float w, float h, int alpha)
{
    const SprRect *r = &SPRITES[id];
    plat_sprite(ATLAS_SPRITES, x, y, w, h, r->x, r->y, r->w, r->h, color_alpha(COL_WHITE, alpha));
}

void gfx_spr(int id, float x, float y)
{
    gfx_spr_s(id, x, y, SPRITES[id].w, SPRITES[id].h, 255);
}

void gfx_spr_c(int id, float cx, float cy, float scale)
{
    float w = SPRITES[id].w * scale, h = SPRITES[id].h * scale;
    gfx_spr_s(id, cx - w / 2, cy - h / 2, w, h, 255);
}

void gfx_button(float x, float y, float w, float h, int selected)
{
    int L = selected ? SPR_BTN_Y_L : SPR_BTN_C_L;
    int M = L + 1, R = L + 2;
    float sc = h / (float)SPRITES[L].h;
    float cw = SPRITES[L].w * sc;
    gfx_spr_s(L, x, y, cw, h, 255);
    gfx_spr_s(M, x + cw, y, w - 2 * cw, h, 255);
    gfx_spr_s(R, x + w - cw, y, cw, h, 255);
}

void gfx_hintbar(float x, float y, float w, const char *s)
{
    gfx_rrect(x + 2, y + 3, w, 26, 13, COL_SHADOW);
    gfx_rrect(x, y, w, 26, 13, COL_WOOD_DARK);
    gfx_rrect(x + 2, y + 2, w - 4, 22, 11, CRGB(166, 92, 40));
    float tw = (float)gfx_text_width(s), tx = x + (w - tw) / 2;
    if (tx < x + 12) tx = x + 12;
    gfx_text_shadow(tx, y + 4, s, CRGB(255, 240, 210));
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
    float r = size * 0.2f, k = size * 0.26f, pr = size * 0.09f;
    float cx = x + size / 2, cy = y + size / 2;
    int i;
    if (highlight) gfx_rrect(x - 4, y - 4, size + 8, size + 8, r + 3, COL_HILITE);
    gfx_rrect(x + 2, y + 3, size, size, r, COL_SHADOW);
    gfx_rrect(x, y, size, size, r, CRGB(60, 30, 14));
    gfx_rrect(x + 2, y + 2, size - 4, size - 4, r - 1, body);
    gfx_rrect(x + 4, y + 3, size - 8, size * 0.22f, r - 2, CRGBA(255, 255, 255, 60));
    if (value < 1 || value > 6) return;
    for (i = 0; PIPS[value][i][0] != 99; i++)
        gfx_circle(cx + PIPS[value][i][0] * k, cy + PIPS[value][i][1] * k, pr, pip);
}

void gfx_dice(float cx, float cy, int veg, int value, int frame, float scale, int alpha)
{
    const DiceFrame *f;
    if (veg < VEG_CARROT || veg > VEG_TOMATO || value < 1 || value > 6) return;
    if (frame < 0 || frame >= DICE_FRAMES) frame = DICE_STILL;
    f = &DICE_FR[veg - 1][value - 1][frame];
    plat_sprite(ATLAS_DICE, cx + f->ox * scale, cy + f->oy * scale, f->w * scale, f->h * scale,
                f->x, f->y, f->w, f->h, color_alpha(COL_WHITE, alpha));
}

void gfx_veg(float cx, float cy, float size, int veg, int alpha)
{
    int id;
    float w, h, sc;
    if (veg < VEG_CARROT || veg > VEG_TOMATO) return;
    if (veg == VEG_CARROT) id = size > 24 ? SPR_CARROT : SPR_CARROT_S;
    else if (veg == VEG_LETTUCE) id = size > 24 ? SPR_LETTUCE : SPR_LETTUCE_S;
    else id = size > 24 ? SPR_TOMATO : SPR_TOMATO_S;
    w = SPRITES[id].w;
    h = SPRITES[id].h;
    sc = size / (w > h ? w : h);
    gfx_spr_s(id, cx - w * sc / 2, cy - h * sc / 2, w * sc, h * sc, alpha);
}

void gfx_pig(float cx, float cy, float size)
{
    int id = size > 24 ? SPR_PIGLET_L : SPR_PIGLET;
    float sc = size / (float)SPRITES[id].w;
    gfx_spr_c(id, cx, cy, sc);
}

void gfx_ribbon(float cx, float cy, int lit)
{
    gfx_spr_c(lit ? SPR_RIBBON : SPR_RIBBON_OFF, cx, cy, 1.0f);
}
