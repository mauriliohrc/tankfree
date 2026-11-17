/* chain.h — the free effects chain: an ordered graph of blocks with optional
 * parallel branches. Bounded by the CPU/MEM budget, not a fixed slot count.
 */
#ifndef TANKFREE_CHAIN_H
#define TANKFREE_CHAIN_H
#include "block.h"

#define TF_CHAIN_MAX   32          /* data-structure ceiling, not a usage cap */

typedef struct {
    tf_block *blocks[TF_CHAIN_MAX];
    uint8_t   nblocks;
    /* parallel branches: a split point, two sub-chains, a mix. Minimal model:
     * branch_at[i] != 0 marks where a parallel branch runs alongside the main. */
    uint8_t   branch_at[TF_CHAIN_MAX];
    float     branch_mix[TF_CHAIN_MAX];
    uint16_t  cpu_used;            /* sum of enabled block costs (meter) */
    uint32_t  mem_used;            /* sum of state_bytes (meter) */
    float     fs;
} tf_chain;

void tf_chain_init(tf_chain *c, float fs);
/* Insert a block type by id at position `pos`. Returns the instance or NULL if
 * it would blow the CPU/MEM budget (engine refuses, like the original meters). */
tf_block *tf_chain_insert(tf_chain *c, const char *id, int pos);
void      tf_chain_remove(tf_chain *c, int pos);
void      tf_chain_move(tf_chain *c, int from, int to);
/* Toggle the first enabled block of a category (GP-5 MIDI CC48-57 facade). */
void      tf_chain_toggle_category(tf_chain *c, tf_category cat, int on);
/* Run the whole chain over an audio frame (in place, stereo). */
void      tf_chain_process(tf_chain *c, float *l, float *r, int n);

/* Budget limits (tuned per sample rate). Over this → inserts are refused and the
 * engine can degrade/disable the most expensive block, reporting it to the editor. */
extern uint16_t TF_CPU_BUDGET;     /* set from clock/sample-rate at boot */
extern uint32_t TF_MEM_BUDGET;

#endif /* TANKFREE_CHAIN_H */
