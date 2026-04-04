#include "burst_fire.h"
#include "esphome/core/log.h"

#include "esp_timer.h"

namespace esphome {
namespace burst_fire {

static const char *const TAG = "burst_fire";

// =============================================================
// ISR Handler (IRAM — no logging, no allocation)
// =============================================================

void IRAM_ATTR BurstFireStore::gpio_intr(BurstFireStore *arg) {
  const uint32_t now = (uint32_t) esp_timer_get_time();

  // Debounce: reject edges closer than min_edge_us (relay EMI protection)
  const uint32_t dt = now - arg->last_edge_us_;
  if (dt < arg->min_edge_us_) {
    return;
  }
  arg->last_edge_us_ = now;
  arg->edge_count_++;

  // Burst-fire: determine if triac should be ON for this half-cycle
  const int pos = arg->window_pos_;
  const bool fire = (pos < arg->active_half_cycles_) && !arg->dry_run_;

  // Drive triac GPIO at zero-crossing (minimal latency)
  arg->isr_triac_pin_.digital_write(fire);

  // Advance window position, wrap at window boundary
  arg->window_pos_ = (pos + 1 >= arg->window_size_) ? 0 : pos + 1;
}

// =============================================================
// Component lifecycle
// =============================================================

void BurstFireController::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Burst Fire Controller...");

  // Triac pin: configure as output, ensure LOW at startup
  this->triac_pin_->setup();
  this->triac_pin_->digital_write(false);
  this->store_.isr_triac_pin_ = this->triac_pin_->to_isr();

  // Zero-cross pin: configure as input, attach ISR on both edges
  this->zc_pin_->setup();
  this->zc_pin_->attach_interrupt(BurstFireStore::gpio_intr, &this->store_,
                                   gpio::INTERRUPT_ANY_EDGE);

#ifdef USE_NUMBER
  if (this->power_number_ != nullptr) {
    this->power_number_->traits.set_max_value(this->store_.window_size_);
  }
#endif

  this->last_update_ms_ = millis();
  ESP_LOGI(TAG, "ISR installed — window=%d, debounce=%u us, dry_run=%s",
           this->store_.window_size_, this->store_.min_edge_us_,
           YESNO(this->store_.dry_run_));
}

void BurstFireController::loop() {
  const uint32_t now = millis();
  if (now - this->last_update_ms_ < 1000) {
    return;
  }
  this->last_update_ms_ = now;

  // Compute ZC frequency: edges counted in the last second
  const uint32_t cur = this->store_.edge_count_;
  this->zc_frequency_ = (float) (cur - this->prev_edge_count_);
  this->prev_edge_count_ = cur;

#ifdef USE_SENSOR
  if (this->zc_frequency_sensor_ != nullptr) {
    this->zc_frequency_sensor_->publish_state(this->zc_frequency_);
  }
#endif
}

void BurstFireController::dump_config() {
  ESP_LOGCONFIG(TAG, "Burst Fire Controller:");
  LOG_PIN("  Zero-Cross Pin: ", this->zc_pin_);
  LOG_PIN("  Triac Pin: ", this->triac_pin_);
  ESP_LOGCONFIG(TAG, "  Window Size: %d half-cycles", this->store_.window_size_);
  ESP_LOGCONFIG(TAG, "  Min Edge Interval: %u us", this->store_.min_edge_us_);
  ESP_LOGCONFIG(TAG, "  Dry Run: %s", YESNO(this->store_.dry_run_));
}

// =============================================================
// Runtime API
// =============================================================

void BurstFireController::set_power(int level) {
  if (level < 0)
    level = 0;
  if (level > this->store_.window_size_)
    level = this->store_.window_size_;

  const int prev = this->store_.active_half_cycles_;
  this->store_.active_half_cycles_ = level;

  // When turning OFF, force triac LOW immediately (don't wait for next ZC edge)
  if (level == 0) {
    this->triac_pin_->digital_write(false);
  }

  if (level != prev) {
    ESP_LOGD(TAG, "Power: %d/%d (%d%%)", level, this->store_.window_size_,
             level * 100 / this->store_.window_size_);

#ifdef USE_SENSOR
    if (this->power_sensor_ != nullptr) {
      this->power_sensor_->publish_state(
          (float) level * 100.0f / (float) this->store_.window_size_);
    }
#endif

#ifdef USE_NUMBER
    if (this->power_number_ != nullptr) {
      this->power_number_->publish_state((float) level);
    }
#endif
  }
}

void BurstFireController::stop() {
  this->store_.active_half_cycles_ = 0;
  this->triac_pin_->digital_write(false);
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

void BurstFireNumber::control(float value) {
  if (this->parent_ != nullptr) {
    this->parent_->set_power((int) value);
  }
  this->publish_state(value);
}

void BurstFireNumber::dump_config() {
  LOG_NUMBER("  ", "Power Level", this);
}

#endif

}  // namespace burst_fire
}  // namespace esphome
