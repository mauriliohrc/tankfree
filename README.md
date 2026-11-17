<div align="center">

# TankFree

**An open, community firmware & toolkit for the M-VAVE Tank-G / Tank-B v2 guitar pedal**
*(SoC: JieLi AC791N · CPU pi32v2)*

[![CI](https://github.com/mauriliohrc/tankfree/actions/workflows/ci.yml/badge.svg)](../../actions)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
![status: alpha](https://img.shields.io/badge/status-early%20alpha-orange)

</div>

> ⚠️ **Independent project — not affiliated with M-VAVE, MOOER, JieLi or Valeton.**
> Installing custom firmware can brick your pedal and voids its warranty. Use at
> your own risk, on hardware you own. Read [`DISCLAIMER.md`](DISCLAIMER.md).

TankFree aims to replicate (and extend) the original Tank-G tones with a
clean-room open firmware, plus the tooling you need to live with it: a flasher
and a browser-based editor.

## What's inside

| Folder | What it is | Status |
|---|---|---|
| [`installer/`](installer/) | Python flasher + offline firmware tools (decrypt/encrypt/extract). Validated JieLi crypto. | crypto ✅ · flashing 🚧 |
| [`manager/`](manager/) | Web editor — Web Serial + Web MIDI (Valeton **GP-5**-style map) + BLE. Free effects chain, presets, captures, IRs. | scaffold 🚧 |
| [`firmware/`](firmware/) | The on-pedal app (pi32v2, JieLi AC79 SDK). | skeleton 🧪 |
| [`docs/`](docs/) | Reverse-engineering notes, firmware spec, GP-5 MIDI map. | ✅ |

## Features (target)

- **Audio** 48 kHz / 24-bit (user-selectable 96 kHz), no internal resample.
- **Amps** NAM A2-Lite capture engine + factory-style captures + algorithmic amps.
- **Cabs** impulse-response convolution; load your own `.wav`.
- **Free effects chain** — arbitrary routing and parallel branches, no fixed slots.
- **Presets** 32 × 4 scenes.
- **MIDI** identical to the Valeton GP-5 over USB-MIDI and BLE-MIDI
  (see [`docs/MIDI_GP5.md`](docs/MIDI_GP5.md)).
- **Editor** in Chrome/Edge over USB — no app install.

## Quick start (works today, no pedal needed)

```bash
# validate the crypto and run the unit tests
python3 installer/tests/test_cipher.py

# serve the web editor locally (Chrome/Edge)
cd manager && python3 -m http.server 8000   # then open http://localhost:8000
```

The flasher and the firmware build need the physical pedal and the pi32v2
toolchain — see [`installer/README.md`](installer/README.md) and
[`firmware/README.md`](firmware/README.md).

## Bring your own files

This repo ships **no proprietary content**. To use the tools you supply your own
firmware, captures and IRs from your device / official sources. Flash dumps and
backups hold your pedal's unique Bluetooth identity — **never share them**.

## Contributing

PRs welcome — please keep it **clean-room** (don't paste decompiled proprietary
code). See [`CONTRIBUTING.md`](CONTRIBUTING.md) and [`DISCLAIMER.md`](DISCLAIMER.md).

## Credits & prior art

Stands on the shoulders of the JieLi RE community: kagaimiq (jl-uboot-tool,
jl-misctools, ghidra-jieli), jvitkauskas (pi32v2-isa / pi32v2-llvm), the FM-1 and
SMK-37 RE projects, the JieLi AC79 SDK, and the open **Neural Amp Modeler**.

## License

[GPL-3.0](LICENSE). *(Default choice — open to discussion before 1.0.)*
