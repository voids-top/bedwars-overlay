const { app, BrowserWindow, dialog, ipcMain, shell } = require("electron");
const path = require("path");
const { ElectronOverlayBackend } = require("./src/backend/backend");
const { getMusicDir, listTracks } = require("./src/backend/music-library");

let mainWindow = null;
let backend = null;
let moveSaveTimer = null;
let appliedOpacity = null;
let appliedWindowLayout = "";

const DEFAULT_WIDTH = 500;
const DEFAULT_HEIGHT = 300;

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
    y: Number(match[2])
  };
}

function applyWindowConfig() {
  if (!mainWindow || !backend) {
    return;
  }

  const snapshot = backend.getSnapshot();
  const opacity = Math.max(0.1, Math.min(1, Number(snapshot.config.opacity || 50) / 100));
  if (appliedOpacity !== null && Math.abs(appliedOpacity - opacity) < 0.001) {
    return;
  }
  appliedOpacity = opacity;
  mainWindow.setOpacity(opacity);
}

function broadcastState() {
  if (!mainWindow || mainWindow.isDestroyed() || !backend) {
    return;
  }
  mainWindow.webContents.send("overlay:state", backend.getSnapshot());
}

function createWindow() {
  const snapshot = backend.getSnapshot();
  const geometry = parseGeometry(snapshot.config.geometry);

  mainWindow = new BrowserWindow({
    width: DEFAULT_WIDTH,
    height: DEFAULT_HEIGHT,
    minWidth: DEFAULT_WIDTH,
    minHeight: DEFAULT_HEIGHT,
    maxWidth: DEFAULT_WIDTH,
    maxHeight: DEFAULT_HEIGHT,
    transparent: true,
    frame: false,
    show: false,
    resizable: false,
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

function setWindowLayout(layout) {
  if (!mainWindow || mainWindow.isDestroyed()) {
    return null;
  }

  const width = Math.max(500, Number(layout?.width || DEFAULT_WIDTH));
  const height = Math.max(50, Number(layout?.height || DEFAULT_HEIGHT));
  const nextLayout = `${width}x${height}`;
  if (nextLayout === appliedWindowLayout) {
    return { width, height };
  }
  appliedWindowLayout = nextLayout;
  mainWindow.setMinimumSize(width, height);
  mainWindow.setMaximumSize(width, height);
  mainWindow.setSize(width, height, true);
  return { width, height };
}

function escapeHtml(value) {
  return String(value ?? "")
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;");
}

function buildPromptHtml({ title, label, value }) {
  const safeTitle = escapeHtml(title || "Prompt");
  const safeLabel = escapeHtml(label || "Enter text");
  const safeValue = escapeHtml(value || "");
  return `<!doctype html>
<html>
  <head>
    <meta charset="utf-8">
    <style>
      html, body { margin: 0; width: 100%; height: 100%; background: #111; color: #fff; font-family: sans-serif; }
      body { box-sizing: border-box; padding: 16px; display: flex; flex-direction: column; gap: 12px; }
      label { font-size: 14px; }
      input { width: 100%; box-sizing: border-box; padding: 8px; border: 1px solid #666; background: #1a1a1a; color: #fff; }
      .row { display: flex; gap: 8px; justify-content: flex-end; }
      button { padding: 6px 12px; border: 1px solid #666; background: #222; color: #fff; cursor: pointer; }
      button:hover { background: #2d2d2d; }
    </style>
  </head>
  <body>
    <label>${safeLabel}</label>
    <form id="form">
      <input id="value" value="${safeValue}" autocomplete="off" spellcheck="false">
      <div class="row" style="margin-top: 12px;">
        <button id="cancel" type="button">Cancel</button>
        <button type="submit">OK</button>
      </div>
    </form>
    <script>
      const { ipcRenderer } = require("electron");
      const input = document.getElementById("value");
      const cancel = document.getElementById("cancel");
      const form = document.getElementById("form");
      input.focus();
      input.select();
      form.addEventListener("submit", (event) => {
        event.preventDefault();
        ipcRenderer.send("${safeTitle}", { accepted: true, value: input.value });
      });
      cancel.addEventListener("click", () => {
        ipcRenderer.send("${safeTitle}", { accepted: false, value: "" });
      });
      document.addEventListener("keydown", (event) => {
        if (event.key === "Escape") {
          ipcRenderer.send("${safeTitle}", { accepted: false, value: "" });
        }
      });
    </script>
  </body>
</html>`;
}

function promptText(options = {}) {
  if (!mainWindow || mainWindow.isDestroyed()) {
    return Promise.resolve(null);
  }

  const channel = `overlay:prompt-result:${Date.now()}:${Math.random().toString(16).slice(2)}`;
  const title = String(options.title || "Prompt");
  const promptWindow = new BrowserWindow({
    width: 360,
    height: 180,
    title,
    parent: mainWindow,
    modal: true,
    resizable: false,
    minimizable: false,
    maximizable: false,
    autoHideMenuBar: true,
    show: false,
    backgroundColor: "#111111",
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false,
      sandbox: false
    }
  });

  return new Promise((resolve) => {
    let settled = false;
    const onPromptResult = (_event, payload) => {
      finish(payload && payload.accepted ? String(payload.value ?? "") : null);
    };

    const finish = (value) => {
      if (settled) {
        return;
      }
      settled = true;
      ipcMain.removeListener(channel, onPromptResult);
      resolve(value);
      if (!promptWindow.isDestroyed()) {
        promptWindow.close();
      }
    };

    ipcMain.once(channel, onPromptResult);

    promptWindow.on("closed", () => {
      finish(null);
    });

    promptWindow.loadURL(`data:text/html;charset=utf-8,${encodeURIComponent(buildPromptHtml({
      title: channel,
      label: options.label || title,
      value: options.value || ""
    }))}`);

    promptWindow.once("ready-to-show", () => {
      promptWindow.show();
    });
  });
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
  ipcMain.handle("overlay:set-window-layout", (_event, layout) => setWindowLayout(layout));
  ipcMain.handle("overlay:open-external", (_event, url) => shell.openExternal(String(url || "")));
  ipcMain.handle("overlay:open-music-folder", async () => shell.openPath(getMusicDir()));
  ipcMain.handle("overlay:list-music-tracks", async () => listTracks());
  ipcMain.handle("overlay:prompt-text", (_event, options) => promptText(options));
  ipcMain.handle("overlay:minimize", () => {
    if (mainWindow && !mainWindow.isDestroyed()) {
      mainWindow.minimize();
    }
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
