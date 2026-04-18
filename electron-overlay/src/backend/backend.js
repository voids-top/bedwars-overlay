const fs = require("fs");
const { execFile } = require("child_process");
const iconv = require("iconv-lite");
const { BusServer } = require("./bus-server");
const { loadConfig, saveConfig, sanitizeConfig } = require("./config");
const { StatsService } = require("./stats-service");
const { BUS_PORTS, CLIENT_LOGS, HOST, VERSION, expandTemplate, resolveRoot } = require("./constants");

function execFileAsync(file, args, options = {}) {
  return new Promise((resolve, reject) => {
    execFile(file, args, options, (error, stdout, stderr) => {
      if (error) {
        reject(Object.assign(error, { stdout, stderr }));
        return;
      }
      resolve({ stdout, stderr });
    });
  });
}

async function runPowerShellJson(command) {
  const { stdout } = await execFileAsync("powershell", [
    "-NoProfile",
    "-Command",
    `${command} | ConvertTo-Json -Compress`
  ]);

  const text = stdout.trim();
  if (!text) {
    return [];
  }

  const parsed = JSON.parse(text);
  return Array.isArray(parsed) ? parsed : [parsed];
}

function scoreDecodedText(text) {
  if (!text) {
    return -1;
  }

  const replacementCount = (text.match(/\uFFFD/g) || []).length;
  const lineCount = (text.match(/\n/g) || []).length;
  return lineCount * 4 - replacementCount * 10;
}

function decodeLogBuffer(buffer) {
  const candidates = [
    iconv.decode(buffer, "utf8"),
    iconv.decode(buffer, "cp932"),
    iconv.decode(buffer, "latin1")
  ];

  candidates.sort((left, right) => scoreDecodedText(right) - scoreDecodedText(left));
  return candidates[0];
}

function normalizeChatLine(line) {
  return line.replace(/§[0-9A-FK-OR]/gi, "");
}

class ElectronOverlayBackend {
  constructor({ rootDir, onStateChange }) {
    this.rootDir = rootDir;
    this.onStateChange = onStateChange;
    this.busServer = new BusServer({
      host: HOST,
      ports: BUS_PORTS,
      onChange: ({ port, sessions }) => {
        this.state.busPort = port;
        this.state.sessions = sessions;
        this.state.injected = sessions.length > 0;
        this.emitState();
      }
    });
    this.statsService = new StatsService();
    this.state = {
      version: VERSION,
      busPort: null,
      client: null,
      logfile: null,
      log: "Started",
      injected: false,
      hidden: false,
      mcid: null,
      players: [],
      playerData: {},
      sessions: [],
      config: loadConfig()
    };
    this.processedLogs = new Set();
    this.processedLogOrder = [];
    this.logTimer = null;
    this.clientTimer = null;
    this.injectTimer = null;
    this.idTimer = null;
    this.lastLogPath = null;
    this.lastIdCheck = 0;
    this.injectAttemptAt = new Map();
  }

  async start() {
    await this.busServer.start();
    this.state.busPort = this.busServer.port;
    this.scheduleLogPolling();
    this.clientTimer = setInterval(() => this.detectClientLog(), 1000);
    this.injectTimer = setInterval(() => this.forceInject(), 5000);
    this.idTimer = setInterval(() => this.refreshIdentity(), 5000);
    await this.detectClientLog();
    await this.forceInject();
    this.emitState();
  }

  async stop() {
    clearInterval(this.clientTimer);
    clearInterval(this.injectTimer);
    clearInterval(this.idTimer);
    clearInterval(this.logTimer);
    await this.busServer.stop();
  }

  getSnapshot() {
    return JSON.parse(JSON.stringify(this.state));
  }

  emitState() {
    if (typeof this.onStateChange === "function") {
      this.onStateChange(this.getSnapshot());
    }
  }

  updateConfig(patch) {
    this.state.config = sanitizeConfig({ ...this.state.config, ...(patch || {}) });
    saveConfig(this.state.config);
    this.scheduleLogPolling();
    this.emitState();
  }

  setLog(message) {
    this.state.log = message;
    this.emitState();
  }

