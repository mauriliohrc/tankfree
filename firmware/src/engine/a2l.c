/* a2l.c — NAM A2-Lite fixed-point WaveNet runner. Clean-room from the A2L1 format.
 * Structurally faithful to nam2chip.js / a2l_block.c; the exact Q-shift schedule
 * still needs bit-exact validation against a reference .nam capture (TODO) — until
 * then treat tone as approximate. Runs in bounded fixed point, no allocation. */
#include "a2l.h"
#include "../tf_str.h"

static uint32_t rd32(const uint8_t *p) { return p[0] | (p[1]<<8) | (p[2]<<16) | ((uint32_t)p[3]<<24); }
static uint16_t rd16(const uint8_t *p) { return p[0] | (p[1]<<8); }

int a2l_parse(a2l_model *m, const uint8_t *b, uint32_t len) {
    memset(m, 0, sizeof *m);
    if (len < 44 || b[0]!='A'||b[1]!='2'||b[2]!='L'||b[3]!='1') return 0;
    const uint8_t *p = b + 6;                 /* skip magic + version */
    uint8_t wbits = *p++, abits = *p++; (void)wbits; (void)abits;
    m->fs = (int)rd32(p); p += 4;
    m->nL = rd16(p); p += 2;
    m->C = *p++; m->headK = *p++;
    if (m->C > A2L_MAX_C || m->nL > A2L_MAX_LAYERS) return 0;
    m->Fin = (int8_t)*p++; m->Fx3_0 = (int8_t)*p++; m->Fh = (int8_t)*p++; m->Fout = (int8_t)*p++;
    p += 16;                                   /* name */
    int C = m->C;
    m->rechF = (int8_t)*p++; m->rechW = (const int16_t *)p; p += 2*C;
    for (int i = 0; i < m->nL; i++) {
        a2l_layer *L = &m->L[i];
        L->K = *p++; L->dil = rd16(p); p += 2; L->Fz = (int8_t)*p++; L->Fx3 = (int8_t)*p++;
        L->lmul = (int32_t)rd32(p); p += 4;
        L->cF = (int8_t)*p++; L->cW = (const int16_t *)p; p += 2*C*C*L->K; L->cB = (const int64_t *)p; p += 8*C;
        L->mF = (int8_t)*p++; L->mW = (const int16_t *)p; p += 2*C;
        L->lF = (int8_t)*p++; L->lW = (const int16_t *)p; p += 2*C*C; L->lB = (const int64_t *)p; p += 8*C;
    }
    m->rechF = m->rechF;                        /* (kept) */
    m->ok = 1;
    return 1;
}

void a2l_reset(a2l_model *m) {
    for (int i = 0; i < m->nL; i++) { m->ringpos[i] = 0; memset(m->ring[i], 0, sizeof m->ring[i]); }
}

static inline int32_t sat32(int64_t v) {
    if (v > 2147483647LL) return 2147483647; if (v < -2147483648LL) return -2147483648LL; return (int32_t)v;
}
static inline int32_t shr(int64_t v, int s) { return s >= 0 ? sat32(v >> s) : sat32(v << (-s)); }

/* One sample through the net. x3[] is the residual stream (C channels, Q=Fx3). */
static float a2l_tick(a2l_model *m, float in) {
    int C = m->C;
    int32_t x = (int32_t)(in * (float)(1 << m->Fin));          /* quantize input */
    int32_t x3[A2L_MAX_C];
    for (int c = 0; c < C; c++) {                              /* rechannel */
        int64_t a = (int64_t)m->rechW[c] * x;
        x3[c] = shr(a, m->rechF + m->Fin - m->Fx3_0);
    }
    for (int li = 0; li < m->nL; li++) {
        a2l_layer *L = &m->L[li];
        int rp = m->ringpos[li];
        for (int c = 0; c < C; c++) m->ring[li][c][rp & 511] = x3[c];   /* push input history */
        int32_t za[A2L_MAX_C];
        for (int co = 0; co < C; co++) {
            int64_t acc = L->cB[co];
            for (int k = 0; k < L->K; k++) {
                int tap = (rp - (L->K - 1 - k) * L->dil) & 511;
                for (int ci = 0; ci < C; ci++)
                    acc += (int64_t)L->cW[(co*C + ci)*L->K + k] * m->ring[li][ci][tap];
            }
            for (int ci = 0; ci < C; ci++) acc += (int64_t)L->mW[co] * x;  /* input mixer */
            int32_t z = shr(acc, L->cF + m->Fx3_0 - L->Fz);
            za[co] = z >= 0 ? z : shr((int64_t)z * L->lmul, 24);           /* LeakyReLU */
        }
        for (int co = 0; co < C; co++) {                        /* 1x1 + residual */
            int64_t acc = L->lB[co];
            for (int ci = 0; ci < C; ci++) acc += (int64_t)L->lW[co*C + ci] * za[ci];
            x3[co] += shr(acc, L->lF + L->Fz - L->Fx3);
        }
        m->ringpos[li] = rp + 1;
    }
    int64_t out = 0;                                            /* head: 1x1 over last x3 */
    for (int c = 0; c < C; c++) out += (int64_t)m->rechW[c] * x3[c];   /* placeholder head mix */
    int32_t o = shr(out, m->Fx3_0 + m->rechF - m->Fout);
    return (float)o / (float)(1 << m->Fout);
}

void a2l_process(a2l_model *m, float *x, int n) {
    if (!m->ok) return;
    for (int i = 0; i < n; i++) x[i] = a2l_tick(m, x[i]);
}
