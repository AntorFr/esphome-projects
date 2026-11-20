#pragma once
// Harry Potter decor of the 32x8 matrix clock (packages/matrix_display.yaml): Hedwig on her perch
// in place of the clock icon, blinking and glancing at the time and, every minute or two,
// something passing over the time (Hedwig herself flying off to deliver a letter, the Hogwarts
// Express, a stag Patronus, a flurry of Hogwarts letters, a levitated feather, or Harry on his
// broom chasing the Golden Snitch). Same drawing contract as matrix_halloween.h.
#include "matrix_decor.h"

namespace matrix_harry_potter {

using matrix_decor::BLACK;
using matrix_decor::Rgb;
using matrix_decor::Sprite;
using matrix_decor::TWO_PI;
using matrix_decor::WIDTH;
using matrix_decor::blit;
using matrix_decor::blit_outlined;
using matrix_decor::put_clipped;
using matrix_decor::scale;

// Hedwig on her perch, 'k' her beak and talons, 'x' the specks of her plumage. Her eyes: 'e' always
// amber, 'u' the left pupil, and the right eye's inner 'n' and outer 'm' cells, which swap when she
// glances at the time (see Decor::draw_perched_hedwig_).
static const Sprite PERCHED_HEDWIG = {10, 8,
                                      "..o....o.."
                                      "..oooooo.."
                                      ".oeeooeeo."
                                      ".oeuoonmo."
                                      "..ookkoo.."
                                      ".oooooooo."
                                      ".oxooooxo."
                                      "..k.kk.k.."};

// The Golden Snitch, its wings beating: its ball stays on rows 1-2.
static const Sprite FLYING_SNITCH_UP = {6, 4,
                                        "w....w"
                                        ".wGGw."
                                        "..GG.."
                                        "......"};
static const Sprite FLYING_SNITCH_DOWN = {6, 4,
                                          "......"
                                          "..GG.."
                                          ".wGGw."
                                          "w....w"};

// Hedwig flying, seen from the front, wings up then down; the letter she carries hangs below.
static const Sprite HEDWIG_UP = {11, 5,
                                 "oo.......oo"
                                 ".oo.ooo.oo."
                                 "..ooeoeoo.."
                                 "....oko...."
                                 "....ooo...."};
static const Sprite HEDWIG_DOWN = {11, 5,
                                   "..........."
                                   "....ooo...."
                                   "...oeoeo..."
                                   "oooookooooo"
                                   "oo..ooo..oo"};

// Going right: a carriage with lit windows, then the engine, its chimney at columns 20-21. The
// wheels' rods ('s') turn between the two frames.
static const Sprite EXPRESS_A = {26, 6,
                                 "...........RRRR.....RR...."
                                 "RRRRRRRRRR.RyyR.....RR...."
                                 "RyyRyyRyyR.RRRRRRRRRRRRRR."
                                 "RRRRRRRRRR.RRRRRRRRRRRRRRy"
                                 "GGGGGGGGGGkRRRRGGGGGGGGGRR"
                                 ".ss....ss...ss.ss...ss.ss."};
static const Sprite EXPRESS_B = {26, 6,
                                 "...........RRRR.....RR...."
                                 "RRRRRRRRRR.RyyR.....RR...."
                                 "RyyRyyRyyR.RRRRRRRRRRRRRR."
                                 "RRRRRRRRRR.RRRRRRRRRRRRRRy"
                                 "GGGGGGGGGGkRRRRGGGGGGGGGRR"
                                 "s.ss..s.ss..s.sss..s.sss.."};
static constexpr int CHIMNEY_X = 20;

// Galloping right, its neck up, its antlers' three tines above its head.
static const Sprite STAG_A = {11, 8,
                              ".....P.P.P."
                              "......PPP.."
                              ".......PPPP"
                              ".......PP.."
                              "PPPPPPPPP.."
                              ".PPPPPPP..."
                              ".PP...PP..."
                              ".P.P..P.P.."};
static const Sprite STAG_B = {11, 8,
                              ".....P.P.P."
                              "......PPP.."
                              ".......PPPP"
                              ".......PP.."
                              "PPPPPPPPP.."
                              ".PPPPPPP..."
                              "P.P..P.P..."
                              "P..P.P...P."};

// An envelope's outline and flap, closed by a red wax seal ('S'). Its inside ('x') is opaque, so
// that the time does not show through.
static const Sprite LETTER = {10, 6,
                              "EEEEEEEEEE"
                              "EExxxxxxEE"
                              "ExExxxxExE"
                              "ExxESSExxE"
                              "ExxxSSxxxE"
                              "EEEEEEEEEE"};

// 'q' is its quill.
static const Sprite FEATHER = {5, 6,
                               "...FF"
                               "..FFF"
                               ".FFF."
                               "FFF.."
                               ".q..."
                               "q...."};

// Flying right on his broom, reaching for the Snitch: 'h' his hair, 'f' his face and hand, 'b'
// the broomstick and 'S' its bristles.
static const Sprite HARRY = {13, 6,
                             ".......hhh..."
                             ".......hff..."
                             "......RRRRff."
                             "SSSbbbbbbbbbb"
                             "SS...RRR....."
                             "......R......"};

static constexpr Rgb SMOKE = {235, 235, 245};
static constexpr Rgb SILVER = {190, 220, 255};

inline Rgb ink(char c) {
  switch (c) {
    case 'G':
      return {255, 170, 0};  // the Snitch's ball; the Express's gold stripe
    case 'w':
      return {190, 190, 200};  // the Snitch's wings
    case 'o':
      return {220, 220, 230};  // Hedwig
    case 'e':
      return {255, 150, 0};  // her eyes
    case 'k':
      return {70, 70, 70};  // her beak and talons; the Express's coupling
    case 'E':
      return {230, 200, 140};  // parchment
    case 'S':
      return {220, 0, 0};  // wax seal
    case 'R':
      return {200, 0, 0};  // the Express; Harry's Quidditch robe
    case 'y':
      return {255, 200, 40};  // lit windows, the engine's lamp
    case 's':
      return {150, 150, 175};  // wheels
    case 'P':
      return {210, 230, 255};  // the Patronus, silvery (not blue: the time often is)
    case 'F':
      return {240, 240, 255};  // feather
    case 'q':
      return {150, 150, 160};  // its quill
    case 'h':
      return {70, 28, 0};  // Harry's hair
    case 'f':
      return {255, 150, 90};  // his face
    case 'b':
      return {150, 70, 10};  // broomstick
    default:
      return BLACK;
  }
}

// The broom's bristles share 'S' with the wax seal in the sprites, not the colour.
inline Rgb harry_ink(char c) { return c == 'S' ? Rgb{220, 150, 20} : ink(c); }

enum Kind : uint8_t { NONE, HEDWIG, EXPRESS, PATRONUS, LETTERS, LEVIOSA, QUIDDITCH, KIND_COUNT };

class Decor : public matrix_decor::Cast {
 public:
  Decor() : Cast(KIND_COUNT - 1, "y") {}

