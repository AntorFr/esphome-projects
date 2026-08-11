#pragma once
extern "C" {
  #include "esp_heap_caps.h"
}

inline bool esphome_has_psram() {
  // S'il existe un heap SPIRAM, alors PSRAM présente
  return heap_caps_get_total_size(MALLOC_CAP_SPIRAM) > 0;
}

inline float esphome_psram_size_mib() {
  size_t total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
  return total ? (float)total / (1024.0f * 1024.0f) : 0.0f;
}
