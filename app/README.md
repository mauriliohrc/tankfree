# TankFree desktop app

The web editor (`../manager/`) wrapped as a native **Electron** app for macOS and
Windows, so it opens like any app and talks to the pedal over **Web Serial** — no
browser needed. (We use Electron, not Tauri: the macOS system webview doesn't
support Web Serial; Electron bundles Chromium, which does.)

`../manager/` stays the single source of truth; `copy.js` mirrors it into
`renderer/` at build time.

## Run from source
```bash
cd app
npm install        # electron + electron-builder
npm start          # copies the editor and launches the app
```

## Build installers
```bash
npm run dist       # → dist/ : .dmg + .zip (macOS) / .exe (Windows) / AppImage (Linux)
```
CI builds all three automatically — see `.github/workflows/app.yml`
(Actions → “Build desktop app”, or a `v*` tag attaches them to the Release).

## How serial works here
`main.js` grants the `serial` permission and auto-selects the pedal when the page
calls `navigator.serial.requestPort()` — it prefers USB VID `0x3654`, then any
`usbmodem`/`ttyACM` port. Click **Conectar** in the app and it connects.
