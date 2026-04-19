const LIMITS = { Player: 14, TAG: 9, WS: 3, FKDR: 5, WLR: 5, Finals: 5, Wins: 5 };
const PLACE_BASE = { Player: 135, TAG: 235, WS: 290, FKDR: 330, WLR: 375, Finals: 420, Wins: 465 };
const MODE_BUTTONS = ["settings", "about", "music"];
const TARGET_FPS = 60;
const FONT_SCALE = 4 / 3;
const STAR_PALETTES = [
  ["#AAAAAA", "#AAAAAA", "#AAAAAA", "#AAAAAA", "#AAAAAA"],
  ["#FFFFFF", "#FFFFFF", "#FFFFFF", "#FFFFFF", "#FFFFFF", "#FFFFFF"],
  ["#FFAA00", "#FFAA00", "#FFAA00", "#FFAA00", "#FFAA00", "#FFAA00"],
  ["#55FFFF", "#55FFFF", "#55FFFF", "#55FFFF", "#55FFFF", "#55FFFF"],
  ["#00AA00", "#00AA00", "#00AA00", "#00AA00", "#00AA00", "#00AA00"],
  ["#00AAAA", "#00AAAA", "#00AAAA", "#00AAAA", "#00AAAA", "#00AAAA"],
  ["#AA0000", "#AA0000", "#AA0000", "#AA0000", "#AA0000", "#AA0000"],
  ["#FF55FF", "#FF55FF", "#FF55FF", "#FF55FF", "#FF55FF", "#FF55FF"],
  ["#5555FF", "#5555FF", "#5555FF", "#5555FF", "#5555FF", "#5555FF"],
  ["#AA00AA", "#AA00AA", "#AA00AA", "#AA00AA", "#AA00AA", "#AA00AA"],
  ["#FF5555", "#FFAA00", "#FFFF55", "#55FF55", "#55FFFF", "#FF55FF", "#AA00AA"],
  ["#AAAAAA", "#FFFFFF", "#FFFFFF", "#FFFFFF", "#FFFFFF", "#AAAAAA", "#AAAAAA"],
  ["#AAAAAA", "#FFFF55", "#FFFF55", "#FFFF55", "#FFFF55", "#FFAA00", "#AAAAAA"],
  ["#AAAAAA", "#55FFFF", "#55FFFF", "#55FFFF", "#55FFFF", "#00AAAA", "#AAAAAA"],
  ["#AAAAAA", "#55FF55", "#55FF55", "#55FF55", "#55FF55", "#00AA00", "#AAAAAA"],
  ["#AAAAAA", "#00AAAA", "#00AAAA", "#00AAAA", "#00AAAA", "#5555FF", "#AAAAAA"],
  ["#AAAAAA", "#FF5555", "#FF5555", "#FF5555", "#FF5555", "#AA0000", "#AAAAAA"],
  ["#AAAAAA", "#FF55FF", "#FF55FF", "#FF55FF", "#FF55FF", "#AA00AA", "#AAAAAA"],
  ["#AAAAAA", "#5555FF", "#5555FF", "#5555FF", "#5555FF", "#0000AA", "#AAAAAA"],
  ["#AAAAAA", "#AA00AA", "#AA00AA", "#AA00AA", "#AA00AA", "#FF55FF", "#AAAAAA"],
  ["#555555", "#AAAAAA", "#FFFFFF", "#FFFFFF", "#AAAAAA", "#AAAAAA", "#555555"],
  ["#FFFFFF", "#FFFFFF", "#FFFF55", "#FFFF55", "#FFAA00", "#FFAA00", "#FFAA00"],
  ["#FFAA00", "#FFAA00", "#FFFFFF", "#FFFFFF", "#55FFFF", "#00AAAA", "#00AAAA"],
  ["#AA00AA", "#AA00AA", "#FF55FF", "#FF55FF", "#FFAA00", "#FFFF55", "#FFFF55"],
  ["#55FFFF", "#55FFFF", "#FFFFFF", "#FFFFFF", "#AAAAAA", "#AAAAAA", "#555555"],
  ["#FFFFFF", "#FFFFFF", "#55FF55", "#55FF55", "#00AA00", "#00AA00", "#00AA00"],
  ["#AA0000", "#AA0000", "#FF5555", "#FF5555", "#FF55FF", "#FF55FF", "#AA00AA"],
  ["#FFFF55", "#FFFF55", "#FFFFFF", "#FFFFFF", "#555555", "#555555", "#555555"],
  ["#55FF55", "#55FF55", "#00AA00", "#00AA00", "#FFAA00", "#FFAA00", "#FFFF55"],
  ["#55FFFF", "#55FFFF", "#00AAAA", "#00AAAA", "#5555FF", "#5555FF", "#0000AA"],
  ["#FFFF55", "#FFFF55", "#FFAA00", "#FFAA00", "#FF5555", "#FF5555", "#AA0000"]
];
const TOGGLE_ITEMS = [
  ["background", "Transparent Background", 150],
  ["inject", "Inject", 170],
  ["deposit_notify", "Deposit Notify", 190],
  ["auto_party_transfer", "Auto Party Transfer", 210],
  ["notify_suspicious_player", "Notify Suspisious Player", 230]
];
const COLORS = {
  notAvailable: "#242424",
  disabled: "#767676",
  enabled: "#FFFFFF",
  on: "#00FF1A",
  off: "#FF0000"
};

const state = {
  snapshot: null,
  uiMode: null,
  layout: { mode: "stats", width: 500, height: 300, addWidth: 0, addHeight: 0, places: { ...PLACE_BASE }, rows: [] },
  pointerKind: null,
  previewConfig: null,
  canvas: null,
  ctx: null,
  dpr: window.devicePixelRatio || 1,
  lastRenderAt: 0,
  measuredFps: 0,
  lastWindowLayout: "",
  music: {
    audio: new Audio(),
    tracks: [],
    trackIndex: -1,
    currentTrack: null,
    artImage: null,
    loaded: false
  }
};

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function calcValue(value, valueRange, min, max) {
  const percent = value / valueRange * 100;
  const range = (max - min) / 100;
  return percent * range + min;
}

