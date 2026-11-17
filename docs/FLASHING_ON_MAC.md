# Flashing & building on a Mac (no Windows)

Honest constraint: the JieLi loader tooling (`jl-uboot-tool`) and the pi32v2
toolchain run on **Linux/Windows, not macOS**. And the cross-platform **USB-MIDI
OTA** flash path needs the pedal to expose MIDI — which **UltraRig doesn't** (it's
serial-only). So on a Mac we run the Linux parts in a **free Linux VM with USB
passthrough**. Everything else (editor, `tankctl`, host builds) stays native.

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
