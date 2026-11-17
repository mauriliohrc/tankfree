/* midi_mvave.c — M-VAVE-compatible MIDI identity ("recovery compatibility").
 *
 * Makes our firmware answer M-UPGRADE's identity query as a legit Tank-G, and
 * handle its enter-upgrade command by jumping to the JieLi loader. Result:
 * M-UPGRADE always recognizes the pedal and can FORCE-RESTORE official firmware
 * even with TankFree installed — a permanent macOS recovery net.
 *
 * RCSP framing reverse-engineered from M-UPGRADE's upgrade_core and verified
 * byte-for-byte against a real Tank-G (see midi_mvave_test.c / PROTOCOL_FLASH.md):
 *   packet = 00 59 <cmd> <len:3 LE> <data…> <cksum>,  cksum = (~Σdata) & 0xFF
 *   identity reply: cmd 0x11, data = <16-byte id, zero-padded> → "TANK-Gv2_099"
 *
 * Clean-room: our own code, built from the observed wire format for interop.
 */
#include <stdint.h>
#include "tf_str.h"

#define RCSP_H0 0x00
#define RCSP_H1 0x59
#define CMD_IDENTITY_QUERY 0x10   /* M-UPGRADE → device (inferred; reply is 0x11) */
#define CMD_IDENTITY_REPLY 0x11   /* device → M-UPGRADE (confirmed from capture)   */
#define CMD_ENTER_UPGRADE  0x12   /* enter-upgrade (to confirm by capture)         */
#define ID_LEN 16                 /* stored identity string buffer                 */
#define ID_FIELD 27               /* identity reply data field (confirmed = 27 B)  */

/* The identity we present. Use the factory version string M-UPGRADE expects so it
 * offers/forces a restore of the matching official firmware. */
static char g_identity[ID_LEN] = "TANK-Gv2_099";

void tf_mvave_set_identity(const char *s) {
    memset(g_identity, 0, ID_LEN);
    strncpy(g_identity, s, ID_LEN - 1);
}

static uint8_t rcsp_cksum(const uint8_t *data, uint32_t n) {
    uint32_t s = 0;
    for (uint32_t i = 0; i < n; i++) s += data[i];
    return (uint8_t)(~s);                          /* ones-complement of the byte sum */
}

/* Build an RCSP packet into `out` (cap bytes). Returns length, or 0 on overflow. */
uint32_t tf_rcsp_build(uint8_t cmd, const uint8_t *data, uint32_t n, uint8_t *out, uint32_t cap) {
    if (6 + n + 1 > cap) return 0;
    out[0] = RCSP_H0; out[1] = RCSP_H1; out[2] = cmd;
    out[3] = n & 0xFF; out[4] = (n >> 8) & 0xFF; out[5] = (n >> 16) & 0xFF;
    memcpy(out + 6, data, n);
    out[6 + n] = rcsp_cksum(data, n);
    return 6 + n + 1;
}

/* Parse an RCSP packet. Returns cmd (or -1); sets payload pointer+length. */
int tf_rcsp_parse(const uint8_t *buf, uint32_t len, const uint8_t **data, uint32_t *n) {
    if (len < 7 || buf[0] != RCSP_H0 || buf[1] != RCSP_H1) return -1;
    uint32_t dn = buf[3] | (buf[4] << 8) | (buf[5] << 16);
    if (6 + dn + 1 > len) return -1;
    if (buf[6 + dn] != rcsp_cksum(buf + 6, dn)) return -1;    /* bad checksum */
    *data = buf + 6; *n = dn;
    return buf[2];
}

/* Build our identity reply packet (what M-UPGRADE reads to recognize us). */
uint32_t tf_mvave_identity_reply(uint8_t *out, uint32_t cap) {
    uint8_t id[ID_FIELD]; memset(id, 0, ID_FIELD);
    memcpy(id, g_identity, strnlen(g_identity, ID_LEN));
    return tf_rcsp_build(CMD_IDENTITY_REPLY, id, ID_FIELD, out, cap);
}

/* Hook the firmware provides: reboot into the JieLi loader so M-UPGRADE can flash.
 * (On AC791N: jump to the mask-ROM/OTA loader; wired to the SDK boot API.) */
void tf_enter_loader(void);   /* defined in the board layer */

/* Call with a decoded RCSP payload (already SysEx-decoded). Returns a reply length
 * in `out` (0 if nothing to send). */
uint32_t tf_mvave_on_packet(const uint8_t *buf, uint32_t len, uint8_t *out, uint32_t cap) {
    const uint8_t *data; uint32_t n;
    int cmd = tf_rcsp_parse(buf, len, &data, &n);
    if (cmd == CMD_IDENTITY_QUERY) return tf_mvave_identity_reply(out, cap);
    if (cmd == CMD_ENTER_UPGRADE) { tf_enter_loader(); return 0; }
    return 0;
}
