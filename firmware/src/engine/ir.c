/* ir.c — cabinet IR: direct FIR convolution (mono, float). Clean-room.
 * Direct form is fine for short IRs (≤~1024 taps). Long IRs (up to 85 ms = 4096
 * taps @48k) should use the chip's FFT accelerator / partitioned conv (TODO). */
#include "ir.h"
#include "../tf_str.h"

void ir_set(ir_conv *c, const float *taps, int ntaps) {
    if (ntaps > IR_MAX_TAPS) ntaps = IR_MAX_TAPS;
    c->n = ntaps;
    for (int i = 0; i < ntaps; i++) c->h[i] = taps[i];
    c->pos = 0;
    memset(c->z, 0, sizeof(float) * IR_MAX_TAPS);
}

void ir_process(ir_conv *c, float *x, int n) {
    if (c->n <= 0) return;
    for (int i = 0; i < n; i++) {
        c->z[c->pos] = x[i];
        float acc = 0.f; int p = c->pos;
        for (int k = 0; k < c->n; k++) { acc += c->h[k] * c->z[p]; p = p ? p - 1 : IR_MAX_TAPS - 1; }
        if (++c->pos >= IR_MAX_TAPS) c->pos = 0;
        x[i] = acc;
    }
}
