const { app, BrowserWindow, dialog, ipcMain } = require("electron");
const path = require("path");
const { ElectronOverlayBackend } = require("./src/backend/backend");

let mainWindow = null;
let backend = null;
let moveSaveTimer = null;
let compactMode = false;
let expandedHeight = 280;

const DEFAULT_WIDTH = 860;
const DEFAULT_HEIGHT = 240;
const MIN_WIDTH = 520;
const MIN_EXPANDED_HEIGHT = 120;
const COMPACT_HEIGHT = 56;

function parseGeometry(geometry) {
  if (!geometry || typeof geometry !== "string") {
    return {};
  }

  const match = geometry.match(/^\+(-?\d+)\+(-?\d+)$/);
  if (!match) {
    return {};
  }

  return {
    x: Number(match[1]),
    y: Number(match[2]),
  };
}

function applyWindowConfig() {
  if (!mainWindow || !backend) {
    return;
  }

  const snapshot = backend.getSnapshot();
  const opacity = Math.max(0.2, Math.min(1, Number(snapshot.config.opacity || 50) / 100));
  mainWindow.setOpacity(opacity);
}

function broadcastState() {
  if (!mainWindow || mainWindow.isDestroyed() || !backend) {
    return;
  }

  mainWindow.webContents.send("overlay:state", backend.getSnapshot());
}

function setCompactMode(isCompact) {
  if (!mainWindow || mainWindow.isDestroyed()) {
    compactMode = Boolean(isCompact);
    return;
  }

  const nextCompact = Boolean(isCompact);
  if (compactMode === nextCompact) {
    return;
  }

  const [width, height] = mainWindow.getSize();
  if (!compactMode && height > COMPACT_HEIGHT) {
    expandedHeight = Math.max(height, MIN_EXPANDED_HEIGHT);
  }

  compactMode = nextCompact;
  mainWindow.setMinimumSize(MIN_WIDTH, compactMode ? COMPACT_HEIGHT : MIN_EXPANDED_HEIGHT);
  mainWindow.setSize(width, compactMode ? COMPACT_HEIGHT : Math.max(expandedHeight, MIN_EXPANDED_HEIGHT), true);
}

function createWindow() {
  const snapshot = backend.getSnapshot();
  const geometry = parseGeometry(snapshot.config.geometry);
  const startCompact = Boolean(snapshot.config.autohide) && snapshot.players.length === 0;

  compactMode = startCompact;
  expandedHeight = DEFAULT_HEIGHT;

  mainWindow = new BrowserWindow({
    width: DEFAULT_WIDTH,
    height: startCompact ? COMPACT_HEIGHT : DEFAULT_HEIGHT,
    minWidth: MIN_WIDTH,
    minHeight: startCompact ? COMPACT_HEIGHT : MIN_EXPANDED_HEIGHT,
    maxWidth: 1400,
    transparent: true,
    frame: false,
    show: false,
    resizable: true,
    hasShadow: false,
    backgroundColor: "#00000000",
    alwaysOnTop: true,
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: false
    },
    ...geometry
  });

  mainWindow.loadFile(path.join(__dirname, "renderer", "index.html"));
  mainWindow.once("ready-to-show", () => {
    applyWindowConfig();
    mainWindow.show();
    broadcastState();
  });

  mainWindow.on("move", () => {
    if (moveSaveTimer) {
      clearTimeout(moveSaveTimer);
    }
    moveSaveTimer = setTimeout(() => {
      if (!mainWindow || mainWindow.isDestroyed() || !backend) {
        return;
      }
      const [x, y] = mainWindow.getPosition();
      backend.updateConfig({ geometry: `+${x}+${y}` });
    }, 250);
  });

  mainWindow.on("resize", () => {
    if (!mainWindow || mainWindow.isDestroyed() || compactMode) {
      return;
    }

    const [, height] = mainWindow.getSize();
    expandedHeight = Math.max(height, MIN_EXPANDED_HEIGHT);
  });

  mainWindow.on("closed", () => {
    mainWindow = null;
  });
}

async function chooseLogFile() {
  const result = await dialog.showOpenDialog(mainWindow, {
    properties: ["openFile"],
    filters: [{ name: "Log Files", extensions: ["log", "txt"] }]
  });

  if (result.canceled || result.filePaths.length === 0) {
    return null;
  }

  const filePath = result.filePaths[0];
  backend.updateConfig({ log_file: filePath });
  return filePath;
}

async function bootstrap() {
  backend = new ElectronOverlayBackend({
    rootDir: __dirname,
    onStateChange: () => {
      applyWindowConfig();
      broadcastState();
    }
  });

  await backend.start();
  createWindow();

  ipcMain.handle("overlay:get-state", () => backend.getSnapshot());
  ipcMain.handle("overlay:update-config", (_event, patch) => {
    backend.updateConfig(patch || {});
    applyWindowConfig();
    return backend.getSnapshot();
  });
  ipcMain.handle("overlay:choose-log-file", async () => chooseLogFile());
  ipcMain.handle("overlay:force-inject", async () => {
    await backend.forceInject();
    return backend.getSnapshot();
  });
  ipcMain.handle("overlay:refresh-who", async () => {
    await backend.sendBusCommand("chat /who", { expectResponse: false });
    return backend.getSnapshot();
  });
  ipcMain.handle("overlay:set-compact-mode", (_event, compact) => {
    setCompactMode(compact);
    return { compactMode };
  });
  ipcMain.handle("overlay:quit", () => {
    app.quit();
  });
}

if (!app.requestSingleInstanceLock()) {
  app.quit();
}

app.whenReady().then(bootstrap);

app.on("window-all-closed", () => {
  app.quit();
});

app.on("before-quit", async () => {
  if (backend) {
    await backend.stop();
  }
});
