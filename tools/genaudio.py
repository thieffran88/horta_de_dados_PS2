#!/usr/bin/env python3
"""Gera a musica e os efeitos sonoros do jogo (sintese procedural, tudo original).

Saidas:
  assets/audio.bin   - todos os sons, mono, 22050 Hz (musicas em mu-law 8 bits, efeitos em PCM 16 bits)
  src/sounds.h       - tabela dos sons (posicao, tamanho, loop, estado inicial do decodificador)
  build-audio/*.wav  - previas em WAV (nao usadas pelo jogo)

Uso: python3 tools/genaudio.py   (requer numpy e scipy)
"""
import os
import wave
import numpy as np
from scipy import signal

SR = 22050
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RNG = np.random.default_rng(20261109)

NOTE = {"C": 0, "C#": 1, "D": 2, "D#": 3, "E": 4, "F": 5, "F#": 6, "G": 7, "G#": 8, "A": 9, "A#": 10, "B": 11}


def hz(n):
    """'A4' -> 440 Hz"""
    name, octv = n[:-1], int(n[-1])
    return 440.0 * 2 ** ((NOTE[name] + 12 * (octv + 1) - 69) / 12)


def t(dur):
    return np.arange(int(dur * SR)) / SR


def env(n, a=0.005, d=0.1, s=0.6, r=0.1):
    e = np.ones(n) * s
    ia, idd, ir = int(a * SR), int(d * SR), int(r * SR)
    ia = max(1, min(ia, n))
    e[:ia] = np.linspace(0, 1, ia)
    idd = max(1, min(idd, n - ia))
    e[ia:ia + idd] = np.linspace(1, s, idd)
    if ir > 0 and ir < n:
        e[-ir:] *= np.linspace(1, 0, ir)
    return e


def pluck(freq, dur, decay=0.996, bright=0.6, twang=0.0):
    """Corda dedilhada (Karplus-Strong): banjo/violao."""
    n = int(dur * SR)
    N = max(2, int(SR / freq))
    exc = RNG.uniform(-1, 1, N)
    if bright < 1:
        exc = signal.lfilter([bright, 1 - bright], [1], exc)
    x = np.zeros(n)
    x[:N] = exc
    a = np.zeros(N + 2)
    a[0] = 1
    a[N] -= decay * 0.5
    a[N + 1] -= decay * 0.5
    y = signal.lfilter([1], a, x)
    if twang:
        y += twang * np.sin(2 * np.pi * freq * 2 * t(dur)[:n]) * np.exp(-t(dur)[:n] * 30)
    y *= np.linspace(1, 0, n) ** 0.3
    k = min(n, int(0.002 * SR))
    y[:k] *= np.linspace(0, 1, k)  # ataque de 2 ms: sem clique
    return y / (np.abs(y).max() + 1e-9)


def saw(freqs):
    ph = np.cumsum(freqs) / SR
    return 2 * (ph - np.floor(ph + 0.5))


def fiddle(freq, dur, vib=5.5):
    tt = t(dur)
    f = freq * (1 + 0.006 * np.sin(2 * np.pi * vib * tt) * np.clip(tt * 4, 0, 1))
    y = saw(f)
    b, a = signal.butter(2, [400 / (SR / 2), 3200 / (SR / 2)], "band")
    y = signal.lfilter(b, a, y)
    y *= env(len(tt), a=0.04, d=0.1, s=0.8, r=min(0.08, dur / 3))
    return y / (np.abs(y).max() + 1e-9)


def whistle(freq, dur):
    tt = t(dur)
    f = freq * (1 + 0.004 * np.sin(2 * np.pi * 5 * tt) * np.clip(tt * 3, 0, 1))
    ph = np.cumsum(f) / SR
    y = np.sin(2 * np.pi * ph) + 0.12 * np.sin(4 * np.pi * ph) + 0.02 * RNG.standard_normal(len(tt))
    y *= env(len(tt), a=0.03, d=0.05, s=0.85, r=min(0.07, dur / 3))
    return y / (np.abs(y).max() + 1e-9)


