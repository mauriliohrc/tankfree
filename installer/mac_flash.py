#!/usr/bin/env python3
"""mac_flash.py — native macOS flasher for the M-VAVE Tank-G (JieLi AC791N/WL82).

CAPTURED GROUND TRUTH (2026-11-17, see docs/PROTOCOL_FLASH.md): the Tank-G OTA flash
is 100% **USB-MIDI SysEx, device-pull**. The device bootloader drives it; the host is
a dumb file server. No HID, no SCSI, no host-side crypto, no loader upload. M-VAVE's
`upgrade_core` helper IS the proven macOS MIDI file-server, so TankFree flashes natively
by driving it with OUR firmware image.

Flow (each step is one `upgrade_core` call):
  1. ks-scan                                   → device<TAB>in<TAB>out<TAB>identity<TAB>hash
  2. ks-midi <hash> <in> <out> <fw> prepare <identity>      → validate + reboot to OTA
  3. ks-scan                                   → identity now "ota-<identity>", new ports
  4. ks-midi <hash'> <in'> <out'> <fw> update ota-<identity> → pull whole image + flash

Usage:
  python3 mac_flash.py scan                    # identify the connected pedal
  python3 mac_flash.py flash app.bin           # DRY-RUN: show exactly what would run
  python3 mac_flash.py flash app.bin --go      # actually flash (destructive; backup first!)
  python3 mac_flash.py flash app.bin --go --helper /path/to/upgrade_core.real

The helper is M-VAVE's (not redistributed here); point --helper at your M-UPGRADE.app's
`Contents/MacOS/upgrade_core.real`. We auto-discover it in the usual spots.
"""
import sys, os, subprocess, glob

# ── locate M-VAVE's upgrade_core.real (the proven MIDI file-server) ──
HELPER_HINTS = [
    os.path.expanduser("~/MU-trace.app/Contents/MacOS/upgrade_core.real"),
    "/Applications/M-UPGRADE.app/Contents/MacOS/upgrade_core.real",
    os.path.expanduser("~/Applications/M-UPGRADE.app/Contents/MacOS/upgrade_core.real"),
]
def find_helper(override=None):
    # check the known spots first (fast) — only fall back to a shallow glob if needed
    for c in ([override] if override else []) + HELPER_HINTS:
        if c and os.path.isfile(c) and os.access(c, os.X_OK):
            return c
    for pat in ("~/*.app/Contents/MacOS/upgrade_core*",
                "~/Applications/*/Contents/MacOS/upgrade_core*"):
        for c in glob.glob(os.path.expanduser(pat)):   # NOT recursive — never walk all of $HOME
            if os.path.isfile(c) and os.access(c, os.X_OK):
                return c
    return None

# ── helper calls ──
def ks_scan(helper):
    """Return (in_port, out_port, identity, devhash) for the connected pedal, or None."""
    out = subprocess.run([helper, "ks-scan"], capture_output=True, text=True, timeout=30).stdout
    for line in out.splitlines():
        f = line.split("\t")
        if f and f[0] == "device" and len(f) >= 5:
            return f[1], f[2], f[3], f[4]      # in, out, identity, hash
    return None

def ks_midi(helper, dev, fw, action, target, trace=True):
    """Run one prepare/update. Streams the device-pull progress to stderr live."""
    in_p, out_p, _ident, h = dev
    argv = [helper, "ks-midi", h, in_p, out_p, fw, action, target]
    env = dict(os.environ, M_UPGRADE_TRACE="1") if trace else os.environ
    print(f"  $ upgrade_core ks-midi {h[:12]}… {in_p} {out_p} {os.path.basename(fw)} {action} {target}")
    p = subprocess.Popen(argv, env=env, stderr=subprocess.PIPE, text=True)
    last = ""
    for line in p.stderr:                       # live progress from the trace
        if "firmware.request" in line:
            a = line.split("address=")[-1].split()[0]
            last = f"\r    flashing… block @ {a}      "
            sys.stdout.write(last); sys.stdout.flush()
        elif "upgrade.enter" in line:
            print("    device entered OTA, pulling image…")
        elif "process.exit" in line:
            print(f"\r    done ({line.strip().split('code=')[-1]})            ")
    return p.wait()

