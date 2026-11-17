/* proto_test.c — exercise the control protocol against the chain engine on host.
 *   cc -I../src -O2 proto_test.c ../src/proto.c ../src/registry.c ../src/chain.c \
 *      ../src/blocks/eq_param.c ../src/blocks/delay_digital.c ../src/blocks/drive_soft.c \
 *      -lm -o proto_test && ./proto_test
 */
#include "../src/chain.h"
#include <stdio.h>
#include <string.h>

void tf_blocks_register_all(void);
void tf_proto_line(tf_chain *c, char *line, void (*out)(const char *));

static int lines = 0, saw_pong = 0, saw_fxpok = 0, saw_err = 0;
static void out(const char *s) {
    lines++;
    if (strstr(s, "PONG")) saw_pong = 1;
    if (strstr(s, "FXP ok")) saw_fxpok = 1;
    if (!strncmp(s, "ERR", 3)) saw_err = 1;
    printf("  « %s\n", s);
}
static void cmd(tf_chain *c, const char *s) { char b[128]; strncpy(b, s, sizeof b - 1); b[sizeof b-1]=0; printf("» %s\n", s); tf_proto_line(c, b, out); }

int main(void) {
    tf_blocks_register_all();
    tf_chain c; tf_chain_init(&c, 48000.0f);
    tf_chain_insert(&c, "drv.soft", -1);
    tf_chain_insert(&c, "eq.param3", -1);
    tf_chain_insert(&c, "dly.digital", -1);

    cmd(&c, "PING");
    cmd(&c, "FXS");
    cmd(&c, "FXP 0 0 80");       /* set drive param 0 to 80% */
    cmd(&c, "FXE 2 0");          /* bypass delay */
    cmd(&c, "FXA eq.param3");    /* add another EQ */
    cmd(&c, "FXS");
    cmd(&c, "BOGUS");           /* should ERR */

    int ok = saw_pong && saw_fxpok && saw_err && c.nblocks == 4;
    printf("\nPROTO TEST: %s  (lines=%d blocks=%d)\n", ok ? "PASS" : "FAIL", lines, c.nblocks);
    return ok ? 0 : 1;
}