def bass(freq, dur):
    tt = t(dur)
    y = np.sin(2 * np.pi * freq * tt) + 0.3 * np.sin(4 * np.pi * freq * tt)
    return y * np.exp(-tt * 3.5) * env(len(tt), a=0.004, d=0.02, s=1, r=0.03)


def noise_burst(dur, lo, hi, decay):
    tt = t(dur)
    y = RNG.standard_normal(len(tt))
    b, a = signal.butter(2, [lo / (SR / 2), min(hi, SR / 2 - 100) / (SR / 2)], "band")
    return signal.lfilter(b, a, y) * np.exp(-tt * decay)


def kick(dur=0.18):
    tt = t(dur)
    f = 110 * np.exp(-tt * 20) + 45
    return np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-tt * 14)


def add(buf, sig, at, gain=1.0):
    i = int(at * SR)
    if i >= len(buf):
        return
    j = min(len(buf), i + len(sig))
    buf[i:j] += sig[:j - i] * gain


def reverb(y, mix=0.18):
    out = y.copy()
    for d, g in [(0.029, 0.5), (0.037, 0.45), (0.044, 0.4), (0.053, 0.35)]:
        n = int(d * SR)
        a = np.zeros(n + 1)
        a[0] = 1
        a[n] = -g
        out += mix * signal.lfilter([1], a, y) * 0.5
    return out


def normalize(y, peak=0.85):
    return y * (peak / (np.abs(y).max() + 1e-9))


# ---------------------------------------------------------------- musica

def chord_notes(name):
    table = {
        "G": ["G2", "D3", "G3", "B3", "D4"], "C": ["C3", "G3", "C4", "E4", "G4"],
        "D": ["D3", "A3", "D4", "F#4", "A4"], "F": ["F2", "C3", "F3", "A3", "C4"],
        "Am": ["A2", "E3", "A3", "C4", "E4"], "D7": ["D3", "A3", "C4", "F#4", "A4"],
    }
    return table[name]


def render_song(bpm, chords, melody, lead, roll, bars_drone, shaker=True, kicks=True):
    beat = 60.0 / bpm
    bars = len(chords)
    total = bars * 4 * beat
    tail = 2.0
    mus = np.zeros(int((total + tail) * SR))
    # base "boom-chick" + violao abafado
    for b, ch in enumerate(chords):
        notes = chord_notes(ch)
        t0 = b * 4 * beat
        add(mus, bass(hz(notes[0]), beat * 0.95), t0, 0.55)
        add(mus, bass(hz(notes[1]) / 2, beat * 0.95), t0 + 2 * beat, 0.5)
        for k in (1, 3):
            for s, n in enumerate(notes[2:]):
                add(mus, pluck(hz(n), beat * 0.6, decay=0.99, bright=0.5), t0 + k * beat + s * 0.012, 0.13)
        # banjo em "roll" com corda solta aguda (drone)
        pattern = roll
        step = beat / 2
        tones = [notes[2], notes[3], notes[4], bars_drone]
        for i, p in enumerate(pattern):
            n = tones[p]
            add(mus, pluck(hz(n), step * 3, decay=0.993, bright=0.72, twang=0.15), t0 + i * step, 0.16)
        if shaker:
            for i in range(8):
                add(mus, noise_burst(0.06, 4000, 9000, 60), t0 + i * step + 0.01, 0.05 if i % 2 else 0.025)
        if kicks:
            add(mus, kick(), t0, 0.35)
            add(mus, kick(), t0 + 2 * beat, 0.25)
    # melodia
    pos = 0.0
    for n, d in melody:
        if n != "r":
            add(mus, lead(hz(n), d * beat * 0.97), pos, 0.30)
        pos += d * beat
    assert abs(pos - total) < 1e-6, (pos, total)
    mus = reverb(mus)
    b, a = signal.butter(2, 6500 / (SR / 2))
    mus = signal.lfilter(b, a, mus)  # tira o "chiado" dos ataques
    # loop sem emenda: o que passa do fim volta para o comeco
    loop = mus[:int(total * SR)].copy()
    rest = mus[int(total * SR):]
    loop[:len(rest)] += rest
    return normalize(loop, 0.8)


