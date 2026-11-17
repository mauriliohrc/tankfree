# TankFree Manager (web editor)

Browser editor that talks to the pedal over **Web Serial** (USB) with
**Web MIDI** (GP-5 map) and **Web Bluetooth** (BLE) planned. Chrome/Edge only.

## Run

Serve the folder over http (Web Serial needs a secure/localhost context):

```bash
cd manager && python3 -m http.server 8000
# open http://localhost:8000 in Chrome/Edge, click "Conectar (USB)"
```

## Files
- `index.html` — editor shell (connect, presets, free chain, amp/cab, log, rate 48/96).
- `js/protocol.js` — `TankLink`: Web Serial line-protocol client (115200, VID 0x3654).
  Implements the reverse-engineered command set (PLIST/PLOAD/FXS/AMPEQ/CLIST…).
- `js/midi.js` — `Gp5Midi`: Valeton GP-5 CC map → protocol actions (see docs/MIDI_GP5.md).
- `css/app.css` — styling.

## Status
Scaffold: connects, lists presets, shows the chain, enables GP-5 MIDI. The
deep editors (per-block params, NAM capture upload via `A2L1`, IR upload, preset
save/scenes) are stubs to fill against the firmware as it comes up. The NAM→chip
converter can be ported from the decoded `A2L1` format (RE_FINDINGS §7).

## TODO
- Port NAM `.nam` → `A2L1` converter (clean-room from decoded format).
- IR `.wav` upload (decode + partitioned-conv metadata).
- Per-block parameter UI driven by a block/param schema.
- BLE-MIDI + BLE control transport.
- Preset export/import (`.opentank.json`).
