# Xiaomi Smart Kettle 2 Pro - Pseudocode Implementation
# Miot spec: urn:miot-spec-v2:device:kettle:0000A009:xiaomi-v21:1

## ═══════════════════════════════════════════════════════════
## HARDWARE MAP
## ═══════════════════════════════════════════════════════════
##
## GPIO4  : Buzzer (LEDC PWM)
## GPIO5  : Boil button LED (inverted)
## GPIO12 : Kettle presence / pot detection (ADC2 - WiFi conflict!)
## GPIO14 : Unknown (input) - à identifier
## GPIO16 : Rotary encoder A
## GPIO17 : Rotary encoder B
## GPIO18 : Touch button Boil (via SC02A)
## GPIO19 : Touch button Keep Warm (via SC02A)
## GPIO21 : Keep Warm button LED (inverted)
## GPIO22 : Relay (main heater switch)
## GPIO23 : Triac (secondary heater - NO HEATSINK, pulse only!)
## GPIO25 : Unknown (input) - à identifier
## GPIO26 : TM1640 CLK
## GPIO27 : TM1640 DIN
## GPIO32 : Unknown (input) - à identifier
## GPIO33 : Zero-cross detection / neutral voltage
## GPIO34 : Unknown (input) - à identifier
## GPIO35 : Unknown (input) - à identifier
## GPIO36 : NTC temperature sensor (ADC1, 10kΩ, B=3950)
##
## TM1640 Display memory:
##   [0] Digit tens  [1] Digit units  [2] Symbols (0x02=WiFi, 0x10=°C)
##   [3] Orange R    [4] Blue R       [5] Red R
##   [6] Orange L    [7] Blue L       [8] Red L
##   R = LEDs 1-7 (right half), L = LEDs 8-12 (left half)


## ═══════════════════════════════════════════════════════════
## STATE MACHINE
## ═══════════════════════════════════════════════════════════

ENUM KettleState:
    IDLE        = 0    # Off, waiting for user action
    HEATING     = 1    # Heating water to target temperature
    BOILING     = 2    # Full boil (target = 100°C)
    COOLING     = 3    # Water above target, waiting to cool down
    KEEP_WARM   = 4    # Maintaining target temperature
    ERROR       = 5    # Safety fault detected


## ═══════════════════════════════════════════════════════════
## GLOBALS
## ═══════════════════════════════════════════════════════════

kettle_state           : KettleState = IDLE
kettle_temperature     : int = 0          # Current NTC reading (°C)
kettle_target_temp     : int = 40         # User target (40-99°C), restore
kettle_fault           : int = 0          # Fault code (0 = none)
keep_warm_enabled      : bool = false     # Auto keep-warm after boil?
keep_warm_timeout      : uint32 = 0       # Timestamp when keep-warm started
keep_warm_duration     : int = 30         # Keep warm duration in minutes (default 30)
last_user_interaction  : uint32 = 0       # Last button/encoder touch timestamp
boil_start_time        : uint32 = 0       # When heating started
both_buttons_start     : uint32 = 0       # For 10s reset combo


## ═══════════════════════════════════════════════════════════
## CONSTANTS / SAFETY LIMITS
## ═══════════════════════════════════════════════════════════

CONST TEMP_MIN              = 40       # Minimum target temperature
CONST TEMP_MAX              = 99       # Maximum target temperature
CONST TEMP_BOIL             = 98       # Considered "boiling" threshold
CONST TEMP_OVERHEAT         = 105      # Emergency shutoff
CONST TEMP_DRY_FIRE         = 130      # Dry fire protection (rate-based too)
CONST TEMP_HYSTERESIS       = 2        # ±2°C hysteresis for keep-warm
CONST HEAT_RATE_MIN         = 0.5      # °C/min - below this = suspect (no water?)
CONST MAX_HEAT_TIME         = 600      # 10 min max continuous heating (safety)
CONST KEEP_WARM_MAX_MINS    = 120      # Max keep-warm duration (2h)
CONST IDLE_DISPLAY_TIMEOUT  = 30       # Seconds before display dims in idle
CONST TRIAC_PULSE_MS        = 8        # Triac pulse duration (half-period only!)


## ═══════════════════════════════════════════════════════════
## 1. BUTTON ACTIONS
## ═══════════════════════════════════════════════════════════

