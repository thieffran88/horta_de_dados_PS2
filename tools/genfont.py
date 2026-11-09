#!/usr/bin/env python3
"""Gera src/font_data.c a partir de uma fonte TrueType (padrao: DejaVu Sans Bold).

Dois atlas de alfa de 4 bits por pixel:
  - pequeno: Latin-1 (32..255), celulas 16x18, atlas 256x256
  - grande : ASCII  (32..127),  celulas 32x40, atlas 512x256
Uso: python3 tools/genfont.py [arquivo.ttf]
"""
import sys
from PIL import Image, ImageDraw, ImageFont

TTF = sys.argv[1] if len(sys.argv) > 1 else "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"

def build(name, size, cw, ch, aw, ah, first, last):
    font = ImageFont.truetype(TTF, size)
    img = Image.new("L", (aw, ah), 0)
    d = ImageDraw.Draw(img)
    cols = aw // cw
    asc, desc = font.getmetrics()
    yoff = (ch - (asc + desc)) // 2
    widths = []
    for i, code in enumerate(range(first, last + 1)):
        cx, cy = (i % cols) * cw, (i // cols) * ch
        assert cy + ch <= ah, name
        c = chr(code)
        adv = int(round(font.getlength(c))) if code >= 32 and not (127 <= code < 160) else 0
        if adv > 0:
            d.text((cx + 1, cy + yoff), c, font=font, fill=255)
        widths.append(min(adv, cw) if adv else 0)
    packed = bytearray()
    px = img.load()
    for y in range(ah):
        for x in range(0, aw, 2):
            a, b = px[x, y] >> 4, px[x + 1, y] >> 4
            packed.append(a | (b << 4))
    out = []
    out.append(f"const unsigned char {name}_widths[{len(widths)}] = {{" + ",".join(map(str, widths)) + "};\n")
    out.append(f"const unsigned char {name}_pixels[{len(packed)}] __attribute__((aligned(16))) = {{\n")
    for i in range(0, len(packed), 32):
        out.append(",".join(f"0x{v:02x}" for v in packed[i:i + 32]) + ",\n")
    out.append("};\n")
    return "".join(out)

src = ["/* Gerado por tools/genfont.py - nao edite. Glifos: DejaVu Sans Bold (licenca Bitstream Vera / DejaVu). */\n"]
src.append(build("font_small", 14, 16, 18, 256, 256, 32, 255))
src.append(build("font_big", 30, 32, 40, 512, 256, 32, 127))
open("src/font_data.c", "w").write("".join(src))
print("ok")
