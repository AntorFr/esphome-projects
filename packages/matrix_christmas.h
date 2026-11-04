#pragma once
// Christmas decor of the 32x8 matrix clock (packages/matrix_display.yaml): a Christmas tree with
// twinkling lights in place of the clock icon and, every minute or two, something passing over
// the time (Santa's sleigh, a snowfall, a garland lighting up above the time, a shooting star,
// or a snowman). Same drawing contract as matrix_halloween.h.
#include "matrix_decor.h"

namespace matrix_christmas {

using matrix_decor::BLACK;
using matrix_decor::Rgb;
using matrix_decor::Sprite;
using matrix_decor::TWO_PI;
using matrix_decor::blit;
using matrix_decor::put_clipped;
using matrix_decor::scale;

// '1'..'6' are the tree's lights, 's' its star (see Decor::draw_tree_).
static const Sprite TREE = {8, 8,
                            "...s...."
                            "..g1g..."
                            ".ggggg.."
                            ".2ggg3.."
                            "ggg4ggg."
                            "g5ggg6g."
                            "rr.t.yy."
                            "rr.t.yy."};

// Flying right: Santa in his sleigh, the reins, the reindeer and its red nose ('n'). The legs
// gallop between the two frames.
static const Sprite SLEIGH_A = {17, 6,
                                ".............a.a."
                                ".wRR..........a.."
                                "..Wf.........dddn"
                                "S.RRRllllddddd..."
                                "SSSSSSSu.ddddd..."
                                "uuuuuuuu.d.d.d..."};
static const Sprite SLEIGH_B = {17, 6,
                                ".............a.a."
                                ".wRR..........a.."
                                "..Wf.........dddn"
                                "S.RRRllllddddd..."
                                "SSSSSSSu.ddddd..."
                                "uuuuuuuu..d.d...."};

// Facing right; it waves an arm between the two frames.
static const Sprite SNOWMAN_A = {7, 7,
                                 "..HHH.."
                                 ".HHHHH."
                                 "..www.."
                                 "..wwo.a"
                                 ".wwwwa."
                                 "awwrww."
                                 ".wwwww."};
static const Sprite SNOWMAN_B = {7, 7,
                                 "..HHH.."
                                 ".HHHHH."
                                 "..www.."
                                 "..wwo.."
                                 ".wwwwaa"
                                 "awwrww."
                                 ".wwwww."};

// Colours a light of the tree or of the garland takes in turn.
static constexpr Rgb LIGHTS[] = {{255, 0, 0}, {255, 150, 0}, {0, 70, 255}, {255, 0, 140}, {255, 220, 160}};
static constexpr int LIGHT_COUNT = sizeof(LIGHTS) / sizeof(LIGHTS[0]);
static constexpr Rgb STAR = {255, 190, 0};
static constexpr Rgb SPARK = {255, 190, 60};
static constexpr Rgb SNOW = {170, 195, 255};

inline Rgb ink(char c) {
  switch (c) {
    case 'g':
      return {0, 110, 20};  // fir
    case 't':
      return {110, 45, 0};  // trunk
    case 'r':
      return {200, 0, 0};  // red present
    case 'y':
      return {220, 150, 0};  // gold present
    case 'R':
      return {255, 0, 0};  // Santa's coat and hat
    case 'w':
      return {200, 210, 255};  // pompom, snow
    case 'W':
      return {140, 140, 140};  // Santa's beard
    case 'f':
      return {255, 120, 70};  // Santa's face
    case 'S':
      return {130, 0, 10};  // sleigh
    case 'u':
      return {220, 150, 0};  // runners
    case 'l':
      return {90, 50, 10};  // reins
    case 'd':
      return {130, 55, 5};  // reindeer
    case 'a':
      return {180, 120, 40};  // antlers, snowman's arms
    case 'n':
      return {255, 0, 0};  // the reindeer's nose
    case 'H':
      return {0, 40, 200};  // snowman's hat
    case 'o':
      return {255, 90, 0};  // carrot
    default:
      return BLACK;
  }
}

enum Kind : uint8_t { NONE, SLEIGH, SNOWFALL, GARLAND, SHOOTING_STAR, SNOWMAN, KIND_COUNT };

class Decor : public matrix_decor::Cast {
 public:
  Decor() : Cast(KIND_COUNT - 1) {}

