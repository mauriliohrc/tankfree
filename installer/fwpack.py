#!/usr/bin/env python3
"""fwpack.py — build a flashable Tank-G firmware image natively (no JieLi .exe tools).

The device's OTA pulls the full SFC-encrypted app-area image (the `m571_g_app.bin`
format): an `app_area_head` directory of 32-byte entries followed by each file's data,
all SFC-scrambled with the chip key 0x980F. JieLi builds this with Windows-only
isd_download/fw_add/ufw_maker; we reproduce it by **transplant** — take a known-good
factory image, swap the `app.bin` payload for ours, fix the directory CRCs/size, and
re-encrypt. The container format was reverse-engineered and is validated byte-for-byte
(see selftest): rebuilding the factory image from its own parts reproduces it exactly.

Directory entry (32 bytes, little-endian):
  0x00 u16  crc16(entry[2:32])     # CRC of this entry's own metadata+name
  0x02 u16  crc16(file data)       # CRC of the file's bytes
  0x04 u32  addr                   # run/entry addr (app.bin=0x120) or flash offset
  0x08 u32  size                   # file size in bytes
  0x0c u32  flags
  0x10 b16  name                   # NUL/0xFF-padded ASCII
The `app_area_head` self-entry (slot 0): its 0x02 field = crc16(area[0x20 : head.size]),
i.e. a CRC over every entry after it plus all file data. jl_crc16 is the JieLi CRC16.

  python3 fwpack.py selftest                         # prove the format (round-trip)
  python3 fwpack.py transplant <factory.bin> <app.bin> <out.bin>
"""
import sys, struct
import jltech
from jltech import jl_crc16

# app-area layout (constant for the Tank-G factory image; verified)
APP_OFF  = 0x120       # app.bin data starts here (right after the 0x120-byte directory)
CFG_OFF  = 0x76aac     # cfg_tool.bin data (fixed flash offset; we never move it)
AREA_END = 0x76c2b     # == app_area_head.size: end of the data area the head CRCs cover
AAH      = 0x00        # app_area_head self-entry slot
APP_ENT  = 0x20        # app.bin directory entry slot

def _u16(b,o,v): struct.pack_into("<H",b,o,v&0xffff)
def _u32(b,o,v): struct.pack_into("<I",b,o,v&0xffffffff)

def transplant(factory_enc: bytes, app_bytes: bytes) -> bytes:
    """Return a flashable (SFC-encrypted) image with app.bin replaced by app_bytes."""
    dec = bytearray(jltech.decrypt_app(factory_enc))
    slot = CFG_OFF - APP_OFF
    if len(app_bytes) > slot:
        raise ValueError(f"app.bin too big: {len(app_bytes)} > slot {slot}")
    # 1. write our app into the app.bin slot; zero the remainder up to cfg_tool.bin
    dec[APP_OFF:APP_OFF+len(app_bytes)] = app_bytes
    for i in range(APP_OFF+len(app_bytes), CFG_OFF):
        dec[i] = 0
    # 2. fix the app.bin entry: size, data-crc, then the entry self-crc (covers both)
    _u32(dec, APP_ENT+0x08, len(app_bytes))
    _u16(dec, APP_ENT+0x02, jl_crc16(bytes(app_bytes)))
    _u16(dec, APP_ENT+0x00, jl_crc16(bytes(dec[APP_ENT+2:APP_ENT+0x20])))
    # 3. fix the app_area_head: area-crc over [0x20:AREA_END], then its own self-crc
    _u16(dec, AAH+0x02, jl_crc16(bytes(dec[0x20:AREA_END])))
    _u16(dec, AAH+0x00, jl_crc16(bytes(dec[AAH+2:AAH+0x20])))
    return jltech.encrypt_app(bytes(dec))

def selftest(factory):
    enc = open(factory,"rb").read()
    dec = bytearray(jltech.decrypt_app(enc))
    assert jltech.encrypt_app(bytes(dec)) == enc, "re-encrypt not identity"
    osize = struct.unpack_from("<I", dec, APP_ENT+8)[0]
    orig_app = bytes(dec[APP_OFF:APP_OFF+osize])
    rebuilt = transplant(enc, orig_app)
    ok = rebuilt == enc
    print(f"[selftest] re-encrypt identity: OK")
    print(f"[selftest] transplant(original app.bin) == factory file byte-for-byte: {ok}")
    print("SELFTEST:", "PASS" if ok else "FAIL")
    return 0 if ok else 1

def main(a):
    if not a: print(__doc__); return 1
    if a[0]=="selftest":
        fac = a[1] if len(a)>1 else "/Users/bad/firmwares/UltraRig-alpha11-v2-win/m571_g_app.bin"
        return selftest(fac)
    if a[0]=="transplant":
        if len(a)<4: print("usage: fwpack.py transplant <factory.bin> <app.bin> <out.bin>"); return 1
        fac, app, out = a[1], a[2], a[3]
        img = transplant(open(fac,"rb").read(), open(app,"rb").read())
        open(out,"wb").write(img)
        print(f"wrote {out}  ({len(img)} bytes)  — SFC-encrypted, CRCs fixed, ready to flash")
        return 0
    print(__doc__); return 1

if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
