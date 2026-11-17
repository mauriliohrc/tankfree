# OpenTank-G Installer

Cross-platform (Python 3) tooling to decrypt, build and flash Tank-G firmware.

## Works offline today (no pedal needed)

```bash
python3 fwtool.py selftest                         # validate crypto
python3 fwtool.py decrypt ../../UltraRig-alpha11-v2-win/m571_g_app.bin app.plain
python3 fwtool.py strings ../../UltraRig-alpha11-v2-win/m571_g_app.bin | less
python3 fwtool.py tones  ../../UltraRig-alpha11-v2-win/factory_usr_TANK-G.bin ../captures/factory
python3 fwtool.py encrypt app.plain app.enc        # re-encrypt for flashing
```

- `jltech/` — validated JieLi crypto (cipher + CRC). Chip key `0x980F`.
- `fwtool.py` — decrypt/encrypt/strings/extract-tones/selftest.

## Needs the pedal (scaffold — validate before trusting writes)

`flasher.py` implements the safe flash sequence but delegates the UBOOT/SCSI
transport to **jl-uboot-tool** (MIT). Set it up:

```bash
git clone https://github.com/kagaimiq/jl-uboot-tool vendor/jl-uboot-tool
# copy wl82loader.bin from UltraRig: _internal/vendor/jl-uboot-tool/wl82loader.bin
```

Then (dry-run by default):

```bash
python3 flasher.py detect
python3 flasher.py backup --out backups/
python3 flasher.py write --app app.enc            # add --go to actually write
```

**Safety:** only `[0x4000, 0x7B000)` is ever written. SPL, VM, BTIF, OTP and
`key_mac` (unique BT identity) are never touched. A full 1 MiB backup is taken
first. No signature check on the pedal, so unsigned firmware boots.

> Before the first real write we should confirm the chip key by reading this
> specific pedal's `chip_key.bin` from a flash dump (should be `0x980F`).
