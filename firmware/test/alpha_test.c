/* alpha_test.c — the Alpha pipeline (gate → NAM → IR → master) runs & is finite.
 *   cc -I../src alpha_test.c ../src/app_alpha.c ../src/engine/ir.c ../src/engine/a2l.c -lm -o a && ./a
 * NAM stays off (no real A2L1 model here); IR + gate + master are exercised.
 */
#include <stdint.h>
#include <stdio.h>
#include <math.h>

void  tf_alpha_init(void);
void  tf_alpha_process(float *buf, int n);
int   tf_alpha_load_ir(const float *taps, int n);
int   tf_alpha_load_nam(const uint8_t *blob, uint32_t len);
void  tf_alpha_set_master(float v);
void  tf_alpha_set_gate(float v);

/* board hooks (host stubs) */
int  tf_board_audio_in(float *b, int n) { (void)b; return n; }
void tf_board_audio_out(const float *b, int n) { (void)b; (void)n; }
int  tf_board_usb_playback(float *b, int n) { (void)b; return n; }

int main(void) {
    tf_alpha_init();
    tf_alpha_set_master(0.8f); tf_alpha_set_gate(0.2f);

    float ir[256]; for (int i = 0; i < 256; i++) ir[i] = (i == 0) ? 0.9f : 0.4f * expf(-i / 40.f);
    int ir_ok = tf_alpha_load_ir(ir, 256);
    uint8_t junk[8] = {0};
    int nam_ok = tf_alpha_load_nam(junk, 8);   /* invalid blob -> must refuse gracefully (0) */

    float buf[128]; double energy = 0; int bad = 0;
    for (int frame = 0; frame < 100; frame++) {
        for (int i = 0; i < 128; i++) buf[i] = 0.3f * sinf(2*3.14159f*220.f*(frame*128+i)/48000.f);
        tf_alpha_process(buf, 128);
        for (int i = 0; i < 128; i++) { if (!isfinite(buf[i])) bad++; energy += buf[i]*buf[i]; }
    }
    int ok = ir_ok && !nam_ok && bad == 0 && energy > 0;
    printf("ir_loaded=%d nam_refused_bad_blob=%d non_finite=%d energy=%.2f\n", ir_ok, !nam_ok, bad, energy);
    printf("ALPHA PIPELINE TEST: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