function calcPos(value, valueRange, min, max) {
  return (value - min) * valueRange / 100 / (max / 100 - min / 100);
}

function calcTime(value) {
  const numeric = Math.max(0, Number(value) || 0);
  if (numeric >= 3600) {
    const hours = Math.floor(numeric / 3600);
    const minutes = String(Math.floor((numeric % 3600) / 60)).padStart(2, "0");
    const seconds = String(Math.floor(numeric % 60)).padStart(2, "0");
    return `${hours}:${minutes}:${seconds}`;
  }
  if (numeric >= 60) {
    const minutes = Math.floor(numeric / 60);
    const seconds = String(Math.floor(numeric % 60)).padStart(2, "0");
    return `${minutes}:${seconds}`;
  }
  return `0:${String(Math.floor(numeric)).padStart(2, "0")}`;
}

function normalizeVolume(value) {
  const numeric = Number(value);
  if (!Number.isFinite(numeric)) {
    return 1;
  }
  if (numeric > 1) {
    return 1;
  }
  return clamp(numeric, 0, 1);
}

function currentConfig() {
  return {
    ...(state.snapshot?.config || {}),
    ...(state.previewConfig || {})
  };
}

function applyPreviewOpacity() {
  const shell = document.querySelector("#shell");
  const config = currentConfig();
  shell.style.opacity = "1";
  document.body.classList.toggle("transparent-background", Boolean(config.background));
}

function rankColor(rank) {
  if (rank === "MVP++") return "#e7a501";
  if ((rank || "").startsWith("VIP")) return "#51f451";
  if ((rank || "").startsWith("MVP")) return "#5afdfd";
  if (rank === "DEFAULT") return "#a0a0a0";
  if (rank === "YOUTUBER") return "#ed5454";
  return "#ffffff";
}

function wsColor(value) {
  const numeric = Number(value);
  if (!Number.isFinite(numeric)) return "#ffffff";
  if (numeric < 10) return "#ffffff";
  if (numeric < 20) return "#55ffff";
  if (numeric < 50) return "#55ff55";
  if (numeric < 100) return "#ffff55";
  if (numeric < 1000) return "#ff5555";
  return "#aa0000";
}

function fkdrColor(value) {
  const numeric = Number(value);
  if (!Number.isFinite(numeric)) return "#ffffff";
  if (numeric < 2) return "#ffffff";
  if (numeric < 5) return "#55ffff";
  if (numeric < 10) return "#55ff55";
  if (numeric < 50) return "#ffff55";
  if (numeric < 100) return "#ff5555";
  return "#aa0000";
}

function wlrColor(value) {
  const numeric = Number(value);
  if (!Number.isFinite(numeric)) return "#ffffff";
  if (numeric < 1) return "#ffffff";
  if (numeric < 2) return "#55ffff";
  if (numeric < 5) return "#55ff55";
  if (numeric < 10) return "#ffff55";
  if (numeric < 50) return "#ff5555";
  return "#aa0000";
}

function finalColor(value) {
  const numeric = Number(value);
  if (!Number.isFinite(numeric)) return "#ffffff";
  if (numeric < 3000) return "#ffffff";
  if (numeric < 5000) return "#55ffff";
  if (numeric < 10000) return "#55ff55";
  if (numeric < 15000) return "#ffff55";
  if (numeric < 20000) return "#ff5555";
  return "#aa0000";
}

function winColor(value) {
  const numeric = Number(value);
  if (!Number.isFinite(numeric)) return "#ffffff";
  if (numeric < 1000) return "#ffffff";
  if (numeric < 3000) return "#55ffff";
  if (numeric < 5000) return "#55ff55";
  if (numeric < 10000) return "#ffff55";
  if (numeric < 20000) return "#ff5555";
  return "#aa0000";
}

function drawText(text, x, y, { align = "left", size = 8, color = "#FFFFFF" } = {}) {
  const ctx = state.ctx;
  const scaledSize = Math.max(1, Math.round(size * FONT_SCALE));
  const drawX = Math.round(x);
  const drawY = Math.round(y);
  ctx.save();
  ctx.font = `${scaledSize}px Minecraftia`;
  ctx.textAlign = align;
  ctx.textBaseline = "middle";
  if (currentConfig().background) {
    ctx.fillStyle = "#000000";
    ctx.fillText(String(text ?? ""), drawX + 1, drawY + 1);
  }
  ctx.fillStyle = color;
  ctx.fillText(String(text ?? ""), drawX, drawY);
  ctx.restore();
}

function measureTextWidth(text, size = 8) {
  const ctx = state.ctx;
  const scaledSize = Math.max(1, Math.round(size * FONT_SCALE));
  ctx.save();
  ctx.font = `${scaledSize}px Minecraftia`;
  const width = ctx.measureText(String(text ?? "")).width;
  ctx.restore();
  return width;
}

function drawLine(x1, y1, x2, y2, color = "#FFFFFF", width = 1) {
  const ctx = state.ctx;
  ctx.save();
  ctx.beginPath();
  ctx.moveTo(x1, y1);
  ctx.lineTo(x2, y2);
  ctx.lineWidth = width;
  ctx.strokeStyle = color;
  ctx.stroke();
  ctx.restore();
}

function drawCircle(x, y, radius, color = "#FFFFFF") {
  const ctx = state.ctx;
  ctx.save();
  ctx.beginPath();
  ctx.arc(x, y, radius, 0, Math.PI * 2);
  ctx.fillStyle = color;
  ctx.fill();
  ctx.restore();
}

