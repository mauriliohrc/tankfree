#!/usr/bin/env python3
"""flasher.py — flash OpenTank-G firmware to the pedal via the JieLi UBOOT loader.

⚠️  This talks to the physical pedal. It is a SCAFFOLD: the safe sequence is
implemented as structured steps, but the low-level UBOOT/SCSI transport is
delegated to `jl-uboot-tool` (kagaimiq, MIT) and must be validated on-device
before trusting a write. NEVER writes the SPL, VM, BTIF, OTP or key_mac areas.

Safe sequence (mirrors the factory installer):
  1. detect pedal (running firmware CDC, or already-in-UBOOT disk)
  2. enter loader mode (soft-key SysEx, or user power-cycles into boot guard)
  3. upload wl82loader.bin to RAM, read chip key + flash id
  4. FULL 1 MiB backup (read twice, compare) -> backups/<date>/
  5. write app area [0x4000, 0x7B000) sector-by-sector: erase, write, verify
  6. leave UBOOT; wait for CDC PING; confirm fw healthy

Run `fwtool.py encrypt` first to SFC-encrypt your plaintext app with chip key.
"""
import os, sys, time, datetime

APP_AREA = (0x4000, 0x7B000)           # Tank-G; never touch [0,0x4000) or >=0x7B000 VM
SPL = (0x0, 0x4000)
VENDOR = os.path.join(os.path.dirname(__file__), "vendor", "jl-uboot-tool")
LOADER = os.path.join(VENDOR, "wl82loader.bin")


class FlashError(Exception): pass


def _require_tool():
    """Import jl-uboot-tool from vendor/. Copy it from the UltraRig package's
    _internal/vendor/jl-uboot-tool/ (MIT) or `git clone kagaimiq/jl-uboot-tool`."""
    if not os.path.isdir(VENDOR):
        raise FlashError(
            f"jl-uboot-tool not found at {VENDOR}.\n"
            "Get it: git clone https://github.com/kagaimiq/jl-uboot-tool "
            f"{VENDOR}  (and place wl82loader.bin alongside).")
    sys.path.insert(0, VENDOR)


def detect():
    """Return one of: 'uboot' (WL82 UBOOT disk present), 'cdc' (running fw),
    'midi' (factory fw), or None. TODO: implement per-OS probing."""
    raise NotImplementedError("device detection — implement with pyserial + OS disk/MSC probe")


def backup(out_dir):
    """Read the full 1 MiB flash twice and compare; write dump.bin + sha256."""
    _require_tool()
    raise NotImplementedError("full-flash backup via jl-uboot-tool read 0x0 0x100000")


def write_app(app_bin_path, out_dir=None, dry_run=True):
    """Flash an SFC-encrypted app image to [0x4000, 0x7B000). Backs up first."""
    _require_tool()
    size = os.path.getsize(app_bin_path)
    if size > APP_AREA[1] - APP_AREA[0]:
        raise FlashError(f"app is {size} B, app area is {APP_AREA[1]-APP_AREA[0]} B")
    out_dir = out_dir or os.path.join("backups", datetime.date.today().isoformat())
    os.makedirs(out_dir, exist_ok=True)
    print(f"[plan] backup -> {out_dir}; write {size} B to 0x{APP_AREA[0]:X}..0x{APP_AREA[1]:X}")
    if dry_run:
        print("[dry-run] nothing written."); return
    raise NotImplementedError("confirm on-device before enabling real writes")


if __name__ == "__main__":
    import argparse
    ap = argparse.ArgumentParser(description="OpenTank-G flasher (scaffold)")
    ap.add_argument("cmd", choices=["detect", "backup", "write"])
    ap.add_argument("--app"); ap.add_argument("--out"); ap.add_argument("--go", action="store_true")
    a = ap.parse_args()
    if a.cmd == "detect": print(detect())
    elif a.cmd == "backup": backup(a.out or "backups")
    elif a.cmd == "write": write_app(a.app, a.out, dry_run=not a.go)
