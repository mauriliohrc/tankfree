# SDK integration — making the Alpha boot + do USB audio

Our DSP (gate → NAM → IR → master, `src/app_alpha.c`) compiles for pi32v2 and runs
on host. To make it a **booting firmware that's a USB soundcard like the factory
Tank-G**, we graft it onto the JieLi AC79 SDK (same chip). This is the big remaining
integration — the pieces exist in the SDK; here's the map.

## SDK pieces we use (from fw-AC79_AIoT_SDK, mirror github.com/amitv87/fw-AC79_AIoT_SDK)
- **CPU / boot:** `cpu/wl82/` — our chip (WL82 / AC791N) startup, SFC/XIP, linker.
- **USB Audio Class (device):** `apps/common/usb/device/uac.c`, `uac_stream.{c,h}`,
  `include_lib/driver/device/usb/device/uac_audio.h`, `apps/common/audio_music/usb_audio_api.c`.
  Gives the PC-visible soundcard: playback (DAW → pedal) and capture (pedal → DAW), 24-bit.
- **Audio in/out / codec:** the SDK audio DAC/ADC + mixer drivers (same ones the
  factory firmware uses for the 1/4" + XLR + phones).
- **OS:** FreeRTOS (the SDK ships it).

## The grafting plan (like Keitark/fm1-nes, Apache-2.0, same chip)
1. Start from a buildable SDK app (fm1-nes `usb-audio` profile is the closest base).
2. **Composite USB device:** enable UAC (soundcard) **+** USB-CDC (our control
   protocol, `src/proto.c`) **+** USB-MIDI (GP-5 map + `src/midi_mvave.c` identity).
3. **Audio callback:** wire the SDK's audio block callback to our pipeline —
   implement the three hooks `app_alpha.c` declares:
   - `tf_board_audio_in()`  ← ADC (guitar) + optional USB playback mix
   - `tf_board_audio_out()` → DAC (phones/XLR) **and** USB capture (to the DAW)
   - run `tf_alpha_process()` in between.
4. **Board bring-up (Tank-G specific):** codec routing + pins differ from the FM-1;
   set them from the factory firmware behavior + the SDK board config.
5. **Build:** `firmware/build/get_toolchain.sh` (pi32v2 clang, native on macOS) +
   the SDK's link step → `app.bin`; SFC-encrypt with `installer/fwtool.py encrypt`.
6. **Flash (macOS):** finish `installer/mac_flash.py` (HID OTA). Until then, the
   M-UPGRADE recovery net (our `midi_mvave.c` identity) always restores factory.

## Alpha scope (what ships first)
- USB soundcard (playback + capture), 48 kHz / 24-bit.
- Fixed chain: Noise Gate → NAM (A2L1) → IR → Master.
- Master + gate controllable. Then we enable more blocks incrementally.

## Honest status
- ✅ DSP (gate/NAM/IR/master) — our code, compiles for the chip, host-tested.
- ⬜ SDK graft (USB audio + codec + boot + Tank-G board) — the big integration.
- ⬜ NAM fixed-point bit-exact validation vs a reference `.nam` (`a2l.c` head is a
  placeholder; tone is approximate until validated).
- 🚧 Mac flashing (`mac_flash.py` HID framing) — to actually load it.