function drawFilledRect(x, y, width, height, color = "#FFFFFF") {
  const ctx = state.ctx;
  ctx.save();
  ctx.fillStyle = color;
  ctx.fillRect(x, y, width, height);
  ctx.restore();
}

function drawRectOutline(centerX, centerY, width, height, text, color = "#FFFFFF", size = 7) {
  const ctx = state.ctx;
  ctx.save();
  ctx.strokeStyle = color;
  ctx.lineWidth = 2;
  ctx.strokeRect(centerX - width / 2, centerY - height / 2, width, height);
  ctx.restore();
  drawText(text, centerX, centerY + 1, { align: "center", size, color });
}

function truncate(text, maxLength) {
  const string = String(text || "");
  return string.length >= maxLength ? `${string.slice(0, maxLength)}...` : string;
}

function drawFilledTriangle(points, color = "#FFFFFF") {
  const ctx = state.ctx;
  ctx.save();
  ctx.beginPath();
  ctx.moveTo(points[0][0], points[0][1]);
  for (const [x, y] of points.slice(1)) {
    ctx.lineTo(x, y);
  }
  ctx.closePath();
  ctx.fillStyle = color;
  ctx.fill();
  ctx.restore();
}

function drawFilledStar(centerX, centerY, outerRadius, innerRadius, color = "#FFFFFF") {
  const ctx = state.ctx;
  ctx.save();
  ctx.beginPath();
  for (let index = 0; index < 10; index += 1) {
    const radius = index % 2 === 0 ? outerRadius : innerRadius;
    const angle = -Math.PI / 2 + index * Math.PI / 5;
    const x = centerX + Math.cos(angle) * radius;
    const y = centerY + Math.sin(angle) * radius;
    if (index === 0) {
      ctx.moveTo(x, y);
    } else {
      ctx.lineTo(x, y);
    }
  }
  ctx.closePath();
  ctx.fillStyle = color;
  ctx.fill();
  ctx.restore();
}

function getStarStyle(stars) {
  const numeric = Math.floor(Number(stars));
  if (!Number.isFinite(numeric) || numeric < 0) {
    return null;
  }

  const tier = clamp(Math.floor(numeric / 100), 0, STAR_PALETTES.length - 1);
  const palette = STAR_PALETTES[tier];
  return {
    text: String(numeric),
    glyph: tier >= 30 ? "❀" : "✫",
    left: palette[0],
    digits: palette.slice(1, -2),
    star: palette.at(-2),
    right: palette.at(-1)
  };
}

function drawStarBadge(stars, startX, centerY) {
  const style = getStarStyle(stars);
  if (!style) {
    drawText(stars, startX, centerY, { align: "left", size: 8, color: "#FFFFFF" });
    return;
  }

  const segments = [{ type: "text", value: "[", color: style.left }];
  for (let index = 0; index < style.text.length; index += 1) {
    segments.push({
      type: "text",
      value: style.text[index],
      color: style.digits[Math.min(index, style.digits.length - 1)] || style.star,
      xOffset: 0,
      yOffset: 0
    });
  }
  segments.push({ type: "text", value: style.glyph, color: style.star, xOffset: 0, yOffset: -1 });
  segments.push({ type: "text", value: "]", color: style.right, xOffset: 1, yOffset: 0 });

  let cursor = startX;
  for (const segment of segments) {
    drawText(segment.value, cursor + (segment.xOffset || 0), centerY + (segment.yOffset || 0), { align: "left", size: 8, color: segment.color });
    cursor += measureTextWidth(segment.value, 8);
  }
}

function drawPlayIcon(centerX, centerY, color = "#FFFFFF") {
  drawFilledTriangle([
    [centerX - 3, centerY - 5],
    [centerX - 3, centerY + 5],
    [centerX + 5, centerY]
  ], color);
}

function drawPauseIcon(centerX, centerY, color = "#FFFFFF") {
  drawFilledRect(centerX - 4, centerY - 5, 3, 10, color);
  drawFilledRect(centerX + 1, centerY - 5, 3, 10, color);
}

function drawSkipPreviousIcon(centerX, centerY, color = "#FFFFFF") {
  drawFilledRect(centerX - 6, centerY - 5, 2, 10, color);
  drawFilledTriangle([
    [centerX - 4, centerY],
    [centerX + 2, centerY - 5],
    [centerX + 2, centerY + 5]
  ], color);
}

function drawSkipNextIcon(centerX, centerY, color = "#FFFFFF") {
  drawFilledTriangle([
    [centerX - 2, centerY - 5],
    [centerX - 2, centerY + 5],
    [centerX + 4, centerY]
  ], color);
  drawFilledRect(centerX + 4, centerY - 5, 2, 10, color);
}

function drawFolderIcon(centerX, centerY, color = "#FFFFFF") {
  const ctx = state.ctx;
  ctx.save();
  ctx.beginPath();
  ctx.moveTo(centerX - 6, centerY - 2);
  ctx.lineTo(centerX - 3, centerY - 2);
  ctx.lineTo(centerX - 1, centerY - 5);
  ctx.lineTo(centerX + 6, centerY - 5);
  ctx.lineTo(centerX + 6, centerY + 5);
  ctx.lineTo(centerX - 6, centerY + 5);
  ctx.closePath();
  ctx.strokeStyle = color;
  ctx.lineWidth = 1.5;
  ctx.stroke();
  ctx.restore();
}

function drawReloadIcon(centerX, centerY, color = "#FFFFFF") {
  const ctx = state.ctx;
  ctx.save();
  ctx.beginPath();
  ctx.arc(centerX, centerY, 4.5, Math.PI * 0.15, Math.PI * 1.55);
  ctx.strokeStyle = color;
  ctx.lineWidth = 1.5;
  ctx.stroke();
  ctx.restore();
  drawFilledTriangle([
    [centerX + 5.8, centerY - 3.5],
    [centerX + 5.2, centerY + 1],
    [centerX + 1.6, centerY - 1.5]
  ], color);
}

