# Mac-native flashing protocol (reverse-engineered from M-UPGRADE)

How M-VAVE's `M-UPGRADE.app` flashes the Tank-G on macOS — so our `mac_flash.py`
can do the same. RE'd from its native helper `upgrade_core` (CoreMIDI + IOKit HID).

## The helper CLI (upgrade_core)
`upgrade_core <command>` is a standalone helper the Flutter UI spawns. It traces to
**stderr** when `M_UPGRADE_TRACE=1` (exactly the string "1"). Verified commands:
- **`ks-scan`** ✅ — enumerate MIDI ports, MIDI **identity handshake**, print device.
- `self-test` ✅ — internal protocol golden-vector test (no device).
- `ks-probe` / `ks-midi` / `identity` / `coremidi` — need extra args ("Invalid helper arguments").
- **`recovery-upgrade`** — the HID OTA flash (destructive; traces `recovery.request`/`reply`).

Run e.g. `M_UPGRADE_TRACE=1 upgrade_core ks-scan 2>&1` to capture traffic.

## MIDI identity (captured live from a real Tank-G, factory fw)
```
event=identity.query
event=packet.received size=34 bytes=00 59 11 1b 00 00 54 41 4e 4b 2d 47 76 32 5f 30 39 39 00…00 b4
event=identity.reply identity=TANK-Gv2_099
```
- Reply = 34 bytes: header `00 59 11 1b 00 00`, then ASCII id `"TANK-Gv2_099"` (model_version),
  zero-padded, trailing checksum `b4`. (Carried as USB-MIDI SysEx.)
- M-UPGRADE uses this to recognize the device before offering an update.

## HID OTA (the Recovery transport) — what we have
- Transport = **USB HID, 64-byte reports** (IOKit). `MacConnection::transact`:
  sends a variable-length output report (the request), reads a **fixed 64-byte
  input report** (reply); throws "Recovery invalid reply length" if not 64.
- **Request framing:** `Recovery::request(cmd, a, b, data)` → builds the vector via
  `Gt::request(...)` then writes a 4-byte magic header **`"SBU2"`** (0x53 42 55 32) at
  the front. `cmd` ∈ 1..5; `data` ≤ 4096 bytes. Reply parsed by `Recovery::response`.
- `Recovery::install(firmware, chipkey_cb → array<u32,16>, progress)` = the flash;
  `Recovery::{scan,open,compatible,inspect}` = discovery + package validation.
  The `chipkey_cb(addr,len,data)→u32[16]` is the crypto/keying hook (our chip key 0x980F).

## Loader command layer (known, reuse — from kagaimiq/jl-uboot-tool, MIT)
Same commands over any transport (SCSI there, HID here). CDB = `cmd(2,BE)+args`, FF-padded to 16B.
- Mask-ROM UBOOT1.00: `WRITE_MEMORY 0xFB06`, `READ_MEMORY 0xFD07`, `JUMP 0xFB08`.
  (RAM only → upload the stage-1 loader `wl82loader.bin`, jump, then flash ops.)
- Stage-1 LoaderV1: `ERASE_BLOCK 0xFB00`, `WRITE_FLASH 0xFB01`, `ERASE_SECTOR 0xFB03`,
  `FLASH_ID 0xFC00`, `GET_ONLINE 0xFC0B`, `SELECT_FLASH 0xFC0C`, `READ_FLASH 0xFD01`.
  WRITE_FLASH args = `addr(4,BE)+len(2,BE)+00+crc16(data,LE)` then data out.

## Still to confirm (for mac_flash.py option B)
1. The **inner `Gt::request` byte layout** (how cmd/offset/len/data pack after "SBU2")
   — from deeper disassembly, or from a **`recovery.request`/`reply` trace** during a
   real `recovery-upgrade` flash (the surest ground truth).
2. The **HID device VID/PID + how the device enters HID-OTA mode** (MIDI `enter-upgrade`
   → re-enumerate as HID). `Recovery::scan` filters it; capture via the trace.
3. The **chipkey callback** behavior (the `array<u32,16>` — likely the chip key / keyed
   per-block transform the loader expects).

## Safest capture plan (when ready)
Wrap `upgrade_core` to tee its **stderr**, run a normal M-UPGRADE flash of the
**correct factory version** (so no bootloader-version mismatch), capture the
`recovery.request`/`recovery.reply` lines, and lock the framing. Then finish
`mac_flash.py` (backup = flash_read; flash = write app area only, backup-first).
