# Flashing & building on a Mac

## ★ Breakthrough: Mac-native flashing is possible (JieLi HID OTA)
`jl-uboot-tool` is Linux/Windows only because it uses the loader's **SCSI
(mass-storage)** transport. But JieLi's loader **also speaks USB HID**, and
**M-VAVE's own `M-UPGRADE.app` flashes the Tank-G on macOS** through it — proven by
reverse-engineering its native `upgrade_core` (links `CoreMIDI` + `IOKit`):

- **`Recovery` / `JieliHidOta`** — USB **HID OTA**, 64-byte in/out reports, via IOKit.
  Symbols: `Recovery::{scan,open,install,compatible,inspect}`,
  `MacConnection::{synchronize,transact,query}`,
  `JieliHidOta::{isDevicePresent,upgradeFirmware,protocolSelfTest}`.
  `Recovery::install(firmware, chipkey_cb → array<u32,16>, progress)` = the write.
- **USB-MIDI SysEx** — identity handshake + `enter-upgrade` (for devices that
  expose MIDI, i.e. factory firmware).
- The pedal (on UltraRig) enumerates as VID **`0x3654`** / PID **`0x5155`**, "Jieli Technology".

**Implication:** a **Python `hidapi` flasher on macOS** can replicate `Recovery`
(64-byte HID reports wrapping the same loader commands jl-uboot-tool already
implements over SCSI) → **flash our firmware on the Mac, no Windows, no VM.**
The loader *commands* are known (jltech/uboot.py); we still need the **HID framing**
(from `upgrade_core` disassembly or by capturing M-UPGRADE's live traffic) and the
**entry method** (how the device reaches HID-OTA mode).

> Open question to pin down: how M-UPGRADE enters flash mode from each state
> (factory MIDI `enter-upgrade` → HID OTA; vs the mask-ROM UBOOT). From UltraRig
> (serial, no MIDI) we can reach mask-ROM UBOOT via its `UBOOT` serial command —
> check whether that mask-ROM also exposes the HID interface M-UPGRADE talks to.

## Fallback: a Linux VM (if the HID path needs more RE)
If the HID OTA RE stalls, run `jl-uboot-tool` + the toolchain in a free Linux VM
(UTM/Lima) with USB passthrough — both flashing and building, still Mac-only.

## One-time: a Linux VM on your Mac with USB passthrough
Use **UTM** (free, App Store / mac.getutm.app) or **Lima**. In the VM:
```bash
sudo apt update && sudo apt install -y python3-pip git cmake
git clone https://github.com/kagaimiq/jl-uboot-tool
pip3 install pyusb
```
Pass the pedal's USB device through to the Linux guest (UTM: USB menu → select the
device when it appears). On Apple Silicon, UTM can run an x86_64 or arm64 Linux
guest; either works for jl-uboot-tool (pure Python + SG_IO).

## Enter loader mode (from the Mac or the VM — no software needed)
Power-cycle the pedal **3× quickly** → the 3-boot guard drops it into the chip
loader; it enumerates as the **`WL82 UBOOT1.00`** mass-storage device. (Or, from
the Mac over serial, `tankctl`/the editor can send the firmware's `UBOOT` command.)
To leave loader mode: power off 10 s, power on. (If stuck: USB-C charger + power on.)

## Step 0 — factory backup (MANDATORY, read-only, zero risk)
In the Linux VM, with the `WL82 UBOOT` device passed through:
```bash
python3 installer/dump.py            # finds the device, reads 1 MiB twice, compares,
                                     # saves backups/<date>/dump.bin + sha256,
                                     # decodes chip_key.bin (confirm 0x980F)
```
This is the only safe way back. Keep it private (it holds your pedal's BT identity).

## Build the firmware (in the same VM)
Follow [`../firmware/BUILD.md`](../firmware/BUILD.md): install the pi32v2 toolchain
(JieLi's Linux build, or compile `pi32v2-llvm`) + the pinned AC79 SDK, then build
the `usb-diag` milestone, SFC-encrypt with `fwtool.py encrypt`, and flash it:
```bash
python3 jl-uboot-tool/jluboottool.py   # then: write 0x4000 app.enc  (app area only)
```
Only ever write `[0x4000, 0x7B000)`. Verify, leave UBOOT, confirm it boots.

## Why not just patch jl-uboot-tool for macOS?
It needs raw SCSI pass-through to a USB mass-storage device. macOS has no `SG_IO`;
it requires IOKit `SCSITaskUserClient` with the device detached from the default
mass-storage driver — a large, fragile, device-specific effort we'd rather not
gate the project on. A Linux VM with passthrough is the reliable, Mac-only answer.
(If someone lands a tested macOS SCSI port of jl-uboot-tool, we drop the VM.)

## What stays native on the Mac (no VM)
- The editor / desktop app (`app/`, `manager/`) — Web Serial works on macOS.
- `tankctl.py`, `probe.py` — control/read a running pedal over serial.
- Host-side firmware tests (`firmware/test/host_test.c`), DSP work, docs.