function getUniquePlayers() {
  return [...new Set(state.snapshot?.players || [])];
}

function buildPlayerRows() {
  return getUniquePlayers().map((name) => state.snapshot.playerData?.[name] || {
    TAG: "Loading",
    star: "-",
    rank: "-",
    Player: name,
    FKDR: "-",
    WLR: "-",
    Finals: "-",
    Wins: "-",
    WS: "-",
    score: 10000
  }).sort((left, right) => Number(right.score || -1) - Number(left.score || -1)).slice(0, 21);
}

function computeStatsLayout() {
  const rows = buildPlayerRows();
  const maximum = { Player: 0, TAG: 0, WS: 0, FKDR: 0, WLR: 0, Finals: 0, Wins: 0 };
  const moves = { Player: 0, TAG: 0, WS: 0, FKDR: 0, WLR: 0, Finals: 0, Wins: 0 };
  const places = { ...PLACE_BASE };

  for (const row of rows) {
    for (const key of Object.keys(LIMITS)) {
      const value = Math.max(0, String(row[key] ?? "").length - LIMITS[key]);
      maximum[key] = Math.max(maximum[key], value);
    }
  }

  let plus = 0;
  for (const key of Object.keys(LIMITS)) {
    const value = maximum[key];
    moves[key] = Math.max(moves[key], plus + value * 4);
    plus += value * 8;
  }

  for (const key of Object.keys(places)) {
    places[key] += moves[key];
  }

  const addWidth = places.Wins - 475;
  const addHeight = rows.length > 11 ? 15 * (rows.length - 11) : 0;

  return {
    mode: "stats",
    width: 500 + addWidth,
    height: 262 + addHeight,
    addWidth,
    addHeight,
    places,
    rows
  };
}

function computeLayout() {
  if (!state.snapshot) {
    return state.layout;
  }

  if (!state.uiMode && state.snapshot.hidden) {
    return {
      mode: "compact",
      width: 500,
      height: 50,
      addWidth: 0,
      addHeight: 0,
      places: { ...PLACE_BASE },
      rows: []
    };
  }

  if (state.uiMode === "settings") {
    return { mode: "settings", width: 500, height: 367, addWidth: 0, addHeight: 105, places: { ...PLACE_BASE }, rows: [] };
  }

  if (state.uiMode === "about") {
    return { mode: "about", width: 500, height: 327, addWidth: 0, addHeight: 65, places: { ...PLACE_BASE }, rows: [] };
  }

  if (state.uiMode === "music") {
    return { mode: "music", width: 500, height: 327, addWidth: 0, addHeight: 65, places: { ...PLACE_BASE }, rows: [] };
  }

  return computeStatsLayout();
}

function resizeCanvas(width, height) {
  const shell = document.querySelector("#shell");
  shell.style.width = `${width}px`;
  shell.style.height = `${height}px`;
  state.canvas.style.width = `${width}px`;
  state.canvas.style.height = `${height}px`;
  const nextWidth = Math.round(width * state.dpr);
  const nextHeight = Math.round(height * state.dpr);
  if (state.canvas.width === nextWidth && state.canvas.height === nextHeight) {
    return;
  }
  state.canvas.width = nextWidth;
  state.canvas.height = nextHeight;
  state.ctx.setTransform(state.dpr, 0, 0, state.dpr, 0, 0);
  state.ctx.imageSmoothingEnabled = false;
}

async function syncWindowLayout() {
  const layout = computeLayout();
  state.layout = layout;
  resizeCanvas(layout.width, layout.height);
  const nextLayoutKey = `${layout.width}x${layout.height}`;
  if (nextLayoutKey !== state.lastWindowLayout) {
    state.lastWindowLayout = nextLayoutKey;
    await window.overlayAPI.setWindowLayout({ width: layout.width, height: layout.height });
  }
}

function updateButtonState() {
  const title = `Void Overlay v${state.snapshot?.version || ""}`.trim();
  document.querySelector("#titleText").textContent = title;
  document.title = title;
  for (const mode of MODE_BUTTONS) {
    const button = document.querySelector(`#${mode}Button`);
    button.classList.toggle("active", state.uiMode === mode);
  }
}

function currentAudio() {
  return state.music.audio;
}

function currentTrack() {
  return state.music.currentTrack;
}

function hasAlbumArt() {
  return Boolean(state.music.artImage?.complete && currentTrack()?.albumArtUrl);
}

function currentTimeValue() {
  return Number(currentAudio().currentTime || 0);
}

function currentDuration() {
  return Number(currentAudio().duration || 0);
}

function ensureArtImage(track) {
  if (!track?.albumArtUrl) {
    state.music.artImage = null;
    return;
  }
  if (state.music.artImage?.datasetSource === track.albumArtUrl) {
    return;
  }
  const image = new Image();
  image.datasetSource = track.albumArtUrl;
  image.src = track.albumArtUrl;
  state.music.artImage = image;
}

async function setCurrentTrack(index, { autoplay = true, startTime = 0 } = {}) {
  if (index < 0 || index >= state.music.tracks.length) {
    return;
  }

  state.music.trackIndex = index;
  state.music.currentTrack = state.music.tracks[index];
  ensureArtImage(state.music.currentTrack);
  const audio = currentAudio();
  audio.src = state.music.currentTrack.url;
  audio.load();

  await window.overlayAPI.updateConfig({ position: index });

  const playTrack = async () => {
    if (startTime > 0 && Number.isFinite(audio.duration)) {
      audio.currentTime = clamp(startTime, 0, audio.duration || startTime);
    }
    if (autoplay) {
      try {
        await audio.play();
      } catch (_error) {
        return;
      }
    } else {
      audio.pause();
    }
  };

  if (audio.readyState >= 1) {
    await playTrack();
    return;
  }

  audio.addEventListener("loadedmetadata", () => {
    playTrack();
  }, { once: true });
}

