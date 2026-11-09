#!/usr/bin/env python3
"""Gera os atlas de sprites do jogo a partir das folhas de arte em art/.

Saidas:
  assets/sprites.bin  - atlas 512x512, 8 bits por pixel + paleta RGBA (256 cores)
  assets/bubble.bin   - atlas 512x128 com as letras "bolha" (A-Z, 0-9), mesmo formato
  src/sprites.h       - retangulos de cada sprite e posicoes dos circulos (porco/mercado)
  assets/preview_*.png - visualizacao dos atlas (nao usada no jogo)

Formato .bin: 1024 bytes de paleta (R,G,B,A x 256) seguidos de largura*altura indices.
Uso: python3 tools/genassets.py   (requer pillow, numpy, scipy)
"""
import os
import numpy as np
from PIL import Image
from scipy import ndimage as nd

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
S1 = np.asarray(Image.open(os.path.join(ROOT, "art/sheet1.png")).convert("RGB"))
S2 = np.asarray(Image.open(os.path.join(ROOT, "art/sheet2.png")).convert("RGB"))
SB = np.asarray(Image.open(os.path.join(ROOT, "art/buttons.png")).convert("RGB"))


def isolate(sheet, box, keep="largest", white=232):
    """Recorta box, remove o fundo branco ligado a borda e (opcional) objetos vizinhos."""
    x0, y0, x1, y1 = box
    c = sheet[y0:y1, x0:x1].astype(np.uint8)
    fg = c.min(axis=2) < white
    lab, n = nd.label(~fg)
    border = set(np.unique(np.concatenate([lab[0], lab[-1], lab[:, 0], lab[:, -1]]))) - {0}
    bg = np.isin(lab, list(border))
    obj = ~bg
    if keep == "largest":
        l2, n2 = nd.label(nd.binary_dilation(obj, iterations=2))
        if n2 > 1:
            sizes = nd.sum(obj, l2, range(1, n2 + 1))
            big = sizes.max()
            ok = [i + 1 for i, s in enumerate(sizes) if s >= big * 0.12]
            obj &= np.isin(l2, ok)
    alpha = obj.astype(np.uint8) * 255
    # suaviza a borda: pixels claros na fronteira ficam semitransparentes
    edge = obj & ~nd.binary_erosion(obj)
    light = c.min(axis=2).astype(int)
    soft = np.clip((255 - light) * 255 // max(1, 255 - 150), 0, 255).astype(np.uint8)
    alpha[edge] = np.minimum(alpha[edge], np.maximum(soft[edge], 90))
    rgba = np.dstack([c, alpha])
    ys, xs = np.nonzero(alpha)
    t = (xs.min(), ys.min(), xs.max() + 1, ys.max() + 1)
    return rgba[t[1]:t[3], t[0]:t[2]], (x0 + t[0], y0 + t[1])


def resize(rgba, w, h):
    a = rgba.astype(np.float32) / 255.0
    pre = a.copy()
    pre[..., :3] *= a[..., 3:4]
    chans = [np.asarray(Image.fromarray((pre[..., i] * 255).astype(np.uint8)).resize((w, h), Image.LANCZOS),
                        dtype=np.float32) / 255.0 for i in range(4)]
    out = np.dstack(chans)
    al = np.clip(out[..., 3:4], 1e-4, 1)
    out[..., :3] = np.clip(out[..., :3] / al, 0, 1)
    out[..., 3] = np.where(out[..., 3] < 0.03, 0, out[..., 3])
    return (out * 255 + 0.5).astype(np.uint8)


def fit(rgba, w=None, h=None):
    sh, sw = rgba.shape[:2]
    if w and h:
        return resize(rgba, w, h)
    if h:
        return resize(rgba, max(1, round(sw * h / sh)), h)
    return resize(rgba, w, max(1, round(sh * w / sw)))


def grey(rgba):
    out = rgba.copy()
    g = (rgba[..., 0] * 0.3 + rgba[..., 1] * 0.59 + rgba[..., 2] * 0.11)
    g = g * 0.7 + 60
    for i in range(3):
        out[..., i] = np.clip(g, 0, 255)
    return out


def white_blobs(sheet, box, min_area=150):
    """Centros dos circulos brancos (relativos ao canto do sprite isolado)."""
    rgba, (ox, oy) = isolate(sheet, box)
    c = rgba[..., :3].astype(int)
    m = (c.min(axis=2) > 235) & (rgba[..., 3] > 0)
    lab, n = nd.label(m)
    cs = []
    for i in range(1, n + 1):
        ys, xs = np.nonzero(lab == i)
        if len(xs) >= min_area:
            cs.append((xs.mean(), ys.mean(), np.sqrt(len(xs) / np.pi)))
    return rgba, cs


# ---------------------------------------------------------------- sprites
SPR = []  # (nome, rgba)


def add(name, rgba):
    SPR.append((name, rgba))


def three_slice(name, rgba, h, cap):
    r = fit(rgba, h=h)
    add(name + "_L", r[:, :cap])
    add(name + "_M", r[:, r.shape[1] // 2 - 2:r.shape[1] // 2 + 2])
    add(name + "_R", r[:, -cap:])


grid, _ = isolate(S2, (1, 34, 602, 338))
GRID = fit(grid, 344, 164)
add("GRID", GRID)


def lines(prof, n):
    """n posicoes de pico (linhas da grade) num perfil 1D."""
    prof = prof.astype(float)
    picks = []
    for _ in range(n):
        i = int(prof.argmax())
        picks.append(i)
        prof[max(0, i - 8):i + 9] = -1
    return sorted(picks)


g = GRID.astype(int)
ln = (g[..., 1] > 90) & (g[..., 0] < 140) & (g[..., 2] < 110) & (g[..., 3] > 128)
H_, W_ = ln.shape
GRID_X = lines(ln[int(H_ * 0.3):int(H_ * 0.7)].sum(axis=0), 7)
GRID_Y = lines(ln[:, int(W_ * 0.3):int(W_ * 0.7)].sum(axis=1), 4)
# as bordas externas sao grossas: extrapola a partir das linhas internas
sx_ = (GRID_X[5] - GRID_X[1]) / 4.0
GRID_X = [round(GRID_X[1] - sx_)] + GRID_X[1:6] + [round(GRID_X[5] + sx_)]
sy_ = (GRID_Y[2] - GRID_Y[1])
GRID_Y = [round(GRID_Y[1] - sy_)] + GRID_Y[1:3] + [round(GRID_Y[2] + sy_)]
print("grade x", GRID_X, "y", GRID_Y)

for i, x in enumerate([622, 775, 933, 1085, 1237, 1389]):
    s, _ = isolate(S2, (x, 15, x + 146, 180))
    add("SIGN%d" % (i + 1), fit(s, h=38))

ribbon, _ = isolate(S2, (1150, 223, 1250, 358))
add("RIBBON", fit(ribbon, h=34))
add("RIBBON_OFF", grey(fit(ribbon, h=34)))
add("RIBBON_S", fit(ribbon, h=22))

pig_big, _ = isolate(S2, (625, 178, 892, 372))
add("PIG", fit(pig_big, h=40))
pig_small, _ = isolate(S2, (900, 268, 1022, 364))
add("PIGLET", fit(pig_small, h=20))
add("PIGLET_L", fit(pig_small, h=30))
arrow, _ = isolate(S2, (1032, 239, 1122, 354))
add("ARROW", fit(arrow, h=22))

# tabuleiro do porco + centros dos circulos
board, blobs = white_blobs(S2, (9, 390, 492, 803))
BW, BH = 150, 128
sx, sy = BW / board.shape[1], BH / board.shape[0]
add("PIGBOARD", fit(board, BW, BH))
circ = sorted([(x * sx, y * sy, r * sx) for (x, y, r) in blobs if r * sx > 4.0 and x < board.shape[1] * 0.68], key=lambda c: (round(c[1] / 8), c[0]))
assert len(circ) == 42, len(circ)
boxes = sorted([(x * sx, y * sy) for (x, y, r) in blobs if board.shape[1] * 0.68 <= x < board.shape[1] * 0.84 and r * sx > 3.0], key=lambda c: c[1])
assert len(boxes) == 7, len(boxes)
rows = [sorted(circ[i * 6:(i + 1) * 6]) for i in range(7)]
PIG_CIRCLES = [c for r in rows for c in r]

# caixotes do mercado + circulos
CRATE_CIRC = []
for i, box in enumerate([(500, 550, 704, 800), (705, 550, 907, 801), (908, 550, 1114, 801)]):
    cr, bl = white_blobs(S2, box)
    W = 54
    s = W / cr.shape[1]
    H = round(cr.shape[0] * s)
    add("CRATE%d" % (i + 1), fit(cr, W, H))
    cc = sorted([(x * s, y * s) for (x, y, r) in bl if r * s > 3], key=lambda c: (round(c[1] / 6), c[0]))
    assert len(cc) == 6, len(cc)
    CRATE_CIRC = cc  # mesma disposicao nos tres

for name, box in [("CARROT", (1126, 553, 1221, 696)), ("TOMATO", (1219, 573, 1323, 683)),
                  ("LETTUCE", (1326, 567, 1449, 683))]:
    v, _ = isolate(S2, box)
    add(name, fit(v, h=40) if name == "CARROT" else fit(v, w=40))
    add(name + "_S", fit(v, h=20) if name == "CARROT" else fit(v, w=20))

flower, _ = isolate(S2, (1453, 580, 1528, 697))
add("FLOWER", fit(flower, h=34))
g1, _ = isolate(S2, (1154, 686, 1317, 803))
add("GRASS", fit(g1, h=30))
g2, _ = isolate(S2, (1340, 702, 1488, 802))
add("GRASS2", fit(g2, h=24))

pm, _ = isolate(S2, (71, 858, 254, 959))
add("POW_VALUE", fit(pm, h=22))
sw, _ = isolate(S2, (336, 842, 501, 969))
add("POW_COLOR", fit(sw, h=26))

# folha 1: mascotes, icones, botoes
for name, box, h in [("M_PIG", (110, 225, 418, 440), 92), ("M_CARROT", (21, 371, 198, 640), 96),
                     ("M_LETTUCE", (198, 440, 395, 620), 80), ("M_TOMATO", (395, 440, 572, 620), 80)]:
    m, _ = isolate(S1, box)
    add(name, fit(m, h=h))
for name, box in [("I_DICE", (600, 228, 742, 338)), ("I_TROPHY", (598, 340, 742, 452)),
                  ("I_GEAR", (600, 452, 740, 566)), ("I_BOOK", (591, 565, 752, 668))]:
    m, _ = isolate(S1, box)
    add(name, fit(m, h=30))
tro, _ = isolate(S1, (598, 340, 742, 452))
add("TROPHY_L", fit(tro, h=44))

yb, _ = isolate(S1, (770, 253, 1304, 377), keep="all")
three_slice("BTN_Y", yb, 40, 22)
cb, _ = isolate(S1, (774, 378, 1309, 474), keep="all")
three_slice("BTN_C", cb, 40, 22)

badge, _ = isolate(S1, (850, 13, 1386, 114), keep="all")
add("BADGE", fit(badge, h=34))

for i, box in enumerate([(1320, 239, 1412, 345), (1418, 250, 1503, 340)]):
    l, _ = isolate(S1, box)
    add("LEAF%d" % (i + 1), fit(l, h=26))
fl2, _ = isolate(S1, (35, 615, 130, 763))
add("FLOWER2", fit(fl2, h=38))

dpad, _ = isolate(S1, (604, 692, 682, 760))
add("DPAD", fit(dpad, h=22))
xb, _ = isolate(S1, (1166, 692, 1240, 760))
add("XBTN", fit(xb, h=22))

# icones dos botoes do controle (art/buttons.png); o logotipo do console nao e usado
for name, box in [("B_L1", (50, 96, 281, 262)), ("B_L2", (462, 29, 675, 262)), ("B_R2", (860, 29, 1068, 262)),
                  ("B_R1", (1242, 95, 1483, 262)), ("B_UP", (143, 362, 258, 496)), ("B_TRI", (564, 337, 686, 469)),
                  ("B_LEFT", (46, 464, 170, 596)), ("B_RIGHT", (231, 464, 354, 596)), ("B_SQR", (449, 461, 573, 596)),
                  ("B_CIR", (682, 461, 806, 596)), ("B_DOWN", (139, 565, 261, 706)), ("B_CRO", (562, 587, 691, 727)),
                  ("B_SEL", (274, 835, 419, 931)), ("B_START", (1104, 832, 1247, 941)), ("B_STICK", (903, 413, 1134, 698))]:
    b, _ = isolate(SB, box)
    add(name, fit(b, h=20))

# brilho amarelo do dado "caindo" (ultima coluna das folhas de dados)
DO = np.asarray(Image.open(os.path.join(ROOT, "art/dice_orange.png")).convert("RGB"))
_by = DO[0:120, 1340:1447].astype(int)
_m = (_by[..., 0] > 200) & (_by[..., 1] > 180) & (_by[..., 2] < 120)
_ys, _xs = np.nonzero(_m)
burst, _ = isolate(DO, (1340 + _xs.min() - 3, _ys.min() - 3, 1340 + _xs.max() + 4, _ys.max() + 4), keep="all")
add("BURST", fit(burst, h=18))

# ---------------------------------------------------------------- empacotamento


def pack(sprites, W, H, pad=2):
    order = sorted(range(len(sprites)), key=lambda i: -sprites[i][1].shape[0])
    atlas = np.zeros((H, W, 4), np.uint8)
    rects = [None] * len(sprites)
    x = y = shelf = 0
    for i in order:
        h, w = sprites[i][1].shape[:2]
        if x + w + pad > W:
            x, y, shelf = 0, y + shelf + pad, 0
        assert y + h + pad <= H, "atlas cheio em %s" % sprites[i][0]
        atlas[y + pad:y + pad + h, x + pad:x + pad + w] = sprites[i][1]
        rects[i] = (x + pad, y + pad, w, h)
        x += w + pad
        shelf = max(shelf, h)
    return atlas, rects


def write_bin(atlas, path):
    # pixels transparentes recebem a cor do pixel visivel mais proximo: o filtro
    # bilinear do GS nao e pre-multiplicado e escureceria as bordas
    atlas = atlas.copy()
    clear = atlas[..., 3] == 0
    if clear.any() and (~clear).any():
        _, (iy, ix) = nd.distance_transform_edt(clear, return_indices=True)
        atlas[..., :3] = atlas[iy, ix, :3]
    img = Image.fromarray(atlas, "RGBA")
    q = img.quantize(256, method=Image.Quantize.FASTOCTREE, dither=Image.Dither.NONE)
    pal = q.getpalette(rawmode="RGBA")[:1024]
    pal += [0] * (1024 - len(pal))
    idx = np.asarray(q, dtype=np.uint8)
    # indice com alfa 0 -> garante RGB neutro para nao "sangrar" no filtro bilinear
    with open(path, "wb") as f:
        f.write(bytes(pal))
        f.write(idx.tobytes())
    back = np.array(pal, np.uint8).reshape(256, 4)[idx]
    Image.fromarray(back, "RGBA").save(path.replace(".bin", "").replace("assets/", "assets/preview_") + ".png")


os.makedirs(os.path.join(ROOT, "assets"), exist_ok=True)
atlas, rects = pack(SPR, 512, 512)
write_bin(atlas, os.path.join(ROOT, "assets/sprites.bin"))

# letras bolha
cuts = [23, 111, 191, 272, 353, 425, 496, 577, 659, 705, 778, 856, 920, 1004, 1078, 1154, 1221, 1301, 1376, 1446, 1519]
GL = []
for i in range(20):
    g, _ = isolate(S1, (cuts[i], 785, cuts[i + 1], 892))
    GL.append((chr(65 + i), g))
row2 = [(23, 116), (120, 209), (212, 323), (324, 414), (415, 498), (497, 585),
        (609, 696), (703, 766), (779, 861), (868, 949), (954, 1044), (1050, 1130), (1137, 1222),
        (1233, 1317), (1320, 1407), (1416, 1501)]
chars2 = "UVWXYZ0123456789"
for ch, (a, b) in zip(chars2, row2):
    g, _ = isolate(S1, (a - 2, 900, b + 2, 1012))
    GL.append((ch, g))
GH = 36
GLs = [(c, fit(g, h=GH)) for c, g in GL]
batlas, brects = pack(GLs, 512, 128)
write_bin(batlas, os.path.join(ROOT, "assets/bubble.bin"))

# ---------------------------------------------------------------- dados rolando
def bands(p, thr=3):
    runs, st = [], None
    for i, v in enumerate(p > thr):
        if v and st is None:
            st = i
        if not v and st is not None:
            runs.append((st, i))
            st = None
    if st is not None:
        runs.append((st, len(p)))
    return [r for r in runs if r[1] - r[0] > 40]


DICE_FILES = ["dice_orange", "dice_green", "dice_red"]  # cenoura, alface, tomate
DICE_FRAMES = []   # (nome, rgba)
DICE_META = {}     # (cor, valor, quadro) -> (indice, ox, oy)
DIE_H = 42         # altura do dado parado (com sombra) na tela
for k, fn in enumerate(DICE_FILES):
    D = np.asarray(Image.open(os.path.join(ROOT, "art/%s.png" % fn)).convert("RGB"))
    fg = D.min(axis=2) < 215
    rows = bands(fg.sum(axis=1))
    cols = bands(fg.sum(axis=0))
    while len(cols) < 7:  # colunas grudadas pelos tracos de movimento: divide no vale
        i = max(range(len(cols)), key=lambda j: cols[j][1] - cols[j][0])
        a, b = cols[i]
        prof = fg[:, a:b].sum(axis=0)
        mid = a + 40 + int(prof[40:-40].argmin())
        cols[i:i + 1] = [(a, mid), (mid, b)]
    assert len(rows) == 6 and len(cols) == 7, (fn, rows, cols)
    xc = [0] + [(cols[i][1] + cols[i + 1][0]) // 2 for i in range(6)] + [D.shape[1]]
    yc = [0] + [(rows[i][1] + rows[i + 1][0]) // 2 for i in range(5)] + [D.shape[0]]
    # escala pela altura do dado parado (coluna final, valor 1)
    ref, _ = isolate(D, (xc[6], yc[0], xc[7], yc[1]))
    sc = DIE_H / ref.shape[0]
    for v in range(6):
        for f in range(6):  # quadros 0..4 girando, 5 = parado
            col = f + 1
            box = (xc[col], yc[v], xc[col + 1], yc[v + 1])
            fr, (ox, oy) = isolate(D, box, keep="largest" if f == 5 else "all")
            w, h = max(1, round(fr.shape[1] * sc)), max(1, round(fr.shape[0] * sc))
            ccx, ccy = (cols[col][0] + cols[col][1]) / 2, (rows[v][0] + rows[v][1]) / 2
            DICE_META[(k, v, f)] = (len(DICE_FRAMES), round((ox - ccx) * sc), round((oy - ccy) * sc))
            DICE_FRAMES.append(("%s_%d_%d" % (fn, v + 1, f), resize(fr, w, h)))
    print(fn, "escala %.3f" % sc)

datlas, drects = pack(DICE_FRAMES, 1024, 512)
write_bin(datlas, os.path.join(ROOT, "assets/dice.bin"))

# ---------------------------------------------------------------- arte de abertura
# 640x448 com pixels nao quadrados (tela 4:3): redimensiona sem manter a proporcao
SPL = Image.open(os.path.join(ROOT, "art/splash.png")).convert("RGB").resize((640, 448), Image.LANCZOS)
q = SPL.quantize(256, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.FLOYDSTEINBERG)
pal = q.getpalette()[:768]
pal += [0] * (768 - len(pal))
with open(os.path.join(ROOT, "assets/splash.bin"), "wb") as f:
    for i in range(256):
        f.write(bytes(pal[i * 3:i * 3 + 3] + [255]))
    f.write(np.asarray(q, dtype=np.uint8).tobytes())
q.convert("RGB").save(os.path.join(ROOT, "assets/preview_splash.png"))

# ---------------------------------------------------------------- cabecalho
with open(os.path.join(ROOT, "src/sprites.h"), "w") as f:
    f.write("/* Gerado por tools/genassets.py - nao edite. */\n#ifndef HORTA_SPRITES_H\n#define HORTA_SPRITES_H\n\n")
    f.write("typedef struct { short x, y, w, h; } SprRect;\n\nenum {\n")
    for n, _ in SPR:
        f.write("    SPR_%s,\n" % n)
    f.write("    SPR_COUNT\n};\n\n")
    f.write("static const SprRect SPRITES[SPR_COUNT] = {\n")
    for (n, _), r in zip(SPR, rects):
        f.write("    { %d, %d, %d, %d }, /* %s */\n" % (r + (n,)))
    f.write("};\n\n/* letras bolha: A-Z e 0-9, altura %d */\n#define BUBBLE_H %d\n" % (GH, GH))
    f.write("static const SprRect BUBBLE[36] = {\n")
    order = {c: r for (c, _), r in zip(GLs, brects)}
    for c in "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789":
        f.write("    { %d, %d, %d, %d }, /* %s */\n" % (order[c] + (c,)))
    f.write("};\n\n/* centros dos 42 circulos do tabuleiro do porco (relativos ao sprite) */\n")
    f.write("static const float PIG_CIRC[42][2] = {\n")
    for x, y, r in PIG_CIRCLES:
        f.write("    { %.1ff, %.1ff },\n" % (x, y))
    f.write("};\n#define PIG_CIRC_R %.1ff\n\n" % (sum(c[2] for c in PIG_CIRCLES) / 42))
    f.write("/* centros das caixas de poder (*) do tabuleiro do porco */\nstatic const float PIG_BOX[7][2] = {\n")
    for x, y in boxes:
        f.write("    { %.1ff, %.1ff },\n" % (x, y))
    f.write("};\n\n/* linhas da grade da horta (relativas ao sprite GRID) */\n")
    f.write("static const short GRID_X[7] = { %s };\n" % ", ".join(map(str, GRID_X)))
    f.write("static const short GRID_Y[4] = { %s };\n\n" % ", ".join(map(str, GRID_Y)))
    f.write("/* centros dos 6 circulos de cada caixote do mercado */\nstatic const float CRATE_CIRC[6][2] = {\n")
    for x, y in CRATE_CIRC:
        f.write("    { %.1ff, %.1ff },\n" % (x, y))
    f.write("};\n\n/* animacao dos dados: [cor 0..2][valor-1][quadro 0..5] (5 = parado).\n")
    f.write(" * x,y,w,h no atlas de dados; ox,oy = canto do sprite relativo ao centro do dado. */\n")
    f.write("typedef struct { short x, y, w, h, ox, oy; } DiceFrame;\n#define DICE_FRAMES 6\n#define DICE_STILL 5\n")
    f.write("static const DiceFrame DICE_FR[3][6][6] = {\n")
    for k in range(3):
        f.write("  {\n")
        for v in range(6):
            items = []
            for fr in range(6):
                i, ox, oy = DICE_META[(k, v, fr)]
                items.append("{ %d, %d, %d, %d, %d, %d }" % (drects[i] + (ox, oy)))
            f.write("    { " + ", ".join(items) + " },\n")
        f.write("  },\n")
    f.write("};\n\n#endif\n")
print("sprites:", len(SPR), " letras:", len(GLs))
