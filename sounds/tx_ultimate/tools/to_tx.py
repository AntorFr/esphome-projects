#!/usr/bin/env python3
"""Turn sound candidates (mp3/ogg/wav/flac) into TX Ultimate sounds (48 kHz mono 16-bit FLAC).

Trims leading/trailing silence, caps the length per gesture, removes the lows the
tiny speaker cannot play, fades out, and matches the RMS level of the existing
packs (soft limiter at -1 dBFS).

Usage: to_tx.py <in_dir> <out_dir>
Files must be named <slot>_<variant>.<ext>, slot being clic, slide, long_press or multi_press.
Needs numpy and ffmpeg.
"""

import subprocess
import sys
from pathlib import Path

import numpy as np

SR = 48_000
# slot -> (max seconds, RMS target dBFS measured on the christmas / explore packs)
SLOTS = {
    "clic": (0.40, -22.0),
    "slide": (1.20, -20.0),
    "long_press": (2.20, -18.0),
    "multi_press": (2.80, -17.5),
}


def decode(path):
    raw = subprocess.run(
        ["ffmpeg", "-loglevel", "error", "-i", str(path), "-ac", "1", "-ar", str(SR), "-f", "s16le", "-"],
        check=True, capture_output=True,
    ).stdout
    return np.frombuffer(raw, dtype="<i2").astype(np.float64) / 32768.0


def trim_silence(x, threshold_db=-45.0):
    level = 10 ** (threshold_db / 20)
    loud = np.flatnonzero(np.abs(x) > level)
    if loud.size == 0:
        return x
    start = max(0, loud[0] - int(0.005 * SR))
    return x[start:loud[-1] + 1]


def highpass(x, cutoff=200.0):
    spec = np.fft.rfft(x)
    freqs = np.fft.rfftfreq(len(x), 1 / SR)
    spec *= 1 / (1 + (cutoff / np.maximum(freqs, 1)) ** 4)
    return np.fft.irfft(spec, len(x))


def finish(x, max_s, rms_db):
    x = x[: int(max_s * SR)]
    fade_in, fade_out = int(0.003 * SR), int(min(0.15, len(x) / SR / 4) * SR)
    x[:fade_in] *= np.linspace(0, 1, fade_in)
    x[-fade_out:] *= np.linspace(1, 0, fade_out) ** 2
    ceiling = 10 ** (-1.0 / 20)
    for _ in range(4):
        x *= 10 ** (rms_db / 20) / (np.sqrt(np.mean(x ** 2)) + 1e-9)
        x = ceiling * np.tanh(x / ceiling)
    return x


def encode(x, path):
    pcm = (x * 32767).astype("<i2").tobytes()
    subprocess.run(
        ["ffmpeg", "-loglevel", "error", "-y", "-f", "s16le", "-ar", str(SR), "-ac", "1", "-i", "-",
         "-c:a", "flac", "-compression_level", "12", str(path)],
        input=pcm, check=True,
    )


def main():
    in_dir, out_dir = Path(sys.argv[1]), Path(sys.argv[2])
    out_dir.mkdir(parents=True, exist_ok=True)
    for mp3 in sorted(f for f in in_dir.iterdir() if f.suffix.lower() in (".mp3", ".ogg", ".wav", ".flac")):
        slot = mp3.stem.rsplit("_", 1)[0]
        max_s, rms_db = SLOTS[slot]
        x = finish(highpass(trim_silence(decode(mp3))), max_s, rms_db)
        out = out_dir / f"{mp3.stem}.flac"
        encode(x, out)
        print(f"{out.name:<20} {len(x) / SR:4.2f} s  {out.stat().st_size // 1024:3d} KiB")


if __name__ == "__main__":
    main()
