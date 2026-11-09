/* Camada de plataforma: PS2 (gsKit + libpad) ou host (render por software). */
#ifndef HORTA_PLAT_H
#define HORTA_PLAT_H

#define SCREEN_W 640
#define SCREEN_H 448

/* Cores como 0xRRGGBBAA */
typedef unsigned int Color;
#define CRGBA(r, g, b, a) (((unsigned)(r) << 24) | ((unsigned)(g) << 16) | ((unsigned)(b) << 8) | (unsigned)(a))
#define CRGB(r, g, b) CRGBA(r, g, b, 255)

/* Botoes logicos */
enum {
    BTN_UP = 1 << 0, BTN_DOWN = 1 << 1, BTN_LEFT = 1 << 2, BTN_RIGHT = 1 << 3,
    BTN_CROSS = 1 << 4, BTN_CIRCLE = 1 << 5, BTN_SQUARE = 1 << 6, BTN_TRIANGLE = 1 << 7,
    BTN_L1 = 1 << 8, BTN_R1 = 1 << 9, BTN_L2 = 1 << 10, BTN_R2 = 1 << 11,
    BTN_START = 1 << 12, BTN_SELECT = 1 << 13
};

/* Texturas: fonte pequena (alfa de 4 bits) e atlas de 8 bits com paleta:
 * sprites 512x512, letras "bolha" 512x128, dados rolando 1024x512, arte de abertura 640x448. */
enum { ATLAS_FONT = 0, ATLAS_SPRITES, ATLAS_BUBBLE, ATLAS_DICE, ATLAS_SPLASH, NUM_ATLAS };
#define SPRITES_W 512
#define SPRITES_H 512
#define BUBBLE_W 512
#define BUBBLE_H_TEX 128
#define DICE_W 1024
#define DICE_H 512
#define SPLASH_W 640
#define SPLASH_H 448

/* Formato dos atlas embutidos: 256 cores RGBA de paleta (alfa 0..255) + indices de 8 bits. */
extern const unsigned char asset_sprites[];
extern const unsigned char asset_bubble[];
extern const unsigned char asset_dice[];
extern const unsigned char asset_splash[];

int  plat_init(void);
void plat_frame_begin(Color clear);
void plat_frame_end(void);
unsigned plat_buttons(void);          /* botoes pressionados agora (todas as portas) */
unsigned plat_seed(void);

void plat_rect(float x, float y, float w, float h, Color c);
void plat_tri(float x1, float y1, float x2, float y2, float x3, float y3, Color c);
/* Desenha uma regiao (u,v,uw,vh) de um atlas, modulada pela cor c
 * (branco = cores originais; na fonte, c e a cor do texto). */
void plat_sprite(int atlas, float x, float y, float w, float h,
                float u, float v, float uw, float vh, Color c);

#endif
