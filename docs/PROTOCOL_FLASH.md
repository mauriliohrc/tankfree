# Mac-native flashing protocol (CAPTURED from a live flash)

**GROUND TRUTH (2026-11-17):** a real M-UPGRADE flash was captured on macOS (v099→
downgrade→v097→v099) with `M_UPGRADE_TRACE=1`. The result overturns the earlier
HID theory: **the Tank-G OTA flash is 100% USB-MIDI SysEx, device-pull.** There is
no HID/SCSI transport, no chip-key crypto on the host side, no loader upload. The
device's own bootloader drives the whole thing; M-UPGRADE is a **dumb file server**.

## The helper CLI (upgrade_core)
`M-UPGRADE.app/Contents/MacOS/upgrade_core.real` is a standalone helper the Flutter
UI spawns. Traces to **stderr** when `M_UPGRADE_TRACE=1`. The flash is three calls:

1. `ks-scan` — enumerate MIDI ports, MIDI identity handshake, print the device
   (`<devhash> <in-port-id> <out-port-id>`).
2. `ks-midi <devhash> <in> <out> <firmware.bin> prepare TANK-Gv2_0XX`
   — sends **`upgrade.enter` (updating=0)**, device validates the image by pulling
   selected regions (header/footer), then **reboots into OTA mode** (its identity
   changes `TANK-Gv2_0XX` → `ota-TANK-Gv2_0XX`; it re-enumerates with new port ids).
3. `ks-midi <devhash'> <in'> <out'> <firmware.bin> update ota-TANK-Gv2_0XX`
   — sends **`upgrade.enter` (updating=1)**, device pulls the **entire image** block
   by block and flashes it internally, then exits (code=0 = success).

Our native flasher (`installer/mac_flash.py`) drives exactly these three calls
against `upgrade_core.real` — proven to work on macOS, no VM.

## The wire protocol (device → host requests; host → device data)
Every packet (carried as USB-MIDI SysEx) is:
```
00 59  <cmd:u8>  <len:u8>  00 00  <data: len bytes>  <cksum:u8>
cksum = (~ (sum of the `data` bytes)) & 0xFF        # verified byte-exact
```
Commands seen:
- **0x11 — identity** (device→host). data = 27-byte ASCII id, zero-padded
  (`TANK-Gv2_099`, or `ota-TANK-Gv2_099` once in OTA mode).
- **0x30 — firmware block request** (device→host, 8 data bytes):
  `address:u32 BE` + `length:u32 BE`. The device asks "send me firmware[address :
  address+length]". Host replies with that slice (the 0x30 response carries the data).

### Transfer behavior (observed)
- **prepare** stage (`stage=prepare-read`): device pulls a handful of regions to
  validate — addr 0, 512, 1024 (re-read), 17408, then the tail near 508128..512256
  (the image footer/signature). ~Σ matches `(~sum)&0xFF`.
- **update** stage (`stage=transfer-read`): device walks the whole image in 512-byte
  blocks (it actually requests them in **descending** address order near the end;
  order is device-driven — the host just serves whatever (addr,len) is asked).
- **completion sentinel:** device issues a final request `address=0xF0000000 length=8`
  (not a real flash address — an end-of-stream marker); host serves it, device exits 0.
- Block size = **512 bytes**; a full image is ~3000 requests (~32 s over MIDI).

## What this means for TankFree
- **Recovery is structurally guaranteed.** OTA is handled by the **bootloader**, in a
  protected region we never flash. So M-UPGRADE can always force-restore the factory
  image regardless of what app we install — the user just needs to trigger OTA mode.
  Our fake identity (`firmware/src/midi_mvave.c`) only matters so M-UPGRADE recognises
  the device while our **app** is running; in OTA mode the bootloader answers for us.
- **Flashing our own firmware = feeding our `app.bin` as `firmware.bin`.** The device
  validates it during `prepare` (reads header + footer). So our image must pass that
  validation: correct version string, correct SFC encryption (chip key 0x980F — we have
  it in `installer/fwtool.py`), correct header/footer the bootloader checks. Getting
  `prepare` to accept our image is the gate; once accepted, `update` flashes it.
- **No host-side crypto/loader needed.** Unlike jl-uboot-tool (SCSI) or the HID theory,
  we don't upload a stage-1 loader or key anything on the host. The bootloader decrypts.

## Native flasher: two implementations
1. **Wrap `upgrade_core.real`** (`mac_flash.py`, default) — proven, ships today. Drives
   ks-scan → prepare → update. Backend is M-VAVE's helper; our value-add is scripting
   it to flash *our* app.bin + parsing progress.
2. **Pure-MIDI reimplementation** (fallback, `--native`) — a CoreMIDI file-server in
   Python (python-rtmidi/mido) that answers 0x30 requests from firmware.bin and sends
   `upgrade.enter`. Needs the **outgoing** byte framing (the enter command + the 0x30
   *response* wrapper), which this stderr trace does NOT dump (inbound packets only).
   Get it by snooping USB-MIDI during a real flash, or deeper disassembly. Until then
   impl #1 is the shipping path.

## Enter-OTA trigger (open)
`prepare` makes the device jump `TANK-Gv2_0XX` → `ota-TANK-Gv2_0XX` via the
`upgrade.enter` command (exact bytes not in the inbound-only trace). Our firmware's
`tf_enter_loader()` must reach the same bootloader entry so the recovery net works
from our app too.
