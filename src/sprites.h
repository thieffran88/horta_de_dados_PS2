/* Gerado por tools/genassets.py - nao edite. */
#ifndef HORTA_SPRITES_H
#define HORTA_SPRITES_H

typedef struct { short x, y, w, h; } SprRect;

enum {
    SPR_GRID,
    SPR_SIGN1,
    SPR_SIGN2,
    SPR_SIGN3,
    SPR_SIGN4,
    SPR_SIGN5,
    SPR_SIGN6,
    SPR_RIBBON,
    SPR_RIBBON_OFF,
    SPR_RIBBON_S,
    SPR_PIG,
    SPR_PIGLET,
    SPR_PIGLET_L,
    SPR_ARROW,
    SPR_PIGBOARD,
    SPR_CRATE1,
    SPR_CRATE2,
    SPR_CRATE3,
    SPR_CARROT,
    SPR_CARROT_S,
    SPR_TOMATO,
    SPR_TOMATO_S,
    SPR_LETTUCE,
    SPR_LETTUCE_S,
    SPR_FLOWER,
    SPR_GRASS,
    SPR_GRASS2,
    SPR_POW_VALUE,
    SPR_POW_COLOR,
    SPR_M_PIG,
    SPR_M_CARROT,
    SPR_M_LETTUCE,
    SPR_M_TOMATO,
    SPR_I_DICE,
    SPR_I_TROPHY,
    SPR_I_GEAR,
    SPR_I_BOOK,
    SPR_TROPHY_L,
    SPR_BTN_Y_L,
    SPR_BTN_Y_M,
    SPR_BTN_Y_R,
    SPR_BTN_C_L,
    SPR_BTN_C_M,
    SPR_BTN_C_R,
    SPR_BADGE,
    SPR_LEAF1,
    SPR_LEAF2,
    SPR_FLOWER2,
    SPR_DPAD,
    SPR_XBTN,
    SPR_COUNT
};

static const SprRect SPRITES[SPR_COUNT] = {
    { 2, 2, 344, 164 }, /* GRID */
    { 347, 266, 35, 38 }, /* SIGN1 */
    { 384, 266, 34, 38 }, /* SIGN2 */
    { 420, 266, 34, 38 }, /* SIGN3 */
    { 456, 266, 34, 38 }, /* SIGN4 */
    { 2, 333, 34, 38 }, /* SIGN5 */
    { 38, 333, 34, 38 }, /* SIGN6 */
    { 142, 333, 25, 34 }, /* RIBBON */
    { 169, 333, 25, 34 }, /* RIBBON_OFF */
    { 286, 373, 16, 22 }, /* RIBBON_S */
    { 156, 266, 53, 40 }, /* PIG */
    { 441, 373, 27, 20 }, /* PIGLET */
    { 412, 333, 40, 30 }, /* PIGLET_L */
    { 304, 373, 17, 22 }, /* ARROW */
    { 348, 2, 150, 128 }, /* PIGBOARD */
    { 429, 168, 54, 66 }, /* CRATE1 */
    { 373, 168, 54, 67 }, /* CRATE2 */
    { 2, 266, 54, 65 }, /* CRATE3 */
    { 211, 266, 26, 40 }, /* CARROT */
    { 470, 373, 13, 20 }, /* CARROT_S */
    { 114, 266, 40, 42 }, /* TOMATO */
    { 419, 373, 20, 21 }, /* TOMATO_S */
    { 74, 333, 40, 38 }, /* LETTUCE */
    { 485, 373, 20, 19 }, /* LETTUCE_S */
    { 196, 333, 21, 34 }, /* FLOWER */
    { 454, 333, 43, 30 }, /* GRASS */
    { 247, 373, 37, 24 }, /* GRASS2 */
    { 323, 373, 41, 22 }, /* POW_VALUE */
    { 161, 373, 34, 26 }, /* POW_COLOR */
    { 67, 168, 133, 92 }, /* M_PIG */
    { 2, 168, 63, 96 }, /* M_CARROT */
    { 202, 168, 88, 80 }, /* M_LETTUCE */
    { 292, 168, 79, 80 }, /* M_TOMATO */
    { 2, 373, 32, 30 }, /* I_DICE */
    { 36, 373, 37, 30 }, /* I_TROPHY */
    { 75, 373, 34, 30 }, /* I_GEAR */
    { 111, 373, 48, 30 }, /* I_BOOK */
    { 58, 266, 54, 44 }, /* TROPHY_L */
    { 239, 266, 22, 40 }, /* BTN_Y_L */
    { 263, 266, 4, 40 }, /* BTN_Y_M */
    { 269, 266, 22, 40 }, /* BTN_Y_R */
    { 293, 266, 22, 40 }, /* BTN_C_L */
    { 317, 266, 4, 40 }, /* BTN_C_M */
    { 323, 266, 22, 40 }, /* BTN_C_R */
    { 219, 333, 191, 34 }, /* BADGE */
    { 197, 373, 22, 26 }, /* LEAF1 */
    { 221, 373, 24, 26 }, /* LEAF2 */
    { 116, 333, 24, 38 }, /* FLOWER2 */
    { 366, 373, 25, 22 }, /* DPAD */
    { 393, 373, 24, 22 }, /* XBTN */
};

