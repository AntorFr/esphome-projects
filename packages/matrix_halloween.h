#pragma once
// Halloween decor of the 32x8 matrix clock (packages/matrix_display.yaml): a jack-o'-lantern in
// place of the clock icon and, every minute or two, a character passing over the time (ghost,
// bat, spider, witch on her broom, or two eyes in place of the colon). Pixels go through a
// put(x, y, Rgb) callable and the time comes in as an argument, so the same code also renders a
// desktop preview.
#include <cmath>
#include <cstdint>

namespace matrix_halloween {

struct Rgb {
  uint8_t r, g, b;
};

static constexpr int WIDTH = 32;
static constexpr int HEIGHT = 8;
static constexpr float TWO_PI = 6.2831853f;

// One character per pixel, row by row: '.' transparent, 'k' opaque black, other letters from
// palette() ('y' follows the candle flicker).
struct Sprite {
  int w, h;
  const char *px;
};

static const Sprite PUMPKIN = {8, 8,
                               "...gg..."
                               ".oooooo."
                               "ooyooyoo"
                               "oyyooyyo"
                               "oooooooo"
                               "oyyyyyyo"
                               "oyoyyoyo"
                               ".oooooo."};

// Facing right (mirrored to go left); the hem waves between the two frames.
static const Sprite GHOST_A = {7, 7,
                               "..www.."
                               ".wwwww."
                               "wwwkwkw"
                               "wwwwwww"
                               "wwwwkww"
                               "wwwwwww"
                               "w.w.w.w"};
static const Sprite GHOST_B = {7, 7,
                               "..www.."
                               ".wwwww."
                               "wwwkwkw"
                               "wwwwwww"
                               "wwwwkww"
                               "wwwwwww"
                               ".w.w.w."};

static const Sprite BAT_UP = {9, 4,
                              "p..p.p..p"
                              "pp.rpr.pp"
                              ".ppppppp."
                              "....p...."};
static const Sprite BAT_DOWN = {9, 4,
                                "...p.p..."
                                ".pprprpp."
                                "pp.ppp.pp"
                                "p.......p"};

static const Sprite SPIDER = {7, 5,
                              ".s...s."
                              "s.rsr.s"
                              ".sssss."
                              "s.sss.s"
                              ".s...s."};

// Flying right: bristles at the back, the hat blown backwards.
static const Sprite WITCH = {11, 7,
                             "...v......."
                             "....vv....."
                             "...vvvvv..."
                             ".....ff...."
                             "....vvvf..."
                             "hhhbbbbbbbb"
                             "....v.v...."};

static constexpr Rgb BLACK = {0, 0, 0};
static constexpr Rgb THREAD = {45, 45, 45};
static constexpr Rgb SPARK = {255, 110, 0};
static constexpr Rgb EYE = {30, 255, 30};

inline Rgb scale(Rgb c, float k) {
  if (k <= 0.0f)
    return BLACK;
  if (k >= 1.0f)
    return c;
  return {(uint8_t) (c.r * k), (uint8_t) (c.g * k), (uint8_t) (c.b * k)};
}

inline Rgb palette(char c, float candle) {
  switch (c) {
    case 'o':
      return {210, 50, 0};  // pumpkin
    case 'g':
      return {30, 140, 0};  // stem
    case 'y':
      return scale({255, 210, 40}, candle);  // carved face, lit by the candle
    case 'w':
      return {170, 180, 255};  // ghost
    case 'p':
      return {110, 0, 255};  // bat
    case 'r':
      return {255, 0, 0};  // bat and spider eyes
    case 's':
      return {120, 120, 120};  // spider
    case 'v':
      return {110, 0, 190};  // witch's hat and cape
    case 'f':
      return {40, 200, 0};  // witch's face
    case 'b':
      return {120, 50, 0};  // broomstick
    case 'h':
      return {220, 150, 20};  // broom bristles
    default:
      return BLACK;  // 'k'
  }
}

template<class Put> inline void put_clipped(Put &put, int x, int y, Rgb c) {
  if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
    put(x, y, c);
}

template<class Put> void blit(Put &put, const Sprite &s, int x0, int y0, bool mirror, float candle = 1.0f) {
  for (int y = 0; y < s.h; y++) {
    for (int x = 0; x < s.w; x++) {
      char c = s.px[y * s.w + (mirror ? s.w - 1 - x : x)];
      if (c != '.')
        put_clipped(put, x0 + x, y0 + y, palette(c, candle));
    }
  }
}

enum Kind : uint8_t { NONE, GHOST, BAT, SPIDER_KIND, WITCH_KIND, EYES, KIND_COUNT };

class Decor {
 public:
  void seed(uint32_t s) { this->rng_ = s != 0 ? s : 1; }
  // Pause between two characters, drawn at random in [min_ms, max_ms].
  void set_gap(uint32_t min_ms, uint32_t max_ms) {
    this->gap_min_ = min_ms;
    this->gap_max_ = max_ms;
  }
  // Starts a character now; NONE picks one at random, never the previous one.
  void start(uint32_t now, Kind kind = NONE) {
    if (kind == NONE) {
      do {
        kind = (Kind) (1 + this->next_() % (KIND_COUNT - 1));
      } while (kind == this->last_kind_);
    }
    this->kind_ = kind;
    this->last_kind_ = kind;
    this->start_at_ = now;
    this->dir_ = this->next_() % 2 ? 1 : -1;
    this->spider_x_ = 11 + (int) (this->next_() % 15);  // over the time
    for (auto &s : this->sparks_)
      s.life = 0;
    this->spark_at_ = now;
  }

