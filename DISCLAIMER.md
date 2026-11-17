# Disclaimer, Legal & Safety

Read this before using anything here.

## Not affiliated
TankFree is an independent, community project. It is **not** made, endorsed,
supported by, or affiliated with **M-VAVE**, **MOOER**, **Zhuhai Jieli
Technology**, **Valeton**, or any other company. All product and company names,
trademarks and logos (M-VAVE, Tank-G, Tank-B, Valeton, GP-5, JieLi, AC791N, NAM,
TONE3000, …) are the property of their respective owners and are used only for
identification and interoperability.

## No warranty — you accept the risk
This software is provided "as is", without warranty of any kind (see `LICENSE`).
Installing unofficial firmware on your pedal **can brick it** and **voids its
warranty**. You do all of this at your own risk. Only use it on hardware you own.

## No proprietary content redistributed here
This repository contains **only original, clean-room work** (our code and docs).
It deliberately does **not** include, and you must not add to it:
- M-VAVE's factory firmware (`.fwsc`), amp captures (AM2/AM3/AM4) or cabinet IRs;
- any part of the **UltraRig** package (PolyForm Noncommercial, source-closed) or
  code derived from disassembling it;
- JieLi's SDK binaries, the RAM loader (`wl82loader.bin`), or `chip_key.bin`;
- **flash dumps or backups of a pedal** — these hold your device's unique
  Bluetooth identity and keys; never commit or share them.

To use the tools you supply those files yourself, from your own device or their
official sources. `.gitignore` is configured to keep them out of the repo.

## Reverse engineering
The reverse-engineering notes in `docs/` describe interfaces and formats for the
purpose of **interoperability** and building independent, compatible software —
a use widely recognized as legitimate. We reimplement behavior from open sources
(the open NAM core, the public JieLi AC79 SDK, published DSP algorithms); we do
not copy third-party proprietary code into this project.

## Clean-room policy for contributors
Do not paste, translate, or transcribe decompiled/disassembled proprietary code
into this project. Study third-party firmware only to learn *behavior, formats
and parameter ranges*, then implement independently. See `CONTRIBUTING.md`.
