/* block.h — common interface every TankFree DSP block implements.
 *
 * The free-chain engine (chain.c) owns a graph of blocks. Each block is a small
 * vtable + opaque state. Audio runs in 48 kHz (or 96 kHz) float frames.
 * Blocks declare a CPU/MEM cost so the engine can enforce the budget meters.
 *
 * Clean-room: block DSP is ported from open sources (FAUST-generated C, Guitarix,
 * BYOD/chowdsp_wdf, mda, Airwindows, Freeverb) — see ../../docs/BLOCK_CATALOG.md.
 */
#ifndef TANKFREE_BLOCK_H
#define TANKFREE_BLOCK_H

#include <stdint.h>
#include <stddef.h>

#define TF_MAX_PARAMS   16
#define TF_FRAME        32          /* samples per process() call (audio ISR block) */

typedef struct tf_block tf_block;

/* A parameter is a normalized 0..1 float with metadata for the UI / MIDI map. */
typedef struct {
    const char *name;               /* "drive", "bass", "mix"… */
    float       value;              /* current, 0..1 normalized */
    float       def;                /* default */
    uint8_t     midi_cc;            /* 0 = unassigned */
} tf_param;

/* Category — used by the GP-5 MIDI facade (CC48-57 toggle first block of a category). */
typedef enum {
    TF_CAT_DYNAMICS, TF_CAT_DRIVE, TF_CAT_EQ, TF_CAT_MOD, TF_CAT_PITCH,
    TF_CAT_DELAY, TF_CAT_REVERB, TF_CAT_AMP, TF_CAT_CAB, TF_CAT_UTIL, TF_CAT__COUNT
} tf_category;

/* Static descriptor for a block type. */
typedef struct {
    const char *id;                 /* "eq.graphic", "amp.brit800", "dly.digital" */
    tf_category category;
    uint16_t    state_bytes;        /* heap needed for one instance */
    uint16_t    cpu_cost;           /* relative units at 48 kHz (budget meter) */
    /* lifecycle */
    void  (*init)(tf_block *b, float fs);
    void  (*reset)(tf_block *b);
    /* audio: in-place stereo process of n (<=TF_FRAME) samples */
    void  (*process)(tf_block *b, float *l, float *r, int n);
    /* called when a param changes (block recomputes coeffs, per-block not per-sample) */
    void  (*set_param)(tf_block *b, int idx, float value01);
    const tf_param *params;
    uint8_t     nparams;
} tf_block_desc;

/* A live block instance in the chain. */
struct tf_block {
    const tf_block_desc *desc;
    void      *state;               /* instance DSP state (state_bytes) */
    tf_param   p[TF_MAX_PARAMS];    /* live param values (copied from desc) */
    uint8_t    enabled;             /* bypass */
    float      fs;                  /* 48000 or 96000 */
};

/* Registry (blocks self-register their descriptor). */
const tf_block_desc *tf_block_find(const char *id);
int                  tf_block_count(void);
const tf_block_desc *tf_block_at(int i);
void                 tf_block_register(const tf_block_desc *d);

#endif /* TANKFREE_BLOCK_H */
