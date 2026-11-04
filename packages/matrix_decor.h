#pragma once
// Shared pieces of the seasonal decors of the 32x8 matrix clock (matrix_halloween.h,
// matrix_christmas.h): sprites drawn through a put(x, y, Rgb) callable, random draws, and the
// schedule of the characters passing over the time.
#include <cmath>
#include <cstdint>

namespace matrix_decor {

struct Rgb {
  uint8_t r, g, b;
};

static constexpr int WIDTH = 32;
static constexpr int HEIGHT = 8;
static constexpr float TWO_PI = 6.2831853f;

// One character per pixel, row by row: '.' transparent, other letters from the decor's palette.
struct Sprite {
  int w, h;
  const char *px;
};

static constexpr Rgb BLACK = {0, 0, 0};

inline Rgb scale(Rgb c, float k) {
  if (k <= 0.0f)
    return BLACK;
  if (k >= 1.0f)
    return c;
  return {(uint8_t) (c.r * k), (uint8_t) (c.g * k), (uint8_t) (c.b * k)};
}

template<class Put> inline void put_clipped(Put &put, int x, int y, Rgb c) {
  if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
    put(x, y, c);
}

// `pal` maps a sprite letter to its colour.
template<class Put, class Pal> void blit(Put &put, const Sprite &s, int x0, int y0, bool mirror, Pal &&pal) {
  for (int y = 0; y < s.h; y++) {
    for (int x = 0; x < s.w; x++) {
      char c = s.px[y * s.w + (mirror ? s.w - 1 - x : x)];
      if (c != '.')
        put_clipped(put, x0 + x, y0 + y, pal(c));
    }
  }
}

// A fading spark of a trail (witch's broom, Santa's sleigh).
struct Spark {
  int8_t x, y;
  uint32_t born;
  uint16_t life;  // ms, 0 = free slot
};

// The characters of a decor take turns over the time: one every gap_min..gap_max ms, never the
// same twice in a row, the first one soon after the decor (re)appears. Kinds run from 1 to
// `kinds`, 0 is none.
class Cast {
 public:
  void seed(uint32_t s) { this->rng_ = s != 0 ? s : 1; }
  // Pause between two characters, drawn at random in [min_ms, max_ms].
  void set_gap(uint32_t min_ms, uint32_t max_ms) {
    this->gap_min_ = min_ms;
    this->gap_max_ = max_ms;
  }

 protected:
  static constexpr uint32_t FIRST_GAP_MIN = 5000;
  static constexpr uint32_t FIRST_GAP_MAX = 20000;

  explicit Cast(int kinds) : kinds_(kinds) {}

  // Picks the character (0: at random, never the previous one) and its direction.
  int pick_(uint32_t now, int kind) {
    if (kind == 0) {
      do {
        kind = 1 + (int) (this->next_() % this->kinds_);
      } while (kind == this->last_kind_);
    }
    this->kind_ = kind;
    this->last_kind_ = kind;
    this->start_at_ = now;
    this->dir_ = this->next_() % 2 ? 1 : -1;
    return kind;
  }

  // Once per frame, before drawing. Returns true when the next character is due (call start()).
  bool due_(uint32_t now) {
    // Back after a pause (other ambiance, night, display off): the next character comes soon,
    // not at a time that went by meanwhile.
    if (!this->drawn_ || now - this->last_draw_ > 1000) {
      this->kind_ = 0;
      this->next_at_ = now + this->between_(FIRST_GAP_MIN, FIRST_GAP_MAX);
    }
    this->drawn_ = true;
    this->last_draw_ = now;
    return this->kind_ == 0 && (int32_t) (now - this->next_at_) >= 0;
  }

  // The current character is over: schedule the next one.
  void done_(uint32_t now) {
    this->kind_ = 0;
    this->next_at_ = now + this->between_(this->gap_min_, this->gap_max_);
  }

  uint32_t next_() {
    uint32_t x = this->rng_;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return this->rng_ = x;
  }
  float unit_() { return (this->next_() & 0xFFFF) / 65535.0f; }
  uint32_t between_(uint32_t lo, uint32_t hi) { return hi > lo ? lo + this->next_() % (hi - lo + 1) : lo; }

  // x of a sprite crossing the whole width in `dur` ms, in the current direction.
  int across_(uint32_t t, uint32_t dur, int w) const {
    float f = (float) t / dur;
    float x = this->dir_ > 0 ? -w + f * (WIDTH + w) : WIDTH - f * (WIDTH + w);
    return (int) std::lround(x);
  }

  int kinds_;
  uint32_t rng_{1};
  uint32_t gap_min_{45000}, gap_max_{120000};
  bool drawn_{false};
  uint32_t last_draw_{0};
  uint32_t next_at_{0};
  int kind_{0}, last_kind_{0};
  uint32_t start_at_{0};
  int dir_{1};
};

}  // namespace matrix_decor
