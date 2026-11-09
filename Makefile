# Horta de Dados PS2 - build do jogo (requer PS2SDK + gsKit, ex.: imagem docker ps2dev/ps2dev)
#   make          -> horta_de_dados.elf
#   make iso      -> horta_de_dados.iso (precisa de mkisofs/genisoimage)
EE_BIN = horta_de_dados.elf
EE_OBJS = src/main_ps2.o src/plat_ps2.o src/ui.o src/gfx.o src/ai.o src/game.o src/lang.o src/audio.o src/font_data.o src/assets.o src/irx.o
EE_INCS = -I$(GSKIT)/include -Isrc
EE_LDFLAGS = -L$(GSKIT)/lib
EE_LIBS = -lgskit_toolkit -lgskit -ldmakit -laudsrv -lpad -lpatches -lm
EE_CFLAGS += -std=gnu99
EE_CFLAGS += -DAUDSRV_IRX_PATH='"$(PS2SDK)/iop/irx/audsrv.irx"' -DLIBSD_IRX_PATH='"$(PS2SDK)/iop/irx/libsd.irx"'
MKISOFS ?= mkisofs

all: $(EE_BIN)

iso: $(EE_BIN)
	rm -rf iso_root && mkdir -p iso_root
	cp $(EE_BIN) iso_root/SLUS_999.99
	printf 'BOOT2 = cdrom0:\\SLUS_999.99;1\r\nVER = 1.00\r\nVMODE = NTSC\r\n' > iso_root/SYSTEM.CNF
	$(MKISOFS) -quiet -iso-level 1 -V HORTA_DE_DADOS -o horta_de_dados.iso iso_root

clean:
	rm -f $(EE_BIN) $(EE_OBJS) horta_de_dados.iso
	rm -rf iso_root

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
