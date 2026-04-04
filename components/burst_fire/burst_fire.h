#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif

namespace esphome {
namespace burst_fire {

/// ISR-safe store — no virtual methods, plain struct safe to pass to IRAM handler.
struct BurstFireStore {
  /// ISR-safe triac pin reference for direct GPIO writes from ISR context.
  ISRInternalGPIOPin isr_triac_pin_;

  volatile uint32_t last_edge_us_{0};
  volatile uint32_t edge_count_{0};
  volatile int window_pos_{0};
  volatile int active_half_cycles_{0};

  int window_size_{10};
  uint32_t min_edge_us_{5000};
  bool dry_run_{false};

  static void IRAM_ATTR gpio_intr(BurstFireStore *arg);
};

class BurstFireController;

#ifdef USE_NUMBER
class BurstFireNumber : public number::Number, public Component {
 public:
  void set_parent(BurstFireController *parent) { parent_ = parent; }
  void dump_config() override;

 protected:
  void control(float value) override;
  BurstFireController *parent_{nullptr};
};
#endif

class BurstFireController : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // --- Configuration (called from generated code) ---
  void set_zero_cross_pin(InternalGPIOPin *pin) { zc_pin_ = pin; }
  void set_triac_pin(InternalGPIOPin *pin) { triac_pin_ = pin; }
  void set_window_size(int size) { store_.window_size_ = size; }
  void set_min_edge_interval(uint32_t us) { store_.min_edge_us_ = us; }
  void set_dry_run(bool dry_run) { store_.dry_run_ = dry_run; }

  // --- Runtime API (call from lambdas) ---
  void set_power(int level);
  int get_power() const { return store_.active_half_cycles_; }
  void stop();
  float get_zc_frequency() const { return zc_frequency_; }
  bool is_zc_healthy() const { return zc_frequency_ >= 80.0f && zc_frequency_ <= 120.0f; }
  int get_window_size() const { return store_.window_size_; }

#ifdef USE_SENSOR
  void set_zc_frequency_sensor(sensor::Sensor *s) { zc_frequency_sensor_ = s; }
  void set_power_sensor(sensor::Sensor *s) { power_sensor_ = s; }
#endif

#ifdef USE_NUMBER
  void set_power_number(BurstFireNumber *n) { power_number_ = n; }
#endif

 protected:
  InternalGPIOPin *zc_pin_{nullptr};
  InternalGPIOPin *triac_pin_{nullptr};
  BurstFireStore store_{};

  uint32_t prev_edge_count_{0};
  float zc_frequency_{0.0f};
  uint32_t last_update_ms_{0};

#ifdef USE_SENSOR
  sensor::Sensor *zc_frequency_sensor_{nullptr};
  sensor::Sensor *power_sensor_{nullptr};
#endif

#ifdef USE_NUMBER
  BurstFireNumber *power_number_{nullptr};
#endif
};

}  // namespace burst_fire
}  // namespace esphome
