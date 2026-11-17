# Building TankFree firmware — the concrete path

Honest status: our firmware is a **host-tested engine skeleton**, not yet a
flashable image. This is the real, proven route to a flashable app for the
Tank-G (JieLi AC791N / WL82), and to the **USB audio interface**.

## The proven base
The M-VAVE **FM-1 uses the same chip**, and two open projects already solved the
hard parts for it:

- **`Keitark/fm1-nes`** (Apache-2.0) — a buildable WL82 firmware kit with
  `firmware/usb-diag/` (CDC serial + return-to-UBOOT, the first milestone, *done*)
  and a **`--usb-audio`** profile (UAC1 composite with CDC, `USB_AUDIO.md`).
- **`jvitkauskas/pi32v2-isa` + `pi32v2-llvm`** — an **open** assembler/linker and
  an LLVM/clang that build FM-1 firmware from source, no JieLi binaries, and run
  on the hardware.
- JieLi **AC79 SDK** (`gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK`) — startup, linker
  layout, OS tasks, drivers (pinned revision).

Apache-2.0 and the open toolchain are compatible with our GPL-3 firmware. We
build on them clean-room: reuse the USB/boot plumbing, write our own DSP/UI.

## Build host
**Windows (or Linux/WSL).** Not macOS: the JieLi toolchain ships as `clang.exe`
(`C:/JL/pi32/bin`), and the open `pi32v2-llvm` has no prebuilt mac image. Needs
Python 3.11, Git, CMake, and the pi32v2 toolchain (JieLi's free download, or
build `pi32v2-llvm`). Set `FM1_SDK_DIR` and `FM1_TOOLCHAIN_DIR`.

## Milestones (each flashed with a factory backup ready to restore)
0. **Factory backup first (MANDATORY).** Loader mode (power-cycle 3× → `WL82 UBOOT`)
   + `jl-uboot-tool` on Windows/Linux → full 1 MiB dump + sha256. This is the only
   safe way back. Also confirms the chip key (`0x980F`) from `chip_key.bin`.
1. **usb-diag** — build `firmware/usb-diag/` (CDC `PING`/diagnostics + `UBOOT` return),
   flash to `[0x4000, 0x7B000)`, confirm boot + recovery on the Tank-G. Proves the
   whole toolchain → build → flash → boot → recover pipeline on *our* pedal.
2. **usb-audio** — build the `--usb-audio` profile; **adapt the audio path from the
   FM-1 to the Tank-G codec/pins** (the boards differ). Result: the Tank-G is a
   native USB soundcard again (UAC1, 48 kHz) + CDC control on one cable.
3. **Audio passthrough** — codec in → out, confirm sound.
4. **Our engine** — drop in `src/` (block.h/chain/blocks, host-tested) + the NAM
   A2-Lite runner + IR convolution (accelerator) + the line protocol + GP-5 MIDI.
5. **Store/UI** — presets/scenes/captures/IR in flash VM; footswitches, LED, tuner.

## Tank-G vs FM-1 — what must be re-done
Same SoC and boot, but **different board**: audio codec and routing, I/O pins,
footswitches/LEDs, no NES. The USB stack, UBOOT return, build system and linker
layout carry over; the **audio bring-up and the control surface are Tank-G-specific**
— informed by the factory firmware behavior and our live-device readout.

## Safety gates (do not skip)
- Never flash without a verified factory backup of *this* pedal.
- Only write `[0x4000, 0x7B000)`; never the bootloader, VM, `key_mac`, BTIF, OTP.
- Keep a forced-download path available (`jl-uboot-tool`, or a hardware boot-key)
  in case a software boot-entry route fails.

## Where our code plugs in
`firmware/src/` (block engine, host-tested) and `firmware/faust/` (amp DSP) are
the clean-room parts we drop onto the WL82 base once milestone 3 gives us audio.
See `../docs/SPEC.md`, `../docs/BLOCK_CATALOG.md`, `../docs/AMP_MODELING.md`.
