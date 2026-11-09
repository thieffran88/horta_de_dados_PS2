/* Modulos do IOP embutidos (so no build do PS2). Os caminhos vem do Makefile:
 * -DAUDSRV_IRX_PATH="$(PS2SDK)/iop/irx/audsrv.irx" e -DLIBSD_IRX_PATH=... */
    .section .rodata
    .balign 16
    .globl irx_audsrv
    .globl irx_audsrv_end
irx_audsrv:
    .incbin AUDSRV_IRX_PATH
irx_audsrv_end:
    .balign 16
    .globl irx_libsd
    .globl irx_libsd_end
irx_libsd:
    .incbin LIBSD_IRX_PATH
irx_libsd_end:
    .balign 16
