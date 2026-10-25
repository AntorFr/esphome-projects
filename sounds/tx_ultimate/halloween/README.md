# Halloween sound pack (TX Ultimate)

Used by the `halloween` sound pack of
[esphome-tx-ultimate](https://github.com/AntorFr/esphome-tx-ultimate) (theme "Halloween").
All files: 48 kHz, mono, 16-bit FLAC, levels set by ear on the switch itself.

| File | Gesture | Sound | Source |
|------|---------|-------|--------|
| `clic.flac` | touch release | short skeleton bones rattle (0.14 s) | [bones rattle](https://opengameart.org/content/bones-rattle), OpenGameArt, CC0 |
| `slide.flac` | swipe | heavy chains (1 s excerpt) | Eneria sound library (`chaines-halloween/05-chains-heavy`) |
| `long_press.flac` | long press | ghost moan (1.7 s) | ElevenLabs Sound Effects — "creepy ghost moan, short, haunted house" |
| `multi_press.flac` | multi-touch | thunder strike (3 s excerpt) | "Thunder Strike" by tanweraman (`tanweraman-thunder-strike-wav-321628`) |

Rule of thumb: these play every day for the whole season, so they stay short and
easy on the ears. Scarier sounds (witch laughs, roars, howls) are kept for the
Home Assistant "scares", which play them on the room speakers.

Processing: `../tools/to_tx.py` (trim, cap length, high-pass at 200 Hz for the tiny
speaker, fade out, RMS level match, FLAC), `../tools/excerpt.py` (cut the most
intense window of a long recording, starting on its attack) and
`../tools/match_level.py` (loudness as the tiny speaker hears it):

- `long_press`: high-pass 450 Hz, harmonics of the moan added (`--excite 10`), 5 dB above
  the chains, measured up to 10 kHz. The moan sits below 500 Hz, which the speaker barely plays.
- `multi_press`: high-pass 450 Hz, +6 dB presence, lookahead limiter (`--limiter smooth`),
  as loud as it gets without clipping. Thunder peaks far above its average: pushing it
  further saturates.
