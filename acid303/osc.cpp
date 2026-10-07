// Acid303 - oscillateur style TB-303 pour Korg NTS-1 (nutekt-digital)
// Saw <-> Square (anti-aliasing polyBLEP), slide, drive, sub, largeur d'impulsion
#include "userosc.h"

typedef struct {
  float phi    = 0.f;   // phase principale
  float phiSub = 0.f;   // phase sub (octave en dessous)
  float w      = 0.f;   // increment de phase courant (avec slide)
  bool  first  = true;
  float slide  = 0.f;   // 0..1
  float drive  = 0.f;   // 0..1
  float sub    = 0.f;   // 0..1
  float pw     = 0.5f;  // 0.1..0.9
} State;

static State s;
static float g_shape = 0.f;

static inline float clampf(float x, float lo, float hi) {
  return x < lo ? lo : (x > hi ? hi : x);
}

static inline float polyblep(float t, float dt) {
  if (t < dt) {
    t /= dt;
    return t + t - t * t - 1.f;
  } else if (t > 1.f - dt) {
    t = (t - 1.f) / dt;
    return t * t + t + t + 1.f;
  }
  return 0.f;
}

// Saturation douce (approximation de tanh)
static inline float softclip(float x) {
  x = clampf(x, -3.f, 3.f);
  return x * (27.f + x * x) / (27.f + 9.f * x * x);
}

void OSC_INIT(uint32_t platform, uint32_t api) {
  s = State();
}

void OSC_CYCLE(const user_osc_param_t * const params,
               int32_t *yn,
               const uint32_t frames) {
  const float target = osc_w0f_for_note((params->pitch) >> 8, params->pitch & 0xFF);

  if (s.first) { s.w = target; s.first = false; }

  // Coefficient de glide: 0 = instantane, 1 = ~300 ms
  const float glideSamples = s.slide * 0.3f * 48000.f;
  const float coef = (glideSamples < 1.f) ? 1.f : (1.f / glideSamples);

  // Shape knob = morph saw (0) -> square (1)
  float shape = clampf(params->shape_lfo * (1.f / 2147483648.f)
                       + 0.f, -1.f, 1.f);                       // LFO shape
  // Valeur de base du knob Shape stockee via OSC_PARAM

  float m = clampf(g_shape + shape * 0.5f, 0.f, 1.f);

  const float driveGain = 1.f + s.drive * 7.f;
  const float driveComp = 1.f / (0.6f + 0.4f * driveGain);

  q31_t * __restrict y = (q31_t *)yn;
  const q31_t * y_e = y + frames;

  for (; y != y_e; ) {
    s.w += (target - s.w) * coef;
    const float dt = s.w;

    // Saw
    float saw = 2.f * s.phi - 1.f;
    saw -= polyblep(s.phi, dt);

    // Square / pulse
    float sq = (s.phi < s.pw) ? 1.f : -1.f;
    sq += polyblep(s.phi, dt);
    float t2 = s.phi + (1.f - s.pw);
    if (t2 >= 1.f) t2 -= 1.f;
    sq -= polyblep(t2, dt);

    float sig = (1.f - m) * saw + m * sq;

    // Sub octave (carre, leger)
    float sb = (s.phiSub < 0.5f) ? 1.f : -1.f;
    sig = sig * (1.f - 0.3f * s.sub) + sb * 0.5f * s.sub;

    // Drive
    sig = softclip(sig * driveGain) * driveComp;

    *(y++) = f32_to_q31(sig * 0.9f);

    s.phi += dt;
    if (s.phi >= 1.f) s.phi -= 1.f;
    s.phiSub += dt * 0.5f;
    if (s.phiSub >= 1.f) s.phiSub -= 1.f;
  }
}


void OSC_NOTEON(const user_osc_param_t * const params) {
  // On ne remet pas la phase a zero: le slide reste continu
}

void OSC_NOTEOFF(const user_osc_param_t * const params) {
}

void OSC_PARAM(uint16_t index, uint16_t value) {
  switch (index) {
    case k_user_osc_param_id1: s.slide = clampf(value * 0.01f, 0.f, 1.f); break;
    case k_user_osc_param_id2: s.drive = clampf(value * 0.01f, 0.f, 1.f); break;
    case k_user_osc_param_id3: s.sub   = clampf(value * 0.01f, 0.f, 1.f); break;
    case k_user_osc_param_shape:
      g_shape = clampf(param_val_to_f32(value), 0.f, 1.f); break;
    case k_user_osc_param_shiftshape:
      s.pw = 0.1f + 0.8f * clampf(param_val_to_f32(value), 0.f, 1.f); break;
    default: break;
  }
}
