# Contributing to TankFree

Thanks for helping! A few ground rules keep this project legally clean and safe.

## Clean-room rule (important)
- **Never** add proprietary code or data: no M-VAVE/UltraRig/JieLi binaries,
  captures, IRs, `.fwsc`, loaders, or flash dumps (see `DISCLAIMER.md`).
- You may study third-party firmware to understand **behavior, formats and
  parameter ranges**, but implement everything **independently** from open
  sources (open NAM core = MIT, public JieLi AC79 SDK = Apache-2.0, published
  DSP algorithms). Do not paste decompiled/disassembled code.
- If you contribute RE notes, describe the *interface/format*, not vendor source.

## Scope
- `installer/` — Python; keep it cross-platform, standard library first.
- `manager/`  — vanilla ES modules; Chrome/Edge Web Serial/MIDI/Bluetooth; no build step.
- `firmware/` — JieLi AC79 SDK app (pi32v2). See `firmware/README.md`.
- `docs/`     — keep `RE_FINDINGS.md`, `SPEC.md`, `MIDI_GP5.md` current.

## Dev
```bash
# crypto unit tests (no hardware, no proprietary data)
python3 -m pytest installer/tests -q      # or: python3 installer/tests/test_cipher.py
```

## PRs
- Small, focused commits. Describe what you tested.
- Anything that **writes to a pedal** must default to dry-run and be clearly marked.
- By contributing you agree your work is licensed under the project `LICENSE` (GPL-3.0).

## Safety
Test flashing only on your own device, with a full backup saved first. Document
any recovery steps you discover.
