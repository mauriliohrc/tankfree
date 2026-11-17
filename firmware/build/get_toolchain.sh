#!/usr/bin/env bash
# Download the open pi32v2 LLVM toolchain (clang + ld.lld) for this host.
# Native binaries exist for macOS (arm64/x64), Linux (x64/arm64) and Windows —
# so the firmware builds on a Mac with no VM. Prints the bin/ dir on stdout.
set -euo pipefail
VER=pi32v2-23.1.5-5
case "$(uname -s)-$(uname -m)" in
  Darwin-arm64)  A=macos-arm64 ;;
  Darwin-x86_64) A=macos-x64 ;;
  Linux-x86_64)  A=linux-x64 ;;
  Linux-aarch64) A=linux-arm64 ;;
  *) echo "unsupported host: $(uname -sm)" >&2; exit 1 ;;
esac
DIR="${1:-.toolchain}"
TCDIR="$DIR/pi32v2-llvm-$VER-$A"
if [ ! -x "$TCDIR/bin/clang-23" ] && [ ! -x "$TCDIR/bin/clang" ]; then
  mkdir -p "$DIR"
  URL="https://github.com/jvitkauskas/pi32v2-llvm/releases/download/$VER/pi32v2-llvm-$VER-$A.tar.xz"
  echo "downloading $URL" >&2
  curl -fsSL "$URL" | tar -xJ -C "$DIR"
fi
echo "$TCDIR/bin"
