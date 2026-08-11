#pragma once

#include <utility>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/light/addressable_light.h"
#include "esphome/components/light/addressable_light_effect.h"

//#include "FastLED.h"
//#include "GradientPalettes.hpp"

namespace esphome {
namespace light {

class AddressableChristmasEffect : public AddressableLightEffect {
 public:
  explicit AddressableChristmasEffect(const std::string &name) : AddressableLightEffect(name) {}
  void set_bit_size(uint8_t bit_size) { this->bit_size_ = bit_size; }
  void set_blank_size(uint8_t blank_size) { this->blank_size_ = blank_size; }
  void start() override {
    this->patern_size_ = (2*this->bit_size_)+this->blank_size_;
  }

  void apply(AddressableLight &it, const Color &current_color) override {
    for (int i = 0; i < it.size(); i++) {
      if(i % this->patern_size_ < this->bit_size_ ) 
        it[i] = Color(255, 0, 18);
      else if (i% this->patern_size_ < (this->bit_size_+ this->blank_size_))
        it[i] = Color::BLACK;
      else
        it[i] = Color(0, 179, 44);
    }
    it.schedule_show();
  }
  
  protected:
    uint8_t bit_size_{1};
    uint8_t blank_size_{0};
    uint16_t patern_size_{2};
};


}  // namespace light
}  // namespace esphome
