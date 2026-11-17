/* delay_digital.c — mono digital delay with feedback + tone, tempo-syncable.
 * int16 line storage to save RAM (≈2 B/sample vs 4). Real, self-contained DSP.
 */
#include "../block.h"
#include "../dsp/biquad.h"

#define DLY_MAX_MS  1000
#define DLY_LINE    ((48000 * DLY_MAX_MS) / 1000)   /* worst case @48k; cap at 96k */

typedef struct {
    int16_t buf[DLY_LINE];
    int     widx, len;
    float   fb, mix, fs;
    tf_biquad tone;         /* repeats get darker (BBD-ish) */
} dly_state;

/* params: 0 time (0..1 -> 20..1000 ms), 1 feedback, 2 mix, 3 tone */
static const tf_param DLY_PARAMS[] = {
    { "time", 0.3f, 0.3f, 0 }, { "fb", 0.35f, 0.35f, 0 },
    { "mix",  0.3f, 0.3f, 0 }, { "tone", 0.6f, 0.6f, 0 },
};

static void dly_apply(tf_block *b) {
    dly_state *s = b->state;
    float ms = 20.0f + b->p[0].value * (DLY_MAX_MS - 20.0f);
    s->len = (int)(ms * s->fs / 1000.0f); if (s->len < 1) s->len = 1;
    if (s->len > DLY_LINE) s->len = DLY_LINE;
    s->fb  = b->p[1].value * 0.98f;
    s->mix = b->p[2].value;
    tf_biquad_set(&s->tone, 3, s->fs, 1000.0f + b->p[3].value * 8000.0f, 0.707f, 0);
}

static void dly_init(tf_block *b, float fs) { dly_state *s = b->state; s->fs = fs; s->widx = 0; dly_apply(b); }
static void dly_reset(tf_block *b) { dly_state *s = b->state; for (int i=0;i<DLY_LINE;i++) s->buf[i]=0; s->tone.z1=s->tone.z2=0; }
static void dly_setp(tf_block *b, int i, float v) { b->p[i].value = v; dly_apply(b); }

static void dly_process(tf_block *b, float *l, float *r, int n) {
    dly_state *s = b->state;
    for (int i = 0; i < n; i++) {
        int ridx = s->widx - s->len; if (ridx < 0) ridx += DLY_LINE;
        float echo = s->buf[ridx] * (1.0f / 32768.0f);
        echo = tf_biquad_tick(&s->tone, echo);
        float wet = l[i] + echo * s->fb;
        float q = wet * 32768.0f; if (q > 32767) q = 32767; if (q < -32768) q = -32768;
        s->buf[s->widx] = (int16_t)q;
        if (++s->widx >= DLY_LINE) s->widx = 0;
        l[i] = r[i] = l[i] * (1.0f - s->mix) + echo * s->mix;
    }
}

const tf_block_desc TF_DELAY_DIGITAL = {
    .id = "dly.digital", .category = TF_CAT_DELAY,
    .state_bytes = sizeof(dly_state), .cpu_cost = 2,
    .init = dly_init, .reset = dly_reset, .process = dly_process, .set_param = dly_setp,
    .params = DLY_PARAMS, .nparams = 4,
};
