#!/bin/sh
# Compila o ELF sem precisar de "make" (util dentro da imagem docker ps2dev/ps2dev,
# que nao traz make instalado):
#   docker run --rm -v "$PWD":/src -w /src ps2dev/ps2dev:latest sh build_ps2.sh
set -e
CC=mips64r5900el-ps2-elf-gcc
CFLAGS="-D_EE -G0 -O2 -Wall -std=gnu99 -I$PS2SDK/ee/include -I$PS2SDK/common/include -I$GSKIT/include -Isrc"
mkdir -p build-ps2
OBJS=""
for f in main_ps2 plat_ps2 ui gfx ai game font_data; do
    $CC $CFLAGS -c src/$f.c -o build-ps2/$f.o
    OBJS="$OBJS build-ps2/$f.o"
done
$CC -T$PS2SDK/ee/startup/linkfile -O2 -o horta_de_dados.elf $OBJS \
    -L$PS2SDK/ee/lib -L$GSKIT/lib -Wl,-zmax-page-size=128 \
    -lgskit_toolkit -lgskit -ldmakit -lpad -lpatches -lm
mips64r5900el-ps2-elf-strip -o horta_de_dados_stripped.elf horta_de_dados.elf
ls -l horta_de_dados.elf horta_de_dados_stripped.elf
