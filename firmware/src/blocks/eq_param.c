/* eq_param.c — 3-band parametric EQ block (low shelf, mid peak, high shelf).
 * Real, self-contained DSP (RBJ cookbook). A template for the block interface.
 */
#include "../block.h"
#include "../dsp/biquad.h"

typedef struct { tf_biquad lo, mid, hi; float fs; } eq_state;

/* params: 0 bass dB, 1 mid dB, 2 mid freq, 3 treble dB  (normalized 0..1) */
static const tf_param EQ_PARAMS[] = {
    { "bass",   0.5f, 0.5f, 0 },   /* -12..+12 dB */
    { "mid",    0.5f, 0.5f, 0 },
    { "midf",   0.4f, 0.4f, 0 },   /* 300..3000 Hz */
    { "treble", 0.5f, 0.5f, 0 },
};

static float db(float v01)  { return (v01 - 0.5f) * 24.0f; }           /* -12..+12 */
static float mf(float v01)  { return 300.0f * powf(10.0f, v01 * 1.0f); } /* 300..3000 */

static void eq_recalc(tf_block *b) {
    eq_state *s = b->state;
    tf_biquad_set(&s->lo, 1, s->fs, 120.0f, 0.707f, db(b->p[0].value));
    tf_biquad_set(&s->mid, 0, s->fs, mf(b->p[2].value), 0.9f, db(b->p[1].value));
    tf_biquad_set(&s->hi, 2, s->fs, 3000.0f, 0.707f, db(b->p[3].value));
}

static void eq_init(tf_block *b, float fs) { eq_state *s = b->state; s->fs = fs; eq_recalc(b); }
static void eq_reset(tf_block *b) { eq_state *s = b->state; s->lo.z1=s->lo.z2=s->mid.z1=s->mid.z2=s->hi.z1=s->hi.z2=0; }
static void eq_setp(tf_block *b, int i, float v) { b->p[i].value = v; eq_recalc(b); }

static void eq_process(tf_block *b, float *l, float *r, int n) {
    eq_state *s = b->state;
    for (int i = 0; i < n; i++) {
        float x = l[i];
        x = tf_biquad_tick(&s->lo, x);
        x = tf_biquad_tick(&s->mid, x);
        x = tf_biquad_tick(&s->hi, x);
        l[i] = r[i] = x;             /* mono path (pedal is mono in) */
    }
}

const tf_block_desc TF_EQ_PARAM = {
    .id = "eq.param3", .category = TF_CAT_EQ,
    .state_bytes = sizeof(eq_state), .cpu_cost = 3,
    .init = eq_init, .reset = eq_reset, .process = eq_process, .set_param = eq_setp,
    .params = EQ_PARAMS, .nparams = 4,
};
