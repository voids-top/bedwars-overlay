# Electron Overlay

Electron rewrite of the original Tkinter overlay.

## What is included

- Frameless always-on-top overlay window
- Native TCP bus listener for the injected DLL
- Auto-inject loop using the existing `injector.exe` and `build/util.dll`
- Minecraft log detection and `/who` driven player list updates
- Bedwars stats lookup via `https://hypixel.voids.top`
- Shared config file at `%APPDATA%\\voidoverlay\\settings.json`

## Run

```powershell
cd electron-overlay
npm install
npm start
```

## Notes

- This version focuses on the overlay, bus, injection, and stats flow.
- It reuses assets and native binaries from the repository root.
