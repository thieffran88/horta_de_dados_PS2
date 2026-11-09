/* Atlas de sprites embutidos no executavel (gerados por tools/genassets.py). */
    .section .rodata
    .balign 16
    .globl asset_sprites
asset_sprites:
    .incbin "assets/sprites.bin"
    .balign 16
    .globl asset_bubble
asset_bubble:
    .incbin "assets/bubble.bin"
    .balign 16
    .globl asset_dice
asset_dice:
    .incbin "assets/dice.bin"
    .balign 16
    .globl asset_splash
asset_splash:
    .incbin "assets/splash.bin"
    .balign 16
    .globl asset_audio
asset_audio:
    .incbin "assets/audio.bin"
    .balign 16
#if defined(__linux__) && defined(__ELF__)
    .section .note.GNU-stack,"",@progbits
#endif