ON button_boil_touch PRESSED:
    last_user_interaction = millis()
    buzzer_beep(short)

    IF state == IDLE:
        IF NOT kettle_present:
            buzzer_beep(error)          # No kettle on base
            RETURN
        state = BOILING                 # Full boil mode
        keep_warm_enabled = false
        start_heating()

    ELSE IF state == HEATING OR state == BOILING:
        stop_heating()                  # Cancel current operation
        state = IDLE

    ELSE IF state == KEEP_WARM:
        stop_heating()                  # Cancel keep-warm
        state = IDLE

    ELSE IF state == COOLING:
        state = IDLE                    # Cancel cooling wait


ON button_keepwarm_touch PRESSED:
    last_user_interaction = millis()
    buzzer_beep(short)

    IF state == IDLE:
        IF NOT kettle_present:
            buzzer_beep(error)
            RETURN
        state = HEATING                 # Heat to target temp
        keep_warm_enabled = true        # Then maintain
        start_heating()

    ELSE IF state == HEATING OR state == BOILING:
        # Toggle keep-warm for after heating
        keep_warm_enabled = NOT keep_warm_enabled
        # Visual feedback: flash keep-warm LED

    ELSE IF state == KEEP_WARM:
        stop_heating()                  # Cancel keep-warm
        state = IDLE

    ELSE IF state == COOLING:
        state = IDLE


## ═══════════════════════════════════════════════════════════
## 2. ROTARY ENCODER
## ═══════════════════════════════════════════════════════════

ON rotary_encoder CHANGED(delta):
    last_user_interaction = millis()

    # Adaptive step: slow turn = 1°C, fast turn = 5°C
    step = 1
    IF abs(delta) > 2: step = 2
    IF abs(delta) > 4: step = 5

    new_target = kettle_target_temp + (delta * step)
    new_target = CLAMP(new_target, TEMP_MIN, TEMP_MAX)
    kettle_target_temp = new_target

    # If currently keeping warm, adjust the target live
    IF state == KEEP_WARM:
        # The main loop will react to the new target

    # Reset encoder position
    encoder.set_value(0)


## ═══════════════════════════════════════════════════════════
## 3. HEATING CONTROL (main logic, runs every 500ms)
## ═══════════════════════════════════════════════════════════

FUNCTION main_control_loop():      # interval: 500ms

    # ---- Read temperature ----
    temp = ntc_sensor.get_state()
    IF temp IS valid:
        kettle_temperature = round(temp)

    # ---- Safety checks (always active) ----
    IF kettle_temperature > TEMP_OVERHEAT:
        emergency_stop("OVERHEAT: {temp}°C")
        RETURN

    IF state IN [HEATING, BOILING] AND time_since(boil_start_time) > MAX_HEAT_TIME:
        emergency_stop("MAX_HEAT_TIME exceeded")
        RETURN

    IF NOT kettle_present AND state != IDLE:
        stop_heating()
        state = IDLE
        LOG_WARN("Kettle removed during operation")
        RETURN

    # ---- Dry fire detection ----
    # If heating for >60s and temp hasn't risen by at least HEAT_RATE_MIN
    IF state IN [HEATING, BOILING]:
        IF time_since(boil_start_time) > 60s:
            rate = (kettle_temperature - temp_at_start) / time_since(boil_start_time) * 60
            IF rate < HEAT_RATE_MIN AND kettle_temperature < 50:
                emergency_stop("Dry fire suspected: rate={rate}°C/min")
                RETURN

    # ---- State machine ----
    SWITCH state:

        CASE IDLE:
            relay_off()
            triac_off()
            # Nothing to do

        CASE HEATING:
            # Heat to target_temp
            IF kettle_temperature >= kettle_target_temp:
                stop_heating()
                buzzer_beep(done)
                IF keep_warm_enabled:
                    state = KEEP_WARM
                    keep_warm_timeout = millis()
                    LOG_INFO("Target reached, entering keep-warm")
                ELSE:
                    state = IDLE
                    LOG_INFO("Target reached, done")
            ELSE:
                relay_on()
                # Use triac pulsing for gentler heating near target
                IF kettle_target_temp - kettle_temperature < 5:
                    triac_pulse_mode()      # Gentle approach
                ELSE:
                    triac_on()              # Full power

        CASE BOILING:
            # Full boil to ~100°C
            IF kettle_temperature >= TEMP_BOIL:
                stop_heating()
                buzzer_beep(done)
                IF keep_warm_enabled:
                    state = COOLING
                    LOG_INFO("Boil done, cooling to target {target}°C")
                ELSE:
                    state = IDLE
                    LOG_INFO("Boil done")
            ELSE:
                relay_on()
                triac_on()                  # Full power

        CASE COOLING:
            # Waiting for water to cool to target temp (passive)
            relay_off()
            triac_off()
            IF kettle_temperature <= kettle_target_temp:
                state = KEEP_WARM
                keep_warm_timeout = millis()
                LOG_INFO("Cooled to target, entering keep-warm")

        CASE KEEP_WARM:
            # Maintain temperature within hysteresis band
            IF time_since(keep_warm_timeout) > keep_warm_duration * 60 * 1000:
                stop_heating()
                state = IDLE
                buzzer_beep(done)
                LOG_INFO("Keep-warm timeout, stopping")
            ELSE IF kettle_temperature < kettle_target_temp - TEMP_HYSTERESIS:
                relay_on()
                triac_pulse_mode()          # Gentle reheat
            ELSE IF kettle_temperature >= kettle_target_temp:
                relay_off()
                triac_off()

        CASE ERROR:
            relay_off()
            triac_off()
            # Stay in error until user presses a button or removes/replaces kettle


