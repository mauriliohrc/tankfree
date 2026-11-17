# OpenTank-G — Firmware Specification

Living spec. Decisions marked ✅; open items marked ❓.

## Audio
- ✅ 24-bit, **no internal resample**.
- ✅ **User-selectable sample rate: 48 kHz (default) or 96 kHz.**
  - 48 kHz: NAM amp runs native, full CPU budget for effects.
  - 96 kHz: engine runs at 96; the NAM amp runs in a 48 kHz island (up/downsample
    around it) to keep model compatibility. ~2× CPU → fewer simultaneous blocks;
    CPU meter enforces. Marketed as an advanced/"hi-res" mode.
- ✅ Quality target: "best possible" while preserving the original Tank-G character.

## MIDI — replicate Valeton GP-5 (see MIDI_GP5.md)
- ✅ Transports: **USB-MIDI** (class compliant) + **BLE-MIDI** (standard GATT).
- ✅ Channel fixed to **1**.
- ✅ **CC0 (0–99) = patch recall** (no Program Change, no CC32 bank select).
- ✅ CC7 patch volume; CC22/23 bank±; CC24/25 patch±; CC29/30 song-patch±.
- ✅ CC48–58 block on/off (0–63 off, 64–127 on); CC58 tuner; CC69 CTL.
- ✅ No MIDI clock (tempo is per-preset).
- ✅ SysEx editor layer (GP-5 scheme: CRC-8/0x07, nibble-split, float32 LE) —
  optional, for editor-app compatibility.
- ✅ **Free-chain reconciliation:** CC48–58 toggle the *first block of each
  category* in our free chain (GP-5-compatible façade). Advanced mode: assignable CCs.

## Effects chain
- ✅ **Free chain** — arbitrary block order/routing + parallel branches (like UltraRig).
- ✅ **No fixed max blocks** — bounded only by the runtime CPU/MEM budget (meters
  enforce, like the original). Data structure ceiling generous (e.g. 32 slots);
  no artificial 16-block cap.
- ❓ Exact block list. Proposed baseline (clean-room):
  gate, compressor, boost, overdrive, distortion, fuzz, EQ (graphic+param),
  hum/noise filter, chorus, flanger, phaser, vibrato, tremolo, rotary,
  pitch, pitch-pro, doubler, delay (tempo-sync), reverbs (room/plate/spring/hall/
  shimmer/spectral), amp (NAM), cab (IR).

## Amps
- ✅ **NAM A2-Lite** capture engine (clean-room from NAM/MIT + decoded A2L1 format).
- ✅ Ship the **factory Tank-G captures** converted to our format (RAT, Marshall-style,
  JCM900, Slash, etc. — already extracted, §4 of RE_FINDINGS) so the "original"
  tones are available.
- ✅ **2–3 algorithmic amps** (clean/crunch/high-gain) for players who don't want captures.
- ❓ Capture slots (UltraRig = 15). Proposed: 15+.

## Cabs / IR
- ✅ IR convolution, 48 kHz, accelerator-offloaded.
- ✅ Ship the 8 factory cab IRs. ✅ Load user `.wav`.
- ❓ IR slots (UltraRig = 8). Proposed: 8+.

## Presets
- ❓ Count (UltraRig = 32) and scenes (×4). Proposed: 32 × 4 scenes.
- ✅ GP-5 MIDI addresses up to 100 patches — reconcile with preset count.

## Control surface
- ❓ Footswitch behavior (short/long/double, Live vs Preset mode).
- ❓ LED ring: tuner, meters, configurable colors.
- ✅ Tuner (chromatic).

## Connectivity
- ✅ USB: Web-Serial control (editor) + USB-MIDI (GP-5 map). ❓ USB audio? (UltraRig dropped it.)
- ✅ BLE: control app + BLE-MIDI.

## Flash budget
- App area `[0x4000, 0x7B000)` = 487 KB for code+data. Presets/captures/IR in VM +
  user area. Must fit the 1 MiB flash with factory unique areas untouched.

## Open decisions to close next
1. Block list + max per preset. 2. Preset/scene/capture/IR counts.
3. Algorithmic amps yes/no. 4. Footswitch + LED behavior. 5. USB audio yes/no.