TITLE_MELODY = [
    ("D5", 1), ("B4", .5), ("D5", .5), ("E5", 1), ("D5", 1),
    ("B4", 1), ("G4", 1), ("A4", 1), ("B4", 1),
    ("C5", 1), ("E5", 1), ("G5", 1), ("E5", 1),
    ("D5", 3), ("r", 1),
    ("D5", 1), ("B4", .5), ("D5", .5), ("E5", 1), ("G5", 1),
    ("E5", 1), ("D5", 1), ("B4", 1), ("G4", 1),
    ("A4", 1), ("B4", 1), ("C5", 1), ("D5", 1),
    ("A4", 2), ("F#4", 1), ("A4", 1),
    ("G5", 1), ("E5", .5), ("D5", .5), ("B4", 1), ("D5", 1),
    ("G5", 1), ("E5", .5), ("D5", .5), ("B4", 2),
    ("C5", 1), ("E5", 1), ("G5", 1), ("A5", 1),
    ("G5", 2), ("D5", 2),
    ("E5", 1), ("G5", 1), ("E5", 1), ("C5", 1),
    ("D5", 1), ("F#5", 1), ("A5", 1), ("F#5", 1),
    ("G5", 1.5), ("D5", .5), ("B4", 1), ("A4", 1),
    ("G4", 3), ("r", 1),
]
TITLE_CHORDS = ["G", "G", "C", "G", "G", "G", "D", "D", "G", "G", "C", "G", "C", "D", "G", "G"]

GAME_MELODY = [
    ("E5", 1), ("G5", 1), ("E5", 1), ("C5", 1),
    ("D5", 1), ("E5", 1), ("D5", 2),
    ("F5", 1), ("A5", 1), ("G5", 1), ("F5", 1),
    ("E5", 3), ("r", 1),
    ("A4", 1), ("C5", 1), ("E5", 1), ("C5", 1),
    ("B4", 1), ("A4", 1), ("E4", 2),
    ("G4", 1), ("B4", 1), ("D5", 1), ("B4", 1),
    ("D5", 3), ("r", 1),
    ("G5", 1), ("E5", 1), ("C5", 1), ("E5", 1),
    ("G5", 1), ("A5", .5), ("G5", .5), ("E5", 2),
    ("A5", 1), ("G5", 1), ("F5", 1), ("A5", 1),
    ("G5", 3), ("r", 1),
    ("F5", 1), ("E5", 1), ("D5", 1), ("C5", 1),
    ("D5", 1), ("E5", 1), ("F5", 1), ("D5", 1),
    ("E5", 1), ("D5", .5), ("C5", .5), ("D5", 1), ("G4", 1),
    ("C5", 3), ("r", 1),
]
GAME_CHORDS = ["C", "C", "F", "C", "Am", "Am", "G", "G", "C", "C", "F", "C", "F", "G", "C", "C"]


# ---------------------------------------------------------------- efeitos

def sfx_tick():
    tt = t(0.035)
    return np.sin(2 * np.pi * 1300 * tt) * np.exp(-tt * 140) + 0.3 * noise_burst(0.035, 2000, 6000, 200)


def sfx_select():
    y = np.zeros(int(0.25 * SR))
    add(y, pluck(hz("E5"), 0.2, bright=0.8), 0, 0.8)
    add(y, pluck(hz("A5"), 0.2, bright=0.8), 0.06, 0.9)
    return y


def sfx_back():
    y = np.zeros(int(0.25 * SR))
    add(y, pluck(hz("A4"), 0.2, bright=0.8), 0, 0.8)
    add(y, pluck(hz("E4"), 0.2, bright=0.8), 0.06, 0.9)
    return y


def wood_tok(f=1800, dur=0.03):
    tt = t(dur)
    return np.sin(2 * np.pi * f * tt) * np.exp(-tt * 160) + 0.5 * noise_burst(dur, 2500, 8000, 250)


