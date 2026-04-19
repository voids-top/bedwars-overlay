const fs = require("fs");
const path = require("path");
const { DEFAULT_CONFIG } = require("./constants");

function getAppDataDir() {
  return path.join(process.env.APPDATA || process.cwd(), "voidoverlay");
}

function getConfigPath() {
  return path.join(getAppDataDir(), "settings.json");
}

function ensureDirs() {
  fs.mkdirSync(getAppDataDir(), { recursive: true });
  fs.mkdirSync(path.join(getAppDataDir(), "music"), { recursive: true });
}

function clampNumber(value, min, max, fallback) {
  const numeric = Number(value);
  if (!Number.isFinite(numeric)) {
    return fallback;
  }
  return Math.max(min, Math.min(max, numeric));
}

function sanitizeVolume(value, fallback) {
  const numeric = Number(value);
  if (!Number.isFinite(numeric)) {
    return fallback;
  }
  if (numeric > 1) {
    return 1;
  }
  return Math.max(0, Math.min(1, numeric));
}

function sanitizeConfig(input) {
  const config = { ...DEFAULT_CONFIG, ...(input || {}) };

  config.mode = "bedwars";
  config.opacity = clampNumber(config.opacity, 0, 100, DEFAULT_CONFIG.opacity);
  config.refresh = clampNumber(config.refresh, 10, 1000, DEFAULT_CONFIG.refresh);
  config.fps = 60;
  config.volume = sanitizeVolume(config.volume, DEFAULT_CONFIG.volume > 1 ? 1 : DEFAULT_CONFIG.volume);
  config.position = Number.isFinite(Number(config.position)) ? Number(config.position) : DEFAULT_CONFIG.position;
  config.geometry = typeof config.geometry === "string" ? config.geometry : DEFAULT_CONFIG.geometry;
  config.log_file = typeof config.log_file === "string" ? config.log_file : DEFAULT_CONFIG.log_file;
  config.background = Boolean(config.background);
  config.inject = Boolean(config.inject);
  config.deposit_notify = Boolean(config.deposit_notify);
  config.auto_party_transfer = Boolean(config.auto_party_transfer);
  config.notify_suspicious_player = Boolean(config.notify_suspicious_player);
  config.autochat = Boolean(config.autochat);

  return config;
}

function loadConfig() {
  ensureDirs();
  const configPath = getConfigPath();

  try {
    const raw = fs.readFileSync(configPath, "utf8");
    return sanitizeConfig(JSON.parse(raw));
  } catch (_error) {
    const config = sanitizeConfig(DEFAULT_CONFIG);
    saveConfig(config);
    return config;
  }
}

function saveConfig(config) {
  ensureDirs();
  fs.writeFileSync(getConfigPath(), JSON.stringify(sanitizeConfig(config), null, 2), "utf8");
}

module.exports = {
  getConfigPath,
  loadConfig,
  saveConfig,
  sanitizeConfig
};
