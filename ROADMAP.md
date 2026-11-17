# TankFree — Roadmap & Status

Where the project is and what's left. ✅ done · 🚧 in progress · ⬜ to do.
(Checkboxes render as a live task list on GitHub.)

## Phase 0 — Reverse engineering ✅ (done)
- [x] Identify hardware: JieLi **AC791N / pi32v2**, 1 MiB flash, FFT accelerator
- [x] Break firmware crypto (**chip key 0x980F**) — decrypt validated on the real app
- [x] Flash map (app `0x4000–0x7B000`, VM, untouched areas) + `.fwsc` format notes
- [x] Full line protocol + data formats (**A2L1** NAM, AM4 captures)
- [x] Extract the factory tones (8 amp captures + 8 cab IRs)
- [x] Live-device validation: protocol, NAM engine (L23/C3), memory (~118 KB arena)
- [x] Capture the full block catalog (`FXT`) + verify real editing (`FXP`) on hardware
- [x] Docs: `RE_FINDINGS`, `PROTOCOL`, `MIDI_GP5`, `AMP_MODELING`, `BLOCK_CATALOG`, `SPEC`

## Phase 1 — Tooling ✅ (mostly done)
- [x] `jltech` crypto lib (decrypt/encrypt/CRC) + unit tests in CI
- [x] `fwtool` (offline decrypt/encrypt/extract tones)
- [x] `tankctl` — terminal control of a live pedal (verified on hardware)
- [x] `probe` — safe read-only serial first-contact
- [x] One-click **installer GUI** (tkinter) + auto-build CI (macOS `.app` built; Windows `.exe` via CI)
- [ ] 🚧 **Desktop app** (Electron) around the editor — Mac/Win
- [ ] Flasher backend: wire `jl-uboot-tool`, backup → write app → verify → recover *(needs device; Win/Linux)*
- [ ] Factory firmware dump + `.fwsc` unpack *(loader mode + Win/Linux)*

## Phase 2 — Manager (web/desktop editor)
- [x] QC-style signal grid + ToneX-style per-block realism (amps + pedals drawn)
- [x] Demo mode (fully navigable, no pedal)
- [x] Read a real pedal: presets / captures / IRs / live CPU-MEM meters
- [ ] 🚧 **Live editing**: knobs → `FXP`, bypass → `FXE`, amp → `AMPEQ`, from `FXS`/`FXT`
- [ ] Real chain view from `FXS` (add/remove/move blocks, parallel lanes)
- [ ] **NAM `.nam` → `A2L1`** converter in-browser (capture upload)
- [ ] IR `.wav` upload
- [ ] Preset export/import (`.tankfree.json`)
- [ ] GP-5 MIDI map (USB-MIDI + BLE-MIDI) wired in the editor
- [ ] BLE transport (Web Bluetooth)

## Phase 3 — Our firmware (the big track) ⬜
- [ ] Set up JieLi **AC79 SDK** + **pi32v2 toolchain**
- [ ] Minimal booting firmware: USB-CDC `PING→PONG` (proves toolchain + flash + boot)
- [ ] Audio passthrough (codec in→out, 48 kHz / 24-bit)
- [x] Free-chain block engine skeleton (compiles + host test in CI)
- [ ] NAM **A2-Lite** fixed-point runner (clean-room) + IR convolution on the accelerator
- [ ] Effect blocks (clean-room from FAUST/Guitarix/BYOD — see `BLOCK_CATALOG`)
- [ ] Algorithmic amps (FAUST tone stacks — `brit800`, `plexi`, `ac30`… — see `AMP_MODELING`)
- [ ] Line protocol handler (compatible surface) + GP-5 MIDI
- [ ] **USB Audio Class** (keep the native soundcard the factory fw had) — optional/measured
- [ ] Preset/scene/capture/IR store in flash VM
- [ ] Footswitches + LED ring + tuner
- [ ] Flash it + configure from the editor → **end-to-end**

## Phase 4 — Polish ⬜
- [ ] Recovery/safety hardening (boot guard, watchdog parity)
- [ ] Docs/site, first tagged release with installers + desktop app
- [ ] Community testing

---
**Legend of "can I use it today?"**
- Configure a pedal running UltraRig/TankFree over serial: ✅ (`tankctl`, editor read)
- Flash *our* firmware: ⬜ (Phase 3 — not yet; needs the firmware built)
- Dump factory firmware: needs loader mode + Windows/Linux
