/* proto.c — TankFree control protocol (ASCII line protocol over USB-CDC / BLE).
 *
 * Clean-room reimplementation of a compatible command surface (we studied the
 * UltraRig/factory protocol for the shape; this is our own code). Drives the
 * free-chain engine. One line in → zero or more reply lines via `out`, ending
 * in "OK" or "ERR ...". See docs/PROTOCOL.md.
 */
#include "chain.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define FW_VERSION "tankfree_fw T0.1 dev"

typedef void (*tf_out)(const char *line);

static int tok(char *s, char *argv[], int max) {   /* split on spaces, in place */
    int n = 0;
    for (char *p = strtok(s, " \t\r\n"); p && n < max; p = strtok(NULL, " \t\r\n")) argv[n++] = p;
    return n;
}
static void emit(tf_out out, const char *fmt, ...) {
    char buf[160]; va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    out(buf);
}

/* dump one block as:  FX <i> id=<i> <type> on=<0|1> p=<v0,v1,...>  */
static void dump_block(tf_out out, int i, tf_block *b) {
    char p[96]; int o = 0;
    for (int k = 0; k < b->desc->nparams && o < (int)sizeof p - 8; k++)
        o += snprintf(p + o, sizeof p - o, "%s%d", k ? "," : "", (int)(b->p[k].value * 100 + 0.5f));
    emit(out, "FX %d id=%d %s on=%d p=%s", i, i, b->desc->id, b->enabled, p);
}

/* main entry: handle one command line against chain `c`. */
void tf_proto_line(tf_chain *c, char *line, tf_out out) {
    char *a[8]; int n = tok(line, a, 8);
    if (n == 0) { out("OK"); return; }
    const char *cmd = a[0];

    if (!strcmp(cmd, "PING")) { emit(out, "PONG %s", FW_VERSION); return; }

    if (!strcmp(cmd, "FXT")) {                       /* block catalog */
        for (int i = 0; i < tf_block_count(); i++) {
            const tf_block_desc *d = tf_block_at(i);
            emit(out, "FXT %d %s cost=%u mem=%u np=%u", i, d->id, d->cpu_cost, d->state_bytes, d->nparams);
            for (int k = 0; k < d->nparams; k++)
                emit(out, "FXT  %d %s", k, d->params[k].name);
        }
        emit(out, "FXT end cpu=%u/%u", c->cpu_used, TF_CPU_BUDGET); out("OK"); return;
    }
    if (!strcmp(cmd, "FXS")) {                        /* current chain snapshot */
        for (int i = 0; i < c->nblocks; i++) dump_block(out, i, c->blocks[i]);
        emit(out, "FXS cpu=%u/%u mem=%lu", c->cpu_used, TF_CPU_BUDGET, (unsigned long)c->mem_used);
        out("OK"); return;
    }
    if (!strcmp(cmd, "FXP") && n >= 4) {              /* set param: FXP <i> <param> <0..100> */
        int i = atoi(a[1]), k = atoi(a[2]); float v = atoi(a[3]) / 100.0f;
        if (i < 0 || i >= c->nblocks) { out("ERR FXP block"); return; }
        tf_block *b = c->blocks[i];
        if (k < 0 || k >= b->desc->nparams) { out("ERR FXP param"); return; }
        if (b->desc->set_param) b->desc->set_param(b, k, v < 0 ? 0 : v > 1 ? 1 : v);
        emit(out, "FXP ok %d", atoi(a[3])); return;
    }
    if (!strcmp(cmd, "FXE") && n >= 3) {              /* bypass: FXE <i> <0|1> */
        int i = atoi(a[1]);
        if (i < 0 || i >= c->nblocks) { out("ERR FXE block"); return; }
        c->blocks[i]->enabled = atoi(a[2]) ? 1 : 0; out("OK"); return;
    }
    if (!strcmp(cmd, "FXA") && n >= 2) {              /* add block by id: FXA <type> */
        tf_block *b = tf_chain_insert(c, a[1], -1);
        if (!b) { out("ERR FXA budget/unknown"); return; }
        emit(out, "FXA id=%d %s", c->nblocks - 1, a[1]); return;
    }
    if (!strcmp(cmd, "FXR") && n >= 2) { tf_chain_remove(c, atoi(a[1])); out("OK"); return; }
    if (!strcmp(cmd, "FXM") && n >= 3) { tf_chain_move(c, atoi(a[1]), atoi(a[2])); out("OK"); return; }
    if (!strcmp(cmd, "MEM")) { emit(out, "MEM cpu=%u/%u mem=%lu arena=%u", c->cpu_used, TF_CPU_BUDGET,
                                    (unsigned long)c->mem_used, TF_MEM_BUDGET); out("OK"); return; }

    emit(out, "ERR unknown %s", cmd);
}
