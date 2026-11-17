/* tf_math.h — math for the DSP blocks.
 * On host: maps to libm (bit-exact tests). On pi32v2 (no libm, no HW transcendentals):
 * fast polynomial/rational approximations — good enough for audio, cheap on the chip.
 * Blocks call TF_SIN/TF_COS/TF_POW/TF_TANH/TF_SQRT/… instead of sinf/cosf/…
 */
#ifndef TF_MATH_H
#define TF_MATH_H

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#if defined(__pi32v2__) || defined(TF_EMBED)
/* ---------------- embedded: self-contained approximations ---------------- */
static inline float tf_fabsf(float x) { return x < 0.f ? -x : x; }
static inline float tf_floorf(float x) { int i = (int)x; return (float)(i > x ? i - 1 : i); }
static inline float tf_sqrtf(float x) { return x <= 0.f ? 0.f : __builtin_sqrtf(x); }

/* sin/cos: range-reduce to [-pi,pi], 5th/6th-order minimax */
static inline float tf_sinf(float x) {
    const float twopi = 6.2831853f, invtwopi = 0.15915494f;
    x -= twopi * tf_floorf(x * invtwopi + 0.5f);          /* -> [-pi,pi] */
    float x2 = x * x;
    return x * (0.9999966f + x2 * (-0.16664824f + x2 * (0.00830629f - x2 * 0.00018363f)));
}
static inline float tf_cosf(float x) { return tf_sinf(x + 1.5707963f); }

/* tanh: Padé-style rational, clamped — classic fast saturator */
static inline float tf_tanhf(float x) {
    if (x > 4.97f) return 1.f; if (x < -4.97f) return -1.f;
    float x2 = x * x;
    return x * (27.f + x2) / (27.f + 9.f * x2);
}

/* exp2/log2 via IEEE-754 bit tricks + poly; exp/log/pow built on them */
static inline float tf_log2f(float x) {
    union { float f; unsigned u; } v = { x };
    float e = (float)((v.u >> 23) & 0xFF) - 127.f;
    v.u = (v.u & 0x007FFFFF) | 0x3F800000; float m = v.f;   /* [1,2) */
    return e + (-1.7417939f + m * (2.8212026f + m * (-1.4699568f + m * (0.44717955f - m * 0.056570851f))));
}
static inline float tf_exp2f(float x) {
    float xi = tf_floorf(x), xf = x - xi;
    float p = 1.f + xf * (0.6931472f + xf * (0.2402265f + xf * (0.0555041f + xf * 0.0096181f)));
    union { float f; unsigned u; } v;
    int e = (int)xi + 127; if (e < 1) e = 1; if (e > 254) e = 254;
    v.u = ((unsigned)e) << 23; return p * v.f;
}
static inline float tf_logf(float x) { return x <= 0.f ? -1e30f : tf_log2f(x) * 0.6931472f; }
static inline float tf_expf(float x) { return tf_exp2f(x * 1.4426950f); }
static inline float tf_powf(float b, float e) { return b <= 0.f ? 0.f : tf_exp2f(e * tf_log2f(b)); }

#define TF_SIN tf_sinf
#define TF_COS tf_cosf
#define TF_TANH tf_tanhf
#define TF_SQRT tf_sqrtf
#define TF_POW tf_powf
#define TF_EXP tf_expf
#define TF_LOG tf_logf
#define TF_LOG2 tf_log2f
#define TF_FLOOR tf_floorf
#define TF_FABS tf_fabsf

#else
/* ---------------- host: use libm (exact, for tests) ---------------- */
#include <math.h>
#define TF_SIN sinf
#define TF_COS cosf
#define TF_TANH tanhf
#define TF_SQRT sqrtf
#define TF_POW powf
#define TF_EXP expf
#define TF_LOG logf
#define TF_LOG2 log2f
#define TF_FLOOR floorf
#define TF_FABS fabsf
#endif

#endif /* TF_MATH_H */