async function nextTrack() {
  if (!state.music.tracks.length) {
    return;
  }
  const nextIndex = state.music.trackIndex + 1 >= state.music.tracks.length ? 0 : state.music.trackIndex + 1;
  await setCurrentTrack(nextIndex, { autoplay: true, startTime: 0 });
}

async function previousTrack() {
  if (!state.music.tracks.length) {
    return;
  }
  const nextIndex = state.music.trackIndex - 1 < 0 ? state.music.tracks.length - 1 : state.music.trackIndex - 1;
  await setCurrentTrack(nextIndex, { autoplay: true, startTime: 0 });
}

async function togglePause() {
  const audio = currentAudio();
  if (!state.music.tracks.length) {
    return;
  }
  if (audio.paused) {
    try {
      await audio.play();
    } catch (_error) {
      return;
    }
  } else {
    audio.pause();
  }
}

async function loadMusicLibrary({ preserveCurrent = true } = {}) {
  const audio = currentAudio();
  const previousPath = preserveCurrent ? currentTrack()?.path : null;
  const previousTime = preserveCurrent ? currentTimeValue() : 0;
  const previousPaused = preserveCurrent ? audio.paused : false;
  state.music.tracks = await window.overlayAPI.listMusicTracks();

  if (!state.music.tracks.length) {
    audio.pause();
    audio.removeAttribute("src");
    audio.load();
    state.music.trackIndex = -1;
    state.music.currentTrack = null;
    state.music.artImage = null;
    return;
  }

  let nextIndex = -1;
  if (previousPath) {
    nextIndex = state.music.tracks.findIndex((track) => track.path === previousPath);
  }
  if (nextIndex < 0) {
    const configIndex = clamp(Number(state.snapshot?.config?.position || 0), 0, Math.max(0, state.music.tracks.length - 1));
    nextIndex = configIndex;
  }

  await setCurrentTrack(nextIndex, { autoplay: !previousPaused, startTime: previousTime });
}

function syncAudioFromConfig() {
  const audio = currentAudio();
  audio.volume = normalizeVolume(currentConfig().volume);
}

async function commitPreviewConfig() {
  if (!state.previewConfig || !state.snapshot) {
    return;
  }
  const patch = { ...state.previewConfig };
  state.previewConfig = null;
  applyPreviewOpacity();
  state.snapshot = await window.overlayAPI.updateConfig(patch);
  syncAudioFromConfig();
  await syncWindowLayout();
}

async function applyConfigPatch(patch) {
  state.previewConfig = null;
  applyPreviewOpacity();
  state.snapshot = await window.overlayAPI.updateConfig(patch);
  syncAudioFromConfig();
  await syncWindowLayout();
}

function setPreviewConfig(patch) {
  state.previewConfig = { ...(state.previewConfig || {}), ...patch };
  applyPreviewOpacity();
}

function drawBackground() {
  const ctx = state.ctx;
  const config = currentConfig();
  ctx.clearRect(0, 0, state.layout.width, state.layout.height);
  if (!config.background) {
    ctx.fillStyle = "#000000";
    ctx.fillRect(0, 0, state.layout.width, state.layout.height);
  }
}

function drawFooter() {
  const { width, height } = state.layout;
  drawLine(0, height - 25, 1000, height - 25, "#FFFFFF", 1);
  drawCircle(width - 6, height - 6, 2.5, state.snapshot.injected ? "#1fff1f" : "#ff1f1f");
  drawText(`Log : ${state.snapshot.log || ""}`, 8, height - 13, { align: "left", size: 8, color: "#FFFFFF" });
}

function drawMiniPlayer(height, width, compact = false) {
  const track = currentTrack();
  const info = track?.info || "";
  const more = hasAlbumArt() ? 24 : 0;
  const image = state.music.artImage;
  const textY = compact ? 37 : height - 35;
  const lineY = compact ? 47 : height - 47;
  const progressY = compact ? 47 : height - 26;

  if (!compact) {
    drawLine(0, lineY, 1000, lineY, "#FFFFFF", 1);
  }

  if (hasAlbumArt()) {
    state.ctx.drawImage(image, 2, textY - 11, 22, 22);
  }

  drawText(truncate(info, 50), 8 + more, textY, { align: "left", size: 8, color: "#FFFFFF" });
  drawText(`${Math.max(0, state.music.trackIndex + 1)}/${state.music.tracks.length} ${calcTime(currentTimeValue())}/${calcTime(currentDuration())}`, width - 8, textY, {
    align: "right",
    size: 8,
    color: "#FFFFFF"
  });

  const duration = currentDuration();
  const progressWidth = Math.max(1, width - more);
  const ratio = duration > 0 ? clamp(currentTimeValue() / duration, 0, 1) : 0;
  const progress = more + ratio * progressWidth;
  drawLine(more, progressY, progress, progressY, "#FFFFFF", 1);
}