  clearPlayers() {
    this.state.players = [];
    this.state.playerData = {};
    this.emitState();
  }

  addPlayer(name) {
    if (!this.state.players.includes(name)) {
      this.state.players.push(name);
      this.emitState();
    }
  }

  setPlayerData(name, data) {
    this.state.playerData[name] = data;
    this.emitState();
  }

  scheduleLogPolling() {
    clearInterval(this.logTimer);
    this.logTimer = setInterval(() => this.pollLog(), this.state.config.refresh);
  }

  resetProcessedLogs(filePath) {
    this.lastLogPath = filePath;
    this.processedLogs.clear();
    this.processedLogOrder = [];
  }

  async detectClientLog() {
    const candidates = {
      ...CLIENT_LOGS,
      "Custom Client": this.state.config.log_file
    };

    for (const [client, template] of Object.entries(candidates)) {
      if (!template || template === "custom") {
        continue;
      }

      const filePath = template.includes("{") ? expandTemplate(template) : template;
      try {
        const stat = fs.statSync(filePath);
        if (Date.now() - stat.mtimeMs > 30000) {
          continue;
        }

        if (this.state.logfile !== filePath) {
          this.state.client = client;
          this.state.logfile = filePath;
          this.setLog(`Connected to ${client}`);
          this.resetProcessedLogs(filePath);
        }
        return;
      } catch (_error) {
        continue;
      }
    }
  }

  pollLog() {
    const filePath = this.state.logfile;
    if (!filePath) {
      return;
    }

    try {
      const stat = fs.statSync(filePath);
      const start = Math.max(0, stat.size - 100000);
      const fd = fs.openSync(filePath, "r");
      const buffer = Buffer.alloc(stat.size - start);
      fs.readSync(fd, buffer, 0, buffer.length, start);
      fs.closeSync(fd);

      const text = decodeLogBuffer(buffer);
      const lines = text.split(/\r?\n/).filter(Boolean);
      for (const line of lines) {
        if (this.processedLogs.has(line)) {
          continue;
        }
        this.processedLogs.add(line);
        this.processedLogOrder.push(line);
        this.processLogLine(line);
      }

      if (this.processedLogOrder.length > 10000) {
        const keep = this.processedLogOrder.slice(-5000);
        this.processedLogs = new Set(keep);
        this.processedLogOrder = keep;
      }
    } catch (_error) {
      return;
    }
  }

  async processLogLine(line) {
    const cleaned = normalizeChatLine(line);
    const marker = "[Client thread/INFO]: ";
    if (!cleaned.includes(marker)) {
      return;
    }

    const chat = cleaned.split(marker)[1];
    if (chat === "[CHAT]                                   Bed Wars" || chat === "[CHAT]                                   SkyWars") {
      await this.sendBusCommand("chat /who", { expectResponse: false });
      return;
    }

    if (chat === "[CHAT]        ") {
      this.clearPlayers();
      this.setLog("server change");
      await this.sendBusCommand("chat /who", { expectResponse: false });
      return;
    }

    if (chat.startsWith("[LC Accounts] Able to login ")) {
      this.state.mcid = chat.replace("[LC Accounts] Able to login ", "");
      this.setLog("account change");
      return;
    }

    if (chat.startsWith("[CHAT] ONLINE: ")) {
      this.clearPlayers();
      this.setLog("checking players");
      const players = chat
        .replace("[CHAT] ONLINE: ", "")
        .split(" [x")[0]
        .split(", ")
        .map((value) => value.trim())
        .filter(Boolean);

      for (const player of players) {
        this.lookupPlayer(player);
      }
      return;
    }

    if (chat.startsWith("[CHAT] Mode: ")) {
      this.clearPlayers();
      this.setLog("checking players");
      return;
    }

    if (chat.startsWith("[CHAT] Team #")) {
      const player = chat.split(": ")[1]?.split(" [x")[0]?.trim();
      if (player) {
        this.setLog("checking players");
        this.lookupPlayer(player);
      }
    }
  }