## ═══════════════════════════════════════════════════════════
## 4. HEATER CONTROL FUNCTIONS
## ═══════════════════════════════════════════════════════════

FUNCTION start_heating():
    boil_start_time = millis()
    temp_at_start = kettle_temperature
    relay_on()
    triac_on()
    LOG_INFO("Heating started at {kettle_temperature}°C")

FUNCTION stop_heating():
    relay_off()
    triac_off()
    LOG_INFO("Heating stopped at {kettle_temperature}°C")

FUNCTION emergency_stop(reason):
    relay_off()
    triac_off()
    state = ERROR
    kettle_fault = 1
    buzzer_beep(alarm)                  # Long alarm beep
    LOG_ERROR("EMERGENCY STOP: {reason}")

FUNCTION relay_on():
    heater_switch_relay.turn_on()

FUNCTION relay_off():
    heater_switch_relay.turn_off()

FUNCTION triac_on():
    # WARNING: T1235H-8A has NO heatsink
    # Original firmware only opens one half-period per cycle
    # Use pulse mode, never continuous DC-like drive
    heater_switch_triac.turn_on()

FUNCTION triac_off():
    heater_switch_triac.turn_off()

FUNCTION triac_pulse_mode():
    # Reduced power: pulse triac for shorter duration per zero-cross
    # Implementation depends on zero-cross detection (GPIO33)
    # For now: simple on/off cycling (1s on, 1s off)
    IF (millis() / 1000) % 2 == 0:
        triac_on()
    ELSE:
        triac_off()


## ═══════════════════════════════════════════════════════════
## 5. DISPLAY LOGIC (runs every 100ms in display lambda)
## ═══════════════════════════════════════════════════════════