  // Draws the pumpkin at (0, 0), then the current character. `colon_x` is the column of the
  // time's colon (drawn before this call), which the eyes take over.
  template<class Put> void draw(Put &&put, uint32_t now, int colon_x) {
    // Back after a pause (other ambiance, night, display off): the next character comes soon,
    // not at a time that went by meanwhile.
    if (!this->drawn_ || now - this->last_draw_ > 1000) {
      this->kind_ = NONE;
      this->next_at_ = now + this->between_(FIRST_GAP_MIN, FIRST_GAP_MAX);
    }
    this->drawn_ = true;
    this->last_draw_ = now;

    this->draw_pumpkin_(put, now);
    if (this->kind_ == NONE && (int32_t) (now - this->next_at_) >= 0)
      this->start(now);
    if (this->kind_ == NONE)
      return;
    uint32_t t = now - this->start_at_;
    bool running = false;
    switch (this->kind_) {
      case GHOST:
        running = this->draw_ghost_(put, t);
        break;
      case BAT:
        running = this->draw_bat_(put, t);
        break;
      case SPIDER_KIND:
        running = this->draw_spider_(put, t);
        break;
      case WITCH_KIND:
        running = this->draw_witch_(put, now, t);
        break;
      case EYES:
        running = this->draw_eyes_(put, t, colon_x);
        break;
      default:
        break;
    }
    if (!running) {
      this->kind_ = NONE;
      this->next_at_ = now + this->between_(this->gap_min_, this->gap_max_);
    }
  }

 protected:
  static constexpr uint32_t FIRST_GAP_MIN = 5000;
  static constexpr uint32_t FIRST_GAP_MAX = 20000;

  struct Spark {
    int8_t x, y;
    uint32_t born;
    uint16_t life;  // ms, 0 = free slot
  };

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

  // Candle: a new random level every 60-160 ms, approached quickly. It never drops below 70 %:
  // dimmer, the carved face blends into the pumpkin.
  template<class Put> void draw_pumpkin_(Put &put, uint32_t now) {
    if ((int32_t) (now - this->flicker_at_) >= 0) {
      this->candle_target_ = 0.7f + 0.3f * this->unit_();
      this->flicker_at_ = now + this->between_(60, 160);
    }
    this->candle_ += (this->candle_target_ - this->candle_) * 0.35f;
    blit(put, PUMPKIN, 0, 0, false, this->candle_);
  }

  // Floats across, bobbing, its hem waving.
  template<class Put> bool draw_ghost_(Put &put, uint32_t t) {
    const uint32_t dur = 3400;
    if (t >= dur)
      return false;
    int y = std::sin(t * TWO_PI / 1000.0f) > 0.0f ? 0 : 1;
    blit(put, (t / 250) % 2 ? GHOST_B : GHOST_A, this->across_(t, dur, GHOST_A.w), y, this->dir_ < 0);
    return true;
  }

