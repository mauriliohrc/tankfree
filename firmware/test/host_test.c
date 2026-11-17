/* host_test.c — compile & run the block engine on a desktop (no SDK/hardware).
 * Proves the chain engine + blocks compile and process audio sanely.
 *   cc -I../src -O2 host_test.c ../src/registry.c ../src/chain.c \
 *      ../src/blocks/eq_param.c ../src/blocks/delay_digital.c \
 *      ../src/blocks/drive_soft.c -lm -o host_test && ./host_test
 */
#include "../src/chain.h"
#include <stdio.h>
#include <math.h>

void tf_blocks_register_all(void);

int main(void) {
    tf_blocks_register_all();
    printf("registered block types: %d\n", tf_block_count());
    for (int i = 0; i < tf_block_count(); i++) printf("  - %s\n", tf_block_at(i)->id);

    tf_chain c;
    tf_chain_init(&c, 48000.0f);

    const char *patch[] = { "drv.soft", "eq.param3", "dly.digital" };
    for (int i = 0; i < 3; i++) {
        tf_block *b = tf_chain_insert(&c, patch[i], -1);
        if (!b) { printf("FAIL: insert %s refused\n", patch[i]); return 1; }
    }
    printf("chain: %d blocks, cpu_used=%u, mem_used=%u\n", c.nblocks, c.cpu_used, c.mem_used);

    /* push an impulse + a sine through several frames, check output is finite */
    float l[TF_FRAME], r[TF_FRAME];
    double energy = 0; int bad = 0;
    for (int frame = 0; frame < 200; frame++) {
        for (int i = 0; i < TF_FRAME; i++) {
            float t = (frame * TF_FRAME + i) / 48000.0f;
            l[i] = (frame == 0 && i == 0) ? 1.0f : 0.3f * sinf(2 * 3.14159f * 220.0f * t);
            r[i] = l[i];
        }
        tf_chain_process(&c, l, r, TF_FRAME);
        for (int i = 0; i < TF_FRAME; i++) { if (!isfinite(l[i])) bad++; energy += l[i] * l[i]; }
    }
    printf("processed 200 frames, non-finite=%d, out_energy=%.3f\n", bad, energy);

    /* exercise a param change + bypass + reorder + remove */
    c.blocks[0]->desc->set_param(c.blocks[0], 0, 0.8f);   /* more drive */
    tf_chain_toggle_category(&c, TF_CAT_DELAY, 0);        /* GP-5-style: delay off */
    tf_chain_move(&c, 0, 2);
    tf_chain_remove(&c, 0);
    printf("after edits: %d blocks, cpu_used=%u\n", c.nblocks, c.cpu_used);

    int ok = (bad == 0) && (energy > 0.0) && (c.nblocks == 2);
    printf("HOST TEST: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