function drawStatsMode() {
  const { places, rows } = state.layout;
  drawLine(0, 45, 1000, 45, "#FFFFFF", 1);
  drawText("Star", 35, 35, { align: "center", size: 8, color: "#FFFFFF" });
  for (const [key, x] of Object.entries(places)) {
    drawText(key, x, 35, { align: "center", size: 8, color: "#FFFFFF" });
  }

  rows.forEach((row, index) => {
    const y = 55 + 15 * index;
    drawStarBadge(row.star, 16, y);
    const playerName = state.snapshot.mcid && String(row.Player || "").toLowerCase() === String(state.snapshot.mcid).toLowerCase()
      ? "You"
      : row.Player;
    drawText(playerName, places.Player, y, { align: "center", size: 8, color: rankColor(row.rank) });
    drawText(row.TAG, places.TAG, y, { align: "center", size: 8, color: "#FFFFFF" });
    drawText(row.WS, places.WS, y, { align: "center", size: 8, color: wsColor(row.WS) });
    drawText(row.FKDR, places.FKDR, y, { align: "center", size: 8, color: fkdrColor(row.FKDR) });
    drawText(row.WLR, places.WLR, y, { align: "center", size: 8, color: wlrColor(row.WLR) });
    drawText(row.Finals, places.Finals, y, { align: "center", size: 8, color: finalColor(row.Finals) });
    drawText(row.Wins, places.Wins, y, { align: "center", size: 8, color: winColor(row.Wins) });
  });
}

function drawToggleState(enabled, y) {
  if (enabled) {
    drawCircle(365, y, 5, "#FFFFFF");
    drawText("On", 390, y, { align: "center", size: 8, color: COLORS.on });
  } else {
    drawCircle(350, y, 5, "#FFFFFF");
    drawText("Off", 390, y, { align: "center", size: 8, color: COLORS.off });
  }
}

function drawSettingsMode() {
  const config = currentConfig();
  drawText("Settings", 250, 45, { align: "center", size: 15, color: "#FFFFFF" });
  drawLine(299, 80, 451, 80, "#FFFFFF", 3);
  drawLine(299, 100, 451, 100, "#FFFFFF", 3);
  drawText("Custom Logfile", 125, 285, { align: "center", size: 8, color: "#FFFFFF" });
  drawText("Mode", 125, 265, { align: "center", size: 8, color: "#FFFFFF" });
  drawText("Reset API Key", 125, 305, { align: "center", size: 8, color: "#FFFFFF" });

  for (const [, label, y] of TOGGLE_ITEMS) {
    drawText(label, 125, y, { align: "center", size: 8, color: "#FFFFFF" });
    drawLine(350, y, 365, y, "#FFFFFF", 3);
  }

  drawText(`Opacity : ${Math.round(Number(config.opacity) || 0)}%`, 125, 80, { align: "center", size: 8, color: "#FFFFFF" });
  drawCircle(300 + calcPos(Number(config.opacity) || 10, 150, 10, 100), 80, 5, "#FFFFFF");

  drawText(`Refresh Log : ${Math.round(Number(config.refresh) || 0)}ms`, 125, 100, { align: "center", size: 8, color: "#FFFFFF" });
  drawCircle(300 + calcPos(Number(config.refresh) || 1000, 150, 1000, 10), 100, 5, "#FFFFFF");

  for (const [key, , y] of TOGGLE_ITEMS) {
    drawToggleState(Boolean(config[key]), y);
  }

  drawRectOutline(280, 305, 52, 16, "Hypixel", COLORS.disabled);
  drawRectOutline(370, 305, 70, 16, "Antisniper");
  drawRectOutline(325, 285, 52, 16, "Change");

  drawRectOutline(275, 265, 32, 16, "bw", "#FFFFFF");
  drawRectOutline(325, 265, 32, 16, "sw", COLORS.notAvailable);
  drawRectOutline(375, 265, 36, 16, "uhc", COLORS.notAvailable);
}

function drawAboutMode() {
  drawText("About", 250, 45, { align: "center", size: 15, color: "#FFFFFF" });
  drawText("Creator / Developer", 250, 80, { align: "center", size: 10, color: "#FFFFFF" });
  drawText("VoidPro (@voidpro_dev)", 250, 100, { align: "center", size: 11, color: "#FFFFFF" });
  drawText("Tester / Ideas", 250, 130, { align: "center", size: 10, color: "#FFFFFF" });
  drawText("Shimamal (@MaybeShim)", 250, 150, { align: "center", size: 10, color: "#FFFFFF" });
  drawText("Special Thanks", 250, 180, { align: "center", size: 10, color: "#FFFFFF" });
  drawText("Antisniper (antisniper.net)", 250, 200, { align: "center", size: 9, color: "#FFFFFF" });
  drawText("Minecraftia", 250, 220, { align: "center", size: 9, color: "#FFFFFF" });
  drawText("Support : https://discord.gg/DZKqfdQGem", 250, 260, { align: "center", size: 9, color: "#FFFFFF" });
}

