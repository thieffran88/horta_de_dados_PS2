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

static void ensure_init(void)
{
    static int init;
    int i;
    if (init) return;
    for (i = 0; i < MAX_VOICES; i++) voices[i].snd = -1;
    init = 1;
}

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
    ensure_init();
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
    int i, k;
    ensure_init();
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

/* ---------------- alimentacao do driver ----------------
 * O audsrv guarda as amostras num anel; a leitura avanca sempre no ritmo do SPU2.
 * Regras: (1) nunca descartar amostras ja mixadas (o driver aceita menos que o pedido
 * quando falta espaco); (2) deixar sempre alguns bytes livres, porque com o anel 100%
 * cheio a posicao de escrita encosta na de leitura e o driver passa a ve-lo como vazio. */
#define FEED_CHUNK 256      /* amostras mixadas por vez */
#define FEED_GUARD 8        /* bytes que ficam sempre livres no anel */

void audio_feed(audio_avail_fn available, audio_play_fn play)
{
    static short buf[FEED_CHUNK];
    static int pend_off, pend_len; /* bytes ainda nao aceitos de buf */
    int guard = 0;
    while (guard++ < 64) {
        int free_bytes, n, sent;
        if (pend_len == 0) {
            audio_mix(buf, FEED_CHUNK);
            pend_off = 0;
            pend_len = FEED_CHUNK * 2;
        }
        free_bytes = available() - FEED_GUARD;
        if (free_bytes < 2) break;
        n = pend_len < free_bytes ? pend_len : free_bytes;
        n &= ~1; /* amostras inteiras de 16 bits */
        sent = play((const char *)buf + pend_off, n);
        if (sent <= 0) break;
        sent &= ~1;
        pend_off += sent;
        pend_len -= sent;
    }
}
