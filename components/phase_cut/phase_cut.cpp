#include "phase_cut.h"
#include "esphome/core/log.h"

#include "esp_timer.h"
#include "esp_rom_sys.h"

namespace esphome {
namespace phase_cut {

static const char *const TAG = "phase_cut";

// Gate pulse width in µs — enough to trigger most triacs
static const uint32_t GATE_PULSE_US = 50;

// Safety margin: don't fire within 500 µs of the next ZC (pulse would be useless)
static const uint32_t ZC_MARGIN_US = 500;

// =============================================================
// Timer callback (IRAM — fires the triac gate after phase delay)
// =============================================================

void IRAM_ATTR PhaseCutStore::gate_timer_cb(void *arg) {
  PhaseCutStore *store = static_cast<PhaseCutStore *>(arg);
  const bool inv = store->inverted_;
  // Pulse: briefly enable gate, then disable
  store->isr_triac_pin_.digital_write(!inv);
  esp_rom_delay_us(GATE_PULSE_US);
  store->isr_triac_pin_.digital_write(inv);
}

// =============================================================
// ZC ISR (IRAM — triggered on each zero-crossing edge)
// =============================================================

void IRAM_ATTR PhaseCutStore::gpio_intr(PhaseCutStore *arg) {
  const uint32_t now = (uint32_t) esp_timer_get_time();

  // Debounce: reject edges closer than min_edge_us
  const uint32_t dt = now - arg->last_edge_us_;
  if (dt < arg->min_edge_us_) {
    return;
  }
  arg->last_edge_us_ = now;
  arg->edge_count_++;

  // Track half-cycle duration with exponential moving average
  // (7/8 old + 1/8 new) — smooth out jitter while adapting to freq changes
  arg->half_cycle_us_ = (arg->half_cycle_us_ * 7 + dt) / 8;

  // Cancel any pending gate timer from the previous half-cycle
  esp_timer_stop(arg->gate_timer_);

  // Triac naturally commutates off at zero-crossing, no need to drive LOW

  const int level = arg->power_level_;
  const int max_level = arg->max_level_;

  if (level <= 0) {
    // No gate pulse → triac stays off for this half-cycle
    return;
  }

  if (level >= max_level) {
    // Full power: fire immediately at zero-crossing
    const bool inv = arg->inverted_;
    arg->isr_triac_pin_.digital_write(!inv);
    esp_rom_delay_us(GATE_PULSE_US);
    arg->isr_triac_pin_.digital_write(inv);
    return;
  }

  // Phase delay: higher level → shorter delay → more power
  // level=1 (min) → longest delay (fire late → little power)
  // level=max-1   → shortest delay (fire early → most power)
  const uint32_t hc = arg->half_cycle_us_;
  uint32_t delay_us = (uint32_t)(max_level - level) * hc / (uint32_t) max_level;

  // Clamp: don't fire too close to next ZC (pointless, nearly zero energy)
  if (delay_us > hc - ZC_MARGIN_US) {
    delay_us = hc - ZC_MARGIN_US;
  }
  // Minimum 1 µs delay for esp_timer
  if (delay_us < 1) {
    delay_us = 1;
  }

  esp_timer_start_once(arg->gate_timer_, delay_us);
}

// =============================================================
// Component lifecycle
// =============================================================

void PhaseCutController::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Phase Cut Controller...");

  // Triac pin: output, start with gate OFF
  this->triac_pin_->setup();
  this->triac_pin_->digital_write(this->store_.inverted_);
  this->store_.isr_triac_pin_ = this->triac_pin_->to_isr();

  // Create gate timer with ISR dispatch for minimal latency
  const esp_timer_create_args_t timer_args = {
      .callback = PhaseCutStore::gate_timer_cb,
      .arg = &this->store_,
      .dispatch_method = ESP_TIMER_ISR,
      .name = "phase_cut_gate",
  };
  esp_err_t err = esp_timer_create(&timer_args, &this->store_.gate_timer_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create gate timer: %s", esp_err_to_name(err));
    this->mark_failed();
    return;
  }

