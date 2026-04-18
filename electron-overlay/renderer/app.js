const state = {
  snapshot: null,
  settingsOpen: false,
  compact: null
};

function setSettingsOpen(isOpen) {
  state.settingsOpen = isOpen;
  document.querySelector("#settingsLayer").classList.toggle("hidden", !isOpen);
  applyPresentation();
  syncCompactMode();
}

function shouldCompact(snapshot = state.snapshot) {
  if (!snapshot) {
    return false;
  }

  return Boolean(snapshot.config.autohide && snapshot.players.length === 0 && !state.settingsOpen);
}

function applyPresentation(snapshot = state.snapshot) {
  if (!snapshot) {
    return;
  }

  const shell = document.querySelector(".shell");
  shell.classList.toggle("compact", shouldCompact(snapshot));
  shell.classList.toggle("transparent-background", Boolean(snapshot.config.background));
}

async function syncCompactMode(snapshot = state.snapshot) {
  if (!snapshot) {
    return;
  }

  const nextCompact = shouldCompact(snapshot);
  if (state.compact === nextCompact) {
    return;
  }

  state.compact = nextCompact;
  try {
    await window.overlayAPI.setCompactMode(nextCompact);
  } catch (_error) {
    state.compact = null;
  }
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

function escapeHtml(value) {
  return String(value ?? "")
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;");
}

function getSortedPlayers(snapshot) {
  const uniquePlayers = [...new Set(snapshot.players)];
  const rows = uniquePlayers.map((name) => snapshot.playerData[name] || {
    Player: name,
    TAG: "Loading",
    rank: "-",
    FKDR: "-",
    WLR: "-",
    Finals: "-",
    Wins: "-",
    WS: "-",
    star: "-",
    score: -1
  });

  rows.sort((left, right) => Number(right.score || -1) - Number(left.score || -1));
  return rows.slice(0, 20);
}

function renderPlayers(snapshot) {
  const body = document.querySelector("#playersBody");
  const players = getSortedPlayers(snapshot);
  if (players.length === 0) {
    body.innerHTML = '<tr><td colspan="8" class="empty-cell">No player data yet.</td></tr>';
    return;
  }

  body.innerHTML = players.map((player) => {
    const displayName = snapshot.mcid && String(player.Player || "").toLowerCase() === String(snapshot.mcid).toLowerCase()
      ? "You"
      : player.Player;

    return `
      <tr>
        <td>${escapeHtml(player.star)}</td>
        <td style="color:${rankColor(player.rank)}">${escapeHtml(displayName)}</td>
        <td>${escapeHtml(player.TAG)}</td>
        <td style="color:${wsColor(player.WS)}">${escapeHtml(player.WS)}</td>
        <td style="color:${fkdrColor(player.FKDR)}">${escapeHtml(player.FKDR)}</td>
        <td style="color:${wlrColor(player.WLR)}">${escapeHtml(player.WLR)}</td>
        <td style="color:${finalColor(player.Finals)}">${escapeHtml(player.Finals)}</td>
        <td style="color:${winColor(player.Wins)}">${escapeHtml(player.Wins)}</td>
      </tr>
    `;
  }).join("");
}

function renderSnapshot(snapshot) {
  state.snapshot = snapshot;
  document.querySelector("#customLogValue").textContent = snapshot.config.log_file || "custom";
  document.querySelector("#opacityInput").value = snapshot.config.opacity;
  document.querySelector("#refreshInput").value = snapshot.config.refresh;
  document.querySelector("#fpsInput").value = snapshot.config.fps;
  document.querySelector("#modeInput").value = snapshot.config.mode;
  document.querySelector("#autohideInput").checked = Boolean(snapshot.config.autohide);
  document.querySelector("#backgroundInput").checked = Boolean(snapshot.config.background);
  renderPlayers(snapshot);
  applyPresentation(snapshot);
  syncCompactMode(snapshot);
}

async function updateConfig(patch) {
  const snapshot = await window.overlayAPI.updateConfig(patch);
  renderSnapshot(snapshot);
}

function bindEvents() {
  const settingsPanel = document.querySelector("#settingsPanel");
  document.querySelector("#settingsButton").addEventListener("click", () => {
    setSettingsOpen(!state.settingsOpen);
  });
  document.querySelector("#closeSettingsButton").addEventListener("click", () => {
    setSettingsOpen(false);
  });
  document.querySelector("#settingsBackdrop").addEventListener("click", () => {
    setSettingsOpen(false);
  });
  document.addEventListener("keydown", (event) => {
    if (event.key === "Escape" && state.settingsOpen) {
      setSettingsOpen(false);
    }
  });
  settingsPanel.addEventListener("click", (event) => {
    event.stopPropagation();
  });
  document.querySelector("#opacityInput").addEventListener("input", (event) => {
    updateConfig({ opacity: Number(event.target.value) });
  });
  document.querySelector("#refreshInput").addEventListener("change", (event) => {
    updateConfig({ refresh: Number(event.target.value) });
  });
  document.querySelector("#fpsInput").addEventListener("change", (event) => {
    updateConfig({ fps: Number(event.target.value) });
  });
  document.querySelector("#modeInput").addEventListener("change", (event) => {
    updateConfig({ mode: event.target.value });
  });
  document.querySelector("#autohideInput").addEventListener("change", (event) => {
    updateConfig({ autohide: event.target.checked });
  });
  document.querySelector("#backgroundInput").addEventListener("change", (event) => {
    updateConfig({ background: event.target.checked });
  });
  document.querySelector("#chooseLogButton").addEventListener("click", async () => {
    const value = await window.overlayAPI.chooseLogFile();
    if (value && state.snapshot) {
      renderSnapshot({
        ...state.snapshot,
        config: {
          ...state.snapshot.config,
          log_file: value
        }
      });
    }
  });
  document.querySelector("#closeButton").addEventListener("click", async () => {
    await window.overlayAPI.quit();
  });
}

async function init() {
  bindEvents();
  const snapshot = await window.overlayAPI.getState();
  renderSnapshot(snapshot);
  window.overlayAPI.onState((nextSnapshot) => renderSnapshot(nextSnapshot));
}

window.addEventListener("DOMContentLoaded", init);
