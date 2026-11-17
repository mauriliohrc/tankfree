# Native Tank-G firmware build (macOS/Linux, no VM) — the proven recipe

First native build of a Tank-G (JieLi AC791N/WL82) firmware on an Apple-Silicon Mac,
2026-11-17. This is the exact, reproducible process — encoded in
`firmware/build/build_app.sh` so it never has to be re-derived.

## One command
```
firmware/build/get_toolchain.sh firmware/build/.toolchain   # once: pi32v2-llvm (macOS/Linux)
firmware/build/build_app.sh demo/demo_hello                 # → sdk.elf + app.bin
```
Prereq: the JieLi SDK cloned at `firmware/sdk/fw-AC79_AIoT_SDK`
(`git clone --depth 1 https://github.com/amitv87/fw-AC79_AIoT_SDK`).

## What had to be reverse-engineered / patched (the non-obvious parts)
The SDK ships for JieLi's own Windows/Linux toolchain (a proprietary `lto-wrapper`
around their LLVM fork). To build with the **open** `pi32v2-llvm` (jvitkauskas), the
board Makefile is patched deterministically from a pristine `Makefile.orig`:

1. **Toolchain swap.** `TOOL_DIR` → our `pi32v2-llvm/bin`; `LD: lto-wrapper → ld.lld`;
   `AR → llvm-ar`; `OBJDUMP/OBJCOPY/OBJSIZEDUMP → llvm-*`. The clang binary stays `clang`.
2. **Drop JieLi-only clang flags** unknown to open clang: `-mfprev1` (r3 FP is enabled
   by `-mcpu=r3`), `-fallow-pointer-null`, `-fprefer-gnu-section`.
3. **Benchmark cruft.** demo_hello drags in coremark/dhrystone which use implicit
   declarations → add `-Wno-implicit-function-declaration -Wno-error=implicit-function-declaration`.
   (Our real app drops these benchmark sources entirely.)
4. **LFLAGS rewrite for ld.lld** (the crux):
   - `-M=sdk.map` → `-Map=sdk.map` (ld.lld spelling).
   - Flatten the **nested `--start-group`** (ld.lld forbids nesting) into one group
     around all `.a` libs.
   - **Drop JieLi's proprietary LLVM passes** the open backend doesn't have:
     all `--plugin-opt=-movable-region-*` (their code-overlay/banking system),
     `-pi32v2-enable-simd`, `-pi32v2-enable-rep-memop`, `-inline-max-allocated-size`,
     `-inline-normal-into-special-section`, `-dont-used-symbol-list`,
     `-used-symbol-file`, `-pi32v2-always-use-itblock`. Kept the ones the open backend
     DOES accept: `-enable-ipra`, `-pi32v2-merge-max-offset`, `-global-merge-on-const`,
     `-inline-threshold`, `-emulated-tls`, `-pi32v2-large-program`, `mcpu=r3`,
     `-mattr=+fprev1`, `--sort-common`.
     Consequence: no movable-region overlays, so the program must fit the flat layout
     (fine for our app; revisit if we ever exceed RAM and need banking).
5. **`_realloc_r` shim.** Without `-dont-used-symbol-list`, newlib's ecvt/fcvt pull
   `_realloc_r`, which the SDK doesn't provide (it provides plain `realloc`). Add
   `tankfree_shim.c`: `void *_realloc_r(void*r,void*p,size_t n){return realloc(p,n);}`.
6. **`LINK_AT=0`.** The Makefile's default link path uses GNU-make 4's `$(file …)`;
   macOS ships make 3.81 which lacks it → pass `LINK_AT=0` to link `$(OBJS)` directly.

## Build stages (what build_app.sh runs)
- **pre_build** — the SDK generates `sdk.ld`, `sdk_used_list.used`, movable section maps,
  and `download.sh` by running them through the **C preprocessor** (`clang -E -P` over
  `sdk_ld.c`, `download.c`, …). Works unmodified with our clang.
- **compile** — clang `--target=pi32v2 -mcpu=r3 -Oz -flto` → LTO bitcode objects. The
  whole SDK app tree compiles natively (FreeRTOS, USB, drivers).
- **link** — `ld.lld` does the LTO + final link against the SDK's prebuilt bitcode `.a`
  libs (`cpu.a`, `system.a`, newlib, libc++) + `sdk.ld` → `sdk.elf`.
- **package → app.bin** — exactly JieLi's `download.bat`: `llvm-objcopy -O binary -j`
  each of `.text .data .ram0_data .cache_ram_data`, then concatenate. ENTRY = 0x2000120
  (matches `.text` VMA 0x02000120 and isd_config.ini `ENTRY`).

Proven result: `sdk.elf` text=168068 data=84 bss=17136; `app.bin` = 168100 bytes.

## REMAINING: the flashable container (the last mile)
`app.bin` is the raw app sections — **NOT yet flashable**. The device's OTA pulls the
**full ~512 KB flash image** (uboot + app + config + resources), SFC-encrypted
(chip key 0x980F), wrapped as a JieLi `jl_isd.fw` → `.ufw`. JieLi builds this with
Windows-only `isd_download.exe` + `fw_add.exe` + `ufw_maker.exe` (and `download.sh`
even offloads to JieLi's cloud via `host-client`). We must reimplement that packer
natively — layout is in the generated `isd_config.ini` (FLASH_SIZE=4M, ENTRY=0x2000120,
single-backup NEW_FLASH_FS), crypto is in `installer/fwtool.py`, and factory `.bin`
images are on hand to diff the container header/partition format. Until the packer
exists, **nothing can be flashed** — the pedal's `prepare` stage validates the
container header/CRC and rejects a raw app.bin. Tracked as `build/build_pack.sh` (TODO).
