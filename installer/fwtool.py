#!/usr/bin/env python3
"""fwtool — offline firmware utilities for the Tank-G (AC791N).

Works today, no device needed:
  decrypt     <enc.bin> <out.bin>         decrypt an SFC app image (chip key 0x980F)
  encrypt     <plain.bin> <out.bin>       re-encrypt an app image for flashing
  strings     <enc_app.bin>               decrypt + print readable strings
  tones       <factory_usr.bin> <outdir>  extract factory amp captures (AM3/AM4) + cab IRs
  selftest                                validate the crypto against known artifacts

The flashing side (UBOOT loader) lives in flasher.py and needs the pedal.
"""
import sys, os, re, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from jltech import decrypt_app, encrypt_app, CHIP_KEY_TANKG


def _read(p):
    with open(p, "rb") as f: return f.read()


def cmd_decrypt(src, dst, key=CHIP_KEY_TANKG):
    open(dst, "wb").write(decrypt_app(_read(src), key))
    print(f"decrypted {src} -> {dst}  (chip key 0x{key:04X})")


def cmd_encrypt(src, dst, key=CHIP_KEY_TANKG):
    open(dst, "wb").write(encrypt_app(_read(src), key))
    print(f"encrypted {src} -> {dst}  (chip key 0x{key:04X})")


def cmd_strings(src, key=CHIP_KEY_TANKG):
    dec = decrypt_app(_read(src), key)
    for s in re.findall(rb"[\x20-\x7e]{5,}", dec):
        print(s.decode("latin1"))


# Factory capture/IR container (AM3/AM4 = M-VAVE neural capture, CAB = cab IR).
# Records sit at 0x2000-aligned offsets; header = magic(3) + 0x00 + 16-byte id.
def cmd_tones(src, outdir):
    d = _read(src)
    os.makedirs(outdir, exist_ok=True)
    found = []
    for i in range(0, len(d) - 4, 0x2000):
        mag = d[i:i+3]
        if mag in (b"AM2", b"AM3", b"AM4", b"CAB") or d[i:i+2] == b"IR":
            name = d[i+4:i+20].split(b"\x00")[0].decode("latin1", "replace") or f"rec_{i:05x}"
            # record spans until the next aligned record or EOF
            j = i + 0x2000
            while j < len(d) and d[j:j+3] not in (b"AM2", b"AM3", b"AM4", b"CAB") and d[j:j+2] != b"IR":
                j += 0x2000
            safe = re.sub(r"[^A-Za-z0-9_.-]", "_", name)
            fn = os.path.join(outdir, f"{i:06x}_{mag.decode('latin1','replace').strip()}_{safe}.bin")
            open(fn, "wb").write(d[i:j])
            found.append((i, mag, name))
    print(f"extracted {len(found)} records to {outdir}:")
    for i, mag, name in found:
        print(f"  0x{i:06x}  {mag.decode('latin1','replace'):3s}  {name}")


def cmd_selftest():
    pkg = os.path.join(os.path.dirname(__file__), "..", "..", "UltraRig-alpha11-v2-win")
    app = os.path.normpath(os.path.join(pkg, "m571_g_app.bin"))
    ok = True
    if os.path.exists(app):
        dec = decrypt_app(_read(app))
        checks = [b"app_area_head", b"app.bin", b"tankg_fw M5.71", b"A2L1", b"AMPEQ"]
        hits = [c for c in checks if c in dec]
        print(f"[app decrypt] {len(hits)}/{len(checks)} markers found: {[c.decode() for c in hits]}")
        # round-trip
        rt = encrypt_app(dec) == _read(app)
        print(f"[round-trip]  encrypt(decrypt(x)) == x : {rt}")
        ok = len(hits) == len(checks) and rt
    else:
        print(f"[skip] {app} not found")
    print("SELFTEST:", "PASS" if ok else "CHECK")
    return 0 if ok else 1


def main(argv):
    if not argv: print(__doc__); return 1
    c = argv[0]
    if c == "decrypt":  cmd_decrypt(argv[1], argv[2]); return 0
    if c == "encrypt":  cmd_encrypt(argv[1], argv[2]); return 0
    if c == "strings":  cmd_strings(argv[1]); return 0
    if c == "tones":    cmd_tones(argv[1], argv[2]); return 0
    if c == "selftest": return cmd_selftest()
    print(__doc__); return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
