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
#include <audsrv.h>
#include "plat.h"
#include "audio.h"

extern const unsigned char font_small_pixels[];

static GSGLOBAL *gs;
static GSTEXTURE tex[NUM_ATLAS];
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

static void init_texture(GSTEXTURE *t, u32 *px, int w, int h, int filter)
{
    FlushCache(0);
    memset(t, 0, sizeof(*t));
    t->Width = (u32)w;
    t->Height = (u32)h;
    t->PSM = GS_PSM_CT32;
    t->Mem = px;
    t->Filter = (u32)filter;
}

/* Fonte: alfa de 4 bits -> branco com alfa (o texto e colorido pela cor do vertice). */
static void make_font_texture(GSTEXTURE *t, const unsigned char *packed, int w, int h)
{
    int i, n = w * h;
    u32 *px = (u32 *)memalign(128, (size_t)n * 4);
    for (i = 0; i < n; i++) {
        int a4 = (packed[i >> 1] >> ((i & 1) * 4)) & 15;
        u32 a = (u32)(a4 * 128 / 15);
        px[i] = 0x00FFFFFFu | (a << 24);
    }
    init_texture(t, px, w, h, GS_FILTER_NEAREST);
}

/* Atlas de 8 bits com paleta (T8 + CLUT de 256 cores CT32). Ocupa 1/4 da VRAM de uma
 * textura CT32. O GS le a CLUT de 256 cores no modo CSM1 com as entradas 8-15 e 16-23 de cada
 * bloco de 32 trocadas; por isso a paleta e "rotacionada" aqui (igual ao carregador PNG do gsKit). */
static void make_t8_texture(GSTEXTURE *t, const unsigned char *bin, int w, int h, int filter)
{
    const unsigned char *pal = bin, *idx = bin + 1024;
    u32 *clut = (u32 *)memalign(128, 256 * 4);
    u8 *px = (u8 *)memalign(128, (size_t)w * h);
    int i;
    for (i = 0; i < 256; i++) {
        const unsigned char *c = pal + i * 4;
        u32 a = ((u32)c[3] + 1) >> 1; /* alfa do GS: 0x80 = opaco */
        int j = ((i & 0x18) == 0x08) ? i + 8 : (((i & 0x18) == 0x10) ? i - 8 : i);
        clut[j] = (u32)c[0] | ((u32)c[1] << 8) | ((u32)c[2] << 16) | (a << 24);
    }
    memcpy(px, idx, (size_t)w * h);
    FlushCache(0);
    memset(t, 0, sizeof(*t));
    t->Width = (u32)w;
    t->Height = (u32)h;
    t->PSM = GS_PSM_T8;
    t->Mem = (u32 *)px;
    t->Clut = clut;
    t->ClutPSM = GS_PSM_CT32;
    t->Filter = (u32)filter;
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

/* ---------------- audio ----------------
 * libsd (driver do SPU2) vem da ROM; se faltar, usa a copia embutida. O audsrv.irx do
 * PS2SDK vai embutido no ELF. O mixer roda no EE e enche o buffer do audsrv a cada quadro. */
extern unsigned char irx_audsrv[], irx_audsrv_end[];
extern unsigned char irx_libsd[], irx_libsd_end[];
static int audio_ok;

static void audio_init(void)
{
    int ret = 0;
    struct audsrv_fmt_t fmt;
    if (SifLoadModule("rom0:LIBSD", 0, NULL) < 0)
        SifExecModuleBuffer(irx_libsd, (u32)(irx_libsd_end - irx_libsd), 0, NULL, &ret);
    if (SifExecModuleBuffer(irx_audsrv, (u32)(irx_audsrv_end - irx_audsrv), 0, NULL, &ret) < 0) return;
    if (audsrv_init() != 0) return;
    fmt.bits = 16;
    fmt.freq = AUDIO_RATE;
    fmt.channels = 1;
    if (audsrv_set_format(&fmt) != 0) return;
    audsrv_set_volume(MAX_VOLUME);
    audio_ok = 1;
}

static int ps2_audio_available(void) { return audsrv_available(); }
static int ps2_audio_play(const char *buf, int bytes) { return audsrv_play_audio(buf, bytes); }

static void audio_update(void)
{
    if (audio_ok) audio_feed(ps2_audio_available, ps2_audio_play);
}

static void open_pads(void)
{
    int port;
    padInit(0);
    for (port = 0; port < 2; port++)
        pad_open[port] = padPortOpen(port, 0, pad_buf[port]) != 0;
}

unsigned plat_buttons(int port)
{
    unsigned out = 0;
    struct padButtonStatus st;
    int state;
    u32 b;
    if (port < 0 || port > 1 || !pad_open[port]) return 0;
    state = padGetState(port, 0);
    if (state != PAD_STATE_STABLE && state != PAD_STATE_FINDCTP1) return 0;
    if (padRead(port, 0, &st) == 0) return 0;
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
    audio_init();

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

    /* VRAM: 2 framebuffers (~2,2 MB) + fonte 256 KB + sprites 256 KB + letras 64 KB
     * + dados 512 KB + abertura 280 KB + paletas = ~3,6 MB, dentro dos 4 MB do GS. */
    make_font_texture(&tex[ATLAS_FONT], font_small_pixels, 256, 256);
    make_t8_texture(&tex[ATLAS_SPRITES], asset_sprites, SPRITES_W, SPRITES_H, GS_FILTER_LINEAR);
    make_t8_texture(&tex[ATLAS_BUBBLE], asset_bubble, BUBBLE_W, BUBBLE_H_TEX, GS_FILTER_LINEAR);
    make_t8_texture(&tex[ATLAS_DICE], asset_dice, DICE_W, DICE_H, GS_FILTER_LINEAR);
    make_t8_texture(&tex[ATLAS_SPLASH], asset_splash, SPLASH_W, SPLASH_H, GS_FILTER_NEAREST);
    return 0;
}

void plat_frame_begin(Color clear)
{
    gsKit_clear(gs, gs_color(clear | 0xFF));
}

void plat_frame_end(void)
{
    audio_update();
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

void plat_sprite(int atlas, float x, float y, float w, float h,
                 float u, float v, float uw, float vh, Color c)
{
    GSTEXTURE *t = &tex[(atlas >= 0 && atlas < NUM_ATLAS) ? atlas : 0];
    if ((c & 0xFF) == 0) return;
    gsKit_TexManager_bind(gs, t);
    gsKit_prim_sprite_texture(gs, t, x, y + yoff, u, v, x + w, y + h + yoff, u + uw, v + vh, 0, gs_tex_color(c));
}
