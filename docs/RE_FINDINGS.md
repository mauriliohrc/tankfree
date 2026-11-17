# Tank-G v2 — Reverse Engineering Findings

Consolidated from analysis of the UltraRig alpha-11 package, the factory
`.fwsc`, and web research. Everything here is established fact unless marked.

## 1. Hardware

| Item | Value |
|---|---|
| SoC | **JieLi (Zhuhai Jieli) AC791N**, ROM/package family **WL82** |
| CPU | **pi32v2** — proprietary, Blackfin-derived, 32-bit, little-endian, dual-core + FPU. ELF machine id `0xF0`. **Not ARM, not MIPS.** |
| Clock | 396 MHz (UltraRig), 360 MHz (factory). Datasheet nominal is ≤320 MHz. |
| Flash | **1 MiB SPI NOR** (JEDEC `0x856014` Puya P25Q80H, or `0xEB6014`) |
| RAM | on-die SRAM (family ~578 KB; exact fitted size unconfirmed) |
| Accelerator | on-chip **FFT/math accelerator** (offloads IR, pitch-shifter, NAM) |
| Radio | Bluetooth (BLE). Wi-Fi present in family, unused here. |
| Chip key | **`0x980F`** (per-model loader encryption key) |
| Audio | 24-bit; factory 44.1 kHz, UltraRig 48 kHz. 1/4"+XLR mono (L), 3.5 mm stereo. |
| I/O | 1/4" in; 1/4"+XLR out; 3.5 mm phones; USB-C (data+charge); 4 footswitches; LED ring + tuner; battery |

Same silicon as the M-VAVE **FM-1** and **SMK-37 Pro** — both have public RE
(aroum/fm1-custom-fw, amalahama/smk37-firmware-custom-mod), directly transferable.

## 2. Firmware encryption (validated — we decrypted the real app)

