# TankFree Firmware (pi32v2 app)

The on-pedal application. Built as a JieLi AC79/WL82 SDK app, like UltraRig.
**Skeleton** — building this needs the pi32v2 toolchain and the physical pedal.

> **The concrete, followable build path is in [`BUILD.md`](BUILD.md)** — it adopts
> the proven FM-1 base (`Keitark/fm1-nes`, Apache-2.0; `usb-diag` + `usb-audio`
> profiles for our exact chip) and the open `pi32v2-llvm` toolchain. Build on
> **Windows/Linux**, flash only after a **factory backup**. Start reading there.

## Toolchain
- SDK: `git clone https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK` (Apache-2.0) +
  `Jieli-Tech/fw-Bootloader` (target `wl82`).
- Compiler: JieLi clang 4.0.1 for pi32v2 (`/opt/jieli`), **or** the open
  `jvitkauskas/pi32v2-isa` + `pi32v2-llvm`.
- Disasm: `kagaimiq/ghidra-jieli` (pi32v2 SLEIGH) to study the factory/UltraRig apps.

## Build target (what we're writing)
- App linked at flash `0x4000`, fits `[0x4000, 0x7B000)` (≤ 487 KB), XIP from SPI NOR.
- FreeRTOS (SDK ships V9.0.0). Audio @ 48/96 kHz, 24-bit, no resample.
- Modules (to implement, clean-room — see ../docs/SPEC.md):
  - `audio/`  — codec setup, I/O, sample-rate switch, the 48 kHz NAM island at 96 kHz.
  - `engine/a2l.c` — NAM A2-Lite WaveNet fixed-point runner (format in RE_FINDINGS §7).
  - `engine/ir.c`  — partitioned IR convolution on the FFT accelerator.
  - `engine/fx/`   — effect blocks (gate, drive, EQ, mod, delay, reverbs, pitch…).
  - `chain.c` — free routing graph + parallel branches.
  - `proto.c` — ASCII line protocol over USB-CDC (+ BLE Nordic-UART).
  - `midi.c`  — USB-MIDI + BLE-MIDI, GP-5 CC map (docs/MIDI_GP5.md).
  - `store.c` — presets/scenes/captures/IR in flash VM (wear-leveled).
  - `ui.c`    — footswitches, LED ring, tuner.
- SDK files to patch (UltraRig touched only 4): `setup.c`, `debug.c`, `init.c`, `usb_config.c`.

## Flashing
Build → `installer/fwtool.py encrypt sdk.app app.enc` → `installer/flasher.py write --app app.enc`.

## Legal
Clean-room. Study UltraRig/factory only for behavior/parameter ranges; copy no
compiled code. NAM core is MIT; DSP from published algorithms.
