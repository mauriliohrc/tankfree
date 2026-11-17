/* biquad.h — RBJ Audio EQ Cookbook biquads (public domain recipe).
 * https://www.w3.org/TR/audio-eq-cookbook/  — the foundation for EQ/filter/wah.
 * Transposed Direct Form II, single-precision, no allocation.
 */
#ifndef TF_BIQUAD_H
#define TF_BIQUAD_H
#include <math.h>

typedef struct { float b0, b1, b2, a1, a2; float z1, z2; } tf_biquad;

static inline float tf_biquad_tick(tf_biquad *s, float x) {
    float y = s->b0 * x + s->z1;
    s->z1 = s->b1 * x - s->a1 * y + s->z2;
    s->z2 = s->b2 * x - s->a2 * y;
    return y;
}

/* kind: 0=peaking, 1=lowshelf, 2=highshelf, 3=lowpass, 4=highpass, 5=notch */
static inline void tf_biquad_set(tf_biquad *s, int kind, float fs, float f0,
                                 float Q, float gain_db) {
    float A  = powf(10.0f, gain_db / 40.0f);
    float w0 = 2.0f * (float)M_PI * f0 / fs;
    float cw = cosf(w0), sw = sinf(w0);
    float alpha = sw / (2.0f * Q);
    float b0, b1, b2, a0, a1, a2;
    switch (kind) {
    case 1: { /* lowshelf */
        float s2 = 2.0f * sqrtf(A) * alpha;
        b0 = A*((A+1)-(A-1)*cw + s2); b1 = 2*A*((A-1)-(A+1)*cw); b2 = A*((A+1)-(A-1)*cw - s2);
        a0 = (A+1)+(A-1)*cw + s2;     a1 = -2*((A-1)+(A+1)*cw);  a2 = (A+1)+(A-1)*cw - s2; } break;
    case 2: { /* highshelf */
        float s2 = 2.0f * sqrtf(A) * alpha;
        b0 = A*((A+1)+(A-1)*cw + s2); b1 = -2*A*((A-1)+(A+1)*cw); b2 = A*((A+1)+(A-1)*cw - s2);
        a0 = (A+1)-(A-1)*cw + s2;     a1 = 2*((A-1)-(A+1)*cw);    a2 = (A+1)-(A-1)*cw - s2; } break;
    case 3: /* lowpass */
        b0 = (1-cw)/2; b1 = 1-cw; b2 = (1-cw)/2; a0 = 1+alpha; a1 = -2*cw; a2 = 1-alpha; break;
    case 4: /* highpass */
        b0 = (1+cw)/2; b1 = -(1+cw); b2 = (1+cw)/2; a0 = 1+alpha; a1 = -2*cw; a2 = 1-alpha; break;
    case 5: /* notch */
        b0 = 1; b1 = -2*cw; b2 = 1; a0 = 1+alpha; a1 = -2*cw; a2 = 1-alpha; break;
    default: /* peaking */
        b0 = 1+alpha*A; b1 = -2*cw; b2 = 1-alpha*A; a0 = 1+alpha/A; a1 = -2*cw; a2 = 1-alpha/A; break;
    }
    s->b0 = b0/a0; s->b1 = b1/a0; s->b2 = b2/a0; s->a1 = a1/a0; s->a2 = a2/a0;
}

#endif /* TF_BIQUAD_H */
