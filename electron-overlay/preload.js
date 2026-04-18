const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("overlayAPI", {
  getState: () => ipcRenderer.invoke("overlay:get-state"),
  updateConfig: (patch) => ipcRenderer.invoke("overlay:update-config", patch),
  chooseLogFile: () => ipcRenderer.invoke("overlay:choose-log-file"),
  forceInject: () => ipcRenderer.invoke("overlay:force-inject"),
  refreshWho: () => ipcRenderer.invoke("overlay:refresh-who"),
  setCompactMode: (compact) => ipcRenderer.invoke("overlay:set-compact-mode", compact),
  quit: () => ipcRenderer.invoke("overlay:quit"),
  onState(listener) {
    const wrapped = (_event, state) => listener(state);
    ipcRenderer.on("overlay:state", wrapped);
    return () => ipcRenderer.removeListener("overlay:state", wrapped);
  }
});
