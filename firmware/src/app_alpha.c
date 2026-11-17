/* app_alpha.c — TankFree Alpha audio pipeline.
 *
 * Target Alpha chain (fixed for now):  input → Noise Gate → NAM → IR → Master
 * Audio I/O is the SDK/board layer (USB Audio Class + codec) — those hooks are
 * `extern` here and implemented in the SDK integration (see docs/SDK_INTEGRATION.md).
 * This file is OUR DSP wiring; it compiles for pi32v2 today.
 *
 * Signal sources/sinks (USB soundcard, like the factory Tank-G):
 *   in  = guitar ADC  (and/or USB playback from the DAW)
 *   out = DAC (headphones/XLR)  AND  USB capture (to the DAW)
 */
#include "engine/a2l.h"
#include "engine/ir.h"
#include "block.h"
#include "dsp/tf_math.h"

/* ---- board/SDK hooks (implemented in the SDK layer) ---- */
extern int  tf_board_audio_in(float *buf, int n);    /* fill buf with input samples */
extern void tf_board_audio_out(const float *buf, int n); /* DAC + USB capture */
extern int  tf_board_usb_playback(float *buf, int n); /* optional DAW→pedal mix */

/* ---- Alpha state ---- */
static a2l_model g_nam;
static ir_conv   g_ir;
static int       g_nam_ok, g_ir_ok;
static float     g_master = 0.7f;    /* master knob 0..1 */
static float     g_gate_thr = 0.01f, g_gate_env, g_gate_gain;

void tf_alpha_set_master(float v)   { g_master = v < 0 ? 0 : v > 1 ? 1 : v; }
void tf_alpha_set_gate(float thr01) { g_gate_thr = TF_POW(10.f, (-70.f + thr01 * 50.f) / 20.f); }

/* Load a NAM capture (A2L1 blob) and a cab IR. */
int tf_alpha_load_nam(const uint8_t *blob, uint32_t len) {
    g_nam_ok = a2l_parse(&g_nam, blob, len);
    if (g_nam_ok) a2l_reset(&g_nam);
    return g_nam_ok;
}
int tf_alpha_load_ir(const float *taps, int n) { ir_set(&g_ir, taps, n); g_ir_ok = n > 0; return g_ir_ok; }

void tf_alpha_init(void) {
    g_nam_ok = g_ir_ok = 0; g_gate_env = g_gate_gain = 0.f;
}

/* Process one audio block in place. Called by the SDK audio callback. */
void tf_alpha_process(float *buf, int n) {
    /* noise gate (simple, inlined) */
    for (int i = 0; i < n; i++) {
        float a = buf[i] < 0 ? -buf[i] : buf[i];
        g_gate_env = a + (g_gate_env - a) * (a > g_gate_env ? 0.5f : 0.999f);
        float target = g_gate_env > g_gate_thr ? 1.f : 0.f;
        g_gate_gain = target + (g_gate_gain - target) * (target > g_gate_gain ? 0.0f : 0.9995f);
        buf[i] *= g_gate_gain;
    }
    if (g_nam_ok) a2l_process(&g_nam, buf, n);     /* amp capture */
    if (g_ir_ok)  ir_process(&g_ir, buf, n);        /* cabinet */
    for (int i = 0; i < n; i++) buf[i] *= g_master; /* master */
}

/* The audio loop the SDK task drives (one block). */
void tf_alpha_tick(int block) {
    float buf[128]; if (block > 128) block = 128;
    tf_board_audio_in(buf, block);
    tf_alpha_process(buf, block);
    tf_board_audio_out(buf, block);
}
