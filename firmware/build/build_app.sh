#!/usr/bin/env bash
# build_app.sh — ONE-SHOT native macOS/Linux build of a Tank-G firmware.
#
# Goes from a pristine JieLi AC79 SDK app to BOTH sdk.elf and app.bin using the open
# pi32v2-llvm toolchain (clang + ld.lld) — NO VM, NO Windows, NO JieLi lto-wrapper.
# Every non-obvious step that was painfully reverse-engineered is encoded here so it
# never has to be re-derived. See docs/BUILD_NATIVE.md for the "why" behind each patch.
#
#   bash build/build_app.sh [app-path]      # default app-path = demo/demo_hello
#
# Output: sdk/.../cpu/wl82/tools/{sdk.elf, app.bin}
#
# Proven milestones this reproduces (2026-11-17, first native Tank-G build on macOS):
#   - compiles the entire SDK app tree with clang --target=pi32v2 -mcpu=r3
#   - links with ld.lld in place of JieLi's proprietary lto-wrapper
#   - extracts + concatenates sections into app.bin exactly like JieLi's download.bat
set -uo pipefail
FW="$(cd "$(dirname "$0")/.." && pwd)"                 # firmware/
SDK="$FW/sdk/fw-AC79_AIoT_SDK"
BIN="$(cd "$FW"/build/.toolchain/pi32v2-llvm-*/bin && pwd)"
APP="${1:-demo/demo_hello}"
BOARD="$SDK/apps/$APP/board/wl82"
MF="$BOARD/Makefile"
APPDIR="$SDK/apps/$APP"
OUT_ELF_REL="../../../../../cpu/wl82/tools/sdk.elf"
TOOLS="$SDK/cpu/wl82/tools"
[ -f "$MF" ] || { echo "no Makefile at $MF"; exit 1; }
[ -x "$BIN/clang" ] || { echo "toolchain missing; run build/get_toolchain.sh"; exit 1; }
echo "toolchain: $BIN"; echo "app:       $APP"

# ── 1. patch the board Makefile deterministically (always from a pristine .orig) ──
[ -f "$MF.orig" ] || cp "$MF" "$MF.orig"
python3 - "$MF" "$BIN" <<'PY'
import sys
mf, binp = sys.argv[1], sys.argv[2]
s = open(mf + ".orig").read()                          # always start pristine

# (a) point the Linux toolchain block at our pi32v2-llvm; swap lto-wrapper → ld.lld
s = s.replace("TOOL_DIR := /opt/jieli/pi32v2/bin", f"TOOL_DIR := {binp}")
s = s.replace("LD    := lto-wrapper", "LD    := ld.lld")
s = s.replace("AR    := lto-ar", "AR    := llvm-ar")
s = s.replace("export OBJDUMP := $(TOOL_DIR)/objdump",       "export OBJDUMP := $(TOOL_DIR)/llvm-objdump")
s = s.replace("export OBJCOPY := $(TOOL_DIR)/objcopy",       "export OBJCOPY := $(TOOL_DIR)/llvm-objcopy")
s = s.replace("export OBJSIZEDUMP := $(TOOL_DIR)/objsizedump","export OBJSIZEDUMP := $(TOOL_DIR)/llvm-size")

# (b) drop JieLi-only clang flags unknown to open pi32v2-llvm (r3 FP comes from -mcpu=r3)
for flag in ("\t-mfprev1 \\\n", "\t-fallow-pointer-null \\\n", "\t-fprefer-gnu-section \\\n"):
    s = s.replace(flag, "")

# (c) demo benchmark cruft (coremark/dhrystone) uses implicit decls — don't fail on them
s = s.replace("\t-w \\\n",
              "\t-w \\\n\t-Wno-implicit-function-declaration \\\n\t-Wno-error=implicit-function-declaration \\\n")

