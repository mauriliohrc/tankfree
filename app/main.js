// TankFree desktop app (Electron). Wraps the manager/ web editor and grants
// Web Serial so it can talk to the pedal as a native app (no browser).
const { app, BrowserWindow, session, Menu } = require("electron");
const path = require("path");

function createWindow() {
  const win = new BrowserWindow({
    width: 1340,
    height: 900,
    minWidth: 980,
    minHeight: 680,
    backgroundColor: "#0c0e12",
    title: "TankFree",
    autoHideMenuBar: true,
    webPreferences: { contextIsolation: true, sandbox: true },
  });
  const shotArg = process.argv.find((a) => a.startsWith("--shot="));
  const shotFile = shotArg ? shotArg.split("=")[1] : null;
  win.loadFile(path.join(__dirname, "renderer", "index.html"), shotFile ? { search: "shot" } : {});
  if (shotFile) {
    win.webContents.once("did-finish-load", () => {
      setTimeout(async () => {
        const img = await win.webContents.capturePage();
        require("fs").writeFileSync(shotFile, img.toPNG());
        app.quit();
      }, 1600);
    });
  }
  return win;
}

function pickPedalPort(portList) {
  // Prefer the pedal: USB VID 0x3654 (13908 dec), else a usbmodem/ACM port, else first.
  return (
    portList.find((p) => p.vendorId === "13908" || p.vendorId === "3654") ||
    portList.find((p) => /usbmodem|ttyACM|ttyUSB/i.test(p.portName || "")) ||
    portList[0]
  );
}

app.whenReady().then(() => {
  const ses = session.defaultSession;
  // allow the Web Serial permission
  ses.setPermissionCheckHandler((_wc, perm) => perm === "serial" || perm === "serial-port");
  ses.setDevicePermissionHandler(() => true);
  // auto-select the pedal when the page calls navigator.serial.requestPort()
  ses.on("select-serial-port", (event, portList, _webContents, callback) => {
    event.preventDefault();
    const pick = pickPedalPort(portList);
    callback(pick ? pick.portId : "");
  });

  Menu.setApplicationMenu(null);
  createWindow();
  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") app.quit();
});