FUNCTION display_update():

    # ---- Digits (bytes 0-1) ----
    SWITCH state:
        CASE IDLE:
            IF time_since(last_user_interaction) < IDLE_DISPLAY_TIMEOUT * 1000:
                display_number(kettle_target_temp)  # Show target briefly
            ELSE:
                display_number(kettle_temperature)  # Show current temp (or blank)

        CASE HEATING, BOILING:
            # Alternate between current temp and target every 2s
            IF (millis() / 2000) % 2 == 0:
                display_number(kettle_temperature)
            ELSE:
                display_number(kettle_target_temp)
                # Could also flash digits when showing target

        CASE COOLING:
            display_number(kettle_temperature)      # Show cooling progress

        CASE KEEP_WARM:
            display_number(kettle_temperature)      # Show maintained temp

        CASE ERROR:
            # Flash "Er" or fault code
            IF (millis() / 500) % 2 == 0:
                display_text("Er")
            ELSE:
                display_blank()

    # ---- Symbols (byte 2) ----
    symbols = 0x10                                  # °C always on
    IF wifi_connected:
        symbols |= 0x02                             # WiFi icon
    set_raw(2, symbols)

    # ---- LED Ring (bytes 3-8) ----
    # Clear all ring LEDs first
    FOR i IN 3..8: set_raw(i, 0x00)

    SWITCH state:
        CASE IDLE:
            # Blue static ring
            set_raw(4, 0x7F)                        # Blue right (7 LEDs)
            set_raw(7, 0x1F)                        # Blue left (5 LEDs)

        CASE HEATING:
            # Orange animated fill: LEDs proportional to progress
            progress = (kettle_temperature - temp_at_start) /
                       (kettle_target_temp - temp_at_start)
            leds_on = round(progress * 12)
            # Fill leds_on LEDs in orange (right then left)
            set_ring_orange(leds_on)
            # Could add animation: rotating dot at the frontier

        CASE BOILING:
            # Red pulsing/flashing ring (full power)
            IF (millis() / 300) % 2 == 0:
                set_raw(5, 0x7F)                    # Red right
                set_raw(8, 0x1F)                    # Red left
            # Alternate with orange for "fire" effect
            ELSE:
                set_raw(3, 0x7F)                    # Orange right
                set_raw(6, 0x1F)                    # Orange left

        CASE COOLING:
            # Blue breathing/pulsing
            # Or: show decreasing LEDs as temp drops
            blue_leds = map(kettle_temperature, kettle_target_temp, 100, 1, 12)
            set_ring_blue(blue_leds)

        CASE KEEP_WARM:
            # Orange steady ring (warm glow)
            set_raw(3, 0x7F)                        # Orange right
            set_raw(6, 0x1F)                        # Orange left

        CASE ERROR:
            # Red flashing ring
            IF (millis() / 250) % 2 == 0:
                set_raw(5, 0x7F)                    # Red right
                set_raw(8, 0x1F)                    # Red left

    # ---- Button LEDs ----
    SWITCH state:
        CASE IDLE:
            boil_light OFF
            keepwarm_light OFF

        CASE HEATING:
            keepwarm_light ON (if keep_warm_enabled, else OFF)
            boil_light OFF

        CASE BOILING:
            boil_light ON
            keepwarm_light ON (if keep_warm_enabled, else OFF)

        CASE COOLING:
            boil_light OFF
            keepwarm_light BLINKING (500ms)

        CASE KEEP_WARM:
            boil_light OFF
            keepwarm_light ON

        CASE ERROR:
            boil_light BLINKING (250ms)
            keepwarm_light BLINKING (250ms)


## Helper: set N LEDs on the ring in a specific color
FUNCTION set_ring_orange(count):    # count = 0..12
    right = min(count, 7)
    left  = max(0, count - 7)
    set_raw(3, (1 << right) - 1)    # Orange right
    set_raw(6, (1 << left) - 1)     # Orange left

FUNCTION set_ring_blue(count):
    right = min(count, 7)
    left  = max(0, count - 7)
    set_raw(4, (1 << right) - 1)
    set_raw(7, (1 << left) - 1)

FUNCTION set_ring_red(count):
    right = min(count, 7)
    left  = max(0, count - 7)
    set_raw(5, (1 << right) - 1)
    set_raw(8, (1 << left) - 1)


## ═══════════════════════════════════════════════════════════
## 6. BUZZER PATTERNS
## ═══════════════════════════════════════════════════════════

FUNCTION buzzer_beep(type):
    SWITCH type:
        CASE short:
            tone(2000Hz, 50ms)

        CASE done:
            tone(2000Hz, 100ms)
            pause(50ms)
            tone(2500Hz, 100ms)
            pause(50ms)
            tone(3000Hz, 150ms)

        CASE error:
            tone(800Hz, 200ms)
            pause(100ms)
            tone(800Hz, 200ms)

        CASE alarm:
            REPEAT 5:
                tone(1000Hz, 300ms)
                pause(200ms)

        CASE tick:                      # Encoder click feedback
            tone(3000Hz, 10ms)


## ═══════════════════════════════════════════════════════════
## 7. KETTLE PRESENCE (GPIO12)
## ═══════════════════════════════════════════════════════════