  // ZC pin: input, ISR on both edges (full-wave detection)
  this->zc_pin_->setup();
  this->store_.last_edge_us_ = (uint32_t) esp_timer_get_time();
  this->zc_pin_->attach_interrupt(PhaseCutStore::gpio_intr, &this->store_,
                                   gpio::INTERRUPT_ANY_EDGE);

#ifdef USE_NUMBER
  if (this->power_number_ != nullptr) {
    this->power_number_->traits.set_max_value(this->store_.max_level_);
  }
#endif

  this->last_update_ms_ = millis();
  ESP_LOGI(TAG, "Phase cut ready — max_level=%d, debounce=%u µs",
           this->store_.max_level_, this->store_.min_edge_us_);
}

void PhaseCutController::loop() {
  const uint32_t now = millis();
  if (now - this->last_update_ms_ < 1000) {
    return;
  }
  this->last_update_ms_ = now;

  const uint32_t cur = this->store_.edge_count_;
  this->zc_frequency_ = (float) (cur - this->prev_edge_count_);
  this->prev_edge_count_ = cur;

#ifdef USE_SENSOR
  if (this->zc_frequency_sensor_ != nullptr) {
    this->zc_frequency_sensor_->publish_state(this->zc_frequency_);
  }
#endif
}

void PhaseCutController::dump_config() {
  ESP_LOGCONFIG(TAG, "Phase Cut Controller:");
  LOG_PIN("  Zero-Cross Pin: ", this->zc_pin_);
  LOG_PIN("  Triac Pin: ", this->triac_pin_);
  ESP_LOGCONFIG(TAG, "  Max Level: %d", this->store_.max_level_);
  ESP_LOGCONFIG(TAG, "  Inverted: %s", this->store_.inverted_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Min Edge Interval: %u µs", this->store_.min_edge_us_);
}

// =============================================================
// Runtime API
// =============================================================

void PhaseCutController::set_power(int level) {
  if (level < 0)
    level = 0;
  if (level > this->store_.max_level_)
    level = this->store_.max_level_;

  const int prev = this->store_.power_level_;
  this->store_.power_level_ = level;

  // When turning OFF, cancel any pending gate timer and force gate OFF
  if (level == 0) {
    esp_timer_stop(this->store_.gate_timer_);
    this->triac_pin_->digital_write(this->store_.inverted_);
  }

  if (level != prev) {
    ESP_LOGD(TAG, "Power: %d/%d (%d%%)", level, this->store_.max_level_,
             level * 100 / this->store_.max_level_);

#ifdef USE_SENSOR
    if (this->power_sensor_ != nullptr) {
      this->power_sensor_->publish_state(
          (float) level * 100.0f / (float) this->store_.max_level_);
    }
#endif

#ifdef USE_NUMBER
    if (this->power_number_ != nullptr) {
      this->power_number_->publish_state((float) level);
    }
#endif
  }
}

void PhaseCutController::stop() {
  this->store_.power_level_ = 0;
  esp_timer_stop(this->store_.gate_timer_);
  this->triac_pin_->digital_write(this->store_.inverted_);
  ESP_LOGW(TAG, "Emergency stop — triac OFF");

#ifdef USE_SENSOR
  if (this->power_sensor_ != nullptr) {
    this->power_sensor_->publish_state(0.0f);
  }
#endif

#ifdef USE_NUMBER
  if (this->power_number_ != nullptr) {
    this->power_number_->publish_state(0.0f);
  }
#endif
}

// =============================================================
// Number sub-component
// =============================================================

#ifdef USE_NUMBER

void PhaseCutNumber::control(float value) {
  if (this->parent_ != nullptr) {
    this->parent_->set_power((int) value);
  }
  this->publish_state(value);
}

#endif

}  // namespace phase_cut
}  // namespace esphome
