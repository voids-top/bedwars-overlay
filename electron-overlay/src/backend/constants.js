const path = require("path");

const HOST = "127.0.0.1";
const BUS_PORTS = [46012, 46011, 46001];
const VERSION = "0.2.1";
const DEFAULT_CONFIG = {
  hypixel_key: null,
  urchin_key: null,
  antisniper_key: null,
  refresh: 100,
  mode: "bedwars",
  fps: 60,
  opacity: 50,
  background: false,
  inject: false,
  deposit_notify: true,
  auto_party_transfer: true,
  notify_suspicious_player: true,
  autochat: false,
  geometry: "",
  volume: 10,
  position: 0,
  log_file: "custom"
};

const CLIENT_LOGS = {
  "Vanilla/Forge Client": "{appdata}\\.minecraft\\logs\\latest.log",
  "Lunar Client": "{userprofile}\\.lunarclient\\offline\\multiver\\logs\\latest.log",
  "Lunar Client ": "{userprofile}\\.lunarclient\\logs\\launcher\\renderer.log",
  "Lunar Client (1.7)": "{userprofile}\\.lunarclient\\profiles\\lunar\\1.7\\logs\\latest.log",
  "Lunar Client (1.8)": "{userprofile}\\.lunarclient\\profiles\\lunar\\1.8\\logs\\latest.log",
  "Lunar Client (1.12)": "{userprofile}\\.lunarclient\\profiles\\lunar\\1.12\\logs\\latest.log",
  "Lunar Client (1.15)": "{userprofile}\\.lunarclient\\profiles\\lunar\\1.15\\logs\\latest.log",
  "Lunar Client (1.16)": "{userprofile}\\.lunarclient\\profiles\\lunar\\1.16\\logs\\latest.log",
  "Lunar Client (1.7) ": "{userprofile}\\.lunarclient\\offline\\1.7\\logs\\latest.log",
  "Lunar Client (1.8) ": "{userprofile}\\.lunarclient\\offline\\1.8\\logs\\latest.log",
  "Lunar Client (1.12) ": "{userprofile}\\.lunarclient\\offline\\1.12\\logs\\latest.log",
  "Lunar Client (1.15) ": "{userprofile}\\.lunarclient\\offline\\1.15\\logs\\latest.log",
  "Lunar Client (1.16) ": "{userprofile}\\.lunarclient\\offline\\1.16\\logs\\latest.log",
  "Lunar Client (1.7)  ": "{userprofile}\\.lunarclient\\offline\\files\\1.7\\logs\\latest.log",
  "Lunar Client (1.8)  ": "{userprofile}\\.lunarclient\\offline\\files\\1.8\\logs\\latest.log",
  "Lunar Client (1.12)  ": "{userprofile}\\.lunarclient\\offline\\files\\1.12\\logs\\latest.log",
  "Lunar Client (1.15)  ": "{userprofile}\\.lunarclient\\offline\\files\\1.15\\logs\\latest.log",
  "Lunar Client (1.16)  ": "{userprofile}\\.lunarclient\\offline\\files\\1.16\\logs\\latest.log",
  "Badlion Client": "{appdata}\\.minecraft\\logs\\blclient\\minecraft\\latest.log",
  "PVP Lounge Client": "{appdata}\\.pvplounge\\logs\\latest.log"
};

function expandTemplate(template) {
  return template
    .replaceAll("{appdata}", process.env.APPDATA || "")
    .replaceAll("{userprofile}", process.env.USERPROFILE || "");
}

function resolveRoot(rootDir, ...parts) {
  return path.resolve(rootDir, "..", ...parts);
}

module.exports = {
  BUS_PORTS,
  CLIENT_LOGS,
  DEFAULT_CONFIG,
  HOST,
  VERSION,
  expandTemplate,
  resolveRoot
};