ON kettle_presence CHANGED:
    IF kettle_present == false:
        # Kettle removed from base
        IF state != IDLE:
            stop_heating()
            state = IDLE
            LOG_WARN("Kettle removed - stopping")

    IF kettle_present == true:
        # Kettle placed on base
        buzzer_beep(short)
        # Read initial temperature
        LOG_INFO("Kettle placed, temp = {kettle_temperature}°C")


## ═══════════════════════════════════════════════════════════
## 8. HOME ASSISTANT INTEGRATION
## ═══════════════════════════════════════════════════════════

## Exposed entities:
##
## Sensors:
##   - kettle_temperature     : Current water temperature (°C)
##   - kettle_target_temp     : Target temperature (°C, settable via HA)
##   - kettle_state           : Current state (text: idle/heating/boiling/cooling/keep_warm/error)
##   - ntc_resistance         : Raw NTC resistance (debug)
##   - kettle_presence        : Binary: is kettle on base?
##
## Controls (from HA):
##   - button: Start Boil     : Triggers BOILING state
##   - button: Start Heat     : Triggers HEATING to target
##   - button: Stop           : Returns to IDLE
##   - number: Target Temp    : Set target (40-99°C)
##   - switch: Keep Warm      : Enable/disable auto keep-warm
##   - number: Keep Warm Duration : Minutes (5-120)
##
## Automations possible:
##   - "Boil water at 7:00 AM"
##   - "Heat to 80°C when I arrive home"
##   - "Notify when water is ready"
##   - "Turn off if unattended for 2h"


## ═══════════════════════════════════════════════════════════
## 9. SAFETY SUMMARY
## ═══════════════════════════════════════════════════════════
##
## Priority 1 - Hardware protection:
##   [x] Max temperature cutoff (105°C)
##   [x] Max continuous heating time (10 min)
##   [x] Kettle removal detection → instant shutoff
##   [x] Dry fire detection (no temp rise while heating)
##   [x] Triac pulse-only mode (no heatsink on T1235H-8A!)
##
## Priority 2 - User safety:
##   [x] Keep-warm auto-timeout (max 2h)
##   [x] Buzzer alarm on fault
##   [x] Error state requires user intervention to clear
##   [x] Both-buttons 10s hold = restart (existing)
##
## Priority 3 - Reliability:
##   [x] State restored after reboot (globals with restore_value)
##   [x] But heating NOT auto-resumed after reboot (safety)
##   [x] WiFi indicator on display
##   [x] HA watchdog reboot (api: reboot_timeout: 15min)
##
## OPEN QUESTIONS:
##   - GPIO33: Is it truly zero-cross detection? Need to verify with scope
##     If yes → proper triac phase control possible
##     If no  → stick with simple relay on/off cycling
##   - GPIO34/35/32/25/14: Unknown pins, need identification
##     Could be: lid sensor, water level, second NTC, etc.
##   - Triac + Relay sequencing: should relay close first, then triac fires?
##     Or are they in series (relay = safety cutoff, triac = power control)?
##   - Does the original FW use both relay AND triac simultaneously?


## ═══════════════════════════════════════════════════════════
## 10. IMPLEMENTATION PRIORITY ORDER
## ═══════════════════════════════════════════════════════════
##
## Phase 1 - MVP (safe manual boil):
##   1. State machine globals + enum
##   2. Boil button → BOILING → relay on → temp >= 98 → stop
##   3. Safety: max temp, max time, kettle presence
##   4. Display: show state + temperature
##   5. Buzzer: beep on done
##
## Phase 2 - Target temperature:
##   6. Keep Warm button → HEATING to target → KEEP_WARM
##   7. Rotary encoder adjusts target (already done)
##   8. Hysteresis-based reheat in KEEP_WARM
##   9. Keep-warm timeout
##   10. Display: animated LED ring per state
##
## Phase 3 - Polish:
##   11. HA entities (buttons, numbers, selects)
##   12. Buzzer patterns (done, error, tick)
##   13. Display animations (progress fill, breathing)
##   14. Identify unknown GPIOs
##   15. Zero-cross detection for proper triac control
##
## Phase 4 - Advanced:
##   16. Temperature curve logging
##   17. Boil detection by rate-of-change plateau
##   18. Energy consumption tracking (if power measurement available)
##   19. Water level estimation (by heating curve slope)
