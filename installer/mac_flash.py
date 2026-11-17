#!/usr/bin/env python3
"""mac_flash.py — native macOS flasher for the JieLi AC791N loader over USB HID.

Why this exists: jl-uboot-tool talks to the loader over SCSI (Linux/Windows only),
but the JieLi loader ALSO speaks USB HID — and M-VAVE's M-UPGRADE.app flashes the
Tank-G on macOS through it (reverse-engineered: Recovery/JieliHidOta, 64-byte HID
reports via IOKit). This replicates that so we flash on a Mac — no Windows, no VM.

Status: the loader COMMAND layer is known (ported from kagaimiq/jl-uboot-tool, MIT).
The HID TRANSPORT framing (how a 16-byte CDB + data packs into 64-byte reports) is
being confirmed empirically from M-UPGRADE's trace (see CONFIRM below). Until then,
writes are DRY-RUN and `scan` + the framing probe are the usable parts.

    python3 mac_flash.py scan         # list HID devices, flag JieLi ones
    python3 mac_flash.py backup out/  # read full flash (once framing confirmed)

Needs: pip install hidapi
"""
import sys, struct

# ── loader command layer (ported from jl-uboot-tool, MIT; transport-agnostic) ──
def crc16(data, crc=0):
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc & 0xFFFF

def make_cdb(cmd, args=b""):
    cdb = cmd.to_bytes(2, "big") + args
    return cdb + b"\xff" * (16 - len(cdb)) if len(cdb) < 16 else cdb

class UBOOT:            # mask-ROM UBOOT1.00 — RAM only; used to upload the stage-1 loader
    WRITE_MEMORY, READ_MEMORY, JUMP = 0xFB06, 0xFD07, 0xFB08
    def __init__(self, t): self.t = t
    def mem_write(self, addr, data):
        self.t.execute(make_cdb(self.WRITE_MEMORY,
            addr.to_bytes(4,"big")+len(data).to_bytes(2,"big")+b"\x00"+crc16(data).to_bytes(2,"little")), data, None)
    def mem_read(self, addr, n):
        return self.t.execute(make_cdb(self.READ_MEMORY, addr.to_bytes(4,"big")+n.to_bytes(2,"big")), None, n)
    def jump(self, addr, arg=0):
        self.t.execute(make_cdb(self.JUMP, addr.to_bytes(4,"big")+arg.to_bytes(2,"big")), None, 16)

class LoaderV1:        # stage-1 flash loader (wl82loader.bin) running in RAM
    ERASE_BLOCK, WRITE_FLASH, ERASE_SECTOR = 0xFB00, 0xFB01, 0xFB03
    FLASH_ID, GET_ONLINE, SELECT_FLASH, READ_FLASH = 0xFC00, 0xFC0B, 0xFC0C, 0xFD01
    def __init__(self, t): self.t = t
    def flash_read(self, addr, n):
        return self.t.execute(make_cdb(self.READ_FLASH, addr.to_bytes(4,"big")+n.to_bytes(2,"big")), None, n)
    def flash_write(self, addr, data):
        self.t.execute(make_cdb(self.WRITE_FLASH,
            addr.to_bytes(4,"big")+len(data).to_bytes(2,"big")+b"\x00"+crc16(data).to_bytes(2,"little")), data, None)
    def erase_sector(self, addr): self.t.execute(make_cdb(self.ERASE_SECTOR, addr.to_bytes(4,"big")), None, 16)
    def flash_id(self):
        r = self.t.execute(make_cdb(self.FLASH_ID), None, 16); return int.from_bytes(r[2:5], "big")

# ── HID transport (macOS-native via hidapi) ──
# CONFIRM via M-UPGRADE trace: set M_UPGRADE_TRACE and read the recovery.request /
# recovery.reply lines (command/sequence/offset/bytes) to lock the exact 64-byte
# report layout. Hypothesis below: report = [report_id=0][16B CDB][dataout...], 64B;
# reply read as 64B input report. Do NOT enable real writes until verified.
REPORT_LEN = 64
class HIDTransport:
    def __init__(self, dev): self.dev = dev
    def execute(self, cdb, data_out, data_in_len):
        raise NotImplementedError(
            "HID framing not yet confirmed — capture it with M_UPGRADE_TRACE first "
            "(see docs/FLASHING_ON_MAC.md).")

# ── device scan ──
JIELI_VIDS = {0x3654, 0x4C4A}   # 0x3654 = M-VAVE/Jieli product; 0x4C4A = JieLi mask-ROM
def cmd_scan():
    try:
        import hid
    except ImportError:
        print("pip install hidapi   (needed for HID access on macOS)"); return 1
    found = 0
    for d in hid.enumerate():
        vid, pid = d["vendor_id"], d["product_id"]
        tag = "  ← JieLi/M-VAVE" if vid in JIELI_VIDS else ""
        if vid in JIELI_VIDS or tag:
            print(f"  VID 0x{vid:04X} PID 0x{pid:04X}  '{d.get('product_string','')}'  "
                  f"usage={d.get('usage_page')}/{d.get('usage')}{tag}")
            found += 1
    print(f"{found} JieLi/M-VAVE HID interface(s). "
          "In loader/OTA mode a 64-byte HID interface should appear here.")
    return 0

def main(a):
    if not a: print(__doc__); return 1
    if a[0] == "scan": return cmd_scan()
    print("backup/flash are gated until the HID framing is confirmed (see CONFIRM in-file).")
    return 1

if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