# ── commands ──
def cmd_scan(helper):
    dev = ks_scan(helper)
    if not dev:
        print("No Tank-G found. Connect the pedal via USB and retry."); return 1
    in_p, out_p, ident, h = dev
    mode = "OTA/recovery mode" if ident.startswith("ota-") else "app mode"
    print(f"Tank-G found: identity={ident}  ({mode})")
    print(f"  in-port={in_p}  out-port={out_p}  hash={h[:16]}…")
    return 0

def cmd_flash(helper, fw, go):
    if not os.path.isfile(fw):
        print(f"firmware image not found: {fw}"); return 1
    size = os.path.getsize(fw)
    dev = ks_scan(helper)
    if not dev:
        print("No Tank-G found. Connect the pedal and retry."); return 1
    in_p, out_p, ident, h = dev
    base = ident[4:] if ident.startswith("ota-") else ident      # strip ota- prefix
    print(f"Pedal: {ident}   image: {fw} ({size} bytes)")
    print(f"Plan: prepare (validate+enter OTA) → update (flash) against target '{base}'.")
    if not go:
        print("\nDRY-RUN (no changes). Re-run with --go to flash. Steps that WOULD run:")
        print(f"  1. ks-midi … prepare {base}")
        print(f"  2. ks-scan  (expect identity → ota-{base})")
        print(f"  3. ks-midi … update ota-{base}")
        print("\n⚠  Back up the factory image first; a bad image can require recovery.")
        return 0

    # 1. prepare — device validates our image, then reboots into OTA mode
    if not ident.startswith("ota-"):
        print("\n[1/3] prepare — validating image, entering OTA mode…")
        rc = ks_midi(helper, dev, fw, "prepare", base)
        if rc != 0:
            print(f"prepare failed (rc={rc}). Image likely rejected by the bootloader "
                  f"(version/header/CRC/SFC-encryption mismatch). Not flashed."); return rc
        # 2. re-scan — the device re-enumerated in OTA mode with new ports
        print("[2/3] re-scanning for the device in OTA mode…")
        dev = None
        for _ in range(20):                     # it takes a couple seconds to re-enumerate
            dev = ks_scan(helper)
            if dev and dev[2].startswith("ota-"):
                break
            dev = None
        if not dev:
            print("device did not re-appear in OTA mode. Power-cycle and use M-UPGRADE "
                  "to recover if needed."); return 1
    else:
        print("\n[1-2/3] device already in OTA mode; skipping prepare.")
    # 3. update — device pulls the whole image and flashes it
    print(f"[3/3] update — flashing {size} bytes (device-pull over MIDI)…")
    rc = ks_midi(helper, dev, fw, "update", dev[2])
    if rc == 0:
        print("✓ flashed. The pedal will reboot into the new firmware.")
    else:
        print(f"✗ update failed (rc={rc}). The device is still in OTA mode — re-run, "
              f"or recover with M-UPGRADE.")
    return rc

def main(a):
    # pull --helper / --go flags out
    helper_override = None
    if "--helper" in a:
        i = a.index("--helper"); helper_override = a[i+1]; del a[i:i+2]
    go = "--go" in a
    a = [x for x in a if x != "--go"]
    if not a:
        print(__doc__); return 1
    helper = find_helper(helper_override)
    if not helper:
        print("Could not find M-VAVE's upgrade_core.real. Install M-UPGRADE.app or pass "
              "--helper /path/to/upgrade_core.real.\n"
              "(It is M-VAVE's helper — the proven macOS MIDI flash backend — not bundled here.)")
        return 1
    if a[0] == "scan":  return cmd_scan(helper)
    if a[0] == "flash":
        if len(a) < 2: print("usage: mac_flash.py flash <app.bin> [--go]"); return 1
        return cmd_flash(helper, a[1], go)
    print(__doc__); return 1

if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