  // Starts a character now; NONE picks one at random, never the previous one.
  void start(uint32_t now, Kind kind = NONE) {
    this->pick_(now, kind);
    for (auto &s : this->sparks_)
      s.life = 0;
    this->spark_at_ = now;
    uint32_t at = 0;
    for (auto &f : this->flakes_) {
      f.x = (int8_t) (this->next_() % matrix_decor::WIDTH);
      f.born = (uint16_t) at;
      f.step = (uint16_t) this->between_(200, 340);
      f.phase = this->unit_() * TWO_PI;
      at += this->between_(120, 320);
    }
    this->star_y_ = (int) (this->next_() % 3);
  }

  // Draws the tree at (0, 0), then the current character. `colon_x` (the column of the time's
  // colon) is unused: same signature as the Halloween decor.
  template<class Put> void draw(Put &&put, uint32_t now, int colon_x) {
    (void) colon_x;
    bool due = this->due_(now);
    this->draw_tree_(put, now);
    if (due)
      this->start(now);
    if (this->kind_ == NONE)
      return;
    uint32_t t = now - this->start_at_;
    bool running = false;
    switch (this->kind_) {
      case SLEIGH:
        running = this->draw_sleigh_(put, now, t);
        break;
      case SNOWFALL:
        running = this->draw_snowfall_(put, t);
        break;
      case GARLAND:
        running = this->draw_garland_(put, t);
        break;
      case SHOOTING_STAR:
        running = this->draw_shooting_star_(put, t);
        break;
      case SNOWMAN:
        running = this->draw_snowman_(put, t);
        break;
      default:
        break;
    }
    if (!running)
      this->done_(now);
  }

 protected:
  struct Flake {
    int8_t x;
    uint16_t born;  // ms after the start
    uint16_t step;  // ms per row
    float phase;    // of its sideways drift
  };

  // Lights: every 150-450 ms one of them takes the next colour, flashing brighter for a moment.
  // The star shimmers between 60 and 100 %.
  template<class Put> void draw_tree_(Put &put, uint32_t now) {
    if ((int32_t) (now - this->twinkle_at_) >= 0) {
      int i = (int) (this->next_() % 6);
      this->light_[i] = (uint8_t) ((this->light_[i] + 1 + this->next_() % (LIGHT_COUNT - 1)) % LIGHT_COUNT);
      this->lit_at_[i] = now;
      this->twinkle_at_ = now + this->between_(150, 450);
    }
    if ((int32_t) (now - this->shimmer_at_) >= 0) {
      this->star_target_ = 0.6f + 0.4f * this->unit_();
      this->shimmer_at_ = now + this->between_(120, 300);
    }
    this->star_ += (this->star_target_ - this->star_) * 0.25f;
    blit(put, TREE, 0, 0, false, [this, now](char c) -> Rgb {
      if (c == 's')
        return scale(STAR, this->star_);
      if (c >= '1' && c <= '6') {
        int i = c - '1';
        uint32_t age = now - this->lit_at_[i];
        return scale(LIGHTS[this->light_[i]], age < 200 ? 1.0f : 0.6f);
      }
      return ink(c);
    });
  }

  // Flies across, bobbing, with a trail of golden sparks behind the sleigh.
  template<class Put> bool draw_sleigh_(Put &put, uint32_t now, uint32_t t) {
    const uint32_t dur = 3800;
    bool flying = t < dur;
    const Sprite &s = (t / 140) % 2 ? SLEIGH_B : SLEIGH_A;
    int x = this->across_(t, dur, s.w);
    int y = std::sin(t * TWO_PI / 1400.0f) > 0.0f ? 1 : 2;
    if (flying && (int32_t) (now - this->spark_at_) >= 0) {
      this->spark_at_ = now + 45;
      for (auto &sp : this->sparks_) {
        if (sp.life != 0)
          continue;
        sp.x = (int8_t) (this->dir_ > 0 ? x - 1 : x + s.w);
        sp.y = (int8_t) (y + 2 + (int) (this->next_() % 4));
        sp.born = now;
        sp.life = (uint16_t) this->between_(300, 800);
        break;
      }
    }
    bool sparks = false;
    for (auto &sp : this->sparks_) {
      if (sp.life == 0)
        continue;
      uint32_t age = now - sp.born;
      if (age >= sp.life) {
        sp.life = 0;
        continue;
      }
      sparks = true;
      put_clipped(put, sp.x, sp.y, scale(SPARK, 1.0f - (float) age / sp.life));
    }
    if (flying) {
      // Rudolph's nose glows.
      float glow = 0.55f + 0.45f * std::sin(t * TWO_PI / 500.0f);
      blit(put, s, x, y, this->dir_ < 0, [glow](char c) { return c == 'n' ? scale(ink(c), glow) : ink(c); });
    }
    return flying || sparks;
  }