/* letras bolha: A-Z e 0-9, altura 36 */
#define BUBBLE_H 36
static const SprRect BUBBLE[36] = {
    { 2, 2, 34, 36 }, /* A */
    { 38, 2, 29, 36 }, /* B */
    { 69, 2, 30, 36 }, /* C */
    { 101, 2, 30, 36 }, /* D */
    { 133, 2, 27, 36 }, /* E */
    { 162, 2, 27, 36 }, /* F */
    { 191, 2, 31, 36 }, /* G */
    { 224, 2, 31, 36 }, /* H */
    { 257, 2, 16, 36 }, /* I */
    { 275, 2, 28, 36 }, /* J */
    { 305, 2, 29, 36 }, /* K */
    { 336, 2, 25, 36 }, /* L */
    { 363, 2, 33, 36 }, /* M */
    { 398, 2, 29, 36 }, /* N */
    { 429, 2, 29, 36 }, /* O */
    { 460, 2, 26, 36 }, /* P */
    { 2, 40, 30, 36 }, /* Q */
    { 34, 40, 28, 36 }, /* R */
    { 64, 40, 26, 36 }, /* S */
    { 92, 40, 28, 36 }, /* T */
    { 122, 40, 33, 36 }, /* U */
    { 157, 40, 31, 36 }, /* V */
    { 190, 40, 39, 36 }, /* W */
    { 231, 40, 31, 36 }, /* X */
    { 264, 40, 29, 36 }, /* Y */
    { 295, 40, 30, 36 }, /* Z */
    { 327, 40, 29, 36 }, /* 0 */
    { 358, 40, 22, 36 }, /* 1 */
    { 382, 40, 28, 36 }, /* 2 */
    { 412, 40, 27, 36 }, /* 3 */
    { 441, 40, 31, 36 }, /* 4 */
    { 474, 40, 27, 36 }, /* 5 */
    { 2, 78, 28, 36 }, /* 6 */
    { 32, 78, 29, 36 }, /* 7 */
    { 63, 78, 29, 36 }, /* 8 */
    { 94, 78, 28, 36 }, /* 9 */
};

/* centros dos 42 circulos do tabuleiro do porco (relativos ao sprite) */
static const float PIG_CIRC[42][2] = {
    { 18.4f, 18.0f },
    { 33.5f, 18.1f },
    { 48.5f, 18.0f },
    { 63.6f, 18.0f },
    { 78.6f, 18.0f },
    { 93.7f, 18.0f },
    { 18.5f, 33.7f },
    { 33.5f, 33.7f },
    { 48.6f, 33.7f },
    { 63.6f, 33.7f },
    { 78.6f, 33.7f },
    { 93.7f, 33.7f },
    { 18.4f, 49.4f },
    { 33.5f, 49.4f },
    { 48.5f, 49.3f },
    { 63.6f, 49.4f },
    { 78.6f, 49.4f },
    { 93.7f, 49.3f },
    { 18.4f, 64.9f },
    { 33.5f, 64.9f },
    { 48.5f, 64.9f },
    { 63.6f, 64.9f },
    { 78.6f, 64.9f },
    { 93.7f, 64.9f },
    { 18.4f, 80.6f },
    { 33.5f, 80.6f },
    { 48.5f, 80.6f },
    { 63.6f, 80.6f },
    { 78.6f, 80.5f },
    { 93.7f, 80.6f },
    { 18.4f, 96.1f },
    { 33.5f, 96.1f },
    { 48.5f, 96.1f },
    { 63.6f, 96.1f },
    { 78.6f, 96.0f },
    { 93.7f, 96.0f },
    { 18.4f, 111.1f },
    { 33.5f, 111.1f },
    { 48.6f, 111.1f },
    { 63.7f, 111.1f },
    { 78.7f, 111.0f },
    { 93.7f, 111.0f },
};
#define PIG_CIRC_R 6.1f

/* centros das caixas de poder (*) do tabuleiro do porco */
static const float PIG_BOX[7][2] = {
    { 115.2f, 15.9f },
    { 115.2f, 31.4f },
    { 115.3f, 47.0f },
    { 115.2f, 62.4f },
    { 115.2f, 78.3f },
    { 115.2f, 94.1f },
    { 115.2f, 109.9f },
};

/* linhas da grade da horta (relativas ao sprite GRID) */
static const short GRID_X[7] = { 8, 62, 116, 170, 224, 278, 332 };
static const short GRID_Y[4] = { 2, 54, 106, 158 };

/* centros dos 6 circulos de cada caixote do mercado */
static const float CRATE_CIRC[6][2] = {
    { 19.9f, 19.6f },
    { 33.2f, 19.6f },
    { 19.9f, 32.2f },
    { 33.2f, 32.3f },
    { 19.8f, 45.0f },
    { 33.2f, 45.0f },
};

#endif
