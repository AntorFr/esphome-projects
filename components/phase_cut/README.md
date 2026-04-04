# Phase Cut Triac Controller

ESPHome component for zero-cross synchronized phase-cut (angle) power control
of resistive loads (heaters, kettles, etc.) via a triac.

## How It Works

Phase-cut control delays the triac gate pulse within each mains half-cycle.
The longer the delay after the zero-crossing, the less energy is delivered
to the load. Unlike burst-fire (which sends complete half-cycles on/off),
phase-cut chops every half-cycle, allowing **true proportional control**
with finer resolution.

```
Half-cycle at ~60% power (level 6/10):

Mains:   ╱╲          ╱╲
        ╱  ╲        ╱  ╲
ZC ────┤    ├──────┤    ├────
       │    │      │    │
       ├─delay─┤   ├─delay─┤
               ▼           ▼
Gate:          ┃           ┃  (50 µs pulse)
Triac:     ───████████  ───████████
           OFF│ ON    │ OFF│ ON    │
```

At 50 Hz mains, one half-cycle is ~10 ms. The gate pulse fires after a
computed delay: short delay = high power, long delay = low power.

## Hardware Requirements

| Signal | GPIO | Direction | Description |
|--------|------|-----------|-------------|
| Zero-cross | Input | IN | Optocoupler output, ~100 Hz square wave (both edges of 50 Hz mains) |
| Triac gate | Output | OUT | Drives triac gate (via NPN transistor or direct) |

The zero-cross detector must be **mains-referenced** (always active
regardless of relay/triac state). The component expects ~100 edges/s for
50 Hz mains or ~120 edges/s for 60 Hz.

Typical circuit topology:

```
Mains ──┬── ZC optocoupler ── GPIO (zero_cross_pin)
        │
        └── Relay ── Triac ── Load
              │         │
           (session)  (phase_cut controls this via triac_pin)
```

## Installation

Place the component directory alongside your ESPHome config:

```
config/
├── your-device.yaml
└── components/
    └── phase_cut/
        ├── __init__.py
        ├── sensor.py
        ├── number.py
        ├── phase_cut.h
        └── phase_cut.cpp
```

In your YAML:

```yaml
external_components:
  - source:
      type: local
      path: components
    components: [phase_cut]
```

## Configuration

```yaml
phase_cut:
  id: triac_ctrl                    # optional, auto-generated
  zero_cross_pin: GPIO33            # REQUIRED — ZC detector input
  triac_pin: GPIO23                 # REQUIRED — triac gate output
  max_level: 10                     # optional, default 10
  min_edge_interval: 5ms            # optional, default 5ms (EMI debounce)
  inverted: false                   # optional, default false
```

### Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `zero_cross_pin` | pin | *required* | GPIO connected to zero-cross detector output |
| `triac_pin` | pin | *required* | GPIO connected to triac gate driver |
| `max_level` | int (2–100) | `10` | Number of discrete power levels. Power resolution = 1/max_level |
| `min_edge_interval` | time | `5ms` | Minimum time between valid ZC edges. Edges closer than this are rejected (relay EMI debounce). Must be less than one half-cycle (~10 ms at 50 Hz) |
| `inverted` | bool | `false` | Invert triac gate logic. When `true`, GPIO HIGH = gate OFF and GPIO LOW = gate ON. Use when the gate is driven through an NPN transistor with a pull-up resistor |

## Sensors (optional)

```yaml
sensor:
  - platform: phase_cut
    # phase_cut_id: triac_ctrl     # optional if only one instance
    zc_frequency:
      name: "ZC Frequency"          # edges/s, ~100 for healthy 50 Hz
    power:
      name: "Phase Cut Power"       # current power as percentage (0–100%)
```

| Sensor | Unit | Description |
|--------|------|-------------|
| `zc_frequency` | Hz | Zero-cross edge rate. ~100 for 50 Hz mains, ~120 for 60 Hz. Updated every 1 s |
| `power` | % | Current power level as percentage of max_level. Updated on change |

## Number (optional)

```yaml
number:
  - platform: phase_cut
    # phase_cut_id: triac_ctrl     # optional if only one instance
    name: "Phase Cut Level"
```