function drawMusicMode() {
  const track = currentTrack() || { title: "", artist: "", album: "" };
  const duration = currentDuration();
  const volumePercent = Math.round(currentAudio().volume * 100);
  const volumeY = 80 + (100 - volumePercent);
  const artCenterX = 250;
  const artCenterY = 120;
  const artSize = 100;
  const artLeft = artCenterX - artSize / 2;
  const artTop = artCenterY - artSize / 2;

  drawText("Music", 250, 45, { align: "center", size: 15, color: "#FFFFFF" });
  drawLine(50, 261, 450, 261, "#FFFFFF", 2);
  drawText("Vol", 475, 60, { align: "center", size: 10, color: "#FFFFFF" });
  drawFolderIcon(486, 267, "#FFFFFF");
  drawReloadIcon(486, 285, "#FFFFFF");
  drawSkipPreviousIcon(220, 275, "#FFFFFF");
  drawSkipNextIcon(280, 275, "#FFFFFF");

  if (hasAlbumArt()) {
    state.ctx.drawImage(state.music.artImage, artLeft, artTop, artSize, artSize);
  } else {
    const ctx = state.ctx;
    ctx.save();
    ctx.fillStyle = "#646464";
    ctx.fillRect(artLeft, artTop, artSize, artSize);
    ctx.restore();
    drawText("?", artCenterX, artCenterY, { align: "center", size: 24, color: "#ACACAC" });
  }

  drawText(`Title : ${truncate(track.title, 40)}`, 50, 200, { align: "left", size: 10, color: "#FFFFFF" });
  drawText(`Artist : ${truncate(track.artist, 40)}`, 50, 220, { align: "left", size: 10, color: "#FFFFFF" });
  drawText(`Album : ${truncate(track.album, 40)}`, 50, 240, { align: "left", size: 10, color: "#FFFFFF" });
  drawText(`${calcTime(currentTimeValue())}/${calcTime(duration)}`, 450, 275, { align: "right", size: 10, color: "#FFFFFF" });

  const position = duration > 0 ? clamp(currentTimeValue() / duration, 0, 1) * 400 : 0;
  drawCircle(50 + position, 260, 5, "#FFFFFF");
  drawText(`${Math.max(0, state.music.trackIndex + 1)}/${state.music.tracks.length}`, 50, 275, { align: "left", size: 10, color: "#FFFFFF" });
  drawText(`${volumePercent}%`, 475, 200, { align: "center", size: 10, color: "#FFFFFF" });
  drawCircle(475, volumeY, 5, "#FFFFFF");
  if (currentAudio().paused) {
    drawPlayIcon(250, 275, "#FFFFFF");
  } else {
    drawPauseIcon(250, 275, "#FFFFFF");
  }
  drawLine(475, 80, 475, 180, "#FFFFFF", 1);
}

function render() {
  if (!state.snapshot) {
    return;
  }

  drawBackground();

  if (state.layout.mode === "compact") {
    drawMiniPlayer(50, 500, true);
    return;
  }

  if (state.layout.mode === "stats") {
    drawStatsMode();
  } else if (state.layout.mode === "settings") {
    drawSettingsMode();
  } else if (state.layout.mode === "about") {
    drawAboutMode();
  } else if (state.layout.mode === "music") {
    drawMusicMode();
  }

  drawFooter();
  if (state.layout.mode !== "music") {
    drawMiniPlayer(state.layout.height, state.layout.width, false);
  }
}

function getPoint(event) {
  const rect = state.canvas.getBoundingClientRect();
  return {
    x: (event.clientX - rect.left) * (state.layout.width / rect.width),
    y: (event.clientY - rect.top) * (state.layout.height / rect.height)
  };
}

function seekToRatio(ratio) {
  const audio = currentAudio();
  const duration = currentDuration();
  if (!duration) {
    return;
  }
  audio.currentTime = clamp(ratio, 0, 1) * duration;
}

function handleMiniSeek(point) {
  const hasArt = hasAlbumArt();
  const more = hasArt ? 24 : 0;
  const width = state.layout.width;
  const ratio = clamp((point.x - more) / Math.max(1, width - more), 0, 1);
  seekToRatio(ratio);
}

function handleDefaultMusicEvent(type, point) {
  if (state.uiMode === "music") {
    return false;
  }
  const lineY = state.layout.mode === "compact" ? 47 : state.layout.height - 26;
  if (type === 1 && point.y >= lineY - 3 && point.y <= lineY + 3) {
    handleMiniSeek(point);
    state.pointerKind = "mini-seek";
    return true;
  }
  if (type === 0 && state.pointerKind === "mini-seek") {
    handleMiniSeek(point);
    return true;
  }
  if (type === 2 && state.pointerKind === "mini-seek") {
    state.pointerKind = null;
    return true;
  }
  return false;
}

async function handleSettingsEvent(type, point) {
  if (type === 1) {
    if (point.x >= 300 && point.x <= 450) {
      if (point.y >= 75 && point.y <= 85) {
        state.pointerKind = "opacity";
        return;
      }
      if (point.y >= 95 && point.y <= 105) {
        state.pointerKind = "refresh";
        return;
      }
    }

    if (point.x >= 300 && point.x <= 350 && point.y >= 277 && point.y <= 293) {
      const value = await window.overlayAPI.chooseLogFile();
      if (value) {
        await syncSnapshot(await window.overlayAPI.getState());
      }
      return;
    }

    if (point.x >= 345 && point.x <= 370) {
      if (point.y >= 145 && point.y <= 155) {
        await applyConfigPatch({ background: !Boolean(currentConfig().background) });
        return;
      }
      if (point.y >= 165 && point.y <= 175) {
        await applyConfigPatch({ inject: !Boolean(currentConfig().inject) });
        return;
      }
      if (point.y >= 185 && point.y <= 195) {
        await applyConfigPatch({ deposit_notify: !Boolean(currentConfig().deposit_notify) });
        return;
      }
      if (point.y >= 205 && point.y <= 215) {
        await applyConfigPatch({ auto_party_transfer: !Boolean(currentConfig().auto_party_transfer) });
        return;
      }
      if (point.y >= 225 && point.y <= 235) {
        await applyConfigPatch({ notify_suspicious_player: !Boolean(currentConfig().notify_suspicious_player) });
        return;
      }
    }

    if (point.y >= 297 && point.y <= 313) {
      if (point.x >= 335 && point.x <= 405) {
        const question = await window.overlayAPI.promptText({
          title: "AntiSniper API Key",
          label: "Enter your AntiSniper API key",
          value: currentConfig().antisniper_key || ""
        });
        if (question !== null) {
          await applyConfigPatch({ antisniper_key: question || null });
        }
        return;
      }
    }
  }

  if (type === 0) {
    if (state.pointerKind === "opacity") {
      const value = clamp(point.x - 300, 0, 150);
      setPreviewConfig({ opacity: Math.round(calcValue(value, 150, 10, 100)) });
      return;
    }
    if (state.pointerKind === "refresh") {
      const value = clamp(point.x - 300, 0, 150);
      setPreviewConfig({ refresh: Math.round(calcValue(value, 150, 1000, 10)) });
    }
  }

  if (type === 2 && ["opacity", "refresh"].includes(state.pointerKind)) {
    state.pointerKind = null;
    await commitPreviewConfig();
  }
}