def sfx_roll():
    """Dados chacoalhando e rolando na mesa (~1 s, acompanha a animacao)."""
    y = np.zeros(int(1.05 * SR))
    tm = 0.0
    gap = 0.03
    while tm < 0.95:
        add(y, wood_tok(RNG.uniform(1400, 2600)), tm, RNG.uniform(0.4, 1.0))
        tm += gap * RNG.uniform(0.6, 1.4)
        gap *= 1.06
    add(y, noise_burst(1.0, 300, 1500, 3) * 0.15, 0)
    return y


def sfx_land():
    tt = t(0.12)
    y = 0.6 * np.sin(2 * np.pi * 180 * tt) * np.exp(-tt * 40)
    add(y, wood_tok(1500, 0.05), 0)
    return y


def sfx_plant():
    tt = t(0.3)
    f = 260 * np.exp(-tt * 9) + 70
    plop = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-tt * 12)
    dirt = noise_burst(0.3, 150, 1200, 18)
    return plop + 0.6 * dirt


def grunt(dur, f0, f1, rough=0.0):
    tt = t(dur)
    f = np.linspace(f0, f1, len(tt)) * (1 + rough * np.sin(2 * np.pi * 28 * tt))
    s = saw(f) + 0.2 * RNG.standard_normal(len(tt))
    out = np.zeros(len(tt))
    for fc, bw, g in [(450, 250, 1.0), (1100, 400, 0.6), (2600, 700, 0.25)]:
        b, a = signal.butter(2, [(fc - bw / 2) / (SR / 2), (fc + bw / 2) / (SR / 2)], "band")
        out += g * signal.lfilter(b, a, s)
    return out * env(len(tt), a=0.01, d=0.03, s=0.8, r=0.04)


def sfx_oink():
    y = np.zeros(int(0.45 * SR))
    add(y, grunt(0.14, 260, 190, 0.15), 0)
    add(y, grunt(0.18, 300, 170, 0.2), 0.19)
    return y


def sfx_moo():
    tt = t(1.3)
    f0 = 105 + 35 * np.sin(np.pi * np.clip(tt / 1.1, 0, 1)) + 2 * np.sin(2 * np.pi * 6 * tt)
    s = saw(f0)
    cut = 250 + 900 * np.sin(np.pi * np.clip(tt / 1.2, 0, 1)) ** 2
    out = np.zeros(len(tt))
    blk = 256
    zi = None
    for i in range(0, len(tt), blk):
        b, a = signal.butter(2, cut[i] / (SR / 2))
        seg, zi = signal.lfilter(b, a, s[i:i + blk], zi=zi if zi is not None else signal.lfilter_zi(b, a) * 0)
        out[i:i + blk] = seg
    return out * env(len(tt), a=0.12, d=0.1, s=0.9, r=0.3)


def rooster_seg(dur, f0, f1):
    tt = t(dur)
    f = np.linspace(f0, f1, len(tt))
    f *= 1 + 0.03 * np.sin(2 * np.pi * 32 * tt)
    s = saw(f) * (1 + 0.4 * np.sin(2 * np.pi * 45 * tt))
    b, a = signal.butter(2, [900 / (SR / 2), 3800 / (SR / 2)], "band")
    return signal.lfilter(b, a, s) * env(len(tt), a=0.015, d=0.05, s=0.85, r=0.05)


def sfx_rooster():
    y = np.zeros(int(1.4 * SR))
    add(y, rooster_seg(0.13, 620, 680), 0.0)
    add(y, rooster_seg(0.12, 700, 760), 0.17)
    add(y, rooster_seg(0.16, 820, 880), 0.33)
    add(y, rooster_seg(0.75, 950, 640), 0.54)
    return y


def sfx_cluck():
    y = np.zeros(int(0.5 * SR))
    for i, at in enumerate([0, 0.11, 0.2]):
        add(y, grunt(0.06, 650 + 60 * i, 520, 0.0) + 0.3 * noise_burst(0.06, 1500, 4000, 40), at)
    return y


