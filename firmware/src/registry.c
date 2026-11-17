/* registry.c — block type registry. Blocks are registered at boot; the chain
 * engine and the editor look them up by id.
 */
#include "block.h"
#include <string.h>

#define TF_MAX_TYPES 128
static const tf_block_desc *g_types[TF_MAX_TYPES];
static int g_ntypes;

void tf_block_register(const tf_block_desc *d) {
    if (g_ntypes < TF_MAX_TYPES && d) g_types[g_ntypes++] = d;
}
int tf_block_count(void) { return g_ntypes; }
const tf_block_desc *tf_block_at(int i) { return (i >= 0 && i < g_ntypes) ? g_types[i] : 0; }

const tf_block_desc *tf_block_find(const char *id) {
    for (int i = 0; i < g_ntypes; i++)
        if (!strcmp(g_types[i]->id, id)) return g_types[i];
    return 0;
}

/* Built-in blocks (extern descriptors from the blocks directory). Add here. */
extern const tf_block_desc TF_EQ_PARAM, TF_DELAY_DIGITAL, TF_DRIVE_SOFT;

void tf_blocks_register_all(void) {
    tf_block_register(&TF_DRIVE_SOFT);
    tf_block_register(&TF_EQ_PARAM);
    tf_block_register(&TF_DELAY_DIGITAL);
    /* TODO: amps (FAUST brit800…), modulation, reverb (freeverb), IR cab */
}
