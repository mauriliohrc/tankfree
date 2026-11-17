/* midi_mvave_test.c — prove our forged identity reply is byte-exact with the one
 * a real Tank-G sends to M-UPGRADE (captured 2026-11-17).
 *   cc -I../src midi_mvave_test.c ../src/midi_mvave.c -o t && ./t
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint32_t tf_mvave_identity_reply(uint8_t *out, uint32_t cap);
void tf_mvave_set_identity(const char *s);
uint32_t tf_rcsp_build(uint8_t cmd, const uint8_t *data, uint32_t n, uint8_t *out, uint32_t cap);
int tf_rcsp_parse(const uint8_t *buf, uint32_t len, const uint8_t **data, uint32_t *n);
void tf_enter_loader(void) {}   /* stub for the host test */

/* exact bytes captured from a real pedal: event=packet.received … identity=TANK-Gv2_099 */
static const uint8_t CAPTURED[34] = {
    0x00,0x59,0x11,0x1b,0x00,0x00, 'T','A','N','K','-','G','v','2','_','0','9','9',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0xb4
};

int main(void) {
    uint8_t out[64];
    tf_mvave_set_identity("TANK-Gv2_099");
    uint32_t n = tf_mvave_identity_reply(out, sizeof out);
    int match = (n == sizeof CAPTURED) && !memcmp(out, CAPTURED, n);

    printf("ours  (%u): ", n); for (uint32_t i = 0; i < n; i++) printf("%02x ", out[i]); printf("\n");
    printf("pedal (%zu): ", sizeof CAPTURED); for (size_t i = 0; i < sizeof CAPTURED; i++) printf("%02x ", CAPTURED[i]); printf("\n");

    /* parse round-trip: our reply must parse back to cmd 0x11 + 16-byte id */
    const uint8_t *d; uint32_t dn; int cmd = tf_rcsp_parse(out, n, &d, &dn);
    int parse_ok = (cmd == 0x11 && dn == 27 && !memcmp(d, "TANK-Gv2_099", 12));

    printf("\nbyte-exact vs real pedal: %s\nparse round-trip: %s\n", match?"YES":"NO", parse_ok?"YES":"NO");
    printf("MVAVE IDENTITY TEST: %s\n", (match && parse_ok) ? "PASS" : "FAIL");
    return (match && parse_ok) ? 0 : 1;
}
