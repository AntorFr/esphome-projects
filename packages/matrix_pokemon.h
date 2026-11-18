#pragma once
// Pokémon decor of the 32x8 matrix clock (packages/matrix_display.yaml): Pikachu's face in place
// of the clock icon, blinking, its cheeks crackling now and then and, every minute or two,
// something passing over the time (Pikachu running, a Poké Ball catching something, a
// thunderbolt, Charmander, Jigglypuff singing, or Magikarp splashing). Same drawing contract as
// matrix_halloween.h.
#include <algorithm>
#include <initializer_list>
#include "matrix_decor.h"

namespace matrix_pokemon {

using matrix_decor::BLACK;
using matrix_decor::Rgb;
using matrix_decor::Sprite;
using matrix_decor::TWO_PI;
using matrix_decor::WIDTH;
using matrix_decor::blit;
using matrix_decor::put_clipped;
using matrix_decor::scale;

// 'x' is opaque black (eyes, the Poké Ball's band). The tips of Pikachu's ears are dark brown
// ('k'): black would not light at all. 'r' are its cheeks, which crackle (see Decor::draw_face_).
static const Sprite FACE = {10, 8,
                            "kk......kk"
                            ".yy....yy."
                            "..yy..yy.."
                            "..yyyyyy.."
                            ".yxyyyyxy."
                            ".ryyyyyyr."
                            ".rryyyyrr."
                            "..yyyyyy.."};

// Running right on all fours, its zigzag tail behind ('b' its brown base).
static const Sprite PIKACHU_A = {12, 8,
                                 "yy......k.k."
                                 ".yy.....y.y."
                                 "yy......yyy."
                                 ".yb....yyxyy"
                                 "..byyyyyyyry"
                                 "...yyyyyyy.."
                                 "...yy...yy.."
                                 "....y....y.."};
static const Sprite PIKACHU_B = {12, 8,
                                 "yy......k.k."
                                 ".yy.....y.y."
                                 "yy......yyy."
                                 ".yb....yyxyy"
                                 "..byyyyyyyry"
                                 "...yyyyyyy.."
                                 "....yy.yy..."
                                 ".....y.y...."};

// 'W' is the button.
static const Sprite BALL = {7, 7,
                            "..RRR.."
                            ".RRRRR."
                            "RRRRRRR"
                            "xxxWxxx"
                            "wwwwwww"
                            ".wwwww."
                            "..www.."};

static const Sprite BOLT = {4, 8,
                            "..ll"
                            ".ll."
                            "ll.."
                            "llll"
                            "..ll"
                            ".ll."
                            ".l.."
                            "l..."};

// Walking right, the flame at the tip of its tail ('F') flickering.
static const Sprite CHARMANDER_A = {8, 8,
                                    "....ooo."
                                    "...ooxoo"
                                    "....oooo"
                                    "F..oo..."
                                    "Fo.oooo."
                                    ".oocco.."
                                    "...oco.."
                                    "...o.o.."};
static const Sprite CHARMANDER_B = {8, 8,
                                    "....ooo."
                                    "...ooxoo"
                                    "....oooo"
                                    "F..oo..."
                                    "Fo.oooo."
                                    ".oocco.."
                                    "...oco.."
                                    "..o..o.."};

// Its ears and curl on top, 'B' its big eyes and 'w' their glint.
static const Sprite JIGGLYPUFF = {7, 7,
                                  "pp.p.pp"
                                  "ppppppp"
                                  "pBBpBBp"
                                  "pBwpBwp"
                                  "ppppppp"
                                  ".ppppp."
                                  "..p.p.."};

static const Sprite NOTE = {3, 4,
                            ".nn"
                            ".n."
                            "nn."
                            "nn."};

// Flopping right, 'g' its fins and whiskers.
static const Sprite MAGIKARP_A = {8, 5,
                                  "...gg..."
                                  "g.mmmmm."
                                  "ggmmmmwm"
                                  "g.mmmmmg"
                                  "...m.m.."};
static const Sprite MAGIKARP_B = {8, 5,
                                  "g..gg..."
                                  ".gmmmmm."
                                  ".gmmmmwm"
                                  "g.mmmmmg"
                                  "...m.m.."};

static constexpr Rgb SPARK = {255, 230, 80};
static constexpr Rgb CHEEK_FLASH = {255, 255, 120};
static constexpr Rgb STAR = {255, 240, 160};
static constexpr Rgb WATER = {0, 90, 255};
// Colours the flame of Charmander's tail takes in turn.
static constexpr Rgb FLAMES[] = {{255, 0, 0}, {255, 90, 0}, {255, 200, 0}};
static constexpr int FLAME_COUNT = sizeof(FLAMES) / sizeof(FLAMES[0]);

inline Rgb ink(char c) {
  switch (c) {
    case 'y':
      return {255, 200, 0};  // Pikachu
    case 'k':
      return {120, 45, 0};  // tips of its ears
    case 'b':
      return {140, 60, 0};  // base of its tail
    case 'r':
      return {255, 0, 0};  // its cheeks
    case 'R':
      return {230, 0, 0};  // Poké Ball, top
    case 'w':
      return {200, 200, 200};  // Poké Ball, bottom; glint of Jigglypuff's eyes, Magikarp's eye
    case 'W':
      return {255, 255, 255};  // Poké Ball's button
    case 'o':
      return {255, 70, 0};  // Charmander
    case 'c':
      return {255, 170, 60};  // its belly
    case 'F':
      return FLAMES[1];  // its flame (flickers, see draw_charmander_)
    case 'p':
      return {255, 60, 120};  // Jigglypuff
    case 'B':
      return {0, 170, 170};  // its eyes
    case 'n':
      return {170, 190, 255};  // its song's notes
    case 'm':
      return {255, 50, 0};  // Magikarp
    case 'g':
      return {255, 200, 90};  // its fins and whiskers
    case 'l':
      return {255, 245, 150};  // thunderbolt
    default:
      return BLACK;  // 'x'
  }
}

// Off the face, around the cheeks: where the sparks of a crackle appear.
static constexpr int8_t CHEEK_SPARKS[][2] = {{0, 4}, {0, 5}, {0, 6}, {0, 7}, {1, 7},
                                             {9, 4}, {9, 5}, {9, 6}, {9, 7}, {8, 7}};
static constexpr int CHEEK_SPARK_COUNT = sizeof(CHEEK_SPARKS) / sizeof(CHEEK_SPARKS[0]);

enum Kind : uint8_t { NONE, PIKACHU, POKE_BALL, THUNDER, CHARMANDER, JIGGLYPUFF_KIND, MAGIKARP, KIND_COUNT };

class Decor : public matrix_decor::Cast {
 public:
  Decor() : Cast(KIND_COUNT - 1) {}