def bell(freq, dur, decay=6):
    tt = t(dur)
    y = sum(g * np.sin(2 * np.pi * freq * r * tt) * np.exp(-tt * decay * r ** 0.5)
            for r, g in [(1, 1), (2.01, 0.5), (2.76, 0.35), (4.07, 0.2)])
    return y


def sfx_market():
    y = np.zeros(int(0.9 * SR))
    add(y, noise_burst(0.1, 800, 5000, 30), 0, 0.6)          # gaveta
    add(y, wood_tok(900, 0.04), 0.0, 0.8)
    add(y, bell(hz("C7"), 0.7, 7), 0.1, 0.6)
    add(y, bell(hz("E7"), 0.7, 7), 0.16, 0.5)
    return y


def sfx_power():
    y = np.zeros(int(0.7 * SR))
    for i, n in enumerate(["C6", "E6", "G6", "C7"]):
        add(y, bell(hz(n), 0.4, 9), i * 0.06, 0.5)
    add(y, noise_burst(0.5, 6000, 10000, 8) * 0.15, 0.05)
    return y


def sfx_power_use():
    tt = t(0.25)
    f = 500 * np.exp(tt * 4.5)
    return np.sin(2 * np.pi * np.cumsum(f) / SR) * env(len(tt), a=0.01, d=0.05, s=0.7, r=0.1)


def sfx_ribbon():
    y = np.zeros(int(0.9 * SR))
    add(y, bell(hz("G5"), 0.8, 5), 0, 0.6)
    add(y, bell(hz("D6"), 0.8, 5), 0.12, 0.6)
    add(y, bell(hz("G6"), 0.8, 5), 0.24, 0.5)
    return y


def sfx_error():
    y = np.zeros(int(0.32 * SR))
    for at in (0, 0.16):
        tt = t(0.12)
        sq = np.sign(np.sin(2 * np.pi * 140 * tt))
        b, a = signal.butter(2, 1200 / (SR / 2))
        add(y, signal.lfilter(b, a, sq) * env(len(tt), a=0.005, d=0.02, s=0.8, r=0.02), at, 0.7)
    return y


def sfx_win():
    """Fanfarra caipira de vitoria (banjo + violino)."""
    y = np.zeros(int(3.2 * SR))
    for i, n in enumerate(["G4", "B4", "D5", "G5", "B5"]):
        add(y, pluck(hz(n), 0.6, bright=0.85, twang=0.2), i * 0.08, 0.5)
    for n in ["G3", "D4", "G4", "B4", "D5"]:
        add(y, pluck(hz(n), 2.2, decay=0.997, bright=0.7), 0.5, 0.25)
    add(y, fiddle(hz("D5"), 0.25), 0.5, 0.5)
    add(y, fiddle(hz("E5"), 0.25), 0.75, 0.5)
    add(y, fiddle(hz("G5"), 1.6), 1.0, 0.6)
    add(y, kick(), 0.5, 0.5)
    add(y, bell(hz("G6"), 1.5, 3), 1.0, 0.25)
    return reverb(y)


# ---------------------------------------------------------------- codificacao
# Musicas: mu-law de 8 bits (G.711), metade do tamanho e ~38 dB de SNR.
# Efeitos: PCM de 16 bits (curtos, ataques fortes que sofreriam com ADPCM).
FMT_PCM16, FMT_ULAW = 0, 1


def ulaw_decode_table():
    out = []
    for u in range(256):
        u = ~u & 0xFF
        sign, exp, man = u & 0x80, (u >> 4) & 7, u & 0x0F
        v = ((man << 3) + 0x84) << exp
        v -= 0x84
        out.append(-v if sign else v)
    return out


ULAW_DEC = np.array(ulaw_decode_table(), np.int32)


def ulaw_encode(pcm):
    # busca o codigo cujo valor decodificado e o mais proximo (exato e simples)
    order = np.argsort(ULAW_DEC)
    vals = ULAW_DEC[order]
    pos = np.clip(np.searchsorted(vals, pcm), 1, 255)
    left, right = vals[pos - 1], vals[pos]
    pick = np.where(np.abs(pcm - left) <= np.abs(right - pcm), pos - 1, pos)
    return order[pick].astype(np.uint8)


