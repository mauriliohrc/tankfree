/* drive_soft.c — soft-clip overdrive (tanh waveshaper) with drive/tone/level.
 * A cheap 🟢 drive; for circuit-accurate pedals port BYOD WDF models later.
 */
#include "../block.h"
#include "../dsp/biquad.h"
#include "../dsp/tf_math.h"

typedef struct { tf_biquad tone; float drive, level, fs; } drv_state;

static const tf_param DRV_PARAMS[] = {
    { "drive", 0.4f, 0.4f, 0 }, { "tone", 0.6f, 0.6f, 0 }, { "level", 0.6f, 0.6f, 0 },
};

static void drv_apply(tf_block *b) {
    drv_state *s = b->state;
    s->drive = 1.0f + b->p[0].value * 40.0f;
    s->level = b->p[2].value * 1.5f;
    tf_biquad_set(&s->tone, 3, s->fs, 800.0f + b->p[1].value * 7000.0f, 0.707f, 0);
}
static void drv_init(tf_block *b, float fs) { drv_state *s = b->state; s->fs = fs; drv_apply(b); }
static void drv_reset(tf_block *b) { drv_state *s = b->state; s->tone.z1 = s->tone.z2 = 0; }
static void drv_setp(tf_block *b, int i, float v) { b->p[i].value = v; drv_apply(b); }

static void drv_process(tf_block *b, float *l, float *r, int n) {
    drv_state *s = b->state;
    float g = s->drive, inv = 1.0f / TF_SQRT(g);
    for (int i = 0; i < n; i++) {
        float x = TF_TANH(l[i] * g) * inv;      /* soft clip, loudness-compensated */
        x = tf_biquad_tick(&s->tone, x) * s->level;
        l[i] = r[i] = x;
    }
}

const tf_block_desc TF_DRIVE_SOFT = {
    .id = "drv.soft", .category = TF_CAT_DRIVE,
    .state_bytes = sizeof(drv_state), .cpu_cost = 2,
    .init = drv_init, .reset = drv_reset, .process = drv_process, .set_param = drv_setp,
    .params = DRV_PARAMS, .nparams = 3,
};