  // Starts a character now; NONE picks one at random, never the previous one.
  void start(uint32_t now, Kind kind = NONE) {
    this->pick_(now, kind);
    this->ball_x_ = 14 + (int) (this->next_() % 9);   // lands over the time
    this->hover_x_ = 16 + (int) (this->next_() % 5);  // Jigglypuff sings over the time
    for (int i = 0; i < 3; i++) {
      this->bolt_x_[i] = 11 + (int) (this->next_() % 18);
      this->bolt_mirror_[i] = this->next_() % 2;
    }
  }

  // Draws Pikachu's face at (0, 0), then the current character. `colon_x` (the column of the
  // time's colon) is unused: same signature as the Halloween decor.
  template<class Put> void draw(Put &&put, uint32_t now, int colon_x) {
    (void) colon_x;
    if (this->due_(now))
      this->start(now);
    this->draw_face_(put, now);
    if (this->kind_ == NONE)
      return;
    uint32_t t = now - this->start_at_;
    bool running = false;
    switch (this->kind_) {
      case PIKACHU:
        running = this->draw_pikachu_(put, t);
        break;
      case POKE_BALL:
        running = this->draw_poke_ball_(put, t);
        break;
      case THUNDER:
        running = this->draw_thunder_(put, t);
        break;
      case CHARMANDER:
        running = this->draw_charmander_(put, now, t);
        break;
      case JIGGLYPUFF_KIND:
        running = this->draw_jigglypuff_(put, t);
        break;
      case MAGIKARP:
        running = this->draw_magikarp_(put, t);
        break;
      default:
        break;
    }
    if (!running)
      this->done_(now);
  }

