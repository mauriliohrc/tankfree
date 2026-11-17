/* chain.c — free chain engine (serial with optional parallel branches).
 * Skeleton: block alloc uses a static pool here; swap for the SDK allocator.
 */
#include "chain.h"
#include "tf_str.h"

uint16_t TF_CPU_BUDGET = 1000;     /* relative units; set at boot from clock */
uint32_t TF_MEM_BUDGET = 512 * 1024;

/* Instance state pool = the effects arena (placeholder; wire to the SDK's
 * effects RAM region on-device, which bounds total delay/reverb memory). */
static tf_block  g_blocks[TF_CHAIN_MAX];
static uint8_t   g_state[512 * 1024];
static uint32_t  g_state_used;
static uint8_t   g_used_mask;

void tf_chain_init(tf_chain *c, float fs) {
    memset(c, 0, sizeof(*c));
    c->fs = fs;
    g_state_used = 0;
}

static tf_block *alloc_block(const tf_block_desc *d, float fs) {
    if (g_state_used + d->state_bytes > sizeof(g_state)) return NULL;
    int slot = -1;
    for (int i = 0; i < TF_CHAIN_MAX; i++)
        if (!(g_used_mask & (1u << (i & 7))) && !g_blocks[i].desc) { slot = i; break; }
    if (slot < 0) return NULL;
    tf_block *b = &g_blocks[slot];
    b->desc = d;
    b->state = &g_state[g_state_used]; g_state_used += d->state_bytes;
    b->fs = fs; b->enabled = 1;
    for (int i = 0; i < d->nparams; i++) b->p[i] = d->params[i];
    if (d->init) d->init(b, fs);
    return b;
}

tf_block *tf_chain_insert(tf_chain *c, const char *id, int pos) {
    const tf_block_desc *d = tf_block_find(id);
    if (!d || c->nblocks >= TF_CHAIN_MAX) return NULL;
    if (c->cpu_used + d->cpu_cost > TF_CPU_BUDGET) return NULL;        /* budget meter */
    if (c->mem_used + d->state_bytes > TF_MEM_BUDGET) return NULL;
    tf_block *b = alloc_block(d, c->fs);
    if (!b) return NULL;
    if (pos < 0 || pos > c->nblocks) pos = c->nblocks;
    for (int i = c->nblocks; i > pos; i--) c->blocks[i] = c->blocks[i-1];
    c->blocks[pos] = b; c->nblocks++;
    c->cpu_used += d->cpu_cost; c->mem_used += d->state_bytes;
    return b;
}

void tf_chain_remove(tf_chain *c, int pos) {
    if (pos < 0 || pos >= c->nblocks) return;
    tf_block *b = c->blocks[pos];
    c->cpu_used -= b->desc->cpu_cost; c->mem_used -= b->desc->state_bytes;
    b->desc = NULL;                 /* free the pool slot (state pool is bump-only) */
    for (int i = pos; i < c->nblocks - 1; i++) c->blocks[i] = c->blocks[i+1];
    c->nblocks--;
}

void tf_chain_move(tf_chain *c, int from, int to) {
    if (from < 0 || from >= c->nblocks || to < 0 || to >= c->nblocks) return;
    tf_block *b = c->blocks[from];
    if (from < to) for (int i = from; i < to; i++) c->blocks[i] = c->blocks[i+1];
    else           for (int i = from; i > to; i--) c->blocks[i] = c->blocks[i-1];
    c->blocks[to] = b;
}

void tf_chain_toggle_category(tf_chain *c, tf_category cat, int on) {
    for (int i = 0; i < c->nblocks; i++)
        if (c->blocks[i]->desc->category == cat) { c->blocks[i]->enabled = on; return; }
}

void tf_chain_process(tf_chain *c, float *l, float *r, int n) {
    for (int i = 0; i < c->nblocks; i++) {
        tf_block *b = c->blocks[i];
        if (b->enabled && b->desc->process) b->desc->process(b, l, r, n);
    }
    /* TODO: parallel branches (branch_at/branch_mix) — split, run sub-chain, mix. */
}