def encode(y, fmt):
    pcm = np.clip(np.round(y * 32767), -32768, 32767).astype(np.int32)
    if fmt == FMT_ULAW:
        return ulaw_encode(pcm).tobytes(), len(pcm)
    return pcm.astype("<i2").tobytes(), len(pcm)


def write_wav(path, y):
    pcm = np.clip(np.round(y * 32767), -32768, 32767).astype("<i2")
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(pcm.tobytes())


def main():
    ROLL = [0, 1, 3, 0, 2, 3, 1, 3]
    sounds = [
        ("MUS_TITLE", True, render_song(120, TITLE_CHORDS, TITLE_MELODY, fiddle, ROLL, "D5")),
        ("MUS_GAME", True, render_song(96, GAME_CHORDS, GAME_MELODY, whistle, [0, 1, 2, 3, 1, 2, 0, 3], "G4",
                                       shaker=True, kicks=False)),
        ("SFX_TICK", False, sfx_tick()), ("SFX_SELECT", False, sfx_select()), ("SFX_BACK", False, sfx_back()),
        ("SFX_ROLL", False, sfx_roll()), ("SFX_LAND", False, sfx_land()), ("SFX_PLANT", False, sfx_plant()),
        ("SFX_OINK", False, sfx_oink()), ("SFX_MOO", False, sfx_moo()), ("SFX_ROOSTER", False, sfx_rooster()),
        ("SFX_CLUCK", False, sfx_cluck()), ("SFX_MARKET", False, sfx_market()), ("SFX_POWER", False, sfx_power()),
        ("SFX_POWER_USE", False, sfx_power_use()), ("SFX_RIBBON", False, sfx_ribbon()),
        ("SFX_ERROR", False, sfx_error()), ("SFX_WIN", False, sfx_win()),
    ]
    os.makedirs(os.path.join(ROOT, "build-audio"), exist_ok=True)
    blob = bytearray()
    table = []
    for name, loop, y in sounds:
        y = normalize(y, 0.8 if name.startswith("MUS") else 0.9)
        y = y[:max(1, len(y))]
        write_wav(os.path.join(ROOT, "build-audio", name.lower() + ".wav"), y)
        fmt = FMT_ULAW if name.startswith("MUS") else FMT_PCM16
        data, n = encode(y, fmt)
        while len(blob) % 16:
            blob.append(0)
        table.append((name, len(blob), n, int(loop), fmt))
        blob += data
        print("%-14s %6.2f s  %7d bytes" % (name, n / SR, len(data)))
    with open(os.path.join(ROOT, "assets/audio.bin"), "wb") as f:
        f.write(blob)
    with open(os.path.join(ROOT, "src/sounds.h"), "w") as f:
        f.write("/* Gerado por tools/genaudio.py - nao edite. */\n#ifndef HORTA_SOUNDS_H\n#define HORTA_SOUNDS_H\n\n")
        f.write("#define AUDIO_RATE %d\n\nenum {\n" % SR)
        for row in table:
            f.write("    %s,\n" % row[0])
        f.write("    NUM_SOUNDS\n};\n\n")
        f.write("enum { SND_PCM16 = 0, SND_ULAW = 1 };\n")
        f.write("/* offset em asset_audio, amostras, loop, formato */\n")
        f.write("typedef struct { int offset, samples, loop, format; } SoundInfo;\n")
        f.write("static const SoundInfo SOUNDS[NUM_SOUNDS] = {\n")
        for name, off, n, loop, fmt in table:
            f.write("    { %d, %d, %d, %d }, /* %s */\n" % (off, n, loop, fmt, name))
        f.write("};\n\n/* decodificacao mu-law (G.711) */\nstatic const short ULAW_TABLE[256] = {\n")
        for i in range(0, 256, 16):
            f.write("    " + ", ".join(str(v) for v in ULAW_DEC[i:i + 16]) + ",\n")
        f.write("};\n\n#endif\n")
    print("total: %d bytes" % len(blob))


if __name__ == "__main__":
    main()
