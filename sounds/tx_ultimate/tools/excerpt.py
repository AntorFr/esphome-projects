#!/usr/bin/env python3
"""Cut the most intense excerpts of a long sound into TX Ultimate candidates.

Usage: excerpt.py <src> <out_prefix> <seconds> <count> <rms_dbfs>
Writes <out_prefix>_1.flac … (48 kHz mono 16-bit), non-overlapping windows
ranked by energy, reusing the conversion steps of to_tx.py.
"""

import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).parent))
from to_tx import SR, decode, encode, finish, highpass  # noqa: E402


def best_windows(x, length, count):
    hop = int(0.05 * SR)
    energy = np.array([np.sum(x[i:i + length] ** 2) for i in range(0, max(1, len(x) - length), hop)])
    picked = []
    for idx in np.argsort(energy)[::-1]:
        start = idx * hop
        if all(abs(start - p) >= length for p in picked):
            picked.append(start)
        if len(picked) == count:
            break
    return sorted(picked)


def main():
    src, prefix, seconds, count, rms_db = sys.argv[1], sys.argv[2], float(sys.argv[3]), int(sys.argv[4]), float(sys.argv[5])
    x = highpass(decode(src))
    length = int(seconds * SR)
    for n, start in enumerate(best_windows(x, length, count), 1):
        # Start just before the attack instead of wherever the energy window began.
        window = x[start:start + length]
        attack = int(np.argmax(np.abs(window) > 0.3 * np.max(np.abs(window))))
        start = max(0, start + attack - int(0.01 * SR))
        seg = finish(x[start:start + length].copy(), seconds, rms_db)
        out = Path(f"{prefix}_{n}.flac")
        encode(seg, out)
        print(f"{out.name:<28} from {start / SR:5.2f} s  {seconds:.2f} s  {out.stat().st_size // 1024:3d} KiB")


if __name__ == "__main__":
    main()
