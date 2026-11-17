# Valeton GP-5 MIDI map (our replication target)

The GP-5 MIDI is minimal and slightly non-standard. We clone it exactly.

## Transports
- **No DIN/TRS MIDI jack.** USB-C **USB-MIDI** (class compliant) + **BLE-MIDI**.
- BLE-MIDI standard GATT: service `03b80e5a-ede8-4b33-a751-6ce34ec4c700`,
  characteristic `7772e5db-3868-4112-a1a9-f2669d106bf3`.
- Channel **fixed to 1**.

## Preset recall
- **CC#0, value 0–99 → patch 00–99.** (GP-5 ignores Program Change entirely.)
- No standard Bank Select (CC0 is the selector; CC32 unused).
- 100 slots (50 factory + 50 user).

## Full CC map (all receive-side)
| CC | Range | Function |
|---|---|---|
| 0 | 0–99 | Patch select 00–99 |
| 7 | 0–100 | Patch volume (per-patch) |
| 22 | 0–127 | Bank − (tens digit) |
| 23 | 0–127 | Bank + |
| 24 | 0–127 | Patch − |
| 25 | 0–127 | Patch + |
| 29 | 0–127 | Patch − in Song List |
| 30 | 0–127 | Patch + in Song List |
| 48 | 0/64 | NR block on/off |
| 49 | 0/64 | PRE block on/off |
| 50 | 0/64 | DST block on/off |
| 51 | 0/64 | N→S (NAM/SnapTone) on/off |
| 52 | 0/64 | AMP on/off |
| 53 | 0/64 | CAB on/off |
| 54 | 0/64 | EQ on/off |
| 55 | 0/64 | MOD on/off |
| 56 | 0/64 | DLY on/off |
| 57 | 0/64 | RVB on/off |
| 58 | 0/64 | Tuner / CTL on/off |
| 69 | 0/64 | CTL footswitch (RE'd, not in printed chart — verify on hardware) |

Block toggles: **0–63 = Off, 64–127 = On.**

## Not supported (confirmed absent on GP-5)
- Program Change, standard Bank Select (CC32).
- MIDI Clock / tempo sync / MIDI tap tempo.
- Scenes, looper, drum, MIDI expression, global master volume.

## SysEx (editor protocol, reverse-engineered — optional for us)
- Frame: `F0` + nibble-split bytes (hi-nibble first) of
  `[CRC8, command, index, length, ...payload]` + `F7`.
- Checksum: **CRC-8/SMBUS (poly 0x07, init 0x00)** over the buffer with CRC byte zeroed.
- Params: **float32 little-endian**.
- Reads: req 0x01 / resp 0x02; selector 0x40 = all patch names, 0x41 = active patch
  body (GP-5 ≈ 507 bytes), 0x24 = SnapTone/NAM, 0x20 = IR.
- Edits: `11 43` select patch · `11 47` change effect · `11 48` change param · `11 49` toggle block.
- Preset file `.prst`: `"GP-5"` header + CRC + 16-byte name + 507-byte body.

## Reconciliation with our FREE chain
The GP-5 assumes a fixed 10-slot chain (NR,PRE,DST,N→S,AMP,CAB,EQ,MOD,DLY,RVB).
We map **CC48–57 → first block of that category** in the free chain, so a GP-5
MIDI controller behaves identically. Advanced mode exposes fully assignable CCs.

## Minimum viable clone
Listen ch 1; CC0→patch; CC7→vol; CC22/23 bank±, CC24/25 patch±, CC29/30 song±;
CC48–57→category bypass; CC58→tuner, CC69→CTL. No PC, no CC32, no clock.

Sources: GP-5 manual (danguitar.dk/pub/assets/VA-GP5.pdf), Morningstar guide,
github helvecioneto/gp5-wc, grzmol/VLTN-Tone-Studio, drewmerc302/valeton-gp50.