  // Starts a character now; NONE picks one at random, never the previous one.
  void start(uint32_t now, Kind kind = NONE) {
    this->pick_(now, kind);
    for (auto &p : this->puffs_)
      p.life = 0;
    this->puff_at_ = now;
    uint32_t at = 0;
    for (auto &l : this->letters_) {
      l.y = (int8_t) (this->next_() % 2);
      l.born = (uint16_t) at;
      at += this->between_(750, 1100);  // same speed, far enough apart not to overlap
    }
    this->feather_x_ = 14 + (int) (this->next_() % 11);
    for (auto &w : this->waypoints_) {
      w[0] = (int8_t) (12 + this->next_() % 13);
      w[1] = (int8_t) (this->next_() % 5);
    }
  }

  // Draws Hedwig on her perch at (0, 0), unless she is out delivering a letter, then the current
  // character. `colon_x` (the column of the time's colon) is unused: same signature as the
  // Halloween decor.
  template<class Put> void draw(Put &&raw, uint32_t now, int colon_x, bool night = false) {
    auto put = this->night_filter_(raw, night);
    (void) colon_x;
    if (this->due_(now))
      this->start(now);
    if (this->kind_ != HEDWIG)
      this->draw_perched_hedwig_(put, now);
    if (this->kind_ == NONE)
      return;
    uint32_t t = now - this->start_at_;
    bool running = false;
    switch (this->kind_) {
      case HEDWIG:
        running = this->draw_hedwig_(put, t);
        break;
      case EXPRESS:
        running = this->draw_express_(put, now, t);
        break;
      case PATRONUS:
        running = this->draw_patronus_(put, now, t);
        break;
      case LETTERS:
        running = this->draw_letters_(put, t);
        break;
      case LEVIOSA:
        running = this->draw_leviosa_(put, t);
        break;
      case QUIDDITCH:
        running = this->draw_quidditch_(put, t);
        break;
      default:
        break;
    }
    if (!running) {
      this->done_(now);
      if (this->last_kind_ == HEDWIG)
        this->draw_perched_hedwig_(put, now);  // back on her perch this very frame
    }
  }

