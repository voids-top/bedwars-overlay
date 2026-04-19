const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("overlayAPI", {
  getState: () => ipcRenderer.invoke("overlay:get-state"),
  updateConfig: (patch) => ipcRenderer.invoke("overlay:update-config", patch),
  chooseLogFile: () => ipcRenderer.invoke("overlay:choose-log-file"),
  forceInject: () => ipcRenderer.invoke("overlay:force-inject"),
  refreshWho: () => ipcRenderer.invoke("overlay:refresh-who"),
  setWindowLayout: (layout) => ipcRenderer.invoke("overlay:set-window-layout", layout),
  openExternal: (url) => ipcRenderer.invoke("overlay:open-external", url),
  openMusicFolder: () => ipcRenderer.invoke("overlay:open-music-folder"),
  listMusicTracks: () => ipcRenderer.invoke("overlay:list-music-tracks"),
  promptText: (options) => ipcRenderer.invoke("overlay:prompt-text", options),
  minimize: () => ipcRenderer.invoke("overlay:minimize"),
  quit: () => ipcRenderer.invoke("overlay:quit"),
  onState(listener) {
    const wrapped = (_event, state) => listener(state);
    ipcRenderer.on("overlay:state", wrapped);
    return () => ipcRenderer.removeListener("overlay:state", wrapped);
  }
});
