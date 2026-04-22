class StatsService {
  constructor() {
    this.dataCache = new Map();
    this.mcidCache = new Map();
    this.inFlight = new Map();
    this.urchinCache = new Map();
  }

  async lookupPlayer(mcid, { config, custom = false, nick = null } = {}) {
    const key = `${mcid.toLowerCase()}:${nick || ""}:${custom ? "1" : "0"}`;
    if (this.inFlight.has(key)) {
      return this.inFlight.get(key);
    }

    const promise = this.lookupPlayerInternal(mcid, { config, custom, nick })
      .finally(() => this.inFlight.delete(key));
    this.inFlight.set(key, promise);
    return promise;
  }

  async lookupPlayerInternal(mcid, { config, custom = false, nick = null } = {}) {
    let mode = "name";
    let queryValue = mcid;
    const cached = this.dataCache.get(mcid.toLowerCase());
    if (cached && cached.lastUpdate > Date.now() - 60000) {
      const parsed = await this.attachUrchinTags(this.parseBedwars(cached.payload, nick, custom), cached.payload, {
        config,
        custom
      });
      return this.resolveHiddenWinstreak(parsed, mcid, config);
    }

    if (mode === "name" && this.mcidCache.has(mcid.toLowerCase())) {
      mode = "uuid";
      queryValue = this.mcidCache.get(mcid.toLowerCase());
    }

    let data = null;
    for (let attempt = 0; attempt < 5; attempt += 1) {
      try {
        const response = await fetch(`https://hypixel.voids.top/v2/player?${mode}=${encodeURIComponent(queryValue)}`, {
          headers: { "user-agent": "void-overlay-electron" }
        });
        data = await response.json();
        break;
      } catch (_error) {
        if (attempt === 4) {
          return { status: "Failed", data: {} };
        }
      }
    }

    if (!data) {
      return { status: "Failed", data: {} };
    }

    if (data.success) {
      if (data.player) {
        data.lastUpdate = Date.now();
        this.dataCache.set(mcid.toLowerCase(), {
          lastUpdate: Date.now(),
          payload: data
        });
        const parsed = await this.attachUrchinTags(this.parseBedwars(data, nick, custom), data, {
          config,
          custom
        });
        return this.resolveHiddenWinstreak(parsed, mcid, config);
      }
      return { status: "Nicked", data: {} };
    }

    if (!data.cause) {
      return this.lookupPlayerInternal(mcid, { config, custom, nick });
    }

    if (data.cause === "Key throttle") {
      return this.lookupPlayerInternal(mcid, { config, custom, nick });
    }

    if (data.cause === "You have already looked up this name recently") {
      const resolved = await this.resolveUuidFromName(mcid);
      if (!resolved) {
        return { status: "Nicked", data: {} };
      }
      this.mcidCache.set(mcid.toLowerCase(), resolved);
      return this.lookupPlayerInternal(resolved, { config, custom, nick: nick || mcid });
    }

    return {
      status: data.cause || "Failed",
      data: {}
    };
  }

  async resolveHiddenWinstreak(result, mcid, config) {
    if (result.status !== "Winstreak Hidden") {
      return result;
    }

    if (!config?.antisniper_key) {
      return result;
    }

    try {
      const response = await fetch(`https://api.antisniper.net/winstreak?key=${encodeURIComponent(config.antisniper_key)}&name=${encodeURIComponent(mcid)}`, {
        headers: { "user-agent": "void-overlay-electron" }
      });
      const data = await response.json();
      if (data.success) {
        result.data.WS = data.player?.data?.overall_winstreak ?? 0;
      }
    } catch (_error) {
      return result;
    }

    return result;
  }

  async resolveUuidFromName(mcid) {
    for (let attempt = 0; attempt < 5; attempt += 1) {
      try {
        const response = await fetch(`https://api.mojang.com/users/profiles/minecraft/${encodeURIComponent(mcid)}`, {
          headers: { "user-agent": "void-overlay-electron" }
        });
        if (response.status === 204) {
          return null;
        }
        if (response.status === 429) {
          continue;
        }
        const data = await response.json();
        return data.id || null;
      } catch (_error) {
        if (attempt === 4) {
          return null;
        }
      }
    }
    return null;
  }

  getRank(player) {
    if (player.rank) {
      return player.rank;
    }
    if (player.newPackageRank) {
      if (player.monthlyPackageRank && player.monthlyPackageRank !== "NONE") {
        return "MVP++";
      }
      return player.newPackageRank;
    }
    return "DEFAULT";
  }

  splitTags(value) {
    if (!value) {
      return [];
    }
    return String(value)
      .split(",")
      .map((tag) => tag.trim())
      .filter(Boolean);
  }

  mergeTags(...groups) {
    const tags = [];
    const seen = new Set();
    for (const group of groups) {
      for (const tag of group) {
        const normalized = String(tag).toLowerCase();
        if (seen.has(normalized)) {
          continue;
        }
        seen.add(normalized);
        tags.push(tag);
      }
    }
    return tags.join(", ");
  }

  async fetchUrchinTags(player, { config, custom = false } = {}) {
    const apiKey = config?.urchin_key;
    const playerId = player?.uuid;
    const playerName = player?.displayname;
    if (!apiKey || !playerId || !playerName) {
      return [];
    }

    const source = custom ? "MANUAL" : "GAME";
    const cacheKey = `${String(playerId).toLowerCase()}:${source}:${apiKey}`;
    const cached = this.urchinCache.get(cacheKey);
    if (cached && cached.lastUpdate > Date.now() - 60000) {
      return cached.tags;
    }

    try {
      const query = new URLSearchParams({
        id: String(playerId),
        key: String(apiKey),
        name: String(playerName),
        sources: source
      });
      const response = await fetch(`https://urchin.ws/cubelify?${query.toString()}`, {
        headers: { "user-agent": "void-overlay-electron" }
      });
      const data = await response.json();
      const tags = Array.isArray(data?.tags)
        ? data.tags
          .map((tag) => String(tag?.tooltip || "").trim())
          .filter(Boolean)
        : [];
      this.urchinCache.set(cacheKey, {
        lastUpdate: Date.now(),
        tags
      });
      return tags;
    } catch (_error) {
      return [];
    }
  }

  async attachUrchinTags(result, payload, { config, custom = false } = {}) {
    if (!["Success", "Winstreak Hidden"].includes(result.status)) {
      return result;
    }

    const player = payload?.player;
    if (!player) {
      return result;
    }

    const tags = await this.fetchUrchinTags(player, { config, custom });
    if (!tags.length) {
      return result;
    }

    return {
      ...result,
      data: {
        ...result.data,
        TAG: this.mergeTags(tags, this.splitTags(result.data?.TAG))
      }
    };
  }

  parseBedwars(payload, nick = null, custom = false) {
    const player = payload.player || {};
    const stats = player.stats?.Bedwars;
    const rank = this.getRank(player);

    if (!stats) {
      return {
        status: "Success",
        data: {
          Player: player.displayname || nick || "Unknown",
          rank,
          TAG: "First Game",
          FKDR: "-",
          WLR: "-",
          Finals: "-",
          Wins: "-",
          WS: "-",
          star: 0,
          score: 10000
        }
      };
    }

    const finalKills = Number(stats.final_kills_bedwars || 0);
    const finalDeaths = Number(stats.final_deaths_bedwars || 0);
    const totalWins = Number(stats.wins_bedwars || 0);
    const totalGames = Number(stats.games_played_bedwars || 0);
    const star = Number(player.achievements?.bedwars_level || 0);
    const fkdr = finalDeaths > 0 ? (finalKills / finalDeaths).toFixed(2) : finalKills.toFixed(2);
    const losses = Math.max(totalGames - totalWins, 0);
    const wlr = losses > 0 ? (totalWins / losses).toFixed(2) : totalWins.toFixed(2);
    const tags = [];

    if (finalDeaths === 0) {
      tags.push("no death");
    }
    if (custom) {
      tags.push("specified");
    }

    let name = player.displayname || nick || "Unknown";
    if (nick) {
      tags.push("nicked");
      name = `${player.displayname} (${nick})`;
    }

    if (typeof stats.winstreak !== "number") {
      tags.push("ws hidden");
      return {
        status: "Winstreak Hidden",
        data: {
          Player: name,
          rank,
          TAG: tags.join(", "),
          FKDR: fkdr,
          WLR: wlr,
          Finals: finalKills,
          Wins: totalWins,
          WS: "-",
          star,
          score: Number(fkdr)
        }
      };
    }

    return {
      status: "Success",
      data: {
        Player: name,
        rank,
        TAG: tags.join(","),
        FKDR: fkdr,
        WLR: wlr,
        Finals: finalKills,
        Wins: totalWins,
        WS: stats.winstreak,
        star,
        score: Number(fkdr)
      }
    };
  }
}

module.exports = {
  StatsService
};
