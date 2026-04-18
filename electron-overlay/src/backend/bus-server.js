const net = require("net");

class BusServer {
  constructor({ host, ports, onChange }) {
    this.host = host;
    this.ports = ports;
    this.onChange = onChange;
    this.server = null;
    this.port = null;
    this.sessions = new Map();
  }

  async start() {
    for (const port of this.ports) {
      try {
        await this.listen(port);
        this.port = port;
        return port;
      } catch (_error) {
        continue;
      }
    }
    throw new Error("failed to bind any bus port");
  }

  listen(port) {
    return new Promise((resolve, reject) => {
      const server = net.createServer((socket) => this.handleSocket(socket));
      server.once("error", reject);
      server.listen(port, this.host, () => {
        server.removeListener("error", reject);
        this.server = server;
        resolve();
      });
    });
  }

  handleSocket(socket) {
    socket.setNoDelay(true);
    socket._helloBuffer = "";
    socket._responseBuffer = Buffer.alloc(0);
    socket._helloDone = false;
    socket._pending = [];
    socket._pid = null;

    socket.on("data", (chunk) => {
      if (!socket._helloDone) {
        this.consumeHello(socket, chunk);
        return;
      }
      this.consumeResponse(socket, chunk);
    });

    socket.on("close", () => {
      this.dropSocket(socket);
    });

    socket.on("error", () => {
      this.dropSocket(socket);
    });
  }

  consumeHello(socket, chunk) {
    socket._helloBuffer += chunk.toString("utf8");
    const newlineIndex = socket._helloBuffer.indexOf("\n");
    if (newlineIndex === -1) {
      return;
    }

    const line = socket._helloBuffer.slice(0, newlineIndex).replace(/\r$/, "");
    const rest = socket._helloBuffer.slice(newlineIndex + 1);
    socket._helloDone = true;
    socket._helloBuffer = "";

    const match = /^hello\s+(\d+)$/i.exec(line.trim());
    if (!match) {
      socket.destroy();
      return;
    }

    const pid = Number(match[1]);
    const existing = this.sessions.get(pid);
    if (existing) {
      existing.socket.destroy();
    }

    socket._pid = pid;
    this.sessions.set(pid, {
      pid,
      socket,
      connectedAt: Date.now()
    });
    this.notifyChange();

    if (rest) {
      this.consumeResponse(socket, Buffer.from(rest, "utf8"));
    }
  }

  consumeResponse(socket, chunk) {
    socket._responseBuffer = Buffer.concat([socket._responseBuffer, chunk]);
    for (;;) {
      const zeroIndex = socket._responseBuffer.indexOf(0);
      if (zeroIndex === -1) {
        break;
      }

      const payload = socket._responseBuffer.slice(0, zeroIndex).toString("utf8");
      socket._responseBuffer = socket._responseBuffer.slice(zeroIndex + 1);
      const pending = socket._pending.shift();
      if (pending) {
        pending.resolve(payload);
      }
    }
  }

  dropSocket(socket) {
    if (socket._pid && this.sessions.get(socket._pid)?.socket === socket) {
      this.sessions.delete(socket._pid);
      this.notifyChange();
    }

    while (socket._pending && socket._pending.length > 0) {
      const pending = socket._pending.shift();
      pending.reject(new Error("bus socket closed"));
    }
  }

  notifyChange() {
    if (typeof this.onChange === "function") {
      this.onChange(this.getSnapshot());
    }
  }

  getSnapshot() {
    return {
      port: this.port,
      sessions: [...this.sessions.keys()]
    };
  }

  getSessions() {
    return [...this.sessions.values()];
  }

  getPrimarySession() {
    return this.getSessions()[0] || null;
  }

  async request(command, { timeout = 4000 } = {}) {
    const session = this.getPrimarySession();
    if (!session) {
      throw new Error("no active bus session");
    }

    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        reject(new Error(`bus timeout for command: ${command}`));
      }, timeout);

      session.socket._pending.push({
        resolve: (value) => {
          clearTimeout(timer);
          resolve(value);
        },
        reject: (error) => {
          clearTimeout(timer);
          reject(error);
        }
      });
      session.socket.write(`${command}\n`);
    });
  }

  async send(command) {
    return this.request(command, { timeout: 4000 });
  }

  async stop() {
    for (const session of this.getSessions()) {
      session.socket.destroy();
    }
    this.sessions.clear();

    if (this.server) {
      await new Promise((resolve) => this.server.close(resolve));
      this.server = null;
    }
  }
}

module.exports = {
  BusServer
};