 protected:
  struct Letter {
    int8_t y;
    uint16_t born;  // ms after the start
  };

  // Blinks every 3-7 s; every 8-20 s she glances at the time for a moment.
  template<class Put> void draw_perched_hedwig_(Put &put, uint32_t now) {
    if ((int32_t) (now - this->blink_at_) >= 0) {
      this->blink_until_ = now + 150;
      this->blink_at_ = now + this->between_(3000, 7000);
    }
    if ((int32_t) (now - this->glance_at_) >= 0) {
      this->glance_until_ = now + 1500;
      this->glance_at_ = now + this->between_(8000, 20000);
    }
    bool closed = (int32_t) (this->blink_until_ - now) > 0;
    bool glance = (int32_t) (this->glance_until_ - now) > 0;
    bool night = this->night_;
    blit(put, PERCHED_HEDWIG, 0, 0, false, [closed, glance, night](char c) -> Rgb {
      bool eye = c == 'e' || c == 'u' || c == 'n' || c == 'm';
      if (closed && eye)
        return ink('o');
      bool pupil = c == 'u' || c == (glance ? 'm' : 'n');
      if (night)  // in one red: her eyes are holes, their pupils lit
        return eye && !pupil ? BLACK : ink(pupil ? 'o' : c);
      if (pupil)
        return BLACK;
      return ink(eye ? 'e' : c);
    });
  }

  // Leaves her perch with a letter, flies over the time and out of sight to deliver it, then comes
  // back without it and lands.
  template<class Put> bool draw_hedwig_(Put &put, uint32_t t) {
    const uint32_t out = 3000, away = 700, back = 2600;
    const int perch_x = -1;  // her flying sprite over the perch
    if (t >= out + away + back)
      return false;
    if (t >= out && t < out + away)
      return true;  // delivering it
    float x;
    if (t < out) {
      float f = (float) t / out;
      x = perch_x + (WIDTH - perch_x) * f * f;  // taking off
    } else {
      float f = (float) (t - out - away) / back;
      x = WIDTH + (perch_x - WIDTH) * (1.0f - (1.0f - f) * (1.0f - f));  // slowing down to land
    }
    const Sprite &s = (t / 220) % 2 ? HEDWIG_DOWN : HEDWIG_UP;
    int ix = (int) std::lround(x), y = (t / 440) % 2 ? 1 : 2;
    this->sprite_(put, s, ix, y, false, ink);
    if (t < out) {
      for (int i = 4; i <= 6; i++)
        put_clipped(put, ix + i, y + s.h, ink('E'));
    }
    return true;
  }