 protected:
  // Blinks every 2.5-6 s. Every 10-25 s, and all through a thunderbolt, its cheeks crackle:
  // they flash and sparks jump around them.
  template<class Put> void draw_face_(Put &put, uint32_t now) {
    if ((int32_t) (now - this->blink_at_) >= 0) {
      this->blink_until_ = now + 130;
      this->blink_at_ = now + this->between_(2500, 6000);
    }
    if ((int32_t) (now - this->crackle_at_) >= 0) {
      this->crackle_until_ = now + 700;
      this->crackle_at_ = now + this->between_(10000, 25000);
    }
    bool closed = (int32_t) (this->blink_until_ - now) > 0;
    bool crackling = (int32_t) (this->crackle_until_ - now) > 0 || this->kind_ == THUNDER;
    uint32_t slot = now / 70;
    bool flash = crackling && slot % 2 == 0;
    blit(put, FACE, 0, 0, false, [closed, flash](char c) -> Rgb {
      if (c == 'x' && closed)
        return ink('y');
      if (c == 'r' && flash)
        return CHEEK_FLASH;
      return ink(c);
    });
    if (!flash)
      return;
    if (slot != this->spark_slot_) {
      this->spark_slot_ = slot;
      this->spark_[0] = (uint8_t) (this->next_() % CHEEK_SPARK_COUNT);
      this->spark_[1] = (uint8_t) (this->next_() % CHEEK_SPARK_COUNT);
    }
    for (uint8_t i : this->spark_)
      put_clipped(put, CHEEK_SPARKS[i][0], CHEEK_SPARKS[i][1], SPARK);
  }

  // Runs across on all fours.
  template<class Put> bool draw_pikachu_(Put &put, uint32_t t) {
    const uint32_t dur = 2600;
    if (t >= dur)
      return false;
    const Sprite &s = (t / 100) % 2 ? PIKACHU_B : PIKACHU_A;
    blit(put, s, this->across_(t, dur, s.w), 0, this->dir_ < 0, ink);
    return true;
  }

  // Thrown in from the side, it lands over the time, wobbles three times and clicks shut, stars
  // twinkling around it: caught! Then it vanishes pixel by pixel (fading would dim it to black
  // over the time).
  template<class Put> bool draw_poke_ball_(Put &put, uint32_t t) {
    const uint32_t fly = 700, settle = 300, wobble = 800, sparkle = 1000, vanish = 400;
    const uint32_t clicked = fly + settle + 3 * wobble;
    if (t >= clicked + sparkle + vanish)
      return false;
    int x = this->ball_x_, y = 1, tilt = 0;
    float gone = 0.0f;
    if (t < fly) {
      // On an arc, from just off the side.
      float f = (float) t / fly;
      int from = this->dir_ > 0 ? -BALL.w : WIDTH;
      x = (int) std::lround(from + (this->ball_x_ - from) * f);
      y = (int) std::lround(3.0f - 2.0f * f - 10.0f * f * (1.0f - f));
    } else if (t >= fly + settle && t < clicked) {
      uint32_t w = (t - fly - settle) % wobble;
      tilt = w < 160 ? -1 : w < 320 ? 1 : 0;
    } else if (t >= clicked) {
      uint32_t s = t - clicked;
      // Three stars twinkle, out of step.
      const int stars[3][2] = {{x - 2, 2}, {x + 3, 0}, {x + 8, 3}};
      for (int i = 0; i < 3; i++) {
        float k = std::sin((s + i * 230) * TWO_PI / 700.0f);
        if (k <= 0.0f || s >= sparkle)
          continue;
        int sx = stars[i][0], sy = stars[i][1];
        put_clipped(put, sx, sy, scale(STAR, 0.4f + 0.6f * k));
        if (k > 0.5f) {
          Rgb arm = scale(STAR, 0.7f * k);
          put_clipped(put, sx - 1, sy, arm);
          put_clipped(put, sx + 1, sy, arm);
          put_clipped(put, sx, sy - 1, arm);
          put_clipped(put, sx, sy + 1, arm);
        }
      }
      if (s >= sparkle)
        gone = (float) (s - sparkle) / vanish;
    }
    // A wobble leans the top half one way, then the other.
    for (int sy = 0; sy < BALL.h; sy++) {
      for (int sx = 0; sx < BALL.w; sx++) {
        char c = BALL.px[sy * BALL.w + sx];
        if (c != '.' && (sx * 5 + sy * 3) % 7 >= gone * 7)
          put_clipped(put, x + sx + (sy < 3 ? tilt : 0), y + sy, ink(c));
      }
    }
    return true;
  }

  // Pikachu's cheeks charge up, then three bolts strike over the time, one after the other.
  template<class Put> bool draw_thunder_(Put &put, uint32_t t) {
    const uint32_t charge = 800, strike = 420, gap = 140;
    if (t < charge)
      return true;
    uint32_t s = t - charge;
    uint32_t i = s / (strike + gap);
    if (i >= 3)
      return false;
    uint32_t u = s % (strike + gap);
    if (u >= strike)
      return true;
    // Flash, flicker off, back on, fade.
    float level = u < 70 ? 1.0f : u < 120 ? 0.0f : u < 260 ? 1.0f : 1.0f - (float) (u - 260) / (strike - 260);
    blit(put, BOLT, this->bolt_x_[i], 0, this->bolt_mirror_[i], [level](char c) { return scale(ink(c), level); });
    return true;
  }