Three JieLi ciphers (from the installer's `jltech.cipher`):

- **`jl_enc_cipher`** — LFSR stream, CRC16-CCITT poly `0x1021`:
  ```
  byte ^= key & 0xFF
  key   = ((key << 1) ^ (0x1021 if key & 0x8000 else 0)) & 0xFFFF
  ```
  Key evolves continuously; the caller resets it per block.
- **`jl_crc_cipher`** ("CrcDecode") — XORs each byte with a CRC16 advanced over
  the gb2312 magic string `孟黎我爱你，玉林`, seeded from a 32-bit key.
- **`jl_rxgp_cipher`** ("RxGp") — minstd LCG (mult 16807, seed 1883732050).

**App/code (SFC) encryption** — how `m571_g_app.bin` and the flash app area are
scrambled, and how we decrypt them:

- Block size **32 bytes**.
- For block starting at **offset `i`** (relative to app-region start, base 0):
  `key = 0x980F XOR ((i >> 2) & 0xFFFF)`, then run `jl_enc_cipher` over the block.
- **Header** (first 32 bytes of a flash image / `.fwsc`) uses fixed key **`0xFFFF`**.

Verified: decrypting `m571_g_app.bin` this way yields plaintext starting
`4a 50 71 fa` + `"app_area_head"` / `"app.bin"`, with readable strings
`tankg_fw M5.71 rel ble Nov 12 2026 16:04:11`, the full command protocol, etc.

CRCs: `jl_crc16` = CCITT (poly `0x1021`, init 0, non-reflected);
`jl_crc32` = poly `0x04C11DB7`, reflected.

## 3. Flash map (from installer `tankg_install.py`)

| Region | Range | Policy |
|---|---|---|
| SPL / 2nd-stage bootloader | `[0x00000, 0x04000)` | **never written** |
| **App area (Tank-G)** | **`[0x04000, 0x7B000)`** | app lives here (`APP_AREA=(16384,503808)`) |
| App area (Tank-B) | `[0x04000, 0x7F000)` | bigger (`TANKB_APP_AREA`) |
| Config VM / presets | from `0x7B000`, 2 banks | log-structured, wear-leveled |
| key_mac (unique BT MAC/key) | `0xFF000` | **never written** |
| BTIF / OTP | — | **never written** |

App keying uses offset relative to `0x4000` → base 0 in the cipher.

## 4. Factory tones — ALREADY EXTRACTED (plaintext)

`factory_usr_TANK-G.bin` (256 KB, in the UltraRig package) is **not encrypted**.
Contents:

| Offset | Format | ID |
|---|---|---|
| `0x00000` | AM4 | `105713` (default) |
| `0x04000` / `0x08000` / `0x10000` | AM3 | — |
| `0x0C000` | AM4 | `4RAT_OD` (RAT-style OD) |
| `0x14000` | AM4 | `6VAVE_DS` (distortion) |
| `0x18000` | AM4 | `7MS_GV2` (Marshall-style) |
| `0x1C000` | AM4 | `8J900_HV` (JCM900 hi-gain) |
| `0x20000` | AM4 | `test` |
| `0x24000`…`0x32000` | CAB | 8 cabinet IRs |

AM4/AM3 = M-VAVE's own capture format (float32 neural weights). These are the
"original Tank-G tones." (UltraRig's own engine uses a different format, `A2L1`.)

## 5. Flashing protocol

- Mask-ROM loader = USB mass-storage disk **"WL82 UBOOT1.00"**, VID `0x4C4A` /
  PID `0x8057`, driven via **SCSI pass-through** (not real disk I/O).
- Tool: **`jl-uboot-tool`** (kagaimiq, MIT) uploads a RAM stage **`wl82loader.bin`**,
  then exposes `read`/`write`/`erase`/`dump` over flash.
- Entry: a "soft key" MIDI SysEx (`F0 22 24 35 7D F7`) from running firmware, or
  the 3-failed-boot guard / watchdog drops to UBOOT automatically.
- **No signature check** → unsigned custom firmware boots.
- Installer sequence: soft-key → UBOOT → read chip key + flash id → **full 1 MiB
  backup** → write app area `[0x4000,0x7B000)` sector-by-sector (erase/write/verify)
  → set boot guard / confirm fw_id → leave UBOOT.

## 6. Runtime protocol (UltraRig reference)

- Transport: **USB Web-Serial @ 115200** (USB VID `0x3654`) and **BLE Nordic-UART**
  (service `6e400001-b5a3-f393-e0a9-e50e24dcca9e`). ASCII, line-based.
- Command vocabulary (master list from the binary):
  `PING EVT LVL LVLSUB STAT MEM MODEL AUDIO FXT FXS FXA FXR FXM FXE FXP FXL FXOS
  FXMIX FXXO FXIR AMP AMPX AMPEQ GEQ GAIN LIM HPF SPUR GLIDE SPILL TUNER TUNE TAP
  BPM IRBEGIN IRB IREND LDBEGIN LDB LDEND PLIST PSTAT PLOAD PSAVE PNAME PNEW PDEL
  PCOLOR SNAP SNAPS SNAPSAVE SNAPDEL SNAPNAME CLIST ILIST CLOAD ILOAD CSAVE ISAVE
  CDEL IDEL CTRIM CALIB GSAVE CREAD IREAD FSA FSMODE FSDO FSPRESS FSLED LEDCFG
  LEDTEST STW BLE CRASH CLKSTATE LIFE AUTH`
- Examples: `AUDIO OFF|THRU|A2L`; `AMPEQ volume=.. bass=.. mid=.. treble=.. presence=.. drive=..`;
  `AUTH` = challenge/nonce ("UltraRig auth" tag). BLE rejects privileged cmds (`ERR <cmd> usb only`).

## 7. Audio engine formats

- **Amp = NAM A2-Lite WaveNet**, quantized to **`A2L1`** (fully decoded in
  `web/nam2chip.js`): 3 channels, 48 kHz, int16 weights, int32 activations,
  int64 biases, per-tensor/stage Q fixed-point, LeakyReLU, dilated convs +
  residual, little-endian, trailing CRC32. Firmware parser rules in `a2l_block.c`
  (shift limits ±62, conv shift 0..31). NAM core is MIT (clean-room OK).
- **Cab = IR convolution** (partitioned/FFT, offloaded to the accelerator).
- Effects (bespoke DSP in UltraRig — reimplement from open algorithms):
  gate (`sgate`), compressor, boost/overdrive/distortion/`fuzzface`, EQ, hum
  filter, chorus/flanger/phaser/vibrato/tremolo, `pitch`/`pitchpro`/`doubler`,
  delay (tempo sync), reverbs (`reverb`/`reverb2`/`plate`/`spring`/`shimmer`/`spectral`),
  IR cab. Capacity: 32 presets × 4 scenes, 15 captures, 8 IRs, 16 blocks/preset.

## 8. The `.fwsc` factory container — OPEN ITEM

`TANK-Gv2.fwsc` is M-VAVE's factory flash image (SPL + app + VM), encrypted.
First 32 bytes decode with key `0xFFFF` (reveals `AC791N`). The app area did
**not** fall out with the simple SFC decode — the installer reads the app
location from a **JLFS (JieLi File System) directory** and there may be an extra
`jl_crc_cipher`/interleave layer. Next step: read `tankg_install.py`'s fwsc/JLFS
parser (already decompiled under `scratchpad/installer_extract/pyz/`) to extract
the factory app precisely. Not required to build our own firmware — the factory
tones (§4) are already open.

## 9b. Live device readout (confirmed on real hardware, 2026-11-17)

Queried a real Tank-G v2 running UltraRig M5.71 over USB serial (`installer/probe.py`,
read-only). Confirms our RE end-to-end:

- **Firmware:** `PONG tankg_fw M5.71 rel ble Nov 12 2026 16:04:11` — matches the
  decrypted build byte-for-byte.
- **NAM engine (STAT/DSP):** `L=23 C=3 rf=6347` — **23 WaveNet layers, 3 channels,
  receptive field 6347 samples (~132 ms @48k)**. Exactly the A2-Lite `A2L1` format (§7).
  Also: `c1_ok=1` (accelerator path up), per-block timing avg ≈ 125k ticks, block ≈ 89.8k.
- **Memory budget (MEM), real figures to design to:**
  - heap ≈ 287 KB, largest free chunk ≈ 9.7 KB, free ≈ 98 KB
  - **effects arena ≈ 118 KB** (`arena=118688`), FX pool ≈ 98 KB (`pool=98304`),
    `fxcap` ≈ 108 KB, `ram0_code_data` ≈ 129 KB, `ram0_bss` ≈ 95 KB
  - → our firmware's simultaneous-effects memory is bounded by a ~**118 KB arena**,
    not the 512 KB host-test placeholder.
- **Storage:** `CLIST slots=15`, `ILIST slots=8` — 15 captures, 8 IRs confirmed.
- **Audio levels:** `pk_in=-45.2 pk_out=-71.8 dBFS`, no clipping.
- **BLE:** advertising as `UltraRig <hex>`, one unique MAC per unit (kept off-repo).
- Protocol commands `PING INFO MEM BLE STAT PLIST CLIST ILIST` all answer as mapped (§6).

## 9. Toolchain & SDK (for building firmware)

- SDK: `gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK` (Apache-2.0, our chip family) +
  `Jieli-Tech/fw-Bootloader` (target `wl82`). UltraRig modifies only 4 SDK files
  (`setup.c`, `debug.c`, `init.c`, `usb_config.c`) and links the rest.
- Compiler: JieLi **clang 4.0.1 for pi32v2** (`/opt/jieli`), or open
  `jvitkauskas/pi32v2-isa` + `pi32v2-llvm`.
- Ghidra: `kagaimiq/ghidra-jieli` (pi32v2 SLEIGH) for disassembly.