async function handleAboutEvent(type, point) {
  if (type === 1 && point.y >= 252 && point.y <= 268 && point.x >= 166 && point.x <= 400) {
    await window.overlayAPI.openExternal("https://discord.gg/DZKqfdQGem");
  }
}

function musicSeekRatio(point) {
  return clamp((point.x - 50) / 400, 0, 1);
}

async function handleMusicEvent(type, point) {
  const audio = currentAudio();
  if (type === 0) {
    if (state.pointerKind === "music-seek") {
      seekToRatio(musicSeekRatio(point));
      return;
    }
    if (state.pointerKind === "music-volume") {
      audio.volume = clamp((180 - point.y) / 100, 0, 1);
      return;
    }
  }

  if (type === 1) {
    if (point.x >= 475 && point.x <= 495 && point.y >= 260 && point.y <= 275) {
      await window.overlayAPI.openMusicFolder();
      return;
    }
    if (point.x >= 475 && point.x <= 495 && point.y >= 275 && point.y <= 295) {
      await loadMusicLibrary({ preserveCurrent: true });
      return;
    }
    if (point.x >= 240 && point.x <= 260 && point.y >= 265 && point.y <= 285) {
      await togglePause();
      return;
    }
    if (point.x >= 210 && point.x <= 230 && point.y >= 265 && point.y <= 285) {
      await previousTrack();
      return;
    }
    if (point.x >= 270 && point.x <= 290 && point.y >= 265 && point.y <= 285) {
      await nextTrack();
      return;
    }
    if (point.y >= 255 && point.y <= 265 && point.x >= 50 && point.x <= 450) {
      seekToRatio(musicSeekRatio(point));
      state.pointerKind = "music-seek";
      return;
    }
    if (point.x >= 470 && point.x <= 480 && point.y >= 80 && point.y <= 180) {
      audio.volume = clamp((180 - point.y) / 100, 0, 1);
      state.pointerKind = "music-volume";
    }
  }

  if (type === 2 && state.pointerKind === "music-volume") {
    state.pointerKind = null;
    await applyConfigPatch({ volume: audio.volume });
    return;
  }

  if (type === 2 && state.pointerKind === "music-seek") {
    state.pointerKind = null;
  }
}

async function handleCanvasEvent(type, event) {
  if (!state.snapshot) {
    return;
  }
  const point = getPoint(event);
  if (point.y < 25) {
    return;
  }

  if (handleDefaultMusicEvent(type, point)) {
    return;
  }

  if (state.uiMode === "settings") {
    await handleSettingsEvent(type, point);
    return;
  }

  if (state.uiMode === "about") {
    await handleAboutEvent(type, point);
    return;
  }

  if (state.uiMode === "music") {
    await handleMusicEvent(type, point);
  }
}

async function syncSnapshot(snapshot) {
  state.snapshot = snapshot;
  syncAudioFromConfig();
  applyPreviewOpacity();
  await syncWindowLayout();
  updateButtonState();
}

function renderLoop(timestamp) {
  const interval = 1000 / TARGET_FPS;
  if (!state.lastRenderAt || timestamp - state.lastRenderAt >= interval) {
    if (state.lastRenderAt) {
      state.measuredFps = 1000 / Math.max(1, timestamp - state.lastRenderAt);
    }
    render();
    state.lastRenderAt = timestamp;
  }
  window.requestAnimationFrame(renderLoop);
}

function bindButtons() {
  for (const mode of MODE_BUTTONS) {
    document.querySelector(`#${mode}Button`).addEventListener("click", async () => {
      state.uiMode = state.uiMode === mode ? null : mode;
      updateButtonState();
      await syncWindowLayout();
    });
  }

  document.querySelector("#minimizeButton").addEventListener("click", async () => {
    await window.overlayAPI.minimize();
  });

  document.querySelector("#closeButton").addEventListener("click", async () => {
    await window.overlayAPI.quit();
  });
}

function bindCanvas() {
  state.canvas.addEventListener("mousedown", (event) => {
    handleCanvasEvent(1, event);
  });

  window.addEventListener("mousemove", (event) => {
    if (!state.pointerKind) {
      return;
    }
    handleCanvasEvent(0, event);
  });

  window.addEventListener("mouseup", (event) => {
    if (!state.pointerKind) {
      return;
    }
    handleCanvasEvent(2, event);
  });
}

function bindAudio() {
  const audio = currentAudio();
  audio.preload = "metadata";
  audio.addEventListener("ended", () => {
    nextTrack();
  });
}

async function waitForFonts() {
  if (!document.fonts?.ready) {
    return;
  }
  try {
    await Promise.all([
      document.fonts.load(`${Math.round(8 * FONT_SCALE)}px Minecraftia`),
      document.fonts.load(`${Math.round(15 * FONT_SCALE)}px Minecraftia`)
    ]);
    await document.fonts.ready;
  } catch (_error) {
    // Ignore font loading errors and continue with fallback rendering.
  }
}

async function init() {
  state.canvas = document.querySelector("#overlayCanvas");
  state.ctx = state.canvas.getContext("2d");
  state.ctx.imageSmoothingEnabled = false;
  await waitForFonts();
  bindButtons();
  bindCanvas();
  bindAudio();

  const snapshot = await window.overlayAPI.getState();
  await syncSnapshot(snapshot);
  await loadMusicLibrary({ preserveCurrent: false });

  window.overlayAPI.onState(async (nextSnapshot) => {
    await syncSnapshot(nextSnapshot);
  });

  window.requestAnimationFrame(renderLoop);
}

window.addEventListener("DOMContentLoaded", init);