  async lookupPlayer(name) {
    this.addPlayer(name);
    const result = await this.statsService.lookupPlayer(name, { custom: false });

    if (result.status === "Success") {
      this.setPlayerData(name, result.data);
      return;
    }

    if (result.status === "Nicked") {
      this.setPlayerData(name, {
        Player: name,
        TAG: "Nicked",
        rank: "-",
        FKDR: "-",
        WLR: "-",
        Finals: "-",
        Wins: "-",
        WS: "-",
        star: "-",
        score: 100000
      });
      return;
    }

    this.setPlayerData(name, {
      Player: name,
      TAG: result.status === "Failed" ? "Failed" : String(result.status || "Unknown"),
      rank: "-",
      FKDR: "-",
      WLR: "-",
      Finals: "-",
      Wins: "-",
      WS: "-",
      star: "-",
      score: 100000
    });
  }

  async listJavaProcesses() {
    try {
      const processes = await runPowerShellJson("Get-Process javaw -ErrorAction SilentlyContinue | Select-Object Id,MainWindowTitle,Path");
      return processes.map((processInfo) => ({
        id: Number(processInfo.Id),
        title: processInfo.MainWindowTitle || "",
        path: processInfo.Path || ""
      }));
    } catch (_error) {
      return [];
    }
  }

  findDllPath() {
    const candidates = [
      resolveRoot(this.rootDir, "build", "util8b.dll"),
      resolveRoot(this.rootDir, "build", "util8.dll"),
      resolveRoot(this.rootDir, "build", "util7.dll"),
      resolveRoot(this.rootDir, "build", "util.dll"),
      resolveRoot(this.rootDir, "util.dll")
    ];

    return candidates.find((candidate) => fs.existsSync(candidate)) || null;
  }

  findInjectorPath() {
    const candidates = [
      resolveRoot(this.rootDir, "injector.exe"),
      resolveRoot(this.rootDir, "ext", "injector.exe")
    ];

    return candidates.find((candidate) => fs.existsSync(candidate)) || null;
  }

  async injectPid(pid) {
    const injectorPath = this.findInjectorPath();
    const dllPath = this.findDllPath();
    if (!injectorPath || !dllPath) {
      this.setLog("injector or dll missing");
      return false;
    }

    try {
      const { stdout } = await execFileAsync(injectorPath, [String(pid), dllPath], {
        cwd: resolveRoot(this.rootDir)
      });
      this.setLog(stdout.trim() || `injected ${pid}`);
      return true;
    } catch (error) {
      this.setLog((error.stdout || error.message || "inject failed").trim());
      return false;
    }
  }

  async forceInject() {
    const injected = new Set(this.busServer.getSnapshot().sessions);
    this.state.injected = injected.size > 0;
    for (const pid of injected) {
      this.injectAttemptAt.delete(pid);
    }
    this.emitState();

    const processes = await this.listJavaProcesses();
    for (const processInfo of processes) {
      if (injected.has(processInfo.id)) {
        continue;
      }
      if (!processInfo.title || !processInfo.title.includes("1.8")) {
        continue;
      }
      const lastAttempt = this.injectAttemptAt.get(processInfo.id) || 0;
      if (Date.now() - lastAttempt < 15000) {
        continue;
      }
      this.injectAttemptAt.set(processInfo.id, Date.now());
      await this.injectPid(processInfo.id);
    }
  }

  async refreshIdentity() {
    if (this.busServer.getSessions().length !== 1) {
      return;
    }
    if (Date.now() - this.lastIdCheck < 3000) {
      return;
    }

    this.lastIdCheck = Date.now();
    try {
      const response = await this.busServer.request("id", { timeout: 3000 });
      if (response.startsWith("id: ")) {
        const value = response.slice(4).trim();
        if (value && value !== "(unknown)" && value !== "(no mc)") {
          this.state.mcid = value;
          this.setLog("injected");
        }
      }
    } catch (_error) {
      return;
    }
  }

  async sendBusCommand(command, { expectResponse = true } = {}) {
    if (!expectResponse) {
      try {
        await this.busServer.send(command);
      } catch (_error) {
        return null;
      }
      return null;
    }

    try {
      return await this.busServer.request(command);
    } catch (_error) {
      return null;
    }
  }
}

module.exports = {
  ElectronOverlayBackend
};
