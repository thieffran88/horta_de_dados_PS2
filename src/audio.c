/* Horta de Dados PS2 - mixer de audio em software.
 * A plataforma pede amostras com audio_mix(); aqui ficam a musica (com fade
 * ao trocar) e ate 8 efeitos tocando ao mesmo tempo. */
#include <string.h>
#include "audio.h"

#define MAX_VOICES 8
#define MUSIC_VOL 150   /* de 256 */
#define SFX_VOL 200
#define FADE_STEP 2     /* por bloco de mixagem */

typedef struct { int snd, pos; } Voice;

static Voice voices[MAX_VOICES];
static int music = -1, music_pos, music_next = -1, music_gain = 256, fading;
static unsigned age;
static unsigned voice_age[MAX_VOICES];

static int sample_at(int snd, int pos)
{
    const SoundInfo *s = &SOUNDS[snd];
    const unsigned char *d = asset_audio + s->offset;
    if (s->format == SND_ULAW) return ULAW_TABLE[d[pos]];
    return (short)(d[pos * 2] | (d[pos * 2 + 1] << 8));
}

void audio_music(int snd)
{
    if (snd == music && !fading) return;
    if (snd == music_next && fading) return;
    if (music < 0) {               /* nada tocando: comeca direto */
        music = snd;
        music_pos = 0;
        music_gain = 256;
        fading = 0;
        return;
    }
    music_next = snd;               /* abaixa a atual e depois troca */
    fading = 1;
}

void audio_sfx(int snd)
{
    int i, best = 0;
    if (snd < 0 || snd >= NUM_SOUNDS) return;
    for (i = 0; i < MAX_VOICES; i++) {
        if (voices[i].snd < 0) { best = i; break; }
        if (voice_age[i] < voice_age[best]) best = i; /* reaproveita a mais antiga */
    }
    voices[best].snd = snd;
    voices[best].pos = 0;
    voice_age[best] = ++age;
}

void audio_mix(short *out, int n)
{
    static int init;
    int i, k;
    if (!init) {
        for (i = 0; i < MAX_VOICES; i++) voices[i].snd = -1;
        init = 1;
    }
    for (k = 0; k < n; k++) {
        int acc = 0;
        if (music >= 0) {
            acc += sample_at(music, music_pos) * MUSIC_VOL / 256 * music_gain / 256;
            if (++music_pos >= SOUNDS[music].samples) music_pos = SOUNDS[music].loop ? 0 : SOUNDS[music].samples - 1;
        }
        for (i = 0; i < MAX_VOICES; i++) {
            Voice *v = &voices[i];
            if (v->snd < 0) continue;
            acc += sample_at(v->snd, v->pos) * SFX_VOL / 256;
            if (++v->pos >= SOUNDS[v->snd].samples) v->snd = -1;
        }
        /* limitador suave: acima de 22000 comprime 3:1 antes de cortar */
        if (acc > 22000) acc = 22000 + (acc - 22000) / 3;
        else if (acc < -22000) acc = -22000 + (acc + 22000) / 3;
        if (acc > 32767) acc = 32767;
        if (acc < -32768) acc = -32768;
        out[k] = (short)acc;
        if (fading && (k & 15) == 0) {
            music_gain -= FADE_STEP;
            if (music_gain <= 0) {
                music = music_next;
                music_next = -1;
                music_pos = 0;
                music_gain = 256;
                fading = 0;
            }
        }
    }
}
