/* Horta de Dados PS2 - plataforma PlayStation 2 (gsKit + libpad). */
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <sbv_patches.h>
#include <libpad.h>
#include <malloc.h>
#include <string.h>
#include <time.h>
#include <timer.h>
#include <gsKit.h>
#include <dmaKit.h>
#include "plat.h"

extern const unsigned char font_small_pixels[];
extern const unsigned char font_big_pixels[];

static GSGLOBAL *gs;
static GSTEXTURE tex[2];
static float yoff;

static char pad_buf[2][256] __attribute__((aligned(64)));
static int pad_open[2];

/* ---------------- cores ---------------- */

static u64 gs_color(Color c)
{
    /* No GS, alfa 0x80 = opaco. */
    unsigned a = ((c & 0xFF) + 1) >> 1;
    return GS_SETREG_RGBAQ((c >> 24) & 0xFF, (c >> 16) & 0xFF, (c >> 8) & 0xFF, a, 0x00);
}

static u64 gs_tex_color(Color c)
{
    /* Textura branca em modo MODULATE: resultado = tex * cor / 128. */
    unsigned a = ((c & 0xFF) + 1) >> 1;
    return GS_SETREG_RGBAQ(((c >> 24) & 0xFF) >> 1, ((c >> 16) & 0xFF) >> 1, ((c >> 8) & 0xFF) >> 1, a, 0x00);
}

/* ---------------- texturas da fonte ---------------- */

static void make_font_texture(GSTEXTURE *t, const unsigned char *packed, int w, int h)
{
    int i, n = w * h;
    u32 *px = (u32 *)memalign(128, (size_t)n * 4);
    for (i = 0; i < n; i++) {
        int a4 = (packed[i >> 1] >> ((i & 1) * 4)) & 15;
        u32 a = (u32)(a4 * 128 / 15);
        px[i] = 0x00FFFFFFu | (a << 24);
    }
    FlushCache(0);
    memset(t, 0, sizeof(*t));
    t->Width = (u32)w;
    t->Height = (u32)h;
    t->PSM = GS_PSM_CT32;
    t->Mem = px;
    t->Filter = GS_FILTER_NEAREST;
}

/* ---------------- controles ---------------- */

static void load_modules(void)
{
    /* Reinicia o IOP para um estado conhecido (independe de onde o ELF foi lancado). */
    SifInitRpc(0);
    while (!SifIopReset("", 0)) {}
    while (!SifIopSync()) {}
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();

    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);
}

static void open_pads(void)
{
    int port;
    padInit(0);
    for (port = 0; port < 2; port++)
        pad_open[port] = padPortOpen(port, 0, pad_buf[port]) != 0;
}

unsigned plat_buttons(void)
{
    unsigned out = 0;
    int port;
    for (port = 0; port < 2; port++) {
        struct padButtonStatus st;
        int state;
        u32 b;
        if (!pad_open[port]) continue;
        state = padGetState(port, 0);
        if (state != PAD_STATE_STABLE && state != PAD_STATE_FINDCTP1) continue;
        if (padRead(port, 0, &st) == 0) continue;
        b = 0xFFFF ^ st.btns;
        if (b & PAD_UP) out |= BTN_UP;
        if (b & PAD_DOWN) out |= BTN_DOWN;
        if (b & PAD_LEFT) out |= BTN_LEFT;
        if (b & PAD_RIGHT) out |= BTN_RIGHT;
        if (b & PAD_CROSS) out |= BTN_CROSS;
        if (b & PAD_CIRCLE) out |= BTN_CIRCLE;
        if (b & PAD_SQUARE) out |= BTN_SQUARE;
        if (b & PAD_TRIANGLE) out |= BTN_TRIANGLE;
        if (b & PAD_L1) out |= BTN_L1;
        if (b & PAD_R1) out |= BTN_R1;
        if (b & PAD_L2) out |= BTN_L2;
        if (b & PAD_R2) out |= BTN_R2;
        if (b & PAD_START) out |= BTN_START;
        if (b & PAD_SELECT) out |= BTN_SELECT;
        /* analogico esquerdo tambem funciona como direcional (modo DualShock) */
        if ((st.mode >> 4) == 0x07) {
            if (st.ljoy_h < 0x30) out |= BTN_LEFT;
            if (st.ljoy_h > 0xD0) out |= BTN_RIGHT;
            if (st.ljoy_v < 0x30) out |= BTN_UP;
            if (st.ljoy_v > 0xD0) out |= BTN_DOWN;
        }
    }
    return out;
}

unsigned plat_seed(void)
{
    return ((unsigned)clock() * 2654435761u) ^ (unsigned)cpu_ticks();
}

/* ---------------- video ---------------- */

int plat_init(void)
{
    load_modules();
    open_pads();

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    /* fila "oneshot" de 2 MB: a tela do jogo gera ~3000 triangulos por quadro */
    gs = gsKit_init_global_custom(2 * 1024 * 1024, GS_RENDER_QUEUE_PER_POOLSIZE);
    gs->PrimAlphaEnable = GS_SETTING_ON;
    gs->ZBuffering = GS_SETTING_OFF;
    gs->DoubleBuffering = GS_SETTING_ON;
    gs->PSM = GS_PSM_CT24;

    gsKit_init_screen(gs);
    gsKit_TexManager_init(gs);
    gsKit_mode_switch(gs, GS_ONESHOT);
    gsKit_set_test(gs, GS_ZTEST_OFF);
    gsKit_set_primalpha(gs, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
    gsKit_set_clamp(gs, GS_CMODE_CLAMP);

    /* layout feito para 640x448 (NTSC); no PAL (512 linhas) centraliza. */
    yoff = gs->Height > SCREEN_H ? (float)((gs->Height - SCREEN_H) / 2) : 0.0f;

    make_font_texture(&tex[ATLAS_SMALL], font_small_pixels, 256, 256);
    make_font_texture(&tex[ATLAS_BIG], font_big_pixels, 512, 256);
    return 0;
}

void plat_frame_begin(Color clear)
{
    gsKit_clear(gs, gs_color(clear | 0xFF));
}

void plat_frame_end(void)
{
    gsKit_queue_exec(gs);
    gsKit_sync_flip(gs);
    gsKit_TexManager_nextFrame(gs);
}

void plat_rect(float x, float y, float w, float h, Color c)
{
    if ((c & 0xFF) == 0) return;
    gsKit_prim_sprite(gs, x, y + yoff, x + w, y + h + yoff, 0, gs_color(c));
}

void plat_tri(float x1, float y1, float x2, float y2, float x3, float y3, Color c)
{
    if ((c & 0xFF) == 0) return;
    gsKit_prim_triangle(gs, x1, y1 + yoff, x2, y2 + yoff, x3, y3 + yoff, 0, gs_color(c));
}

void plat_glyph(int atlas, float x, float y, float w, float h,
                float u, float v, float uw, float vh, Color c)
{
    GSTEXTURE *t = &tex[atlas ? 1 : 0];
    if ((c & 0xFF) == 0) return;
    gsKit_TexManager_bind(gs, t);
    gsKit_prim_sprite_texture(gs, t, x, y + yoff, u, v, x + w, y + h + yoff, u + uw, v + vh, 0, gs_tex_color(c));
}
