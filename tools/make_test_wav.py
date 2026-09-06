"""Generuje TEST.wav do sprawdzenia samej sciezki odtwarzania czytnika.

Format dobrany pod AudioRecorder::playbackTaskLoop(), ktore robi
file.seek(sizeof(WavHeader)) = 44 bajty i czyta dalej surowe probki:
16-bit PCM, mono, 16 kHz, kanoniczny 44-bajtowy naglowek bez zadnych
dodatkowych chunkow (LIST/fact zepsulyby offset i dalyby szum).

Tresc celowo diagnostyczna: cztery czyste tony rosnaco, cisza, sweep,
cisza. Melodia slyszalna = wyjscie dziala. Buczenie w blokach ciszy =
problem lezy po stronie ES8311/wzmacniacza, nie w danych.
"""

import math
import struct
import sys

SAMPLE_RATE = 16000
AMPLITUDE = 0.5
FADE_MS = 5


def fade(i, total, fade_samples):
    if i < fade_samples:
        return i / fade_samples
    if i >= total - fade_samples:
        return (total - i) / fade_samples
    return 1.0


def tone(freq, duration_s):
    total = int(SAMPLE_RATE * duration_s)
    fade_samples = max(1, int(SAMPLE_RATE * FADE_MS / 1000))
    out = []
    for i in range(total):
        v = math.sin(2 * math.pi * freq * i / SAMPLE_RATE)
        out.append(v * AMPLITUDE * fade(i, total, fade_samples))
    return out


def silence(duration_s):
    return [0.0] * int(SAMPLE_RATE * duration_s)


def sweep(f0, f1, duration_s):
    total = int(SAMPLE_RATE * duration_s)
    fade_samples = max(1, int(SAMPLE_RATE * FADE_MS / 1000))
    out = []
    phase = 0.0
    for i in range(total):
        f = f0 + (f1 - f0) * (i / total)
        phase += 2 * math.pi * f / SAMPLE_RATE
        out.append(math.sin(phase) * AMPLITUDE * fade(i, total, fade_samples))
    return out


samples = []
for f in (440, 554, 659, 880):
    samples += tone(f, 0.5)
samples += silence(0.5)
samples += sweep(200, 3000, 2.0)
samples += silence(0.5)

pcm = b"".join(struct.pack("<h", max(-32768, min(32767, int(s * 32767)))) for s in samples)

header = struct.pack(
    "<4sI4s4sIHHIIHH4sI",
    b"RIFF", 36 + len(pcm), b"WAVE",
    b"fmt ", 16, 1, 1, SAMPLE_RATE, SAMPLE_RATE * 2, 2, 16,
    b"data", len(pcm),
)
assert len(header) == 44, len(header)

for path in sys.argv[1:]:
    with open(path, "wb") as f:
        f.write(header)
        f.write(pcm)
    print(f"{path}: {len(header) + len(pcm)} bajtow, {len(samples) / SAMPLE_RATE:.2f} s")
