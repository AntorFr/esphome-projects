#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif

#include "esp_timer.h"

namespace esphome {
namespace phase_cut {

/// ISR-safe store — plain struct, no virtuals, safe for IRAM.
struct PhaseCutStore {
  ISRInternalGPIOPin isr_triac_pin_;

  volatile int power_level_{0};   // 0 = off, max_level = full
  int max_level_{10};

  // Measured half-cycle duration (µs), ~10000 for 50 Hz
  volatile uint32_t half_cycle_us_{10000};

  // Inverted gate logic (true = GPIO HIGH turns triac OFF)
  bool inverted_{false};

  // ZC edge tracking
  uint32_t min_edge_us_{5000};
  uint32_t last_edge_us_{0};
  volatile uint32_t edge_count_{0};

  // Gate timer handle (one-shot, ISR dispatch)
  esp_timer_handle_t gate_timer_{nullptr};

  /// ZC ISR: on each zero-crossing, schedule a delayed gate pulse.
  static void IRAM_ATTR gpio_intr(PhaseCutStore *arg);

  /// Timer ISR: fire a short gate pulse to trigger the triac.
  static void IRAM_ATTR gate_timer_cb(void *arg);
};

class PhaseCutController;

#ifdef USE_NUMBER
class PhaseCutNumber : public number::Number, public Component {
 public:
  void set_parent(PhaseCutController *parent) { parent_ = parent; }
  void dump_config() override {}

 protected:
  void control(float value) override;
  PhaseCutController *parent_{nullptr};
};
#endif

class PhaseCutController : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // Configuration (called from generated code)
  void set_zero_cross_pin(InternalGPIOPin *pin) { zc_pin_ = pin; }
  void set_triac_pin(InternalGPIOPin *pin) { triac_pin_ = pin; }
  void set_max_level(int max) { store_.max_level_ = max; }
  void set_min_edge_interval(uint32_t us) { store_.min_edge_us_ = us; }
  void set_inverted(bool inv) { store_.inverted_ = inv; }

  // Runtime API (call from lambdas)
  void set_power(int level);
  int get_power() const { return store_.power_level_; }
  void stop();
  float get_zc_frequency() const { return zc_frequency_; }
  bool is_zc_healthy() const { return zc_frequency_ >= 80.0f && zc_frequency_ <= 120.0f; }
  int get_max_level() const { return store_.max_level_; }

#ifdef USE_SENSOR
  void set_zc_frequency_sensor(sensor::Sensor *s) { zc_frequency_sensor_ = s; }
  void set_power_sensor(sensor::Sensor *s) { power_sensor_ = s; }
#endif

#ifdef USE_NUMBER
  void set_power_number(PhaseCutNumber *n) { power_number_ = n; }
#endif

 protected:
  InternalGPIOPin *zc_pin_{nullptr};
  InternalGPIOPin *triac_pin_{nullptr};
  PhaseCutStore store_{};

  uint32_t prev_edge_count_{0};
  float zc_frequency_{0.0f};
  uint32_t last_update_ms_{0};

#ifdef USE_SENSOR
  sensor::Sensor *zc_frequency_sensor_{nullptr};
  sensor::Sensor *power_sensor_{nullptr};
#endif

#ifdef USE_NUMBER
  PhaseCutNumber *power_number_{nullptr};
#endif
};

}  // namespace phase_cut
}  // namespace esphome
