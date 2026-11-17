# TankFree Installer

Put TankFree on your pedal — and get back out — with **one double-click, nothing
to install**. macOS and Windows.

## For players (the easy way)

1. **Download** the installer for your system from the project
   [Releases](../../releases) (or the latest [Actions](../../actions) build):
   - macOS → `TankFree-Installer.app`
   - Windows → `TankFree-Installer.exe`
2. Plug the pedal into a **USB port on the computer itself** (no hub), with a
   **data** cable, and switch it on.
3. **Open the installer** and click **Install TankFree**. It:
   - saves a **full backup** of your pedal first (your way back),
   - writes TankFree and verifies it,
   - never touches the bootloader.
4. Done. Open the editor (`manager/`) and build your tone.

Changed your mind? The same app has **Back to my backup** and **Restore factory
firmware** (from an official M-VAVE `.fwsc`). If a write is ever interrupted, the
bootloader is untouched and the pedal drops into recovery so you can reinstall.

> No pedal connected? The app runs a safe **demo** so you can see the whole flow —
> it writes nothing.

## For the curious — offline tools (no pedal)

`fwtool.py` works today, no device:
```bash
python3 fwtool.py selftest
python3 fwtool.py tones ../../UltraRig-alpha11-v2-win/factory_usr_TANK-G.bin ../captures/factory
python3 fwtool.py decrypt <enc_app.bin> app.plain   # encrypt = same cmd
```
- `jltech/` — validated JieLi crypto (chip key `0x980F`).
- `gui/tankfree_installer.py` — the GUI (tkinter, zero extra deps).
- `flasher.py` — the flashing backend (UBOOT loader via `jl-uboot-tool`).

## Flashing backend (needs the pedal — validate before trusting writes)

`flasher.py` only ever writes `[0x4000, 0x7B000)`; SPL, VM, BTIF, OTP and the
unique `key_mac` are never touched, and a full backup is taken first. It delegates
the UBOOT/SCSI transport to **jl-uboot-tool** (MIT). Set it up for real writes:
```bash
git clone https://github.com/kagaimiq/jl-uboot-tool vendor/jl-uboot-tool
# place wl82loader.bin (from the UltraRig package) next to it
```
Writes default to dry-run until confirmed on a device.

## Building the installer yourself

The **end user** installs nothing; only a **builder** needs Python + PyInstaller.
CI builds both binaries automatically — see `.github/workflows/release.yml`
(Actions → “Build installers”, or push a `v*` tag for a Release).

Locally:
```bash
bash installer/build/build.sh      # macOS/Linux → dist/TankFree-Installer.app
installer\build\build.bat          # Windows     → dist\TankFree-Installer.exe
```
`build.sh` uses `uv` to fetch a standalone CPython (with tkinter) so the build is
reproducible without touching your system Python.
