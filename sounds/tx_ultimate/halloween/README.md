# Halloween sound pack (TX Ultimate)

Used by the `halloween` sound pack of
[esphome-tx-ultimate](https://github.com/AntorFr/esphome-tx-ultimate) (theme "Halloween").
All files: 48 kHz, mono, 16-bit FLAC, levels matched to the other packs.

| File | Gesture | Sound | Source |
|------|---------|-------|--------|
| `clic.flac` | touch release | short skeleton bones rattle (0.14 s) | [bones rattle](https://opengameart.org/content/bones-rattle), OpenGameArt, CC0 |
| `slide.flac` | swipe | heavy chains (1 s excerpt) | Eneria sound library (`chaines-halloween/05-chains-heavy`) |
| `long_press.flac` | long press | ghost moan (1.7 s) | ElevenLabs Sound Effects — "creepy ghost moan, short, haunted house" |
| `multi_press.flac` | multi-touch | thunder crack and rumble (2.3 s) | ElevenLabs Sound Effects — "close thunder crack followed by a short rumble, storm" |

Rule of thumb: these play every day for the whole season, so they stay short and
easy on the ears. Scarier sounds (witch laughs, roars, howls) are kept for the
Home Assistant "scares", which play them on the room speakers.

Processing: `../tools/to_tx.py` (trim, cap length, high-pass at 200 Hz for the tiny
speaker, fade out, RMS level match, FLAC) and `../tools/excerpt.py` (cut the most
intense window of a long recording, starting on its attack).
