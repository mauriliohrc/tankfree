#!/usr/bin/env python3
"""dump.py — read-only full-flash backup of the Tank-G via jl-uboot-tool.

Runs on **Linux or Windows** (jl-uboot-tool uses SG_IO / SCSI pass-through; no
macOS — on a Mac run this inside a Linux VM with USB passthrough, see
docs/FLASHING_ON_MAC.md). Reads the whole 1 MiB flash TWICE and compares, so a
bad read can't become a bad backup. Writes nothing to the pedal.

Prereq: the pedal in loader mode (power-cycle 3× → `WL82 UBOOT`), and jl-uboot-tool:
    git clone https://github.com/kagaimiq/jl-uboot-tool   # next to this file, or set --tool

    python3 dump.py                      # -> backups/<date>/dump.bin + .sha256
"""
import os, sys, subprocess, hashlib, datetime, argparse, re

HERE = os.path.dirname(os.path.abspath(__file__))
FLASH_SIZE = 0x100000  # 1 MiB


def find_tool(explicit):
    for c in [explicit, os.path.join(HERE, "jl-uboot-tool"),
              os.path.join(HERE, "vendor", "jl-uboot-tool"),
              os.path.join(HERE, "..", "jl-uboot-tool")]:
        if c and os.path.isfile(os.path.join(c, "jluboottool.py")):
            return os.path.abspath(c)
    sys.exit("jl-uboot-tool not found. git clone https://github.com/kagaimiq/jl-uboot-tool "
             "next to this script, or pass --tool <dir>.")


def read_flash(tool, out_path):
    """Drive jl-uboot-tool to dump the whole flash (it brings up the RAM loader on connect)."""
    script = f"read 0x0 0x{FLASH_SIZE:X} {out_path}\nexit\n"
    r = subprocess.run([sys.executable, os.path.join(tool, "jluboottool.py")],
                       input=script, text=True, cwd=tool)
    if r.returncode != 0 or not os.path.isfile(out_path) or os.path.getsize(out_path) != FLASH_SIZE:
        sys.exit("read failed. Is the pedal in WL82 UBOOT mode and passed through to this OS?")


def sha256(p):
    h = hashlib.sha256()
    with open(p, "rb") as f:
        for b in iter(lambda: f.read(65536), b""):
            h.update(b)
    return h.hexdigest()


def find_chipkey(dump):
    """Best-effort: locate the chip-series marker and report. The authoritative
    key (0x980F for Tank-G) is confirmed elsewhere; this just sanity-checks."""
    i = dump.find(b"AC791N")
    return f"chip marker 'AC791N' at 0x{i:X}" if i >= 0 else "chip marker not found in plaintext (region may be encrypted)"


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--tool"); ap.add_argument("--out", default=os.path.join(HERE, "backups"))
    a = ap.parse_args()
    if sys.platform == "darwin":
        sys.exit("macOS can't talk to the JieLi loader. Run this in a Linux VM with USB "
                 "passthrough (docs/FLASHING_ON_MAC.md).")
    tool = find_tool(a.tool)
    day = datetime.date.today().isoformat()
    outdir = os.path.join(a.out, "backup_" + day)
    os.makedirs(outdir, exist_ok=True)
    p1 = os.path.join(outdir, "dump.bin")
    p2 = os.path.join(outdir, "dump_verify.bin")

    print("[1/2] reading flash…");  read_flash(tool, p1)
    print("[2/2] reading again to verify…"); read_flash(tool, p2)
    d1, d2 = open(p1, "rb").read(), open(p2, "rb").read()
    if d1 != d2:
        sys.exit("the two reads differ — unreliable. Replug/retry; do NOT trust this backup.")
    os.remove(p2)
    digest = sha256(p1)
    with open(p1 + ".sha256", "w") as f:
        f.write(digest + "  dump.bin\n")
    print(f"\n✓ backup OK: {p1}")
    print(f"  size {len(d1)} B   sha256 {digest}")
    print(f"  {find_chipkey(d1)}")
    print("Keep this backup private and safe — it's your only way back.")


if __name__ == "__main__":
    main()