# (d) rewrite LFLAGS: ld.lld-compatible. Flatten nested --start-group, -M= → -Map=,
#     drop JieLi's proprietary LLVM passes (movable-region overlays, inline tuning,
#     used-symbol-file, simd, rep-memop, dont-used-symbol-list) the open backend lacks.
import re
new = r'''LFLAGS := \
\t--plugin-opt=-enable-ipra=true \
\t--plugin-opt=-pi32v2-merge-max-offset=4096 \
\t--plugin-opt=mcpu=r3 \
\t--plugin-opt=-global-merge-on-const \
\t--plugin-opt=-inline-threshold=5 \
\t--plugin-opt=-emulated-tls \
\t--plugin-opt=-pi32v2-large-program=true \
\t--sort-common \
\t-T../../../../../cpu/wl82/sdk.ld \
\t-Map=../../../../../cpu/wl82/tools/sdk.map \
\t--start-group \
\t../../../../../include_lib/newlib/pi32v2-lib/libm.a \
\t../../../../../include_lib/newlib/pi32v2-lib/libc.a \
\t../../../../../include_lib/newlib/pi32v2-lib/libcompiler_rt.a \
\t../../../../../cpu/wl82/liba/cpu.a \
\t../../../../../cpu/wl82/liba/event.a \
\t../../../../../cpu/wl82/liba/system.a \
\t../../../../../cpu/wl82/liba/cfg_tool.a \
\t../../../../../cpu/wl82/liba/fs.a \
\t../../../../../cpu/wl82/liba/common_lib.a \
\t../../../../../cpu/wl82/liba/update.a \
\t../../../../../include_lib/c++/libstdc++/libemutls.a \
\t../../../../../include_lib/c++/libstdc++/libc++.a \
\t../../../../../include_lib/c++/libstdc++/libc++abi.a \
\t--end-group \
\t--plugin-opt=mcpu=r3 \
\t--plugin-opt=-mattr=+fprev1
'''.replace("\\t", "\t").replace("\\\n", "\\\n")
s = re.sub(r'LFLAGS := \\.*?\n\n\nc_OBJS', new + "\n\nc_OBJS", s, flags=re.DOTALL)

# (e) register our reentrant-alloc shim (newlib _realloc_r → SDK realloc)
anchor_app = [ln for ln in s.splitlines() if ln.strip().endswith("app_main.c \\")]
if anchor_app and "tankfree_shim.c" not in s:
    a = anchor_app[0] + "\n"
    shim = "\t" + "/".join(a.strip().split("/")[:-1]) + "/tankfree_shim.c \\\n"
    s = s.replace(a, a + shim)

open(mf, "w").write(s)
print("Makefile patched (tools, flags, LFLAGS, shim)")
PY

# ── 2. create the alloc shim next to app_main.c ──
cat > "$APPDIR/tankfree_shim.c" <<'EOF'
/* TankFree: forward newlib's reentrant allocator to the SDK's plain realloc().
 * Our open-toolchain link omits JieLi's -dont-used-symbol-list, so newlib's
 * ecvt/fcvt pull _realloc_r; the SDK provides realloc(). */
#include <stddef.h>
extern void *realloc(void *, size_t);
void *_realloc_r(void *reent, void *ptr, size_t n) { (void)reent; return realloc(ptr, n); }
EOF

cd "$BOARD"
ulimit -n 8192 2>/dev/null || true

# ── 3. pre_build: generate sdk.ld / used-list / section maps from .c via the preprocessor
echo "=== pre_build ==="
make pre_build 2>&1 | tail -3 || { echo "pre_build failed"; exit 1; }

# ── 4. compile every object + link with ld.lld (LINK_AT=0: macOS make 3.81 lacks \$(file))
echo "=== compile + link → sdk.elf ==="
make LINK_AT=0 "$OUT_ELF_REL" 2>&1 | grep -iE "error:|undefined|cannot open|\+LINK" | grep -v note: | tail -20
if [ ! -f "$TOOLS/sdk.elf" ]; then echo "✗ link failed"; exit 1; fi
echo "✓ sdk.elf"; "$BIN/llvm-size" "$TOOLS/sdk.elf"

# ── 5. package sections → app.bin (exactly JieLi's download.bat: text+data+ram0+cache_ram)
echo "=== app.bin ==="
cd "$TOOLS"
OC="$BIN/llvm-objcopy"
for sec in text data ram0_data cache_ram_data; do
  "$OC" -O binary -j ".$sec" sdk.elf "$sec.bin" 2>/dev/null || : > "$sec.bin"
done
cat text.bin data.bin ram0_data.bin cache_ram_data.bin > app.bin
rm -f text.bin data.bin ram0_data.bin cache_ram_data.bin
ls -la "$TOOLS/app.bin"
echo "✓ DONE → $TOOLS/app.bin  (next: build/build_pack.sh to wrap it into a flashable image)"
