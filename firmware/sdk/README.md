# Native SDK build (macOS, no VM)

Proven: the open **pi32v2-llvm** toolchain runs natively on macOS and **compiles
the JieLi AC79 SDK's C** (walks FreeRTOS/USB/driver headers) with the real flags —
so a Tank-G firmware builds on a Mac. This is the setup; completing it to a booting
`app.bin` is the remaining (native) integration work.

## Toolchain (native)
`firmware/build/get_toolchain.sh` downloads `pi32v2-llvm` for the host
(macOS arm64/x64, Linux, Windows). Gives `clang-23`, `ld.lld`, `llvm-*`.

## SDK
JieLi **fw-AC79_AIoT_SDK** (mirror: `github.com/amitv87/fw-AC79_AIoT_SDK`; upstream
gitee `Jieli-Tech/fw-AC79_AIoT_SDK`). Our chip = **WL82**. The canonical app build is
`apps/demo/demo_hello/board/wl82/Makefile`.

## The native build recipe (verified pieces)
Compile flags (from the SDK Makefile, minus JieLi-only ones):
```
--target=pi32v2 -mcpu=r3 -Oz -fno-common -integrated-as
# drop JieLi's -mfprev1 (unknown to pi32v2-llvm; the r3 FP is enabled by -mcpu=r3)
```
Defines: `-DCONFIG_CPU_WL82 -DCONFIG_FREE_RTOS_ENABLE -DCONFIG_AUDIO_ONCHIP -D__GCC_PI32 …`
Includes: the Makefile's full `-I` list (include_lib, driver, cpu/wl82, system,
system/generic, newlib/include, FreeRTOS port dir, the app's board + include dirs).
→ `clang-23` compiles the SDK C natively on macOS (confirmed: app_main.c → includes.h
→ FreeRTOS → … resolving the real header tree).

## Build plan (to finish — all native)
1. **Adapt `demo_hello/board/wl82/Makefile`** to our toolchain:
   `TOOL_DIR` → our `pi32v2-llvm/bin`; `CC`=`clang-23`; `LD`=`ld.lld`; `AR`=`llvm-ar`;
   `OBJCOPY/OBJDUMP`=`llvm-objcopy/llvm-objdump`; drop `-mfprev1`.
2. **Link** our code + the SDK sources + the SDK precompiled **bitcode** `.a` libs
   with `ld.lld` (it does the LTO the JieLi `lto-wrapper` did) + the SDK linker
   scripts (`include_lib/driver/cpu/wl82/system.ld`, …) → `sdk.elf`.
3. **Package** `sdk.elf` → `app.bin` with OUR tools (we decoded the format): extract
   the app area and SFC-encrypt with `installer/fwtool.py encrypt` (chip key 0x980F),
   instead of JieLi's `download.sh`/`isd_download` (Windows).
4. **Swap the app:** replace demo_hello's `app_main.c` with our `app_alpha.c` +
   `board/board_wl82.c` (USB audio glue) + the engine/proto modules.
5. **Flash:** `installer/mac_flash.py` (native HID) or M-UPGRADE recovery as the net.

## Status
- ✅ Toolchain native on macOS; compiles our firmware (6/6 modules) AND SDK C.
- 🚧 Full Makefile adaptation + LTO link of the SDK bitcode libs + packaging.
- 🚧 Tank-G board bring-up (codec pins, USB descriptors) — needs hardware iteration.
- Our DSP (gate/NAM/IR/master) + USB-audio glue (`board_wl82.c`) are ready to graft.
