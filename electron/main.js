const { app, BrowserWindow, globalShortcut } = require("electron");
const path = require("path");

let mainWindow;

function createWindow() {
  mainWindow = new BrowserWindow({
    fullscreen: true,
    kiosk: true,
    autoHideMenuBar: true,
    width: 1440,
    height: 900,
    minWidth: 1024,
    minHeight: 720,
    backgroundColor: "#0f172a",
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true
    }
  });

  mainWindow.setMenu(null);

  mainWindow.loadURL("http://localhost:3000");

  mainWindow.on("closed", () => {
    mainWindow = null;
  });
}

app.whenReady().then(() => {
  process.env.NODE_ENV = "production";

  // Start backend server (compiled bundle) when the app launches.
  try {
    require(path.resolve(__dirname, "../dist/index.js"));
  } catch (error) {
    console.error("Não foi possível iniciar o servidor Express:", error);
  }
  createWindow();

  const blockedShortcuts = [
    "CommandOrControl+W",
    "CommandOrControl+T",
    "Alt+F4",
    "F11",
    "CommandOrControl+N",
    "CommandOrControl+Shift+I",
    "F12",
    "Alt+Space"
  ];
  blockedShortcuts.forEach((shortcut) => {
    globalShortcut.register(shortcut, () => {});
  });

  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") {
    app.quit();
  }
});

app.on("will-quit", () => {
  globalShortcut.unregisterAll();
});
