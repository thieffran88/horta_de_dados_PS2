/* Teste do envio de audio ao audsrv (roda no PC: make -f Makefile.host test).
 * Simula o anel do audsrv.irx igual ao codigo do PS2SDK (iop/sound/audsrv/src/audsrv.c):
 * 22050 Hz mono 16 bits -> anel de 10 blocos de 470 bytes, leitura de 470 bytes a cada
 * 512 amostras de saida a 48 kHz. Verifica que a musica sai na velocidade certa e sem cortes. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../src/audio.h"

#define FEED 470
#define RING (FEED * 10)
static unsigned char ring[RING];
static int writepos, readpos;
static long total_written, total_read, underruns;
static short out[22050 * 40];
static long out_n;

static int ring_available(void)
{
    if (writepos <= readpos) return readpos - writepos;
    return RING - (writepos - readpos);
}

static int ring_queued(void)
{
    if (writepos < readpos) return RING - (readpos - writepos);
    return writepos - readpos;
}

static int ring_play(const char *buf, int len)
{
    int sent = 0, avail = ring_available();
    if (len > avail) len = avail;
    while (len > 0) {
        int copy = len;
        if (writepos >= readpos && RING - writepos < copy) copy = RING - writepos;
        memcpy(ring + writepos, buf, (size_t)copy);
        buf += copy; len -= copy; sent += copy;
        writepos += copy;
        if (writepos >= RING) writepos = 0;
    }
    total_written += sent;
    return sent;
}

static void reader_step(void)
{
    /* o SPU2 consome um bloco: grava as amostras "ouvidas" */
    int k;
    if (total_read + FEED > total_written + RING / 2) underruns++; /* leu dado velho */
    for (k = 0; k < FEED; k += 2) {
        int pos = readpos + k;
        /* como no audsrv: a primeira leitura comeca desalinhada (2348) e passa do fim do
         * anel; o driver le memoria de fora (aqui: silencio) e depois volta para 0 */
        short v = pos + 1 < RING ? (short)(ring[pos] | (ring[pos + 1] << 8)) : 0;
        if (out_n < (long)(sizeof(out) / sizeof(out[0]))) out[out_n++] = v;
    }
    readpos += FEED;
    if (readpos >= RING) readpos = 0;
    total_read += FEED;
}

/* alimentador antigo (com o bug): confiava em audsrv_queued() e ignorava o retorno */
static void old_feed(void)
{
    static short buf[512];
    int guard = 0;
    while (ring_queued() < 2048 * 2 && guard++ < 8) {
        audio_mix(buf, 512);
        ring_play((const char *)buf, 512 * 2);
    }
}

int main(int argc, char **argv)
{
    const double secs = 20.0, fps = 59.94;
    const double block_t = 512.0 / 48000.0;
    double t = 0, next_frame = 0, next_block = 0;
    long k, mismatch = 0, first_bad = -1;
    int use_old = argc > 1 && !strcmp(argv[1], "old");
    const unsigned char *mus = asset_audio + SOUNDS[MUS_TITLE].offset;

    writepos = 0;
    readpos = (FEED * 5) & ~3;
    total_read = -(long)readpos; /* o leitor comeca adiantado (dado vazio) */
    audio_music(MUS_TITLE);
    while (t < secs) {
        if (next_frame <= next_block) {
            t = next_frame;
            if (use_old) old_feed(); else audio_feed(ring_available, ring_play);
            next_frame += 1.0 / fps;
        } else {
            t = next_block;
            reader_step();
            next_block += block_t;
        }
    }
    /* a saida "ouvida" deve ser a musica do inicio, amostra por amostra, sem pular nada.
     * Os primeiros blocos sao o silencio inicial do anel: procura o comeco da musica. */
    {
        long start = 0, n;
        int expect0 = ULAW_TABLE[mus[0]] * 150 / 256;
        while (start < out_n && !(out[start] == expect0 && out[start + 1] == ULAW_TABLE[mus[1]] * 150 / 256)) start++;
        n = out_n - start;
        for (k = 0; k < n && k < SOUNDS[MUS_TITLE].samples; k++) {
            int expect = ULAW_TABLE[mus[k]] * 150 / 256;
            if (out[start + k] != expect) { if (first_bad < 0) first_bad = k; mismatch++; }
        }
        printf("%s: %.1f s simulados, %ld amostras ouvidas (%.0f/s), musica pulou em %ld de %ld amostras%s\n",
               use_old ? "alimentador ANTIGO" : "alimentador novo", secs, out_n, out_n / secs, mismatch, n,
               underruns ? ", COM falta de dados" : "");
        if (!use_old && (mismatch || underruns)) {
            printf("FALHOU: primeira divergencia na amostra %ld, %ld faltas de dados\n", first_bad, underruns);
            return 1;
        }
    }
    return 0;
}
