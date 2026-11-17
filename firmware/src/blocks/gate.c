/* gate.c — noise gate (threshold + hysteresis + program-dependent release).
 * Cheap, clean-room. One of the Alpha blocks. */
#include "../block.h"
#include "../dsp/tf_math.h"

typedef struct { float env, gain, fs, thr, rel_c, open_c; } gate_state;

/* params: 0 threshold (dB-ish via 0..1), 1 release */
static const tf_param GATE_PARAMS[] = {
    { "threshold", 0.3f, 0.3f, 0 }, { "release", 0.4f, 0.4f, 0 },
};

static void gate_apply(tf_block *b) {
    gate_state *s = b->state;
    /* threshold 0..1 -> -70..-20 dBFS linear */
    float db = -70.f + b->p[0].value * 50.f;
    s->thr = TF_POW(10.f, db / 20.f);
    float rel_ms = 20.f + b->p[1].value * 480.f;               /* 20..500 ms */
    s->rel_c = TF_EXP(-1.f / (rel_ms * 0.001f * s->fs));
    s->open_c = TF_EXP(-1.f / (0.001f * s->fs));               /* 1 ms attack */
}
static void gate_init(tf_block *b, float fs) { gate_state *s = b->state; s->fs = fs; s->env = s->gain = 0.f; gate_apply(b); }
static void gate_reset(tf_block *b) { gate_state *s = b->state; s->env = 0.f; s->gain = 0.f; }
static void gate_setp(tf_block *b, int i, float v) { b->p[i].value = v; gate_apply(b); }

static void gate_process(tf_block *b, float *l, float *r, int n) {
    gate_state *s = b->state;
    for (int i = 0; i < n; i++) {
        float x = l[i], a = x < 0 ? -x : x;
        s->env = a + (s->env - a) * (a > s->env ? 0.5f : 0.999f);   /* fast peak follow */
        float target = s->env > s->thr ? 1.f : 0.f;
        float c = target > s->gain ? s->open_c : s->rel_c;          /* attack vs release */
        s->gain = target + (s->gain - target) * c;
        l[i] = r[i] = x * s->gain;
    }
}

const tf_block_desc TF_GATE = {
    .id = "dyn.gate", .category = TF_CAT_DYNAMICS,
    .state_bytes = sizeof(gate_state), .cpu_cost = 1,
    .init = gate_init, .reset = gate_reset, .process = gate_process, .set_param = gate_setp,
    .params = GATE_PARAMS, .nparams = 2,
};
