/* Horta de Dados PS2 - mixer de audio (musica em loop + efeitos simultaneos). */
#ifndef HORTA_AUDIO_H
#define HORTA_AUDIO_H
#include "sounds.h"

extern const unsigned char asset_audio[];

void audio_music(int snd);          /* troca a musica (com fade); -1 = silencio */
void audio_sfx(int snd);            /* toca um efeito */
void audio_mix(short *out, int n);  /* gera n amostras mono de 16 bits a AUDIO_RATE */

#endif
