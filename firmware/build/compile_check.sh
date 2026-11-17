#!/usr/bin/env bash
# Compile our firmware DSP/engine modules for pi32v2 — proves they build for the
# AC791N (no VM; native clang on macOS/Linux). Full link into a bootable image
# needs the SDK/board layer (see BUILD.md); this is the chip-compile gate.
set -euo pipefail
cd "$(dirname "$0")/.."                      # firmware/
BIN="${1:-$(build/get_toolchain.sh build/.toolchain)}"
CLANG="$BIN/clang-23"; [ -x "$CLANG" ] || CLANG="$BIN/clang"
FLAGS="--target=pi32v2 -O2 -ffreestanding -DTF_EMBED -Isrc -Wall"
mkdir -p build/obj
n=0
for f in src/chain.c src/registry.c src/midi_mvave.c src/blocks/*.c; do
  "$CLANG" $FLAGS -c "$f" -o "build/obj/$(basename "${f%.c}").o"
  echo "  ✓ $f"; n=$((n+1))
done
echo "$n modules compile for pi32v2 (AC791N)."
"$BIN/llvm-size" build/obj/*.o 2>/dev/null | tail -n +1 || true