Exposes a 0–N control (where N = `max_level`) in Home Assistant. Useful
for manual testing and debugging. In normal operation, your control loop
calls `set_power()` from lambdas which also updates this entity.

## Lambda API

All methods are available on the component ID from ESPHome lambdas:

```cpp
// Set power level: 0 = OFF, max_level = 100%
id(triac_ctrl).set_power(7);       // 7/10 = 70%

// Emergency stop: immediate triac OFF + power to 0
id(triac_ctrl).stop();

// Read current power level
int p = id(triac_ctrl).get_power();

// Read ZC frequency (edges/s, updated every 1s)
float f = id(triac_ctrl).get_zc_frequency();

// Check if ZC signal is healthy (80–120 Hz)
bool ok = id(triac_ctrl).is_zc_healthy();

// Read configured max level
int m = id(triac_ctrl).get_max_level();
```

## Phase Delay Calculation

The delay from zero-crossing to gate pulse is:

$$\text{delay} = \frac{(\text{max\_level} - \text{level})}{\text{max\_level}} \times T_{half}$$

Where $T_{half}$ is the measured half-cycle duration (~10 ms at 50 Hz),
tracked via an exponential moving average (7/8 old + 1/8 new).

- `level = max_level` → delay = 0 → gate fires immediately at ZC (full power)
- `level = 1` → delay ≈ 90% of half-cycle → minimal power
- `level = 0` → no gate pulse → triac stays off

A safety margin of 500 µs prevents firing too close to the next
zero-crossing (where the pulse would be useless).

## Gate Pulse Timing

The gate pulse is delivered by a one-shot `esp_timer` with
**ISR dispatch** (`ESP_TIMER_ISR`), ensuring minimal and deterministic
latency (~1–5 µs). The gate pulse width is 50 µs — sufficient for most
triacs.

Required sdkconfig option:

```yaml
esp32:
  framework:
    type: esp-idf
    sdkconfig_options:
      CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD: y
```

## Inverted Gate Logic

Some circuits drive the triac gate through an NPN transistor with pull-up
resistors:

```
GPIO23 ──► NPN base
            │
           R (pull-up to gate supply)
            │
           Triac gate
```

In this topology, GPIO HIGH turns the NPN ON, pulling the gate LOW (triac
OFF). GPIO LOW turns the NPN OFF, letting the pull-up drive the gate HIGH
(triac ON). Set `inverted: true` to account for this.

## Safety Notes

- The component only controls the **triac**. A separate relay should be used
  as a session power switch, managed by your application logic.
- `stop()` forces the triac gate to the OFF state immediately. Call it from
  all safety fault paths.
- `is_zc_healthy()` returns false if the ZC frequency is outside the
  80–120 Hz range, indicating a mains or detector fault.
- The ISR debounce (`min_edge_interval`) prevents false triggers from relay
  contact EMI. The default 5 ms is conservative for 50 Hz mains (half-cycle
  = 10 ms).

## Technical Details

- The ZC ISR is placed in **IRAM** for deterministic latency.
- The gate timer uses `ESP_TIMER_ISR` dispatch — the callback runs directly
  in timer ISR context, not deferred to a task. This gives <5 µs jitter on
  the gate pulse timing.
- `set_power()` is safe to call from any non-ISR context. It performs a
  single atomic write to the ISR-shared variable.
- ZC frequency is computed every 1 s by comparing edge counts in `loop()`.
- Half-cycle tracking uses EMA to smooth jitter while adapting to actual
  mains frequency.
- The component uses `setup_priority::DATA` (600) — runs after GPIO hardware
  init but before sensors start publishing.

## Comparison with Burst Fire

| Feature | Phase Cut | Burst Fire |
|---------|-----------|------------|
| Control method | Delayed gate pulse per half-cycle | Complete half-cycles on/off |
| Power resolution | Continuous (delay-based) | 1/window_size steps |
| EMI/harmonics | Higher (chopped waveform) | Minimal (full cycles only) |
| Flicker | None (every half-cycle has some power) | Possible at low power |
| Gate timing | Critical (µs precision needed) | Simple (on/off at ZC) |
| Use case | Fine proportional control | Simple on/off duty cycling |