  // Flakes fall one after another over the whole clock, drifting from side to side.
  template<class Put> bool draw_snowfall_(Put &put, uint32_t t) {
    bool falling = false;
    for (const auto &f : this->flakes_) {
      if (t < f.born) {
        falling = true;
        continue;
      }
      int y = (int) ((t - f.born) / f.step) - 1;
      if (y >= matrix_decor::HEIGHT)
        continue;
      falling = true;
      int x = f.x + (int) std::lround(0.8f * std::sin(t * TWO_PI / 1800.0f + f.phase));
      put_clipped(put, x, y, SNOW);
    }
    return falling;
  }

  // A garland hangs above the time and lights up, its colours chasing along it.
  template<class Put> bool draw_garland_(Put &put, uint32_t t) {
    const uint32_t dur = 5500, fade = 500;
    if (t >= dur)
      return false;
    float level = 1.0f;
    if (t < fade)
      level = (float) t / fade;
    else if (t > dur - fade)
      level = (float) (dur - t) / fade;
    int step = (int) (t / 220);
    // Swags between hooks 6 columns apart: bulbs on row 0 near the hooks, row 1 in between.
    for (int i = 0, x = 11; x < matrix_decor::WIDTH; i++, x += 2) {
      int y = (x - 11) % 6 == 0 ? 0 : 1;
      put_clipped(put, x, y, scale(LIGHTS[(i + step) % LIGHT_COUNT], level));
    }
    return true;
  }

  // Streaks across the top with a fading tail, slightly downwards.
  template<class Put> bool draw_shooting_star_(Put &put, uint32_t t) {
    const uint32_t dur = 1500;
    const int tail = 7;
    if (t >= dur)
      return false;
    float f = (float) t / dur;
    float travel = matrix_decor::WIDTH + 2 * tail;
    for (int k = 0; k <= tail; k++) {
      float d = f * travel - k;  // distance covered by this point of the tail
      int x = this->dir_ > 0 ? (int) std::lround(d - tail) : matrix_decor::WIDTH - 1 - (int) std::lround(d - tail);
      int y = this->star_y_ + (int) (d / 9.0f);
      Rgb c = k == 0 ? Rgb{255, 240, 200} : scale(STAR, 0.8f * (1.0f - (float) k / (tail + 1)));
      put_clipped(put, x, y, c);
    }
    return true;
  }

  // Slides across on the ice, rocking, and waves.
  template<class Put> bool draw_snowman_(Put &put, uint32_t t) {
    const uint32_t dur = 5000;
    if (t >= dur)
      return false;
    int y = (t / 400) % 2 ? 1 : 0;
    blit(put, (t / 300) % 2 ? SNOWMAN_B : SNOWMAN_A, this->across_(t, dur, SNOWMAN_A.w), y, this->dir_ < 0, ink);
    return true;
  }

  uint8_t light_[6]{0, 1, 2, 3, 4, 0};
  uint32_t lit_at_[6]{};
  uint32_t twinkle_at_{0};
  float star_{1.0f}, star_target_{1.0f};
  uint32_t shimmer_at_{0};
  matrix_decor::Spark sparks_[16]{};
  uint32_t spark_at_{0};
  Flake flakes_[18]{};
  int star_y_{0};
};

}  // namespace matrix_christmas

// The clock's instance, drawn from the display lambda.
inline matrix_christmas::Decor christmas_decor;