  // Steams across along the bottom, puffs of smoke rising from its chimney and drifting behind.
  template<class Put> bool draw_express_(Put &put, uint32_t now, uint32_t t) {
    const uint32_t dur = 5200, rise = 450, drift = 120;
    const int y = 2;
    bool going = t < dur;
    const Sprite &s = (t / 160) % 2 ? EXPRESS_B : EXPRESS_A;
    int x = this->across_(t, dur, s.w);
    int chimney = this->dir_ > 0 ? x + CHIMNEY_X : x + s.w - 2 - CHIMNEY_X;
    if (going && (int32_t) (now - this->puff_at_) >= 0) {
      this->puff_at_ = now + this->between_(110, 200);
      for (auto &p : this->puffs_) {
        if (p.life != 0)
          continue;
        p.x = (int8_t) (chimney + (int) (this->next_() % 2));
        p.y = (int8_t) (y - 1);
        p.born = now;
        p.life = (uint16_t) this->between_(700, 900);
        break;
      }
    }
    bool smoke = false;
    for (auto &p : this->puffs_) {
      if (p.life == 0)
        continue;
      uint32_t age = now - p.born;
      if (age >= p.life) {
        p.life = 0;
        continue;
      }
      smoke = true;
      int px = p.x - this->dir_ * (int) (age / drift);
      int py = p.y - (int) (age / rise);
      float f = (float) age / p.life;
      put_clipped(put, px, py, scale(SMOKE, 1.0f - f * f));
    }
    if (going)
      this->sprite_(put, s, x, y, this->dir_ < 0, ink);
    return going || smoke;
  }

  // Gallops across, leaving a trail of silver sparks.
  template<class Put> bool draw_patronus_(Put &put, uint32_t now, uint32_t t) {
    const uint32_t dur = 3400;
    bool galloping = t < dur;
    const Sprite &s = (t / 150) % 2 ? STAG_B : STAG_A;
    int x = this->across_(t, dur, s.w);
    if (galloping && (int32_t) (now - this->puff_at_) >= 0) {
      this->puff_at_ = now + 40;
      for (auto &p : this->puffs_) {
        if (p.life != 0)
          continue;
        p.x = (int8_t) (this->dir_ > 0 ? x : x + s.w - 1);
        p.y = (int8_t) (3 + (int) (this->next_() % 4));
        p.born = now;
        p.life = (uint16_t) this->between_(300, 700);
        break;
      }
    }
    bool sparks = false;
    for (auto &p : this->puffs_) {
      if (p.life == 0)
        continue;
      uint32_t age = now - p.born;
      if (age >= p.life) {
        p.life = 0;
        continue;
      }
      sparks = true;
      put_clipped(put, p.x, p.y, scale(SILVER, 1.0f - (float) age / p.life));
    }
    if (galloping)
      blit_outlined(put, s, x, 0, this->dir_ < 0, ink);
    return galloping || sparks;
  }

  // Hogwarts letters fly across one after another, bobbing up and down.
  template<class Put> bool draw_letters_(Put &put, uint32_t t) {
    const uint32_t dur = 2400;
    bool flying = false;
    for (const auto &l : this->letters_) {
      if (t < l.born) {
        flying = true;
        continue;
      }
      uint32_t age = t - l.born;
      if (age >= dur)
        continue;
      flying = true;
      int y = l.y + (int) (((age + l.born) / 300) % 2);
      this->sprite_(put, LETTER, this->across_(age, dur, LETTER.w), y, false, ink);
    }
    return flying;
  }

