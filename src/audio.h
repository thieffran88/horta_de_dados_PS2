/* Horta de Dados PS2 - mixer de audio (musica em loop + efeitos simultaneos). */
#ifndef HORTA_AUDIO_H
#define HORTA_AUDIO_H
#include "sounds.h"

extern const unsigned char asset_audio[];

void audio_music(int snd);          /* troca a musica (com fade); -1 = silencio */
void audio_sfx(int snd);            /* toca um efeito */
void audio_mix(short *out, int n);  /* gera n amostras mono de 16 bits a AUDIO_RATE */

/* Alimenta o driver de saida sem perder amostras: envia apenas o que cabe
 * (available() = bytes livres) e guarda a sobra para a proxima chamada.
 * play() retorna quantos bytes aceitou. Usado pelo PS2 (audsrv) e pelo teste no PC. */
typedef int (*audio_avail_fn)(void);
typedef int (*audio_play_fn)(const char *buf, int bytes);
void audio_feed(audio_avail_fn available, audio_play_fn play);

#endif
