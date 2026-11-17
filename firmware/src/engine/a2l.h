/* a2l.h — NAM A2-Lite (WaveNet) fixed-point runner for the AC791N.
 * Parses the A2L1 blob (format decoded from nam2chip.js; see docs/RE_FINDINGS §7,
 * docs/BLOCK_CATALOG) and runs the 3-channel A2-Lite net in fixed point. Clean-room.
 *
 * A2L1 layout (little-endian): "A2L1", u16 ver, u8 wbits(16), u8 abits(32),
 *   u32 fs, u16 nLayers, u8 C(3), u8 head_K, i8 Fin,Fx3_0,Fh,Fout, char name[16],
 *   i8 rech_F, i16 rech_W[C],
 *   per layer: u8 K, u16 dil, i8 Fz, i8 Fx3, i32 lmul,
 *              i8 cF, i16 cW[C*C*K], i64 cB[C], i8 mF, i16 mW[C],
 *              i8 lF, i16 lW[C*C], i64 lB[C],
 *   i8 hF, i16 hW[C*head_K], i64 hB, u32 crc32.
 */
#ifndef TF_A2L_H
#define TF_A2L_H
#include <stdint.h>

#define A2L_MAX_LAYERS 24
#define A2L_MAX_C      3
#define A2L_MAX_RF     8192     /* receptive-field ring buffer per channel */

typedef struct {
    uint8_t  K; uint16_t dil;
    int8_t   Fz, Fx3; int32_t lmul;            /* LeakyReLU slope, Q24 */
    int8_t   cF, mF, lF;
    const int16_t *cW, *mW, *lW;               /* conv / input-mixer / 1x1 weights */
    const int64_t *cB, *lB;                    /* biases */
} a2l_layer;

typedef struct {
    int ok, C, nL, headK, fs;
    int8_t Fin, Fx3_0, Fh, Fout, rechF;
    const int16_t *rechW, *hW; const int64_t hB;
    a2l_layer L[A2L_MAX_LAYERS];
    /* runtime state: per-channel dilation ring buffers (x3 of each layer) */
    int32_t ring[A2L_MAX_LAYERS][A2L_MAX_C][512];
    int     ringpos[A2L_MAX_LAYERS];
} a2l_model;

/* Parse a blob into `m`. Returns 1 on success (CRC + shift rules ok), 0 otherwise. */
int  a2l_parse(a2l_model *m, const uint8_t *blob, uint32_t len);
void a2l_reset(a2l_model *m);
/* Process `n` mono samples in place (float, -1..1). */
void a2l_process(a2l_model *m, float *x, int n);

#endif
