# Burst Fire Triac Controller

ESPHome component for zero-cross synchronized burst-fire power control of
resistive loads (heaters, kettles, etc.) via a triac.

## How It Works

Burst-fire (also called integral cycle control) switches a triac ON or OFF
only at mains zero-crossings. Instead of chopping each half-cycle (phase
control), it delivers **complete half-cycles** in groups, producing no
high-frequency harmonics and no audible buzzing.

A configurable **window** of N half-cycles repeats continuously. Within each
window, the first M half-cycles are powered and the remaining N−M are off.
Power = M/N.

```
Window = 10 half-cycles, Power = 3/10 (30%)

ZC edges:  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  | 10  |  1  | ...
Triac:      ON    ON    ON    OFF   OFF   OFF   OFF   OFF   OFF   OFF   ON   ...
           └── 3 ON ──┘└────────── 7 OFF ─────────────┘
           = 30% average power
```

At 50 Hz mains, one half-cycle is 10 ms. A window of 10 = 100 ms. The load
sees full mains voltage during ON half-cycles and zero during OFF. Thermal
mass of the heater smooths the output.

## Hardware Requirements

| Signal | GPIO | Direction | Description |
|--------|------|-----------|-------------|
| Zero-cross | Input | IN | Optocoupler output, ~100 Hz square wave (both edges of 50 Hz mains) |
| Triac gate | Output | OUT | Drives triac gate driver (e.g. MOC3021 or direct) |

The zero-cross detector must be **mains-referenced** (always active regardless
of relay/triac state). The component expects ~100 edges/s for 50 Hz mains or
~120 edges/s for 60 Hz.

Typical circuit topology:

```
Mains ──┬── ZC optocoupler ── GPIO (zero_cross_pin)
        │
        └── Relay ── Triac ── Load
              │         │
           (session)  (burst_fire controls this via triac_pin)
```

## Installation

Place the component directory alongside your ESPHome config:

```
config/
├── smart-kettle-kitchen.yaml
└── components/
    └── burst_fire/
        ├── __init__.py
        ├── sensor.py
        ├── number.py
        ├── burst_fire.h
        └── burst_fire.cpp
```

In your YAML:

```yaml
external_components:
  - source:
      type: local
      path: components
    components: [burst_fire]
```

## Configuration

```yaml
burst_fire:
  id: burst_ctrl                    # optional, auto-generated
  zero_cross_pin: GPIO33            # REQUIRED — ZC detector input
  triac_pin: GPIO23                 # REQUIRED — triac gate output
  window_size: 10                   # optional, default 10 half-cycles
  min_edge_interval: 5ms            # optional, default 5ms (EMI debounce)
  dry_run: false                    # optional, default false
```

### Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `zero_cross_pin` | pin | *required* | GPIO connected to zero-cross detector output |
| `triac_pin` | pin | *required* | GPIO connected to triac gate driver |
| `window_size` | int (2–100) | `10` | Number of half-cycles per burst window. Power resolution = 1/window_size |
| `min_edge_interval` | time | `5ms` | Minimum time between valid ZC edges. Edges closer than this are rejected (relay EMI debounce). Must be less than one half-cycle (~10 ms at 50 Hz) |
| `dry_run` | bool | `false` | When true, ISR runs and counts edges but never drives the triac HIGH. Useful for testing without actual heating |

## Sensors (optional)

```yaml
sensor:
  - platform: burst_fire
    # burst_fire_id: burst_ctrl     # optional if only one instance
    zc_frequency:
      name: "ZC Frequency"          # edges/s, ~100 for healthy 50 Hz
    power:
      name: "Burst Power"           # current power as percentage (0–100%)
```

| Sensor | Unit | Description |
|--------|------|-------------|
| `zc_frequency` | Hz | Zero-cross edge rate. ~100 for 50 Hz mains, ~120 for 60 Hz. Updated every 1 s |
| `power` | % | Current power level as percentage of window_size. Updated on change |

## Number (optional)

```yaml
number:
  - platform: burst_fire
    # burst_fire_id: burst_ctrl     # optional if only one instance
    name: "Burst Power Level"
```

Exposes a 0–N control (where N = `window_size`) in Home Assistant. Useful
for manual testing and debugging. In normal operation, your control loop
calls `set_power()` from lambdas which also updates this entity.

## Lambda API

All methods are available on the component ID from ESPHome lambdas:

```cpp
// Set power level: 0 = OFF, window_size = 100%
id(burst_ctrl).set_power(7);       // 7/10 = 70%

// Emergency stop: immediate triac OFF + power to 0
id(burst_ctrl).stop();

// Read current power level (half-cycles per window)
int p = id(burst_ctrl).get_power();

// Read ZC frequency (edges/s, updated every 1s)
float f = id(burst_ctrl).get_zc_frequency();

// Check if ZC signal is healthy (80–120 Hz)
bool ok = id(burst_ctrl).is_zc_healthy();

// Read configured window size
int w = id(burst_ctrl).get_window_size();
```

## Safety Notes

- The component only controls the **triac**. A separate relay should be used
  as a session power switch, managed by your application logic.
- `stop()` forces the triac LOW immediately without waiting for the next
  zero-crossing. Call it from all safety fault paths.
- The `dry_run` mode lets you verify ISR operation and ZC signal quality
  without energizing the load.
- `is_zc_healthy()` returns false if the ZC frequency is outside the
  80–120 Hz range, indicating a mains or detector fault.
- The ISR debounce (`min_edge_interval`) prevents false triggers from relay
  contact EMI. The default 5 ms is conservative for 50 Hz mains (half-cycle
  = 10 ms).

## Technical Details

- The ISR is placed in **IRAM** for deterministic latency (<10 µs from ZC
  edge to triac state change).
- GPIO is driven via ESPHome's `ISRInternalGPIOPin::digital_write()` which
  calls `gpio_set_level()` — ISR-safe on ESP32.
- `set_power()` is safe to call from any non-ISR context. It performs a
  single atomic write to the ISR-shared variable.
- ZC frequency is computed every 1 s by comparing edge counts in `loop()`.
- The component uses `setup_priority::DATA` (600) — runs after GPIO hardware
  init but before sensors start publishing.
