#!/usr/bin/env python3
"""Match the loudness of a TX Ultimate sound to a reference, as the tiny speaker hears it.

Loudness is measured in the 1–5 kHz band (--band-hi widens it to count hiss), on the
louder half of 20 ms frames so short sounds are not diluted by their tail. An optional
presence boost lifts sounds whose energy sits too low for the speaker (e.g. a deep moan).
A sound with almost nothing above 1 kHz (a moan, a thunder rumble) stays faint whatever
the gain: --highpass drops the bass the speaker cannot play (it only eats headroom) and
--excite adds harmonics of the low band where the speaker works, so the ear still hears
the deep sound. --offset aims above (or below) the reference level.
Peaks are limited once (linear below -6 dBFS, soft up to -1 dBFS): no compounding.
That soft limiter bends the waveform; when a lot of gain is needed (thunder) it
saturates, so --limiter smooth lowers the gain ahead of each peak instead.

Usage: match_level.py <src> <dst.flac> <reference.flac>
                      [--presence DB] [--highpass HZ] [--excite DB] [--offset DB]
                      [--band-hi HZ] [--compress RATIO] [--limiter soft|smooth]
Needs numpy and ffmpeg.
"""

import argparse
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).parent))
from to_tx import SR, decode, encode, gain_to, highpass  # noqa: E402


def band(x, lo=1000.0, hi=5000.0):
    spec = np.fft.rfft(x)
    freqs = np.fft.rfftfreq(len(x), 1 / SR)
    spec[(freqs < lo) | (freqs > hi)] = 0
    return np.fft.irfft(spec, len(x))


def active_level(x, hi=5000.0):
    b = band(x, 1000.0, hi)
    frame = int(0.02 * SR)
    levels = sorted(np.sqrt(np.mean(b[i:i + frame] ** 2)) for i in range(0, max(1, len(b) - frame), frame))
    loud = levels[len(levels) // 2:] or levels
    return float(np.mean(loud)) + 1e-12


def presence(x, gain_db):
    """Smooth bell centred on ~2.5 kHz (1–5 kHz), gain in dB."""
    if gain_db == 0:
        return x
    spec = np.fft.rfft(x)
    freqs = np.fft.rfftfreq(len(x), 1 / SR)
    bell = np.exp(-0.5 * (np.log2(np.maximum(freqs, 1) / 2500) / 0.9) ** 2)
    spec *= 10 ** (gain_db * bell / 20)
    return np.fft.irfft(spec, len(x))


def excite(x, gain_db, lo=150.0, hi=700.0):
    """Adds harmonics of the lo–hi band, kept in 700 Hz–4 kHz, at gain_db relative to that band."""
    if gain_db is None:
        return x
    low = band(x, lo, hi)
    peak = np.max(np.abs(low)) + 1e-12
    sat = np.tanh(4 * low / peak)
    # The rectified part brings even harmonics (an octave up), tanh the odd ones.
    harm = band(sat + 0.5 * np.abs(sat), 700.0, 4000.0)
    rms = lambda v: np.sqrt(np.mean(v ** 2)) + 1e-12  # noqa: E731
    return x + harm * rms(low) / rms(harm) * 10 ** (gain_db / 20)


def smooth_limit(x, ceiling_db=-1.0, lookahead_ms=3.0, release_ms=80.0):
    """Lookahead peak limiter: the gain falls before a peak and recovers slowly, no clipping."""
    c = 10 ** (ceiling_db / 20)
    need = np.minimum(1.0, c / np.maximum(np.abs(x), 1e-12))
    n = max(1, int(lookahead_ms * SR / 1000))
    ahead = need.copy()
    for k in range(1, 2 * n):  # lowest gain needed over the next 2n samples
        ahead[:-k] = np.minimum(ahead[:-k], need[k:])
    rel = np.exp(-1 / (release_ms * SR / 1000))
    g = np.empty_like(ahead)
    cur = 1.0
    for i, v in enumerate(ahead):
        cur = v if v < cur else v + (cur - v) * rel
        g[i] = cur
    # Causal average over n samples: stays under the need of every peak (window 2n above).
    g = np.convolve(g, np.ones(n) / n)[: len(g)]
    return np.clip(x * g, -c, c)


def compress(x, ratio, depth_db=18.0, attack_ms=5.0, release_ms=150.0):
    """Slow compressor: above (peak - depth_db) the level grows 1/ratio as fast.

    The gain follows an envelope (fast attack, slow release), so the waveform keeps its
    shape: it brings a rumble up under its crack without the grit of a clipper."""
    if not ratio or ratio <= 1:
        return x
    att = np.exp(-1 / (attack_ms * SR / 1000))
    rel = np.exp(-1 / (release_ms * SR / 1000))
    env = np.empty_like(x)
    cur = 0.0
    for i, v in enumerate(np.abs(x)):
        cur = v + (cur - v) * (att if v > cur else rel)
        env[i] = cur
    env_db = 20 * np.log10(env + 1e-9)
    threshold = env_db.max() - depth_db
    gain_db = np.minimum(0.0, (threshold - env_db) * (1 - 1 / ratio))
    return x * 10 ** (gain_db / 20)


def gain_to_smooth(x, measure, target, passes=4):
    """Same as to_tx.gain_to, with the lookahead limiter."""
    g = target / (measure(x) + 1e-12)
    for _ in range(passes):
        g *= target / (measure(smooth_limit(g * x)) + 1e-12)
    return smooth_limit(g * x)


def main():
    ap = argparse.ArgumentParser(description="Match a TX Ultimate sound to a reference level.")
    ap.add_argument("src")
    ap.add_argument("dst", type=Path)
    ap.add_argument("ref")
    ap.add_argument("--presence", type=float, default=0.0, help="presence boost around 2.5 kHz, dB")
    ap.add_argument("--highpass", type=float, default=None, help="drop the bass below this frequency, Hz")
    ap.add_argument("--excite", type=float, default=None, help="harmonics level relative to the low band, dB")
    ap.add_argument("--offset", type=float, default=0.0, help="target level above the reference, dB")
    ap.add_argument("--compress", type=float, default=None, help="compressor ratio (e.g. 4), before the gain")
    ap.add_argument("--limiter", choices=["soft", "smooth"], default="soft",
                    help="soft: bend the peaks (default); smooth: lookahead gain, no saturation")
    ap.add_argument("--band-hi", type=float, default=5000.0,
                    help="upper edge of the measured band, Hz (10000 counts the hiss of chains, bones...)")
    a = ap.parse_args()

    level = lambda v: active_level(v, a.band_hi)  # noqa: E731
    target = level(decode(a.ref)) * 10 ** (a.offset / 20)
    x = decode(a.src)
    if a.highpass:
        x = highpass(x, a.highpass)
    x = compress(presence(excite(x, a.excite), a.presence), a.compress)
    x = (gain_to_smooth if a.limiter == "smooth" else gain_to)(x, level, target)
    encode(x, a.dst)
    print(f"{a.dst.name:<18} level {20 * np.log10(level(x)):6.1f} dB (target {20 * np.log10(target):6.1f}), "
          f"peak {20 * np.log10(np.max(np.abs(x))):5.1f} dBFS, {np.mean(np.abs(x) > 0.5) * 100:4.1f}% above -6 dBFS")


if __name__ == "__main__":
    main()