  // Walks across, the flame of its tail flickering.
  template<class Put> bool draw_charmander_(Put &put, uint32_t now, uint32_t t) {
    const uint32_t dur = 5500;
    if (t >= dur)
      return false;
    if ((int32_t) (now - this->flame_at_) >= 0) {
      this->flame_ = (uint8_t) ((this->flame_ + 1 + this->next_() % (FLAME_COUNT - 1)) % FLAME_COUNT);
      this->flame_at_ = now + this->between_(60, 140);
    }
    Rgb flame = FLAMES[this->flame_];
    const Sprite &s = (t / 250) % 2 ? CHARMANDER_B : CHARMANDER_A;
    blit(put, s, this->across_(t, dur, s.w), 0, this->dir_ < 0, [flame](char c) { return c == 'F' ? flame : ink(c); });
    return true;
  }

  // Floats in, stops over the time and sings, notes rising on both sides, then floats away.
  template<class Put> bool draw_jigglypuff_(Put &put, uint32_t t) {
    const uint32_t in = 1000, sing = 3200, out = 1000, note_life = 1400, note_every = 500;
    if (t >= in + sing + note_life)
      return false;
    const int w = JIGGLYPUFF.w;
    float from = this->dir_ > 0 ? -w : WIDTH, to = this->dir_ > 0 ? WIDTH : -w;
    float x = this->hover_x_;
    int y = 1;
    if (t < in) {
      float f = (float) t / in;
      x = from + (this->hover_x_ - from) * (1.0f - (1.0f - f) * (1.0f - f));
      y = (t / 250) % 2;
    } else if (t < in + sing) {
      x += (t / 500) % 2;  // sways to its song
    } else {
      float f = std::min(1.0f, (float) (t - in - sing) / out);
      x += (to - this->hover_x_) * f * f;
      y = (t / 250) % 2;
    }
    for (uint32_t born = in + 200; born < in + sing && born <= t; born += note_every) {
      uint32_t age = t - born;
      if (age >= note_life)
        continue;
      int k = (born - in) / note_every;
      int drift = (int) (age / 350);
      int nx = k % 2 ? this->hover_x_ + w + drift : this->hover_x_ - NOTE.w - drift;
      int ny = 3 - (int) (age / 230);
      float level = age < note_life - 300 ? 1.0f : (float) (note_life - age) / 300;
      blit(put, NOTE, nx, ny, false, [level](char c) { return scale(ink(c), level); });
    }
    blit(put, JIGGLYPUFF, (int) std::lround(x), y, false, ink);
    return true;
  }

  // Flops across in hops, splashing where it lands.
  template<class Put> bool draw_magikarp_(Put &put, uint32_t t) {
    const uint32_t hop = 900, hops = 5, dur = hop * hops, splash = 450;
    if (t >= dur)
      return false;
    const int w = MAGIKARP_A.w;
    float p = (float) (t % hop) / hop;
    int x = this->across_(t, dur, w);
    int y = 3 - (int) std::lround(4.0f * std::sin(p * TWO_PI / 2.0f));
    // Drops fly up on both sides of where it last landed.
    uint32_t landed = t - t % hop;
    if (landed > 0 && t - landed < splash) {
      float s = (float) (t - landed) / splash;
      int lx = this->across_(landed, dur, w) + w / 2;
      int dy = (int) std::lround(10.0f * s * (1.0f - s));
      Rgb c = scale(WATER, 1.0f - 0.5f * s);
      for (int d : {-2, -1, 1, 2}) {
        int dx = (int) std::lround(d * (1.0f + 2.0f * s));
        put_clipped(put, lx + dx, 7 - dy + (d == -1 || d == 1 ? -1 : 0), c);
      }
    }
    blit(put, (t / 150) % 2 ? MAGIKARP_B : MAGIKARP_A, x, y, this->dir_ < 0, ink);
    return true;
  }

  int ball_x_{17};
  int hover_x_{17};
  int bolt_x_[3]{};
  bool bolt_mirror_[3]{};
  uint32_t blink_at_{0}, blink_until_{0};
  uint32_t crackle_at_{0}, crackle_until_{0};
  uint32_t spark_slot_{0};
  uint8_t spark_[2]{};
  uint8_t flame_{0};
  uint32_t flame_at_{0};
};

}  // namespace matrix_pokemon

// The clock's instance, drawn from the display lambda.
inline matrix_pokemon::Decor pokemon_decor;