  // Flutters across, wings flapping, on a wavy path.
  template<class Put> bool draw_bat_(Put &put, uint32_t t) {
    const uint32_t dur = 2400;
    if (t >= dur)
      return false;
    int y = 2 + (int) std::lround(1.6f * std::sin(t * TWO_PI / 800.0f));
    blit(put, (t / 110) % 2 ? BAT_DOWN : BAT_UP, this->across_(t, dur, BAT_UP.w), y, false);
    return true;
  }

  // Lets itself down on its thread over the time, sways a little, climbs back up.
  template<class Put> bool draw_spider_(Put &put, uint32_t t) {
    const uint32_t down = 1100, hang = 2200, up = 1100;
    if (t >= down + hang + up)
      return false;
    const float top = -SPIDER.h, low = 2;  // hidden above, then its top on row 2
    auto ease = [](float f) { return f * f * (3 - 2 * f); };
    float y;
    int sway = 0;
    if (t < down) {
      y = top + (low - top) * ease((float) t / down);
    } else if (t < down + hang) {
      y = low;
      sway = (int) std::lround(std::sin((t - down) * TWO_PI / 900.0f));
    } else {
      y = low + (top - low) * ease((float) (t - down - hang) / up);
    }
    int yi = (int) std::lround(y);
    int x = this->spider_x_ + sway;
    for (int ty = 0; ty < yi; ty++)
      put_clipped(put, x + SPIDER.w / 2, ty, THREAD);
    blit(put, SPIDER, x, yi, false);
    return true;
  }

  // Rides her broom across, leaving a trail of fading sparks.
  template<class Put> bool draw_witch_(Put &put, uint32_t now, uint32_t t) {
    const uint32_t dur = 3000;
    bool flying = t < dur;
    int x = this->across_(t, dur, WITCH.w);
    int y = std::sin(t * TWO_PI / 1200.0f) > 0.0f ? 0 : 1;
    if (flying && (int32_t) (now - this->spark_at_) >= 0) {
      this->spark_at_ = now + 50;
      for (auto &s : this->sparks_) {
        if (s.life != 0)
          continue;
        s.x = (int8_t) (this->dir_ > 0 ? x : x + WITCH.w - 1);
        s.y = (int8_t) (y + 4 + (int) (this->next_() % 3));
        s.born = now;
        s.life = (uint16_t) this->between_(300, 700);
        break;
      }
    }
    bool sparks = false;
    for (auto &s : this->sparks_) {
      if (s.life == 0)
        continue;
      uint32_t age = now - s.born;
      if (age >= s.life) {
        s.life = 0;
        continue;
      }
      sparks = true;
      put_clipped(put, s.x, s.y, scale(SPARK, 1.0f - (float) age / s.life));
    }
    if (flying)
      blit(put, WITCH, x, y, this->dir_ < 0);
    return flying || sparks;
  }

  // Two eyes open in place of the colon, blink, and close.
  template<class Put> bool draw_eyes_(Put &put, uint32_t t, int colon_x) {
    const uint32_t dur = 3200, fade = 400;
    if (t >= dur)
      return false;
    for (int y = 3; y <= 5; y++)
      put_clipped(put, colon_x, y, BLACK);
    float level = 1.0f;
    if (t < fade)
      level = (float) t / fade;
    else if (t > dur - fade)
      level = (float) (dur - t) / fade;
    auto closed = [t](uint32_t at) { return t >= at && t < at + 120; };
    if (closed(1300) || closed(1550) || closed(2400))
      level = 0.0f;
    Rgb c = scale(EYE, level);
    put_clipped(put, colon_x - 1, 4, c);
    put_clipped(put, colon_x + 1, 4, c);
    return true;
  }

  uint32_t rng_{1};
  uint32_t gap_min_{45000}, gap_max_{120000};
  bool drawn_{false};
  uint32_t last_draw_{0};
  uint32_t next_at_{0};
  Kind kind_{NONE}, last_kind_{NONE};
  uint32_t start_at_{0};
  int dir_{1};
  int spider_x_{16};
  float candle_{1.0f}, candle_target_{1.0f};
  uint32_t flicker_at_{0};
  Spark sparks_[12]{};
  uint32_t spark_at_{0};
};

}  // namespace matrix_halloween

// The clock's instance, drawn from the display lambda.
inline matrix_halloween::Decor halloween_decor;