  // Wingardium Leviosa: the feather rises over the time, swaying, floats a while at the top,
  // sparkling, then drifts back down.
  template<class Put> bool draw_leviosa_(Put &put, uint32_t t) {
    const uint32_t rise = 2200, hover = 1800, fall = 1800;
    if (t >= rise + hover + fall)
      return false;
    float y;
    if (t < rise)
      y = 8.0f - 8.0f * (float) t / rise;
    else if (t < rise + hover)
      y = 0.0f;
    else
      y = 8.0f * (float) (t - rise - hover) / fall;
    float sway = std::sin(t * TWO_PI / 1600.0f);
    int x = this->feather_x_ + (int) std::lround(2.0f * sway);
    if (t < rise + hover) {
      // Three sparkles twinkle around it, out of step.
      const int around[3][2] = {{-2, 4}, {FEATHER.w + 1, 1}, {1, -2}};
      for (int i = 0; i < 3; i++) {
        float k = std::sin((t + i * 240) * TWO_PI / 720.0f);
        if (k <= 0.2f)
          continue;
        put_clipped(put, x + around[i][0], (int) std::lround(y) + around[i][1], scale(SILVER, k));
      }
    }
    // Leaning the way it swings.
    this->sprite_(put, FEATHER, x, (int) std::lround(y), std::cos(t * TWO_PI / 1600.0f) < 0.0f, ink);
    return true;
  }

  // The Snitch zips in and darts about over the time; Harry flies in after it on his broom, it
  // shoots off to the right and he chases it out of sight.
  template<class Put> bool draw_quidditch_(Put &put, uint32_t t) {
    const uint32_t hop = 450, dash = 400;
    const int hops = sizeof(this->waypoints_) / sizeof(this->waypoints_[0]);
    const uint32_t darting = hops * hop;
    const uint32_t enter = 700, gone = darting + 1500;
    if (t >= gone)
      return false;
    float sx = WIDTH, sy = 0;
    if (t < darting) {
      // From the left edge, then from one random point over the time to the next, slowing down at
      // each.
      int i = (int) (t / hop);
      float f = (float) (t % hop) / hop;
      f = 1.0f - (1.0f - f) * (1.0f - f);
      float fx = i == 0 ? -FLYING_SNITCH_UP.w : this->waypoints_[i - 1][0];
      float fy = i == 0 ? 2 : this->waypoints_[i - 1][1];
      sx = fx + (this->waypoints_[i][0] - fx) * f;
      sy = fy + (this->waypoints_[i][1] - fy) * f;
    } else if (t < darting + dash) {
      float f = (float) (t - darting) / dash;
      sx = this->waypoints_[hops - 1][0] + (WIDTH - this->waypoints_[hops - 1][0]) * f * f;
      sy = this->waypoints_[hops - 1][1];
    }
    // Harry: in from the left as it darts about, closing in once it shoots off. The Snitch stays
    // in front of him.
    if (t >= enter) {
      float hx = t < darting ? -HARRY.w + (8.0f + HARRY.w) * (float) (t - enter) / (darting - enter)
                             : 8.0f + (WIDTH - 8.0f) * (float) (t - darting) / (gone - darting);
      this->sprite_(put, HARRY, (int) std::lround(hx), (t / 300) % 2 ? 1 : 2, false, harry_ink);
    }
    if (sx < WIDTH)
      this->sprite_(put, (t / 70) % 2 ? FLYING_SNITCH_DOWN : FLYING_SNITCH_UP, (int) std::lround(sx),
           (int) std::lround(sy) - 1, false, ink);
    return true;
  }

  uint32_t blink_at_{0}, blink_until_{0};
  uint32_t glance_at_{0}, glance_until_{0};
  matrix_decor::Spark puffs_[16]{};
  uint32_t puff_at_{0};
  Letter letters_[4]{};
  int feather_x_{18};
  int8_t waypoints_[4][2]{};
};

}  // namespace matrix_harry_potter

// The clock's instance, drawn from the display lambda.
inline matrix_harry_potter::Decor harry_potter_decor;
